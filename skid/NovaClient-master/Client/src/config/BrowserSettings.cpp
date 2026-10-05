#include "config/BrowserSettings.h"
#include "chrome/BrowserService.h"
#include <shlobj.h>
#include <filesystem>
#include <algorithm>
namespace nova::config {
namespace {std::wstring File(){PWSTR p{};if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&p)))return {};auto dir=std::filesystem::path(p)/L"Nova"/L"Browser";CoTaskMemFree(p);std::error_code e;std::filesystem::create_directories(dir,e);return (dir/L"settings.ini").wstring();}}
BrowserPlacement LoadBrowserPlacement(){BrowserPlacement s;const auto path=File();if(path.empty())return s;
    auto read=[&](const wchar_t* key,int value){return std::clamp(int(GetPrivateProfileIntW(L"Window",key,value,path.c_str())),0,10000);};
    s.x=read(L"X",s.x);s.y=read(L"Y",s.y);s.width=read(L"Width",s.width);s.height=read(L"Height",s.height);return s;}
void SaveBrowserPlacement(const BrowserPlacement& s){const auto path=File();if(path.empty())return;
    auto write=[&](const wchar_t* key,int value){WritePrivateProfileStringW(L"Window",key,std::to_wstring(value).c_str(),path.c_str());};
    write(L"X",s.x);write(L"Y",s.y);write(L"Width",s.width);write(L"Height",s.height);}
BrowserSettings LoadBrowserSettings(){BrowserSettings s;const auto path=File();if(path.empty())return s;wchar_t home[2048]{};GetPrivateProfileStringW(L"Browser",L"Home",L"about:blank",home,2048,path.c_str());s.home=chrome::Utf8(home);s.fps=std::clamp(int(GetPrivateProfileIntW(L"Browser",L"FPS",30,path.c_str())),15,60);s.zoom=std::clamp(int(GetPrivateProfileIntW(L"Browser",L"Zoom",100,path.c_str())),50,200);s.audio=GetPrivateProfileIntW(L"Browser",L"Audio",0,path.c_str())!=0;return s;}
void SaveBrowserSettings(const BrowserSettings& s){const auto path=File();if(path.empty())return;WritePrivateProfileStringW(L"Browser",L"Home",chrome::Wide(s.home).c_str(),path.c_str());WritePrivateProfileStringW(L"Browser",L"FPS",std::to_wstring(s.fps).c_str(),path.c_str());WritePrivateProfileStringW(L"Browser",L"Zoom",std::to_wstring(s.zoom).c_str(),path.c_str());WritePrivateProfileStringW(L"Browser",L"Audio",s.audio?L"1":L"0",path.c_str());}
}
