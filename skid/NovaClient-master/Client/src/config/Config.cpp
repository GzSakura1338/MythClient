#include "config/Config.h"
#include "proxy/ProxyService.h"
#include "config/HostConfig.h"
#include "config/PropertyCodec.h"
#include "core/Instance.h"
#include "ui/overlay/WindowPlacement.h"
#include <atomic>
#include <cctype>
#include <fstream>
#include <iterator>
#include <system_error>
#define NOMINMAX
#include <windows.h>

namespace nova::config {
namespace {
struct Context {
    HostConfig host;
    bool installed=false;
    const std::vector<Properties::PropDesc>* clientRows=nullptr;
};
Context& State() {return Instance<Context>([]{return new Context();});}
}
bool InstallHostConfig(HostConfig host) {
    auto& state=State();
    if(state.installed || host.directory.empty())return false;
    state.host=std::move(host);state.installed=true;return true;
}
void SetClientSettings(const std::vector<Properties::PropDesc>* rows) {State().clientRows=rows;}
std::filesystem::path ConfigDirectory() {return State().host.directory;}
std::string SanitizeConfigName(std::string_view name) {
    std::string out;
    for(char c:name) {
        if(std::isalnum(static_cast<unsigned char>(c)) || c==' ' || c=='-' || c=='_')out.push_back(c);
        else if(c=='.' && !out.ends_with(".json"))out.push_back(c);
    }
    while(!out.empty() && (out.front()==' ' || out.front()=='.'))out.erase(out.begin());
    while(!out.empty() && (out.back()==' ' || out.back()=='.'))out.pop_back();
    auto lower=out;for(char& c:lower)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if(lower.ends_with(".json"))out.erase(out.size()-5);
    if(out.empty() || out=="." || out=="..")return {};
    return out;
}
Config::Config(std::string name,std::filesystem::path file):name_(std::move(name)),file_(std::move(file)) {}
void Config::setName(std::string name) {name_=std::move(name);data_["name"]=name_;}
void Config::Save() {
    auto root=json::Value::Object();root["name"]=name_;
    const auto& state=State();
    if(state.host.save)state.host.save(root);
    if(state.clientRows)root["client"]=SaveRows(*state.clientRows);
    root["windows"]=ui::SaveWindowPlacements();
    root["proxy"]=proxy::Save();
    data_=std::move(root);
}
void Config::Load() const {
    proxy::Load(data_.Find("proxy"));
    if(const auto* windows=data_.Find("windows"))ui::LoadWindowPlacements(*windows);
    const auto& state=State();
    if(state.host.load)state.host.load(data_);
    if(state.clientRows)if(const auto* client=data_.Find("client"))LoadRows(*client,*state.clientRows);
}
bool Config::Read() {
    std::ifstream in(file_,std::ios::binary);if(!in)return false;
    const std::string text((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    const auto parsed=json::Parse(text);if(!parsed || !parsed->isObject())return false;
    data_=*parsed;return true;
}
bool Config::Write() const {
    std::error_code error;
    if(!file_.parent_path().empty())std::filesystem::create_directories(file_.parent_path(),error);
    if(error)return false;
    // A unique sibling keeps failed or overlapping writes from truncating the
    // current valid file. MoveFileEx publishes only a fully flushed document.
    static std::atomic<unsigned> serial{0};
    auto temporary=file_;
    temporary+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(serial.fetch_add(1));
    const auto text=data_.Dump();
    HANDLE out=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(out==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;
    const bool saved=text.size()<=MAXDWORD && WriteFile(out,text.data(),static_cast<DWORD>(text.size()),&written,nullptr) &&
        written==text.size() && FlushFileBuffers(out);
    CloseHandle(out);
    if(saved && MoveFileExW(temporary.c_str(),file_.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return true;
    DeleteFileW(temporary.c_str());return false;
}
bool Config::WriteAs(std::string name,std::filesystem::path file) const {
    Config copy=*this;
    copy.setName(std::move(name));copy.file_=std::move(file);
    return copy.Write();
}
}
