#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <wincrypt.h>
#include "proxy/ProxyService.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace nova::proxy {
using Clock=std::chrono::steady_clock;
struct Tunnel {
    std::atomic<bool> cancelled{false},done{false};
    SOCKET upstream=INVALID_SOCKET,listener=INVALID_SOCKET;
    std::thread worker;
    ~Tunnel(){if(worker.joinable())worker.join();}
};
namespace {
std::mutex mutex;
Config config;
Status status;
std::atomic<bool> stopping{false},testCancel{false},testRunning{false};
std::thread testWorker;
std::vector<std::shared_ptr<Tunnel>> tunnels;
struct Socket {
    SOCKET value=INVALID_SOCKET;
    Socket()=default;explicit Socket(SOCKET s):value(s){}
    ~Socket(){if(value!=INVALID_SOCKET)closesocket(value);}
    SOCKET release(){const auto s=value;value=INVALID_SOCKET;return s;}
};
void Check(bool ok,const char* error){if(!ok)throw std::runtime_error(error);}
void Publish(std::string text){std::lock_guard lock(mutex);status.message=std::move(text);}
bool Cancelled(const Cancel& c){return stopping.load()||(c&&c());}
void Pending(const Cancel& c,Clock::time_point deadline){
    Check(!Cancelled(c),"Cancelled");Check(Clock::now()<deadline,"Proxy connection timed out");
}
void Ready(SOCKET s,short events,const Cancel& c,Clock::time_point deadline){
    for(;;){Pending(c,deadline);WSAPOLLFD fd{s,events,0};const int n=WSAPoll(&fd,1,50);
        Check(n!=SOCKET_ERROR,"Proxy socket failed");if(n>0){
            Check(!(fd.revents&(POLLERR|POLLNVAL)),"Proxy socket failed");
            if(fd.revents&(events|POLLHUP))return;
        }
    }
}
void Nonblocking(SOCKET s){u_long enabled=1;Check(ioctlsocket(s,FIONBIO,&enabled)==0,"Could not configure proxy socket");}
void Send(SOCKET s,std::string_view bytes,const Cancel& c,Clock::time_point end){
    while(!bytes.empty()){
        Pending(c,end);const int n=send(s,bytes.data(),int(bytes.size()),0);
        if(n>0){bytes.remove_prefix(n);continue;}
        Check(n==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK,"Proxy send failed");Ready(s,POLLWRNORM,c,end);
    }
}
std::string Receive(SOCKET s,size_t length,const Cancel& c,Clock::time_point end){
    std::string out(length,'\0');size_t used=0;
    while(used<length){Pending(c,end);int n=recv(s,out.data()+used,int(length-used),0);
        if(n>0){used+=n;continue;}Check(n!=0,"Proxy closed during handshake");
        Check(WSAGetLastError()==WSAEWOULDBLOCK,"Proxy receive failed");Ready(s,POLLRDNORM,c,end);
    }return out;
}
std::wstring Wide(std::string_view text){
    const int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);
    Check(size>0,"Invalid UTF-8 hostname");std::wstring out(size,0);
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),out.data(),size);return out;
}
Socket* Connect(const Config& cfg,const Cancel& c,Clock::time_point end){
    WSADATA data{};Check(WSAStartup(MAKEWORD(2,2),&data)==0,"WinSock initialization failed");
    struct WinsockScope{~WinsockScope(){WSACleanup();}} scope;
    auto host=Wide(cfg.host),service=std::to_wstring(cfg.port);
    ADDRINFOEXW hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;hints.ai_protocol=IPPROTO_TCP;
    PADDRINFOEXW list=nullptr;OVERLAPPED operation{};operation.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    Check(operation.hEvent!=nullptr,"Could not create resolver event");
    HANDLE request=nullptr;
    int error=GetAddrInfoExW(host.c_str(),service.c_str(),NS_DNS,nullptr,&hints,&list,nullptr,&operation,nullptr,&request);
    if(error==WSA_IO_PENDING){
        while(WaitForSingleObject(operation.hEvent,50)==WAIT_TIMEOUT){
            if(Cancelled(c)||Clock::now()>=end){GetAddrInfoExCancel(&request);WaitForSingleObject(operation.hEvent,INFINITE);break;}
        }
        error=GetAddrInfoExOverlappedResult(&operation);
    }
    CloseHandle(operation.hEvent);
    struct Addresses{PADDRINFOEXW value;~Addresses(){if(value)FreeAddrInfoExW(value);}} addresses{list};
    Pending(c,end);Check(error==0&&list,"Proxy hostname could not be resolved");
    // The caller holds a WinSock reference for the lifetime of returned sockets.
    for(auto at=list;at;at=at->ai_next){
        auto candidate=std::make_unique<Socket>(socket(at->ai_family,SOCK_STREAM,IPPROTO_TCP));
        if(candidate->value==INVALID_SOCKET)continue;
        Nonblocking(candidate->value);
        const int result=connect(candidate->value,at->ai_addr,int(at->ai_addrlen));
        if(result!=0){if(WSAGetLastError()!=WSAEWOULDBLOCK)continue;
            try{Ready(candidate->value,POLLWRNORM,c,end);}catch(...){Pending(c,end);continue;}
            int failure=0,len=sizeof(failure);
            if(getsockopt(candidate->value,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&failure),&len)!=0||failure)continue;
        }
        int enabled=1;setsockopt(candidate->value,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<char*>(&enabled),sizeof(enabled));
        return candidate.release();
    }
    throw std::runtime_error("Could not connect to proxy");
}
std::string Base64(std::string_view input){
    constexpr char table[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;for(size_t i=0;i<input.size();i+=3){
        unsigned n=unsigned(static_cast<unsigned char>(input[i]))<<16;
        if(i+1<input.size())n|=unsigned(static_cast<unsigned char>(input[i+1]))<<8;
        if(i+2<input.size())n|=static_cast<unsigned char>(input[i+2]);
        out+=table[n>>18];out+=table[(n>>12)&63];out+=i+1<input.size()?table[(n>>6)&63]:'=';out+=i+2<input.size()?table[n&63]:'=';
    }return out;
}
bool CleanHost(std::string_view host){
    return !host.empty()&&host.size()<=253&&std::none_of(host.begin(),host.end(),[](unsigned char c){return c<=32||c==127||c=='/'||c=='@'||c=='['||c==']';});
}
void Handshake(SOCKET s,const Config& cfg,const std::string& target,int port,const Cancel& c,Clock::time_point end){
    Check(CleanHost(target)&&port>0&&port<=65535,"Invalid target address");
    IN_ADDR v4{};IN6_ADDR v6{};
    const bool ipv4=InetPtonA(AF_INET,target.c_str(),&v4)==1,ipv6=InetPtonA(AF_INET6,target.c_str(),&v6)==1;
    if(cfg.protocol==Protocol::Http){
        const auto authority=(ipv6?"["+target+"]":target)+":"+std::to_string(port);
        auto request="CONNECT "+authority+" HTTP/1.1\r\nHost: "+authority+"\r\n";
        if(cfg.authentication)request+="Proxy-Authorization: Basic "+Base64(cfg.username+":"+cfg.password)+"\r\n";
        request+="\r\n";Send(s,request,c,end);SecureZeroMemory(request.data(),request.size());
        for(int interim=0;interim<5;++interim){
            // Read exactly through the header; application bytes remain in the
            // socket even when the proxy coalesces them with its success reply.
            std::string header;while(!header.ends_with("\r\n\r\n")){
                Check(header.size()<32768,"Proxy response header too large");header+=Receive(s,1,c,end);
            }
            const auto lineEnd=header.find("\r\n"),space=header.find(' ');
            Check((header.starts_with("HTTP/1.1 ")||header.starts_with("HTTP/1.0 "))&&space+4<=lineEnd,"Invalid HTTP proxy response");
            const auto digits=header.substr(space+1,3);Check(std::all_of(digits.begin(),digits.end(),[](char ch){return ch>='0'&&ch<='9';}),"Invalid HTTP proxy status");
            const int code=std::stoi(digits);
            if(code>=200&&code<300)return;
            if(code>=100&&code<200)continue;
            if(code==407)throw std::runtime_error("Proxy authentication failed (407)");
            if(code==403)throw std::runtime_error("Proxy refused target (403)");
            throw std::runtime_error("HTTP proxy refused tunnel ("+std::to_string(code)+")");
        }throw std::runtime_error("Too many interim proxy responses");
    }
    Send(s,cfg.authentication?std::string("\5\1\2",3):std::string("\5\1\0",3),c,end);
    auto method=Receive(s,2,c,end);
    Check(method[0]==5&&static_cast<unsigned char>(method[1])==(cfg.authentication?2:0),"SOCKS5 authentication method rejected");
    if(cfg.authentication){
        auto auth=std::string(1,'\1')+char(cfg.username.size())+cfg.username+char(cfg.password.size())+cfg.password;
        Send(s,auth,c,end);SecureZeroMemory(auth.data(),auth.size());const auto answer=Receive(s,2,c,end);
        Check(answer[0]==1&&answer[1]==0,"SOCKS5 authentication failed");
    }
    std::string connectRequest("\5\1\0",3);
    if(ipv4){connectRequest+='\1';connectRequest.append(reinterpret_cast<char*>(&v4),4);}
    else if(ipv6){connectRequest+='\4';connectRequest.append(reinterpret_cast<char*>(&v6),16);}
    else{connectRequest+='\3';connectRequest+=char(target.size());connectRequest+=target;}
    connectRequest+=char(port>>8);connectRequest+=char(port&255);Send(s,connectRequest,c,end);
    const auto reply=Receive(s,4,c,end);Check(reply[0]==5&&reply[2]==0,"Invalid SOCKS5 response");
    const auto failure=static_cast<unsigned char>(reply[1]);
    if(failure){
        constexpr const char* reasons[]{"","general failure","target forbidden","network unreachable","host unreachable",
            "connection refused","TTL expired","CONNECT unsupported","address type unsupported"};
        throw std::runtime_error("SOCKS5 "+std::string(failure<9?reasons[failure]:"unknown failure")+" (REP="+std::to_string(failure)+")");
    }
    const unsigned type=static_cast<unsigned char>(reply[3]);size_t size=type==1?4:type==4?16:0;
    if(type==3)size=static_cast<unsigned char>(Receive(s,1,c,end)[0]);
    Check(size>0,"Invalid SOCKS5 bound address");Receive(s,size+2,c,end);
}
bool SameProcess(SOCKET accepted){
    sockaddr_in peer{},local{};int n=sizeof(peer);
    if(getpeername(accepted,reinterpret_cast<sockaddr*>(&peer),&n)!=0)return false;
    n=sizeof(local);if(getsockname(accepted,reinterpret_cast<sockaddr*>(&local),&n)!=0)return false;
    ULONG size=0;GetExtendedTcpTable(nullptr,&size,FALSE,AF_INET,TCP_TABLE_OWNER_PID_ALL,0);
    std::vector<unsigned char> bytes(size);
    if(GetExtendedTcpTable(bytes.data(),&size,FALSE,AF_INET,TCP_TABLE_OWNER_PID_ALL,0)!=NO_ERROR)return false;
    const auto* table=reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(bytes.data());
    for(DWORD i=0;i<table->dwNumEntries;++i){const auto& row=table->table[i];
        if(row.dwLocalAddr==peer.sin_addr.s_addr&&row.dwRemoteAddr==local.sin_addr.s_addr&&
           row.dwLocalPort==peer.sin_port&&row.dwRemotePort==local.sin_port&&row.dwOwningPid==GetCurrentProcessId())return true;
    }return false;
}
void Relay(const std::shared_ptr<Tunnel>& t){
    struct WinsockScope{~WinsockScope(){WSACleanup();}} scope;
    Socket upstream(t->upstream),listener(t->listener),client;
    auto cancel=[&]{return t->cancelled.load();};
    try{
        auto deadline=Clock::now()+std::chrono::seconds(3);
        while(client.value==INVALID_SOCKET){Ready(listener.value,POLLRDNORM,cancel,deadline);
            Socket next(accept(listener.value,nullptr,nullptr));if(next.value==INVALID_SOCKET)continue;
            if(!SameProcess(next.value))continue;Nonblocking(next.value);client.value=next.release();
        }
        closesocket(listener.release());Publish("Connected through proxy");
        struct Pipe{std::string bytes;size_t sent=0;bool eof=false,closed=false;};
        Pipe outgoing,incoming;
        auto step=[](SOCKET source,SOCKET dest,Pipe& p){
            if(!p.eof&&p.bytes.size()-p.sent<256*1024){char data[32768];int n=recv(source,data,sizeof(data),0);
                if(n>0)p.bytes.append(data,n);else if(n==0)p.eof=true;
                else Check(WSAGetLastError()==WSAEWOULDBLOCK,"Proxy relay receive failed");
            }
            if(p.sent<p.bytes.size()){int n=send(dest,p.bytes.data()+p.sent,int(p.bytes.size()-p.sent),0);
                if(n>0)p.sent+=n;else Check(n==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK,"Proxy relay send failed");
            }
            if(p.sent==p.bytes.size()){p.bytes.clear();p.sent=0;
                if(p.eof&&!p.closed){shutdown(dest,SD_SEND);p.closed=true;}
            }else if(p.sent>32768){p.bytes.erase(0,p.sent);p.sent=0;}
        };
        auto closedAt=Clock::time_point{};
        while(!Cancelled(cancel)){
            step(client.value,upstream.value,outgoing);step(upstream.value,client.value,incoming);
            if(outgoing.closed&&incoming.closed)break;
            if(outgoing.eof||incoming.eof){if(closedAt==Clock::time_point{})closedAt=Clock::now();
                if(Clock::now()-closedAt>std::chrono::seconds(2))break;
            }
            // Read/write readiness with bounded latency, without spin on a
            // normally writable socket when no bytes are queued.
            WSAPOLLFD fd[2]{{client.value,short((outgoing.eof||outgoing.bytes.size()-outgoing.sent>=256*1024?0:POLLRDNORM)|(incoming.bytes.empty()?0:POLLWRNORM)),0},
                           {upstream.value,short((incoming.eof||incoming.bytes.size()-incoming.sent>=256*1024?0:POLLRDNORM)|(outgoing.bytes.empty()?0:POLLWRNORM)),0}};
            WSAPoll(fd,2,10);
        }
    }catch(const std::exception& e){if(!Cancelled(cancel))Publish(e.what());}
    t->done=true;
}
void Reap(){
    std::vector<std::shared_ptr<Tunnel>> retired;
    {std::lock_guard lock(mutex);std::erase_if(tunnels,[&](const auto& t){if(t->done){retired.push_back(t);return true;}return false;});}
    for(auto& t:retired)if(t->worker.joinable())t->worker.join();
}
std::string Protect(std::string_view text,bool decrypt){
    if(text.empty())return {};DATA_BLOB in{DWORD(text.size()),reinterpret_cast<BYTE*>(const_cast<char*>(text.data()))},out{};
    const bool ok=decrypt?CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)!=FALSE:
        CryptProtectData(&in,L"Nova Proxy",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)!=FALSE;
    Check(ok,"Proxy credentials could not be protected/restored");
    std::string result(reinterpret_cast<char*>(out.pbData),out.cbData);SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);return result;
}
}
Config ReadConfig(){std::lock_guard lock(mutex);return config;}
Status ReadStatus(){Reap();std::lock_guard lock(mutex);auto value=status;value.testing=testRunning;value.active=int(tunnels.size());return value;}
std::string Validate(const Config& c){
    if(!CleanHost(c.host))return "Enter a hostname or IP without a URL";
    if(c.port<1||c.port>65535)return "Port must be between 1 and 65535";
    if(c.authentication){
        if(c.username.empty()||c.password.empty()||c.username.size()>255||c.password.size()>255)return "Username/password must contain 1-255 UTF-8 bytes";
        if(c.username.find_first_of("\r\n")!=c.username.npos||c.password.find_first_of("\r\n")!=c.password.npos||c.username.find('\0')!=c.username.npos||c.password.find('\0')!=c.password.npos)return "Invalid authentication text";
        if(c.protocol==Protocol::Http&&c.username.find(':')!=c.username.npos)return "HTTP username cannot contain a colon";
    }return {};
}
bool Apply(Config next,std::string& error){
    error=Validate(next);if(!error.empty()&&next.enabled)return false;
    std::lock_guard lock(mutex);
    if(next.enabled&&!status.supported){error="Proxy unavailable on this client";return false;}
    config=std::move(next);++status.revision;
    status.message=config.enabled?(status.supported?"Ready - next server connection":"Proxy unavailable on this client"):"Disabled";return true;
}
void SetSupported(bool supported){std::lock_guard lock(mutex);status.supported=supported;}
void ReportError(const std::string& text){Publish(text);}
Prepared Prepare(const Config& cfg,const std::string& target,int port,const Cancel& cancel){
    Reap();Check(Validate(cfg).empty(),Validate(cfg).c_str());
    WSADATA data{};Check(WSAStartup(MAKEWORD(2,2),&data)==0,"WinSock initialization failed");
    bool transferred=false;struct Cleanup{bool& value;~Cleanup(){if(!value)WSACleanup();}} cleanup{transferred};
    const auto end=Clock::now()+std::chrono::seconds(15);
    Publish("Connecting to proxy");std::unique_ptr<Socket> upstream(Connect(cfg,cancel,end));
    Publish("Establishing proxy tunnel");Handshake(upstream->value,cfg,target,port,cancel,end);
    Socket listener(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));Check(listener.value!=INVALID_SOCKET,"Could not create local relay");
    BOOL exclusive=TRUE;setsockopt(listener.value,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive));
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    Check(bind(listener.value,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0&&listen(listener.value,4)==0,"Could not start local relay");
    Nonblocking(listener.value);int size=sizeof(address);Check(getsockname(listener.value,reinterpret_cast<sockaddr*>(&address),&size)==0,"Could not read local relay address");
    Pending(cancel,end);auto t=std::make_shared<Tunnel>();t->upstream=upstream->release();t->listener=listener.release();
    try{
        // Publish the session and its joinable thread under one lock. Reaping
        // cannot race with thread assignment, even if shutdown is immediate.
        std::lock_guard lock(mutex);tunnels.push_back(t);t->worker=std::thread([t]{Relay(t);});
    }catch(...){
        {std::lock_guard lock(mutex);std::erase(tunnels,t);}
        closesocket(t->upstream);closesocket(t->listener);throw;
    }
    transferred=true;return {t,int(ntohs(address.sin_port))};
}
void Abandon(const Prepared& p){if(p.tunnel)p.tunnel->cancelled=true;}
void Test(const std::string& target,int port){
    if(testRunning.exchange(true))return;
    if(testWorker.joinable())testWorker.join();testCancel=false;const auto cfg=ReadConfig();
    try{testWorker=std::thread([cfg,target,port]{
        try{const auto start=Clock::now();auto prepared=Prepare(cfg,target,port,[]{return testCancel.load();});
            Abandon(prepared);Publish("Tunnel ready in "+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-start).count())+" ms");
        }catch(const std::exception& e){Publish(e.what());}testRunning=false;
    });}catch(...){testRunning=false;Publish("Could not start proxy test worker");}
}
void CancelTest(){testCancel=true;}
void BeginShutdown(){stopping=true;testCancel=true;std::lock_guard lock(mutex);for(auto& t:tunnels)t->cancelled=true;}
bool WaitStopped(unsigned milliseconds){
    const auto deadline=Clock::now()+std::chrono::milliseconds(milliseconds);
    while(Clock::now()<deadline){Reap();{std::lock_guard lock(mutex);if(tunnels.empty()&&!testRunning){if(testWorker.joinable())testWorker.join();return true;}}Sleep(10);}return false;
}
json::Value Save(){
    auto c=ReadConfig();auto root=json::Value::Object();root["schema"]=1;root["enabled"]=c.enabled;
    root["protocol"]=c.protocol==Protocol::Http?"http":"socks5";root["host"]=c.host;root["port"]=c.port;root["authentication"]=c.authentication;
    if(c.authentication){try{auto clear=json::Value::Object();clear["username"]=c.username;clear["password"]=c.password;
        const auto encrypted=Protect(clear.Dump(),false);constexpr char hex[]="0123456789abcdef";std::string text;
        for(unsigned char ch:encrypted){text+=hex[ch>>4];text+=hex[ch&15];}root["credentials"]=text;
    }catch(...){root["credentials"]="";}}
    return root;
}
void Load(const json::Value* root){
    Config c;std::string error;
    if(root&&root->isObject()){
        auto read=[&](const char* key){return root->Find(key);};
        if(auto p=read("enabled"))c.enabled=p->asBool(false);if(auto p=read("authentication"))c.authentication=p->asBool(false);
        if(auto p=read("protocol"))c.protocol=p->asString()=="http"?Protocol::Http:Protocol::Socks5;
        if(auto p=read("host"))c.host=p->asString();if(auto p=read("port")){
            const double number=p->asNumber(0);c.port=std::isfinite(number)&&number>=1&&number<=65535?int(number):0;
        }
        if(c.authentication){try{
            const auto p=read("credentials");Check(p&&p->isString(),"Proxy credentials missing");const auto text=p->asString();
            Check(text.size()%2==0&&text.size()<16384,"Invalid proxy credentials");std::string bytes;
            auto digit=[](char x){if(x>='0'&&x<='9')return x-'0';if(x>='a'&&x<='f')return x-'a'+10;return -1;};
            for(size_t i=0;i<text.size();i+=2){int a=digit(text[i]),b=digit(text[i+1]);Check(a>=0&&b>=0,"Invalid proxy credentials");bytes+=char(a*16+b);}
            const auto clear=Protect(bytes,true);const auto value=json::Parse(clear);Check(value&&value->isObject(),"Invalid proxy credentials");
            if(auto u=value->Find("username"))c.username=u->asString();if(auto pass=value->Find("password"))c.password=pass->asString();
        }catch(...){error="Re-enter proxy authentication on this Windows account";}}
    }
    std::lock_guard lock(mutex);
    if(c.enabled&&!status.supported){c.enabled=false;error="Proxy disabled: this client version is not supported";}
    config=std::move(c);++status.revision;
    status.message=!error.empty()?error:config.enabled?"Ready - next server connection":"Disabled";
}
}
