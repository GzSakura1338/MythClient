#include "minecraft/accounts/AccountManager.h"
#include "minecraft/accounts/lunar/LunarAccountAdapter.h"
#include "minecraft/accounts/AccountValidation.h"
#include "minecraft/accounts/AccountPortraits.h"
#include "minecraft/accounts/MicrosoftAuth.h"
#include "minecraft/accounts/CredentialStore.h"
#include "minecraft/events/GameEvents.h"
#include "contracts/AccountService.h"
#include "core/Instance.h"
#include "utils/Json.h"
#include <shlobj.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <thread>

namespace nova::minecraft::accounts {
namespace {
using namespace nova::accounts;
struct Record { Account account; std::string token,clientId,refresh; };
struct State {
    std::mutex mutex;
    event::Subscription tick;
    std::vector<Record> local;
    std::set<std::string> hidden;
    std::atomic<bool> stop{false},done{true},cancel{false};
    std::thread worker;
    Prepared prepared;
    bool active=false;
    std::string status,clientId;
    std::mutex progressMutex;
    std::string progress;
    ULONGLONG nextPoll=0;
};
State& Data(){return Instance<State>();}
void Wipe(std::string& value){if(!value.empty())SecureZeroMemory(value.data(),value.size());value.clear();}
std::filesystem::path MetadataFile() {
    PWSTR folder=nullptr;if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&folder)))return {};
    auto path=std::filesystem::path(folder)/L"Nova"/L"Accounts";CoTaskMemFree(folder);
    std::error_code error;std::filesystem::create_directories(path,error);if(error)return {};return path/L"metadata.json";
}
void Load(State& s) {
    const auto path=MetadataFile();std::error_code ec;
    if(path.empty()||std::filesystem::file_size(path,ec)>3*1024*1024||ec)return;
    std::ifstream file(path,std::ios::binary);std::string data((std::istreambuf_iterator<char>(file)),{});
    const auto root=json::Parse(data);if(!root)return;
    if(auto id=root->Find("clientId");id&&microsoft::ValidClientId(id->asString()))s.clientId=id->asString();
    if(auto encrypted=root->Find("microsoft")) {
        auto plain=UnprotectCredentials(encrypted->asString());auto records=json::Parse(plain);Wipe(plain);
        if(records)for(const auto& entry:records->Items()) {
            auto name=entry.Find("name"),uuid=entry.Find("uuid"),id=entry.Find("clientId"),refresh=entry.Find("refresh");
            if(!name||!uuid||!id||!refresh||!ValidName(name->asString())||NormalizeUuid(uuid->asString()).empty()||!microsoft::ValidClientId(id->asString())||!ValidToken(refresh->asString())||s.local.size()>=128)continue;
            Account a;a.uuid=NormalizeUuid(uuid->asString());a.id="microsoft:"+a.uuid;a.name=name->asString();a.source=Source::Microsoft;
            s.local.push_back({a,{},id->asString(),refresh->asString()});
        }
    }
    if(auto names=root->Find("offline"))for(const auto& entry:names->Items()) {
        auto name=entry.Find("name"),uuid=entry.Find("uuid");
        if(!name||!uuid||!ValidName(name->asString())||NormalizeUuid(uuid->asString()).empty()||s.local.size()>=128)continue;
        Account a;a.uuid=NormalizeUuid(uuid->asString());a.id="offline:"+a.uuid;a.name=name->asString();a.source=Source::Offline;
        if(std::none_of(s.local.begin(),s.local.end(),[&](const Record& r){return r.account.id==a.id;}))s.local.push_back({a,{}});
    }
    if(auto hidden=root->Find("hiddenLunar"))for(const auto& id:hidden->Items())if(id.asString().starts_with("lunar:")&&s.hidden.size()<128)s.hidden.insert(id.asString());
}
bool Save(const State& s) {
    const auto path=MetadataFile();if(path.empty())return false;
    auto root=json::Value::Object();auto names=json::Value::Array();auto hidden=json::Value::Array();
    for(const auto& row:s.local)if(row.account.source==Source::Offline){auto entry=json::Value::Object();entry["name"]=row.account.name;entry["uuid"]=row.account.uuid;names.Push(std::move(entry));}
    for(const auto& id:s.hidden)hidden.Push(id);
    root["offline"]=std::move(names);root["hiddenLunar"]=std::move(hidden);
    root["clientId"]=s.clientId;
    auto credentials=json::Value::Array();
    for(const auto& row:s.local)if(row.account.source==Source::Microsoft&&!row.refresh.empty()) {
        auto item=json::Value::Object();item["name"]=row.account.name;item["uuid"]=row.account.uuid;item["clientId"]=row.clientId;item["refresh"]=row.refresh;credentials.Push(std::move(item));
    }
    auto plain=credentials.Dump(0);const auto protectedData=ProtectCredentials(plain);Wipe(plain);
    if(protectedData.empty())return false;root["microsoft"]=protectedData;
    auto tmp=path;tmp+=L".tmp";std::ofstream file(tmp,std::ios::binary|std::ios::trunc);if(!file)return false;
    const auto data=root.Dump();file.write(data.data(),data.size());file.close();if(!file)return false;
    return MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
}
void Launch(State& s,Login login) {
    if(s.worker.joinable())s.worker.join();
    JniAttach attach;if(!attach){Wipe(login.token);Wipe(login.refresh);s.status="Minecraft session unavailable.";return;}
    s.prepared.Release(attach.env);s.prepared={};s.prepared.login=std::move(login);
    s.prepared.expectedUser=CaptureSession(attach.env);s.done=false;s.cancel=false;s.active=true;s.status="Validating and preparing account...";
    {std::lock_guard lock(s.progressMutex);s.progress.clear();}
    try{s.worker=std::thread([&s]{
        try {
            auto& login=s.prepared.login;
            if(login.account.source==Source::Microsoft) {
                auto tokens=microsoft::Login(login.clientId,login.refresh,s.cancel,[&s](const char* text){std::lock_guard lock(s.progressMutex);s.progress=text;});
                Wipe(login.token);Wipe(login.refresh);login.token=std::move(tokens.access);login.refresh=std::move(tokens.refresh);
            }
            Prepare(s.prepared,s.cancel);
        }catch(const std::string& error){s.prepared.error=error;}catch(...){s.prepared.error="Account sign-in failed.";}
        s.done.store(true,std::memory_order_release);
    });}
    catch(...){s.active=false;s.done=true;s.prepared.Release(attach.env);s.status="Could not start account verification.";}
}
void Process(State& s,JNIEnv* env,RuntimeView& runtime) {
    Request request{Action::Select,{}};
    const bool hasRequest=Take(request);
    // Observe a queued cancellation before a completed job can commit.
    if(hasRequest&&request.action==Action::Cancel)s.cancel=true;
    if(s.active){std::lock_guard lock(s.progressMutex);if(!s.progress.empty())s.status=s.progress;}
    if(s.active&&s.done.load(std::memory_order_acquire)) {
        if(s.worker.joinable())s.worker.join();s.active=false;
        auto& p=s.prepared;
        if(!p.error.empty())s.status=p.error;
        else if(s.stop.load()||s.cancel.load())s.status="Account switch cancelled.";
        else if(Commit(env,p)) {
            if(p.login.account.source!=Source::Lunar) {
                auto it=std::find_if(s.local.begin(),s.local.end(),[&](const Record& r){return r.account.id==p.login.account.id || (r.account.source==p.login.account.source&&r.account.name==p.login.account.name);});
                Record row{p.login.account,p.login.account.source==Source::Token||p.login.account.source==Source::Microsoft?p.login.token:std::string{},p.login.clientId,p.login.refresh};
                if(it==s.local.end())s.local.push_back(std::move(row));else{Wipe(it->token);Wipe(it->refresh);*it=std::move(row);}
                s.status=Save(s)?"Account switched.":"Account switched; local metadata could not be saved.";
            }else s.status="Account switched.";
            runtime=Inspect(env);
        } else s.status="Switch cancelled: leave the world or finish the other login, then select the account again.";
        p.Release(env);
    }
    if(!hasRequest)return;
    if(request.action==Action::Cancel){s.status=s.active?"Cancelling sign-in...":"Sign-in cancelled.";Wipe(request.value);return;}
    if(s.active||runtime.lunarBusy){Wipe(request.value);s.status="Wait for the current login to finish.";return;}
    const bool metadataOnly=request.action==Action::Remove||request.action==Action::RestoreHidden||request.action==Action::SetClientId;
    if(!runtime.snapshot.supported||(!metadataOnly&&runtime.snapshot.inWorld)){Wipe(request.value);s.status="Return to the main menu before switching accounts.";return;}
    switch(request.action) {
    case Action::Microsoft:
        if(s.local.size()>=128){s.status="Remove a saved account first.";break;}
        {Login login;login.account.source=Source::Microsoft;login.clientId=microsoft::ResolveClientId(s.clientId);login.expectedUuid=runtime.currentUuid;Launch(s,std::move(login));}break;
    case Action::SetClientId:
        if(!request.value.empty()&&!microsoft::ValidClientId(request.value)){s.status="Enter a valid Microsoft application Client ID.";break;}
        s.clientId=request.value;s.status=Save(s)?"Microsoft Client ID saved.":"Could not save Client ID.";break;
    case Action::Cancel:break;
    case Action::Offline:
        if(!ValidName(request.value)){s.status="Use 3-16 letters, numbers or underscores.";break;}
        if(s.local.size()>=128){s.status="Remove a saved account first (128 account limit).";break;}
        {Login login;login.account.name=request.value;login.account.source=Source::Offline;login.expectedUuid=runtime.currentUuid;Launch(s,std::move(login));}break;
    case Action::Token:
        if(!ValidToken(request.value)){s.status="Paste a valid Minecraft access token (no spaces or line breaks).";break;}
        if(s.local.size()>=128){s.status="Remove a saved account first (128 account limit).";break;}
        {Login login;login.account.source=Source::Token;login.token=std::move(request.value);login.expectedUuid=runtime.currentUuid;Launch(s,std::move(login));}break;
    case Action::Select:
        {auto saved=std::find_if(s.local.begin(),s.local.end(),[&](const Record& r){return r.account.id==request.value||(request.value.starts_with("lunar:")&&r.account.source==Source::Microsoft&&r.account.uuid==request.value.substr(6));});
        if(saved!=s.local.end()){Launch(s,Login{saved->account,saved->token,runtime.currentUuid,saved->clientId,saved->refresh});break;}}
        if(request.value.starts_with("lunar:")) {
            auto it=std::find_if(runtime.snapshot.rows.begin(),runtime.snapshot.rows.end(),[&](const Account& r){return r.id==request.value;});
            if(it==runtime.snapshot.rows.end()){s.status="Account no longer exists. Refresh the list.";break;}
            if(it->invalid){
                Login login;login.account=*it;login.account.id="microsoft:"+it->uuid;login.account.source=Source::Microsoft;login.account.invalid=false;login.clientId=microsoft::ResolveClientId(s.clientId);login.expectedUuid=runtime.currentUuid;Launch(s,std::move(login));
            }
            else Launch(s,ReadLunarLogin(env,it->id,runtime.currentUuid));
        }else {
            auto it=std::find_if(s.local.begin(),s.local.end(),[&](const Record& r){return r.account.id==request.value;});
            if(it!=s.local.end())Launch(s,Login{it->account,it->token,runtime.currentUuid});
        }break;
    case Action::Remove:
        if(request.value.starts_with("lunar:"))s.hidden.insert(request.value);
        else {for(auto& r:s.local)if(r.account.id==request.value){Wipe(r.token);Wipe(r.refresh);}std::erase_if(s.local,[&](const Record& r){return r.account.id==request.value;});}
        s.status=Save(s)?"Removed from Nova. The current session is unchanged.":"Removed for this session; metadata could not be saved.";break;
    case Action::RestoreHidden:s.hidden.clear();s.status=Save(s)?"Lunar accounts restored.":"Restored for this session; metadata could not be saved.";break;
    }
    Wipe(request.value);
}
void Tick(event::GameTickEvent& event) {
    if(event.type!=event::EventType::Pre)return;
    auto& s=Data();if(s.stop.load())return;std::lock_guard lock(s.mutex);if(s.stop.load())return;
    const auto now=GetTickCount64();if(now<s.nextPoll)return;s.nextPoll=now+250;
    JniAttach attach;if(!attach)return;
    try {
        auto runtime=Inspect(attach.env);Process(s,attach.env,runtime);
        auto& snapshot=runtime.snapshot;
        std::erase_if(snapshot.rows,[&](const Account& r){return s.hidden.contains(r.id);});
        for(const auto& row:s.local){auto a=row.account;a.current=!a.uuid.empty()&&a.uuid==runtime.currentUuid;
            if(a.source==Source::Microsoft)std::erase_if(snapshot.rows,[&](const Account& original){return original.source==Source::Lunar&&original.uuid==a.uuid;});
            snapshot.rows.push_back(std::move(a));}
        if(!snapshot.currentName.empty()&&std::none_of(snapshot.rows.begin(),snapshot.rows.end(),[](const Account& a){return a.current;}))snapshot.rows.push_back({"current:"+runtime.currentUuid,snapshot.currentName,runtime.currentUuid,Source::Current,true});
        std::stable_sort(snapshot.rows.begin(),snapshot.rows.end(),[](const Account& a,const Account& b){return a.current!=b.current?a.current>b.current:a.name<b.name;});
        portraits::Update(snapshot);
        snapshot.busy=s.active||runtime.lunarBusy;
        snapshot.clientId=s.clientId;snapshot.authorizing=s.active&&s.prepared.login.account.source==Source::Microsoft;
        snapshot.hasHidden=!s.hidden.empty();
        if(snapshot.supported)snapshot.status=runtime.lunarBusy?"Lunar is updating accounts. Please wait.":s.status;
        Publish(std::move(snapshot));
    }catch(const std::string& error){s.status=error;auto view=Read();view.busy=s.active;view.status=s.status;Publish(std::move(view));}
    catch(...){if(attach.env->ExceptionCheck())attach.env->ExceptionClear();s.status="Account operation failed. The client mapping or session services are unavailable.";auto view=Read();view.busy=s.active;view.status=s.status;Publish(std::move(view));
        if(!s.active&&s.done.load())s.prepared.Release(attach.env);}
}
}
void Start(){auto& s=Data();Load(s);s.tick=event::EventBus::Get().Subscribe<event::GameTickEvent>(Tick,event::Priority::High);}
void BeginShutdown(){auto& s=Data();s.stop=true;s.cancel=true;Stop();portraits::Stop();std::lock_guard lock(s.mutex);s.tick.Reset();}
bool WaitStopped(unsigned milliseconds) {
    if(!portraits::WaitStopped(milliseconds))return false;
    auto& s=Data();const auto deadline=GetTickCount64()+milliseconds;
    while(!s.done.load(std::memory_order_acquire)&&GetTickCount64()<deadline)Sleep(10);
    if(!s.done.load(std::memory_order_acquire))return false;
    std::lock_guard lock(s.mutex);if(s.worker.joinable())s.worker.join();JniAttach attach;if(!attach)return false;
    s.prepared.Release(attach.env);for(auto& record:s.local){Wipe(record.token);Wipe(record.refresh);}s.local.clear();return true;
}
}
