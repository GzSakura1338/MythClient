#include <windows.h>
#include <shellapi.h>
#include <dispatcherqueue.h>
#include <d3d11.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <windows.ui.composition.interop.h>
#include <wrl.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <vector>
#include "chrome/ipc/Protocol.h"

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;
using namespace winrt;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
namespace nova::chrome {
namespace {
std::wstring Take(LPWSTR value){std::wstring result=value?value:L"";CoTaskMemFree(value);return result;}
void Check(HRESULT hr){check_hresult(hr);}
std::wstring Json(std::wstring_view value){std::wstring result=L"\"";for(wchar_t c:value){if(c==L'\\'||c==L'\"'){result+=L'\\';result+=c;}else if(c<32){wchar_t escaped[7]{};swprintf_s(escaped,L"\\u%04x",unsigned(c));result+=escaped;}else result+=c;}return result+L'\"';}
struct Tab {
    ComPtr<ICoreWebView2CompositionController> composition;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> web;
    ContainerVisual visual{nullptr};
    std::wstring title=L"Chrome",error;
    bool loading{};
};
class Host {
public:
    HWND window{};
    Shared* shared{};
    HANDLE mutex{},parent{};
    ComPtr<ICoreWebView2Environment> environment;
    winrt::Windows::System::DispatcherQueueController dispatcher{nullptr};
    Compositor compositor{nullptr};
    ContainerVisual root{nullptr};
    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget target{nullptr};
    GraphicsCaptureItem item{nullptr};
    Direct3D11CaptureFramePool pool{nullptr};
    GraphicsCaptureSession session{nullptr};
    IDirect3DDevice device{nullptr};
    ComPtr<ID3D11Device> d3d;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Texture2D> staging;
    std::map<int,Tab> tabs;
    int selected{},nextId=1,pending{},width=800,height=600,fps=30;
    double zoom=1;
    bool visible{},audio{},focused{},closing{};
    uint64_t lastFrame{};
    std::set<int> keys;
    bool buttons[3]{};
    POINT mouse{};
    Status state{};
    std::wstring fatal;

    Tab* Current(){auto it=tabs.find(selected);return it==tabs.end()?nullptr:&it->second;}
    void Fail(HRESULT hr,const wchar_t* operation){fatal=std::wstring(operation)+L" ("+std::to_wstring(uint32_t(hr))+L")";}
    void InitGraphics(){
        DispatcherQueueOptions options{sizeof(options),DQTYPE_THREAD_CURRENT,DQTAT_COM_STA};
        Check(CreateDispatcherQueueController(options,reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(put_abi(dispatcher))));
        compositor=Compositor();root=compositor.CreateContainerVisual();root.Size({float(width),float(height)});
        auto desktop=compositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();
        Check(desktop->CreateDesktopWindowTarget(window,FALSE,reinterpret_cast<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget**>(put_abi(target))));
        target.Root(root);
        Check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&d3d,nullptr,&context));
        ComPtr<IDXGIDevice> dxgi;Check(d3d.As(&dxgi));
        Check(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(),reinterpret_cast<IInspectable**>(put_abi(device))));
        item=GraphicsCaptureItem::CreateFromVisual(root);
        pool=Direct3D11CaptureFramePool::Create(device,DirectXPixelFormat::B8G8R8A8UIntNormalized,2,{width,height});
        session=pool.CreateCaptureSession(item);session.IsCursorCaptureEnabled(false);session.StartCapture();
    }
    void Resize(int w,int h){
        if(!ValidSize(w,h))return;width=w;height=h;
        SetWindowPos(window,nullptr,0,0,w,h,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
        root.Size({float(w),float(h)});
        for(auto& [id,t]:tabs){t.visual.Size({float(w),float(h)});t.controller->put_Bounds({0,0,w,h});}
        pool.Recreate(device,DirectXPixelFormat::B8G8R8A8UIntNormalized,2,{w,h});staging.Reset();
    }
    void Visibility(){
        for(auto& [id,t]:tabs){t.visual.IsVisible(visible&&id==selected);t.controller->put_IsVisible(visible&&id==selected);
            ComPtr<ICoreWebView2_8> sound;if(SUCCEEDED(t.web.As(&sound)))sound->put_IsMuted(!audio&&(!visible||id!=selected));}
    }
    HWND InputWindow(){
        if(auto t=Current()){t->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);}
        HWND target=GetFocus();return target&&target!=window?target:nullptr;
    }
    void Key(UINT message,WPARAM key,LPARAM data){
        if(HWND target=InputWindow()){DWORD_PTR result{};SendMessageTimeoutW(target,message,key,data,SMTO_ABORTIFHUNG,100,&result);}
    }
    void DevInput(const wchar_t* method,const std::wstring& json){if(auto t=Current())t->web->CallDevToolsProtocolMethod(method,json.c_str(),nullptr);}
    void LoseFocus(){
        if(auto t=Current()){
            for(int k:keys)Key(WM_KEYUP,k,0xC0000001);
            const COREWEBVIEW2_MOUSE_EVENT_KIND up[]={COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_UP,COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_UP,COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_UP};
            for(int i=0;i<3;++i)if(buttons[i])t->composition->SendMouseInput(up[i],COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_NONE,0,mouse);
        }
        keys.clear();std::fill(std::begin(buttons),std::end(buttons),false);focused=false;SetFocus(window);
    }
    void Select(int id){if(!tabs.contains(id))return;LoseFocus();selected=id;Visibility();}
    void CloseTab(int id){auto it=tabs.find(id);if(it==tabs.end())return;if(id==selected)LoseFocus();root.Children().Remove(it->second.visual);it->second.controller->Close();tabs.erase(it);if(selected==id)selected=tabs.empty()?0:tabs.begin()->first;Visibility();}
    void New(std::wstring url=L"about:blank"){
        if(closing||tabs.size()+pending>=MaxTabs||!environment)return;
        ComPtr<ICoreWebView2Environment3> env3;Check(environment.As(&env3));++pending;
        auto hr=env3->CreateCoreWebView2CompositionController(window,Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
            [this,url](HRESULT result,ICoreWebView2CompositionController* controller)->HRESULT{
                --pending;if(closing)return S_OK;
                if(FAILED(result)||!controller){Fail(result,L"Create browser view");return S_OK;}
                try{
                    const int id=nextId++;auto& t=tabs[id];t.composition=controller;Check(t.composition.As(&t.controller));Check(t.controller->get_CoreWebView2(&t.web));
                    t.visual=compositor.CreateContainerVisual();t.visual.Size({float(width),float(height)});root.Children().InsertAtTop(t.visual);
                    Check(t.composition->put_RootVisualTarget(reinterpret_cast<IUnknown*>(get_abi(t.visual))));
                    Check(t.controller->put_Bounds({0,0,width,height}));t.controller->put_ZoomFactor(zoom);
                    ComPtr<ICoreWebView2Controller3> scale;if(SUCCEEDED(t.controller.As(&scale))){scale->put_ShouldDetectMonitorScaleChanges(FALSE);scale->put_RasterizationScale(1);scale->put_BoundsMode(COREWEBVIEW2_BOUNDS_MODE_USE_RAW_PIXELS);}
                    ComPtr<ICoreWebView2Settings> settings;t.web->get_Settings(&settings);settings->put_AreDefaultContextMenusEnabled(FALSE);settings->put_AreDevToolsEnabled(FALSE);settings->put_IsStatusBarEnabled(FALSE);
                    EventRegistrationToken token{};
                    t.web->add_DocumentTitleChanged(Callback<ICoreWebView2DocumentTitleChangedEventHandler>([this,id](ICoreWebView2* web,IUnknown*)->HRESULT{if(tabs.contains(id)){LPWSTR value{};web->get_DocumentTitle(&value);tabs[id].title=Take(value);}return S_OK;}).Get(),&token);
                    t.web->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>([this,id](ICoreWebView2*,ICoreWebView2NavigationStartingEventArgs* args)->HRESULT{LPWSTR value{};args->get_Uri(&value);auto url=Take(value);if(!AllowedUrl(url))args->put_Cancel(TRUE);else if(tabs.contains(id)){tabs[id].loading=true;tabs[id].error.clear();}return S_OK;}).Get(),&token);
                    t.web->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>([this,id](ICoreWebView2*,ICoreWebView2NavigationCompletedEventArgs* args)->HRESULT{if(tabs.contains(id)){tabs[id].loading=false;BOOL success{};args->get_IsSuccess(&success);if(!success){COREWEBVIEW2_WEB_ERROR_STATUS error{};args->get_WebErrorStatus(&error);if(error!=COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED)tabs[id].error=L"Navigation failed ("+std::to_wstring(error)+L")";}}return S_OK;}).Get(),&token);
                    t.web->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>([this](ICoreWebView2*,ICoreWebView2NewWindowRequestedEventArgs* args)->HRESULT{args->put_Handled(TRUE);BOOL gesture{};args->get_IsUserInitiated(&gesture);LPWSTR value{};args->get_Uri(&value);auto url=Take(value);if(gesture&&AllowedUrl(url))New(url);return S_OK;}).Get(),&token);
                    t.web->add_PermissionRequested(Callback<ICoreWebView2PermissionRequestedEventHandler>([](ICoreWebView2*,ICoreWebView2PermissionRequestedEventArgs* args)->HRESULT{args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);return S_OK;}).Get(),&token);
                    t.web->add_ProcessFailed(Callback<ICoreWebView2ProcessFailedEventHandler>([this,id](ICoreWebView2*,ICoreWebView2ProcessFailedEventArgs*)->HRESULT{if(tabs.contains(id))tabs[id].error=L"This tab stopped. Reload to recover.";return S_OK;}).Get(),&token);
                    t.composition->add_CursorChanged(Callback<ICoreWebView2CursorChangedEventHandler>([this,id](ICoreWebView2CompositionController* sender,IUnknown*)->HRESULT{if(id==selected){UINT cursor{};sender->get_SystemCursorId(&cursor);state.cursor=cursor==32513?1:cursor==32649?2:0;}return S_OK;}).Get(),&token);
                    ComPtr<ICoreWebView2_4> downloads;if(SUCCEEDED(t.web.As(&downloads)))downloads->add_DownloadStarting(Callback<ICoreWebView2DownloadStartingEventHandler>([](ICoreWebView2*,ICoreWebView2DownloadStartingEventArgs* args)->HRESULT{args->put_Cancel(TRUE);args->put_Handled(TRUE);return S_OK;}).Get(),&token);
                    Select(id);Check(t.web->Navigate(url.c_str()));
                }catch(const hresult_error& e){Fail(e.code(),L"Initialize browser view");}return S_OK;
            }).Get());
        if(FAILED(hr)){--pending;Fail(hr,L"Create browser view");}
    }
    void CommandReceived(const Command& c){
        if(c.op==Op::Shutdown){closing=true;PostQuitMessage(0);return;}
        if(c.op==Op::NewTab){New();return;}if(c.op==Op::SelectTab){Select(c.a);return;}if(c.op==Op::CloseTab){CloseTab(c.a);return;}
        if(c.op==Op::Resize){Resize(c.a,c.b);return;}if(c.op==Op::Visible){visible=c.a!=0;if(!visible)LoseFocus();Visibility();return;}
        if(c.op==Op::BackgroundAudio){audio=c.a!=0;Visibility();return;}if(c.op==Op::Fps){fps=std::clamp(c.a,15,60);return;}
        if(c.op==Op::Zoom){zoom=std::clamp(c.a,50,200)/100.0;for(auto& [id,t]:tabs)t.controller->put_ZoomFactor(zoom);return;}
        auto t=Current();if(!t)return;const std::wstring text(c.text,std::find(c.text,c.text+2048,L'\0'));
        auto flags=COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_NONE;
        if(c.modifiers&2)flags=static_cast<COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS>(flags|COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_SHIFT);
        if(c.modifiers&4)flags=static_cast<COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS>(flags|COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_CONTROL);
        if(c.op==Op::MouseButton&&c.c>=0&&c.c<3)buttons[c.c]=!(c.d&1);
        const COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS held[]={COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_LEFT_BUTTON,COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_MIDDLE_BUTTON,COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_RIGHT_BUTTON};
        for(int i=0;i<3;++i)if(buttons[i])flags=static_cast<COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS>(flags|held[i]);
        switch(c.op){
        case Op::Navigate:if(AllowedUrl(text))t->web->Navigate(text.c_str());break;
        case Op::Back:t->web->GoBack();break;case Op::Forward:t->web->GoForward();break;case Op::Reload:t->web->Reload();break;
        case Op::Focus:if(c.a){focused=true;InputWindow();}else LoseFocus();break;
        case Op::MouseMove:mouse={c.a,c.b};t->composition->SendMouseInput(c.c?COREWEBVIEW2_MOUSE_EVENT_KIND_LEAVE:COREWEBVIEW2_MOUSE_EVENT_KIND_MOVE,flags,0,mouse);break;
        case Op::MouseButton:if(c.c>=0&&c.c<3){mouse={c.a,c.b};buttons[c.c]=!(c.d&1);const UINT down[]={WM_LBUTTONDOWN,WM_MBUTTONDOWN,WM_RBUTTONDOWN};const UINT up[]={WM_LBUTTONUP,WM_MBUTTONUP,WM_RBUTTONUP};const UINT dbl[]={WM_LBUTTONDBLCLK,WM_MBUTTONDBLCLK,WM_RBUTTONDBLCLK};t->composition->SendMouseInput(static_cast<COREWEBVIEW2_MOUSE_EVENT_KIND>((c.d&1)?up[c.c]:(c.d&2)?dbl[c.c]:down[c.c]),flags,0,mouse);}break;
        case Op::Wheel:mouse={c.a,c.b};if(c.d)t->composition->SendMouseInput(COREWEBVIEW2_MOUSE_EVENT_KIND_WHEEL,flags,c.d,mouse);if(c.c)t->composition->SendMouseInput(COREWEBVIEW2_MOUSE_EVENT_KIND_HORIZONTAL_WHEEL,flags,c.c,mouse);break;
        case Op::Key:if(c.c)keys.erase(c.a);else keys.insert(c.a);Key(c.d?(c.c?WM_SYSKEYUP:WM_SYSKEYDOWN):(c.c?WM_KEYUP:WM_KEYDOWN),c.a,c.b);break;
        case Op::Character:Key(WM_CHAR,c.a,c.b);break;
        case Op::Ime:DevInput(L"Input.imeSetComposition",L"{\"text\":"+Json(text)+L",\"selectionStart\":"+std::to_wstring(std::clamp(c.a,0,int(text.size())))+L",\"selectionEnd\":"+std::to_wstring(std::clamp(c.a,0,int(text.size())))+L"}");break;
        case Op::ImeCommit:DevInput(L"Input.insertText",L"{\"text\":"+Json(text)+L"}");break;
        case Op::ImeCancel:DevInput(L"Input.imeSetComposition",L"{\"text\":\"\",\"selectionStart\":0,\"selectionEnd\":0}");break;
        case Op::Edit:{const int key=c.a==0?'C':c.a==1?'V':'A';const auto tail=L",\"modifiers\":2,\"windowsVirtualKeyCode\":"+std::to_wstring(key)+L"}";DevInput(L"Input.dispatchKeyEvent",L"{\"type\":\"rawKeyDown\""+tail);DevInput(L"Input.dispatchKeyEvent",L"{\"type\":\"keyUp\""+tail);break;}
        default:break;
        }
    }
    void Capture(){
        if(!visible||!Current()){while(pool.TryGetNextFrame()){}return;}
        if(GetTickCount64()-lastFrame<1000u/fps)return;
        auto frame=pool.TryGetNextFrame();if(!frame)return;
        auto size=frame.ContentSize();if(size.Width!=width||size.Height!=height)return;
        ComPtr<ID3D11Texture2D> texture;auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();Check(access->GetInterface(IID_PPV_ARGS(&texture)));
        if(!staging){D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Check(d3d->CreateTexture2D(&desc,nullptr,&staging));}
        context->CopyResource(staging.Get(),texture.Get());D3D11_MAPPED_SUBRESOURCE mapped{};Check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
        DWORD wait=WaitForSingleObject(mutex,0);if(wait==WAIT_OBJECT_0){for(int y=0;y<height;++y)memcpy(shared->pixels+size_t(y)*width*4,static_cast<const unsigned char*>(mapped.pData)+size_t(y)*mapped.RowPitch,size_t(width)*4);shared->width=width;shared->height=height;++shared->sequence;ReleaseMutex(mutex);}else if(wait==WAIT_ABANDONED){ReleaseMutex(mutex);closing=true;PostQuitMessage(1);}
        context->Unmap(staging.Get(),0);lastFrame=GetTickCount64();
    }
    void Poll(){
        if(closing)return;if(WaitForSingleObject(parent,0)!=WAIT_TIMEOUT){closing=true;PostQuitMessage(0);return;}
        std::vector<Command> commands;DWORD wait=WaitForSingleObject(mutex,0);if(wait==WAIT_ABANDONED){ReleaseMutex(mutex);closing=true;PostQuitMessage(1);return;}if(wait!=WAIT_OBJECT_0)return;
        const bool valid=shared->magic==Magic&&shared->version==ProtocolVersion&&shared->write-shared->read<=QueueSize;
        if(valid)while(shared->read!=shared->write)commands.push_back(shared->commands[shared->read++%QueueSize]);ReleaseMutex(mutex);
        if(!valid){closing=true;PostQuitMessage(1);return;}
        for(const auto& c:commands)CommandReceived(c);if(closing)return;
        state.ready=!tabs.empty()||!fatal.empty();state.count=0;state.selected=selected;state.url[0]=state.error[0]=0;state.loading=state.back=state.forward=0;
        for(auto& [id,t]:tabs){auto& info=state.tabs[state.count++];info.id=id;CopyText(info.title,t.title);if(id==selected){LPWSTR url{};t.web->get_Source(&url);CopyText(state.url,Take(url));CopyText(state.error,t.error);state.loading=t.loading;BOOL back{},forward{};t.web->get_CanGoBack(&back);t.web->get_CanGoForward(&forward);state.back=back;state.forward=forward;}}
        if(!fatal.empty())CopyText(state.error,fatal);
        wait=WaitForSingleObject(mutex,0);if(wait==WAIT_OBJECT_0){shared->status=state;ReleaseMutex(mutex);}else if(wait==WAIT_ABANDONED){ReleaseMutex(mutex);closing=true;PostQuitMessage(1);}
        Capture();
    }
    void Close(){closing=true;LoseFocus();for(auto& [id,t]:tabs)t.controller->Close();tabs.clear();if(session)session.Close();if(pool)pool.Close();if(compositor)compositor.Close();}
};
Host* active{};
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM w,LPARAM l){return DefWindowProcW(window,message,w,l);}
void CALLBACK Timer(HWND,UINT,UINT_PTR,DWORD){if(active)try{active->Poll();}catch(const hresult_error& e){active->Fail(e.code(),L"Browser rendering");}}
}
int Run(HINSTANCE instance){
    init_apartment(apartment_type::single_threaded);SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    int argc{};LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::map<std::wstring,std::wstring> args;
    for(int i=1;i<argc;++i){std::wstring value=argv[i];auto equal=value.find(L'=');if(equal!=value.npos)args[value.substr(0,equal)]=value.substr(equal+1);}LocalFree(argv);
    auto channel=args[L"--nova-channel"];if(!channel.starts_with(L"Local\\NovaBrowser-")||channel.size()>100)return 11;
    HANDLE memory=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,channel.c_str());HANDLE mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,(channel+L"-lock").c_str());
    auto shared=memory?static_cast<Shared*>(MapViewOfFile(memory,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared))):nullptr;
    if(!shared||!mutex||shared->magic!=Magic||shared->version!=ProtocolVersion)return 12;
    HANDLE parent=OpenProcess(SYNCHRONIZE,FALSE,wcstoul(args[L"--nova-parent"].c_str(),nullptr,10));if(!parent)return 13;
    int result=0;
    {
        Host host;host.shared=shared;host.mutex=mutex;host.parent=parent;active=&host;
        WNDCLASSW cls{};cls.lpfnWndProc=WindowProc;cls.hInstance=instance;cls.lpszClassName=L"NovaWebViewHost";RegisterClassW(&cls);
        host.window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,cls.lpszClassName,L"Nova Browser",WS_POPUP,-30000,-30000,800,600,nullptr,nullptr,instance,nullptr);
        ShowWindow(host.window,SW_SHOWNOACTIVATE);
        try{
            host.InitGraphics();
            wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);auto runtime=std::filesystem::path(path).parent_path()/L"runtime";
            if(!std::filesystem::exists(runtime/L"msedgewebview2.exe"))throw hresult_error(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
            auto options=Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();options->put_Language(L"zh-CN");options->put_AdditionalBrowserArguments(L"--disable-features=CalculateNativeWinOcclusion");
            Check(CreateCoreWebView2EnvironmentWithOptions(runtime.c_str(),args[L"--nova-profile"].c_str(),options.Get(),Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([&host](HRESULT hr,ICoreWebView2Environment* env)->HRESULT{if(FAILED(hr)||!env)host.Fail(hr,L"Start fixed WebView2 runtime");else{host.environment=env;host.New();}return S_OK;}).Get()));
            const auto timer=SetTimer(nullptr,0,10,Timer);MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}KillTimer(nullptr,timer);
        }catch(const hresult_error& e){CopyText(shared->status.error,e.message().c_str());shared->status.ready=1;result=14;}
        host.Close();active=nullptr;DestroyWindow(host.window);
    }
    UnmapViewOfFile(shared);CloseHandle(memory);CloseHandle(mutex);CloseHandle(parent);uninit_apartment();return result;
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){return nova::chrome::Run(instance);}
