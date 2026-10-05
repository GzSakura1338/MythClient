#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "chrome/BrowserService.h"
#include "core/Instance.h"
#include <shlobj.h>
#include <sddl.h>
#include <objbase.h>
#include <imm.h>
#include <atomic>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

namespace nova::chrome {
namespace {
struct Handle {HANDLE h{};~Handle(){if(h && h!=INVALID_HANDLE_VALUE)CloseHandle(h);} };
struct Mapping {Shared* p{};~Mapping(){if(p)UnmapViewOfFile(p);} };
struct State {
    std::mutex lock,lifecycle;
    Snapshot snapshot;
    std::deque<Command> pending;
    std::thread worker;
    std::atomic<bool> stop{false},done{true},focused{false},teardown{false},addressFocus{false};
    bool visible=false;int width=800,height=600;
    uint64_t generation=0;
    float x{},y{},regionW=1,regionH=1;
    int inputWidth=1,inputHeight=1;
    std::filesystem::path dataRoot;
};
State& S(){return Instance<State>();}
void Error(std::string value){auto& s=S();std::lock_guard lock(s.lock);s.snapshot.error=std::move(value);}
std::filesystem::path Local(){if(!S().dataRoot.empty())return S().dataRoot;PWSTR p{};if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&p)))return {};std::filesystem::path r(p);CoTaskMemFree(p);return r/L"Nova"/L"Browser";}
void Run() {
    auto& s=S();
    try {
        auto runtime=Local()/L"runtime"/RuntimeVersion;
        std::ifstream pointer(runtime/L"current.txt");std::string version;
        if(std::getline(pointer,version) && !version.empty() && version.size()<80 &&
            std::all_of(version.begin(),version.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-';}))runtime/=Wide(version);
        const auto exe=runtime/L"NovaBrowserHost.exe";
        if(!std::filesystem::exists(exe))throw std::runtime_error("Browser runtime missing. Run the updated Nova injector or install-browser.ps1.");
        GUID guid{};if(FAILED(CoCreateGuid(&guid)))throw std::runtime_error("Cannot create browser instance ID.");
        wchar_t uuid[40];StringFromGUID2(guid,uuid,40);
        const std::wstring name=L"Local\\NovaBrowser-"+std::wstring(uuid),mutexName=name+L"-lock";
        Handle token; if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.h))throw std::runtime_error("Cannot read browser user.");
        DWORD bytes=0;GetTokenInformation(token.h,TokenUser,nullptr,0,&bytes);std::vector<unsigned char> user(bytes);
        if(!GetTokenInformation(token.h,TokenUser,user.data(),bytes,&bytes))throw std::runtime_error("Cannot read browser SID.");
        LPWSTR sid{};if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid,&sid))throw std::runtime_error("Cannot encode browser SID.");
        std::wstring acl=L"D:P(A;;GA;;;SY)(A;;GA;;;"+std::wstring(sid)+L")";LocalFree(sid);
        PSECURITY_DESCRIPTOR descriptor{};
        if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(),SDDL_REVISION_1,&descriptor,nullptr))throw std::runtime_error("Cannot secure browser channel.");
        SECURITY_ATTRIBUTES sa{sizeof(sa),descriptor,FALSE};
        Handle memory{CreateFileMappingW(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE,0,sizeof(Shared),name.c_str())};
        Handle mutex{CreateMutexW(&sa,FALSE,mutexName.c_str())};LocalFree(descriptor);
        Mapping map{memory.h?static_cast<Shared*>(MapViewOfFile(memory.h,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared))):nullptr};
        if(!map.p || !mutex.h)throw std::runtime_error("Cannot create browser shared memory.");
        map.p->magic=Magic;map.p->version=ProtocolVersion;
        Handle job{CreateJobObjectW(nullptr,nullptr)};
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if(!job.h || !SetInformationJobObject(job.h,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))throw std::runtime_error("Cannot create browser process group.");
        // PID-stable profile permits reinjection without mixing two running games.
        auto profile=Local()/L"profiles"/(L"game-"+std::to_wstring(GetCurrentProcessId()));
        std::filesystem::create_directories(profile);
        std::wstring cmd=L"\""+exe.wstring()+L"\" --nova-channel=\""+name+L"\" --nova-parent="+std::to_wstring(GetCurrentProcessId())+L" --nova-profile=\""+profile.wstring()+L"\"";
        STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};
        if(!CreateProcessW(exe.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED|CREATE_NO_WINDOW,nullptr,runtime.c_str(),&si,&pi))throw std::runtime_error("Cannot start browser host: "+std::to_string(GetLastError()));
        Handle process{pi.hProcess},thread{pi.hThread};
        if(!AssignProcessToJobObject(job.h,process.h)){TerminateProcess(process.h,1);throw std::runtime_error("Cannot attach browser process group.");}
        ResumeThread(thread.h);
        uint64_t seen=0;const auto started=GetTickCount64();ULONGLONG closing=0;
        while(WaitForSingleObject(process.h,10)==WAIT_TIMEOUT) {
            if(s.stop && !closing)closing=GetTickCount64();
            if(closing && GetTickCount64()-closing>4000)break;
            if(!closing && GetTickCount64()-started>25000){std::lock_guard lock(s.lock);if(!s.snapshot.status.ready){s.snapshot.error="Browser startup timed out.";break;}}
            const DWORD wait=WaitForSingleObject(mutex.h,5);
            if(wait==WAIT_ABANDONED){ReleaseMutex(mutex.h);Error("Browser channel was interrupted.");break;}
            if(wait!=WAIT_OBJECT_0)continue;
            std::shared_ptr<Frame> frame;
            Status status{};
            bool malformed=map.p->magic!=Magic || map.p->version!=ProtocolVersion || map.p->write-map.p->read>QueueSize;
            if(!malformed) {
                {
                std::lock_guard lock(s.lock);
                if(closing){map.p->read=map.p->write;Command c;c.op=Op::Shutdown;map.p->commands[map.p->write++%QueueSize]=c;s.pending.clear();}
                else while(!s.pending.empty() && map.p->write-map.p->read<QueueSize){map.p->commands[map.p->write++%QueueSize]=s.pending.front();s.pending.pop_front();}
                }
                status=map.p->status;
                status.count=std::clamp(status.count,0,MaxTabs);status.url[2047]=0;status.error[255]=0;for(auto& tab:status.tabs)tab.title[159]=0;
                if(map.p->sequence!=seen && ValidSize(map.p->width,map.p->height)){
                    frame=std::make_shared<Frame>();frame->width=map.p->width;frame->height=map.p->height;frame->sequence=(s.generation<<32)|map.p->sequence;
                    frame->rgba.assign(map.p->pixels,map.p->pixels+size_t(frame->width)*frame->height*4);seen=map.p->sequence;
                }
            }
            ReleaseMutex(mutex.h);
            if(malformed){Error("Browser protocol mismatch.");break;}
            if(frame)for(size_t i=0;i<frame->rgba.size();i+=4)std::swap(frame->rgba[i],frame->rgba[i+2]);
            {std::lock_guard lock(s.lock);s.snapshot.status=status;s.snapshot.running=true;s.snapshot.starting=!status.ready;if(frame)s.snapshot.frame=std::move(frame);}
        }
        if(!s.stop){DWORD code=0;GetExitCodeProcess(process.h,&code);std::lock_guard lock(s.lock);if(s.snapshot.error.empty())s.snapshot.error="Browser host stopped ("+std::to_string(code)+"). Retry to reopen.";}
        // Closing the job also drains child processes; no browser code lives in DLL.
        TerminateJobObject(job.h,0);WaitForSingleObject(process.h,3000);
    } catch(const std::exception& e){Error(e.what());}
    {std::lock_guard lock(s.lock);s.snapshot.running=false;s.snapshot.starting=false;s.snapshot.status.ready=0;s.snapshot.frame.reset();s.pending.clear();}
    s.focused=false;s.done=true;
}
int Modifiers(){int m=0;if(GetKeyState(VK_SHIFT)&0x8000)m|=2;if(GetKeyState(VK_CONTROL)&0x8000)m|=4;if(GetKeyState(VK_MENU)&0x8000)m|=8;return m;}
}
std::wstring Wide(std::string_view s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);std::wstring r(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),r.data(),n);return r;}
std::string Utf8(std::wstring_view s){if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);std::string r(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),r.data(),n,nullptr,nullptr);return r;}
void Start(std::wstring dataRoot){auto& s=S();std::unique_lock lifecycle(s.lifecycle,std::try_to_lock);if(!lifecycle||s.teardown||!s.done)return;if(s.worker.joinable())s.worker.join();s.dataRoot=std::move(dataRoot);{std::lock_guard lock(s.lock);s.snapshot={};s.snapshot.starting=true;s.pending.clear();s.visible=false;s.width=800;s.height=600;++s.generation;}s.stop=false;s.done=false;s.worker=std::thread(Run);}
void Stop(){S().focused=false;S().stop=true;}
void Shutdown(){auto& s=S();s.teardown=true;std::lock_guard lifecycle(s.lifecycle);Stop();if(s.worker.joinable())s.worker.join();}
Snapshot Read(){auto& s=S();std::lock_guard lock(s.lock);return s.snapshot;}
void Send(Command c){auto& s=S();if(s.done || s.stop)return;std::lock_guard lock(s.lock);if(s.pending.size()>=QueueSize){s.pending.clear();s.focused=false;Command reset;reset.op=Op::Focus;s.pending.push_back(reset);return;}s.pending.push_back(c);}
void Navigate(std::wstring url){
    const auto first=url.find_first_not_of(L" \r\n\t"),last=url.find_last_not_of(L" \r\n\t");url=first==url.npos?L"about:blank":url.substr(first,last-first+1);
    if(url.find(L":")==std::wstring::npos){
        if(url.find(L' ')!=url.npos||url.find(L'.')==url.npos){
            auto bytes=Utf8(url);url=L"https://www.bing.com/search?q=";constexpr wchar_t hex[]=L"0123456789ABCDEF";
            for(unsigned char c:bytes){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9'))url+=wchar_t(c);else {url+=L'%';url+=hex[c>>4];url+=hex[c&15];}}
        }else url=L"https://"+url;
    }
    if(!AllowedUrl(url)||url.size()>=2048){Error("Use an http(s) address or search text (up to 2047 characters).");return;}
    Error({});Command c;c.op=Op::Navigate;CopyText(c.text,url);Send(c);
}
void Viewport(int w,int h){w=std::clamp(w,1,MaxWidth);h=std::clamp(h,1,MaxHeight);auto& s=S();{std::lock_guard lock(s.lock);if(s.width==w&&s.height==h)return;s.width=w;s.height=h;}Command c;c.op=Op::Resize;c.a=w;c.b=h;Send(c);}
void Visible(bool v){auto& s=S();{std::lock_guard lock(s.lock);if(s.visible==v)return;s.visible=v;}Command c;c.op=Op::Visible;c.a=v;Send(c);if(!v)Focus(false);}
void Focus(bool v){if(S().focused.exchange(v)==v)return;Command c;c.op=Op::Focus;c.a=v;Send(c);}
bool Focused(){return S().focused;}
bool TakeAddressFocus(){return S().addressFocus.exchange(false);}
void InputRegion(float x,float y,float w,float h,int pw,int ph){auto& s=S();std::lock_guard lock(s.lock);s.x=x;s.y=y;s.regionW=w;s.regionH=h;s.inputWidth=pw;s.inputHeight=ph;}
bool WindowMessage(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    if(msg==WM_KILLFOCUS){Focus(false);return false;}
    if(!Focused())return false;
    if((msg==WM_KEYDOWN||msg==WM_KEYUP||msg==WM_SYSKEYDOWN||msg==WM_SYSKEYUP) && w==VK_ESCAPE){Focus(false);return false;}
    Command c;c.modifiers=Modifiers();
    if(msg==WM_KEYDOWN&&(c.modifiers&4)&&w=='L'){Focus(false);S().addressFocus=true;return true;}
    if(msg==WM_KEYDOWN&&((w=='R'&&(c.modifiers&4))||w==VK_F5)){c.op=Op::Reload;Send(c);return true;}
    if(msg==WM_KEYDOWN||msg==WM_KEYUP||msg==WM_SYSKEYDOWN||msg==WM_SYSKEYUP){c.op=Op::Key;c.a=int(w);c.b=int(l);c.c=(msg==WM_KEYUP||msg==WM_SYSKEYUP);c.d=(msg==WM_SYSKEYDOWN||msg==WM_SYSKEYUP);Send(c);return !c.c;}
    if(msg==WM_CHAR||msg==WM_SYSCHAR){c.op=Op::Character;c.a=int(w);c.b=int(l);Send(c);return true;}
    if(msg==WM_IME_STARTCOMPOSITION||msg==WM_IME_COMPOSITION||msg==WM_IME_ENDCOMPOSITION){
        auto imc=ImmGetContext(hwnd);if(!imc)return true;
        if(msg==WM_IME_ENDCOMPOSITION){c.op=Op::ImeCancel;Send(c);}
        else {
            auto& s=S();{std::lock_guard lock(s.lock);CANDIDATEFORM form{};form.dwStyle=CFS_CANDIDATEPOS;form.ptCurrentPos={LONG(s.x+s.snapshot.status.caretX*s.regionW/std::max(1,s.inputWidth)),LONG(s.y+(s.snapshot.status.caretY+s.snapshot.status.caretHeight)*s.regionH/std::max(1,s.inputHeight))};ImmSetCandidateWindow(imc,&form);}
            if(l&GCS_RESULTSTR){c.op=Op::ImeCommit;LONG bytes=ImmGetCompositionStringW(imc,GCS_RESULTSTR,c.text,sizeof(c.text)-2);if(bytes>=0){c.text[bytes/2]=0;Send(c);}}
            if(l&GCS_COMPSTR){c.op=Op::Ime;LONG bytes=ImmGetCompositionStringW(imc,GCS_COMPSTR,c.text,sizeof(c.text)-2);if(bytes>=0){c.text[bytes/2]=0;c.a=ImmGetCompositionStringW(imc,GCS_CURSORPOS,nullptr,0);Send(c);}}
        }ImmReleaseContext(hwnd,imc);return true;
    }
    return false;
}
}
