#include "chrome/BrowserService.h"
#include <cstdio>
#include <fstream>
#include <functional>
using namespace nova::chrome;
bool Wait(const std::function<bool(const Snapshot&)>& predicate,int ms=15000) {
    const auto until=GetTickCount64()+ms;
    while(GetTickCount64()<until){auto s=Read();if(predicate(s))return true;if(!s.error.empty()){printf("ERROR: %s\n",s.error.c_str());return false;}Sleep(20);}return false;
}
void Check(bool value,const char* reason){if(!value){auto s=Read();printf("FAIL: %s status=%s\n",reason,Utf8(s.status.error).c_str());for(int i=0;i<s.status.count;++i)printf("TAB: %s\n",Utf8(s.status.tabs[i].title).c_str());if(s.frame){std::ofstream out("browser-failure.ppm",std::ios::binary);out<<"P6\n"<<s.frame->width<<" "<<s.frame->height<<"\n255\n";for(size_t i=0;i<s.frame->rgba.size();i+=4)out.write(reinterpret_cast<const char*>(s.frame->rgba.data()+i),3);}Shutdown();exit(1);}}
void Cmd(Op op,int a=0,int b=0,int c=0,int d=0){Command m;m.op=op;m.a=a;m.b=b;m.c=c;m.d=d;Send(m);}
void Click(int x,int y){Cmd(Op::MouseMove,x,y);Cmd(Op::MouseButton,x,y,0,0);Cmd(Op::MouseButton,x,y,0,1);}
std::wstring Title(const Snapshot& s){for(int i=0;i<s.status.count;++i)if(s.status.tabs[i].id==s.status.selected)return s.status.tabs[i].title;return {};}
bool Rendered(const Snapshot& s){if(!s.frame)return false;size_t lit=0;for(size_t i=0;i<s.frame->rgba.size();i+=4)if(s.frame->rgba[i]>5||s.frame->rgba[i+1]>5||s.frame->rgba[i+2]>5)++lit;return lit>size_t(s.frame->width)*s.frame->height/2;}
int main(int argc,char** argv) {
    std::wstring dataRoot;
    for(int i=1;i+1<argc;++i)if(std::string_view(argv[i])=="--data-root")dataRoot=Wide(argv[++i]);
    Check(AllowedUrl(L"https://example.com")&&!AllowedUrl(L"file:///C:/Windows")&&!AllowedUrl(L"javascript:alert(1)"),"navigation protocol policy");
    Check(ValidSize(1920,1080)&&!ValidSize(1921,1080)&&!ValidSize(-1,1),"frame bounds");
    Start(dataRoot);Check(Wait([](auto& s){return s.status.ready!=0;},25000),"sandboxed host startup");
    if(argc>1&&std::string_view(argv[1])=="--h264"){
        Visible(true);Viewport(800,600);Navigate(L"http://127.0.0.1:18795/h264.html");
        Check(Wait([](auto& s){return Title(s).starts_with(L"H264 PASS")||Title(s).starts_with(L"H264 FAIL");},20000),"real H.264 media test");
        const auto title=Title(Read());printf("%s\n",Utf8(title).c_str());
        Check(title.starts_with(L"H264 PASS"),"H.264 playback, seek and resume");Shutdown();return 0;
    }
    if(argc>1&&(std::string_view(argv[1])=="--cloud"||std::string_view(argv[1])=="--cloud-h264")){
        const bool h264=std::string_view(argv[1])=="--cloud-h264";
        Visible(true);Viewport(800,600);Navigate(h264?L"http://127.0.0.1:18795/cloud.html?h264":L"http://127.0.0.1:18795/cloud.html");
        Check(Wait([](auto& s){return Title(s).starts_with(L"CLOUD PASS")||Title(s).starts_with(L"CLOUD FAIL");},30000),"WebRTC local streaming diagnostics");
        const auto title=Title(Read());printf("%s\n",Utf8(title).c_str());
        Check(title.starts_with(L"CLOUD PASS")&&(!h264||title.find(L"codec=video/H264")!=std::wstring::npos),"WebRTC decoded video and data channel");Shutdown();return 0;
    }
    if(argc>1&&std::string_view(argv[1])=="--media"){
        Visible(true);Viewport(800,600);Navigate(L"http://127.0.0.1:18795/media.html");
        Check(Wait([](auto& s){return Title(s).starts_with(L"MEDIA PASS")||Title(s).starts_with(L"MEDIA FAIL");}),"media playback diagnostics");
        const auto title=Title(Read());printf("%s\n",Utf8(title).c_str());
        Check(title.starts_with(L"MEDIA PASS"),"HTML5 video clock advances");Shutdown();return 0;
    }
    Visible(true);Viewport(640,480);Navigate(argc>1?Wide(argv[1]):L"http://127.0.0.1:18795/test.html");
    Check(Wait([](auto& s){return Title(s)==L"Nova Browser Test"&&Rendered(s)&&s.frame->width==640;}),"HTTP navigation and nonblack rendered pixels");
    Focus(true);Click(70,125);
    Check(Wait([](auto& s){return Title(s)==L"clicked";}),"mouse click reaches webpage");
    Click(90,75);Cmd(Op::Character,'N');Cmd(Op::Character,'o');Cmd(Op::Character,'v');Cmd(Op::Character,'a');
    Check(Wait([](auto& s){return Title(s)==L"input:Nova";}),"keyboard input reaches webpage");
    Command ime;ime.op=Op::Ime;CopyText(ime.text,L"中文");ime.a=2;Send(ime);ime.op=Op::ImeCommit;Send(ime);
    Check(Wait([](auto& s){return Title(s)==L"input:Nova中文";}),"Unicode IME commit");
    Cmd(Op::Edit,2);Cmd(Op::Character,'X');Check(Wait([](auto& s){return Title(s)==L"input:X";}),"select-all command uses native editor input");
    Cmd(Op::NewTab);Check(Wait([](auto& s){return s.status.count==2;}),"new tab");
    auto s=Read();int id=s.status.selected;Cmd(Op::CloseTab,id);Check(Wait([](auto& t){return t.status.count==1;}),"close tab");
    Viewport(800,500);Check(Wait([](auto& t){return t.frame&&t.frame->width==800&&t.frame->height==500;}),"resize/repaint");
    Visible(false);Sleep(350);auto seq=Read().frame->sequence;Sleep(300);Check(Read().frame->sequence==seq,"hidden browser stops frame publication");
    Visible(true);Check(Wait([&](auto& t){return Rendered(t)&&t.frame->sequence>seq;}),"resume nonblack visible frame");
    s=Read();std::ofstream out("browser-smoke.ppm",std::ios::binary);out<<"P6\n"<<s.frame->width<<" "<<s.frame->height<<"\n255\n";for(size_t i=0;i<s.frame->rgba.size();i+=4)out.write(reinterpret_cast<const char*>(s.frame->rgba.data()+i),3);out.close();
    Stop();Check(Wait([](auto& t){return !t.running&&!t.starting;}),"stop joined");Sleep(50);
    Start(dataRoot);Check(Wait([](auto& t){return t.status.ready!=0;},25000),"restart after shutdown");Shutdown();
    puts("PASS: sandboxed Chromium, HTTP, OSR, mouse, keys, IME, tabs, resize, hide/resume, shutdown and restart");
}
