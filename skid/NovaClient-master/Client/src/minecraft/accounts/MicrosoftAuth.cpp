#include "minecraft/accounts/MicrosoftAuth.h"
#include "minecraft/accounts/AccountValidation.h"
#include "utils/Json.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <shellapi.h>
#include <algorithm>
#include <array>
#include <chrono>

namespace nova::minecraft::accounts::microsoft {
namespace {
struct Internet { HINTERNET h{};~Internet(){if(h)WinHttpCloseHandle(h);} };
struct Socket { SOCKET h=INVALID_SOCKET;~Socket(){if(h!=INVALID_SOCKET)closesocket(h);} };
struct Winsock { bool ok=false;Winsock(){WSADATA d;ok=WSAStartup(MAKEWORD(2,2),&d)==0;}~Winsock(){if(ok)WSACleanup();} };
struct Erase {std::string& s;~Erase(){Wipe(s);} };
void Check(const std::atomic<bool>& cancel){if(cancel.load())throw std::string("Sign-in cancelled.");}
std::string Text(const json::Value& v,const char* key){const auto* p=v.Find(key);return p&&p->isString()?p->asString():std::string{};}
int Number(const json::Value& v,const char* key,int fallback=0){const auto* p=v.Find(key);return p?static_cast<int>(p->asNumber(fallback)):fallback;}
struct Response {DWORD status{};json::Value value;};
Response Post(const wchar_t* host,const wchar_t* path,const std::string& body,bool form,const std::atomic<bool>& cancel) {
    Check(cancel);
    Internet session{WinHttpOpen(L"Nova Accounts/1.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,nullptr,nullptr,0)};
    if(!session.h)throw std::string("Unable to initialize sign-in.");
    WinHttpSetTimeouts(session.h,3000,3000,5000,5000);
    Internet connection{WinHttpConnect(session.h,host,INTERNET_DEFAULT_HTTPS_PORT,0)};
    Internet request{connection.h?WinHttpOpenRequest(connection.h,L"POST",path,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr};
    if(!request.h)throw std::string("Unable to connect to sign-in services.");
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if(!WinHttpSetOption(request.h,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects)))throw std::string("Unable to secure sign-in request.");
    const wchar_t* headers=form?L"Content-Type: application/x-www-form-urlencoded\r\n":L"Content-Type: application/json\r\nAccept: application/json\r\n";
    if(!WinHttpSendRequest(request.h,headers,DWORD(-1),const_cast<char*>(body.data()),DWORD(body.size()),DWORD(body.size()),0)||!WinHttpReceiveResponse(request.h,nullptr))throw std::string("Sign-in request failed or timed out.");
    Check(cancel);Response result;DWORD size=sizeof(result.status);
    if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&result.status,&size,nullptr))throw std::string("Invalid sign-in response.");
    std::string data;Erase erase{data};const auto deadline=GetTickCount64()+15000;
    for(;;){Check(cancel);if(GetTickCount64()>deadline)throw std::string("Sign-in request timed out.");char buffer[4096];DWORD count{};
        if(!WinHttpReadData(request.h,buffer,sizeof(buffer),&count))throw std::string("Unable to read sign-in response.");
        if(!count)break;if(data.size()+count>1024*1024)throw std::string("Sign-in response too large.");data.append(buffer,count);}
    auto parsed=json::Parse(data);if(!parsed||!parsed->isObject())throw std::string("Invalid sign-in response.");result.value=std::move(*parsed);return result;
}
std::string Base64Url(const BYTE* bytes,DWORD size) {
    DWORD count=0;CryptBinaryToStringA(bytes,size,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&count);
    std::string out(count,'\0');if(!CryptBinaryToStringA(bytes,size,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,out.data(),&count))throw std::string("Unable to secure sign-in.");
    out.resize(count);while(!out.empty()&&(out.back()=='='||out.back()=='\0'))out.pop_back();
    for(auto& c:out){if(c=='+')c='-';else if(c=='/')c='_';}return out;
}
std::string Random(){std::array<BYTE,32> bytes{};if(BCryptGenRandom(nullptr,bytes.data(),DWORD(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0)throw std::string("Unable to secure sign-in.");return Base64Url(bytes.data(),DWORD(bytes.size()));}
std::string Challenge(const std::string& verifier) {
    std::array<BYTE,32> hash{};BCRYPT_ALG_HANDLE alg=nullptr;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)!=0)throw std::string("Unable to secure sign-in.");
    const auto result=BCryptHash(alg,nullptr,0,reinterpret_cast<PUCHAR>(const_cast<char*>(verifier.data())),ULONG(verifier.size()),hash.data(),ULONG(hash.size()));BCryptCloseAlgorithmProvider(alg,0);
    if(result!=0)throw std::string("Unable to secure sign-in.");return Base64Url(hash.data(),DWORD(hash.size()));
}
std::string Decode(std::string_view value) {
    auto hex=[](char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;return -1;};
    std::string out;for(size_t i=0;i<value.size();++i){char c=value[i];if(c=='%'){if(i+2>=value.size())return {};const int a=hex(value[i+1]),b=hex(value[i+2]);if(a<0||b<0)return {};c=char(a*16+b);i+=2;}else if(c=='+')c=' ';if(static_cast<unsigned char>(c)<32)return {};out+=c;}return out;
}
std::string BrowserCode(OAuthProfile& profile,const std::string& verifier,const std::atomic<bool>& cancel,const Progress& progress) {
    Winsock sockets;if(!sockets.ok)throw std::string("Unable to open browser sign-in callback.");
    Socket listener{socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)};if(listener.h==INVALID_SOCKET)throw std::string("Unable to open browser sign-in callback.");
    BOOL exclusive=TRUE;setsockopt(listener.h,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&exclusive),sizeof(exclusive));
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    unsigned short boundPort=0;
    const auto tryPort=[&](unsigned short port){address.sin_port=htons(port);if(bind(listener.h,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0)return false;boundPort=port;return true;};
    if(profile.pkce)tryPort(CallbackPort);
    else for(auto port:DefaultPorts)if(tryPort(port))break;
    if(!boundPort||listen(listener.h,4)!=0)throw std::string("Browser sign-in ports are busy. Close the other sign-in and retry.");
    profile=Profile(profile.clientId,boundPort);
    u_long nonblocking=1;ioctlsocket(listener.h,FIONBIO,&nonblocking);
    const std::string state=Random();
    const std::string url=AuthorizationUrl(profile,state,verifier);
    Check(cancel);const std::wstring wide(url.begin(),url.end());
    if(reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr,L"open",wide.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)throw std::string("Could not open the default browser.");
    progress("Complete Microsoft sign-in in your browser.");
    const auto deadline=GetTickCount64()+5*60*1000;
    while(GetTickCount64()<deadline){Check(cancel);fd_set read;FD_ZERO(&read);FD_SET(listener.h,&read);timeval wait{0,100000};if(select(0,&read,nullptr,nullptr,&wait)<=0)continue;
        Socket peer{accept(listener.h,nullptr,nullptr)};if(peer.h==INVALID_SOCKET)continue;
        ioctlsocket(peer.h,FIONBIO,&nonblocking);std::string request;Erase erase{request};const auto peerDeadline=GetTickCount64()+2000;
        while(GetTickCount64()<peerDeadline&&request.size()<16384&&request.find("\r\n\r\n")==std::string::npos){Check(cancel);char buffer[2048];int n=recv(peer.h,buffer,sizeof(buffer),0);if(n>0)request.append(buffer,n);else if(n==0)break;else if(WSAGetLastError()!=WSAEWOULDBLOCK)break;Sleep(10);}
        std::string code=CallbackCode(request,state,profile.callbackPath);
        const bool denied=code=="!denied";const bool ok=!code.empty()&&!denied;
        const std::string body=ok?"Sign-in received. You can close this tab and return to Minecraft.":denied?"Sign-in cancelled. Return to Minecraft.":"Invalid sign-in callback.";
        const std::string response=std::string("HTTP/1.1 ")+(ok||denied?"200 OK":"400 Bad Request")+"\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n"+body;
        send(peer.h,response.data(),int(response.size()),0);
        if(denied)throw std::string("Microsoft sign-in was cancelled.");if(ok)return code;
    }
    throw std::string("Browser sign-in timed out. Try again.");
}
std::string RequireToken(const Response& response,const char* key,const char* error){auto value=Text(response.value,key);if(response.status!=200||value.empty())throw std::string(error);return value;}
}
void Wipe(std::string& s){if(!s.empty())SecureZeroMemory(s.data(),s.size());s.clear();}
bool ValidClientId(std::string_view id){return id.size()==36&&id[8]=='-'&&id[13]=='-'&&id[18]=='-'&&id[23]=='-'&&!NormalizeUuid(id).empty();}
std::string ResolveClientId(std::string_view configured){return configured.empty()?std::string(DefaultClientId):std::string(configured);}
OAuthProfile Profile(std::string_view configured,unsigned short port) {
    OAuthProfile p;p.clientId=ResolveClientId(configured);
    if(!ValidClientId(p.clientId))throw std::string("Invalid Microsoft Client ID.");
    const bool builtin=NormalizeUuid(p.clientId)==NormalizeUuid(DefaultClientId);
    p.pkce=!builtin;p.callbackPath=builtin?DefaultCallbackPath:"/";
    p.scope=builtin?"XboxLive.signin XboxLive.offline_access":"XboxLive.signin offline_access";
    p.redirectUri=builtin?"http://localhost:"+std::to_string(port?port:DefaultPorts[0])+p.callbackPath:RedirectUri;
    return p;
}
std::string AuthorizationUrl(const OAuthProfile& p,std::string_view state,const std::string& verifier) {
    auto url=std::string(AuthorizeEndpoint)+"?client_id="+Encode(p.clientId)+"&response_type=code&redirect_uri="+Encode(p.redirectUri)+"&scope="+Encode(p.scope)+"&prompt=select_account&state="+Encode(state);
    if(p.pkce)url+="&response_mode=query&code_challenge="+Challenge(verifier)+"&code_challenge_method=S256";
    return url;
}
std::string TokenRequest(const OAuthProfile& p,std::string_view code,std::string_view verifier,std::string_view refresh) {
    auto body="client_id="+Encode(p.clientId)+"&scope="+Encode(p.scope);
    if(refresh.empty()) {
        body+="&grant_type=authorization_code&code="+Encode(code)+"&redirect_uri="+Encode(p.redirectUri);
        if(p.pkce)body+="&code_verifier="+Encode(verifier);
    }else {
        body+="&grant_type=refresh_token&refresh_token="+Encode(refresh);
        if(p.pkce)body+="&redirect_uri="+Encode(p.redirectUri);
    }
    return body;
}
std::string Encode(std::string_view value){constexpr char hex[]="0123456789ABCDEF";std::string out;for(unsigned char c:value){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')out+=char(c);else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;}
std::string CallbackCode(std::string_view request,std::string_view state,std::string_view path) {
    const std::string prefix="GET "+std::string(path)+"?";
    if(request.size()>16384||!request.starts_with(prefix))return {};
    const auto end=request.find(' ',4);if(end==request.npos)return {};
    auto query=request.substr(prefix.size(),end-prefix.size());std::string received,code,error;bool sawState=false,sawCode=false,sawError=false;
    while(!query.empty()){auto next=query.find('&');auto pair=query.substr(0,next);auto eq=pair.find('=');if(eq!=pair.npos){const auto key=pair.substr(0,eq);auto value=Decode(pair.substr(eq+1));
        if(key=="state"){if(sawState)return {};sawState=true;received=std::move(value);}else if(key=="code"){if(sawCode)return {};sawCode=true;code=std::move(value);}else if(key=="error"){if(sawError)return {};sawError=true;error=std::move(value);}}
        if(next==query.npos)break;query.remove_prefix(next+1);}
    if(!sawState||received!=state||state.empty()||(sawCode&&sawError))return {};
    if(sawError)return "!denied";if(code.empty()||code.size()>8192||code=="!denied")return {};return code;
}
Tokens Login(const std::string& clientId,const std::string& refresh,const std::atomic<bool>& cancel,const Progress& progress) {
    Check(cancel);auto profile=Profile(clientId);
    std::string request;Erase eraseRequest{request};
    if(refresh.empty()) {
        progress("Opening Microsoft sign-in...");auto verifier=profile.pkce?Random():std::string{};Erase eraseVerifier{verifier};auto code=BrowserCode(profile,verifier,cancel,progress);Erase eraseCode{code};
        request=TokenRequest(profile,code,verifier,{});
    } else {progress("Refreshing Microsoft sign-in...");request=TokenRequest(profile,{},{},refresh);}
    auto oauth=Post(TokenHost,TokenPath,request,true,cancel);
    if(!refresh.empty()&&oauth.status==400&&Text(oauth.value,"error")=="invalid_grant") {
        // Revoked/expired refresh tokens require consent again, never Launcher IPC.
        return Login(clientId,{},cancel,progress);
    }
    auto access=RequireToken(oauth,"access_token","Microsoft authorization failed. Check Client ID / redirect URI or sign in again.");Erase eraseAccess{access};
    Tokens result;result.refresh=Text(oauth.value,"refresh_token");if(result.refresh.empty())result.refresh=refresh;
    try {
        progress("Signing in to Xbox Live...");
        auto body=json::Value::Object();body["Properties"]=json::Value::Object();body["Properties"]["AuthMethod"]="RPS";body["Properties"]["SiteName"]="user.auth.xboxlive.com";body["Properties"]["RpsTicket"]="d="+access;body["RelyingParty"]="http://auth.xboxlive.com";body["TokenType"]="JWT";
        auto xbl=Post(L"user.auth.xboxlive.com",L"/user/authenticate",body.Dump(0),false,cancel);
        auto xbox=RequireToken(xbl,"Token","Xbox Live sign-in failed. Check the account's Xbox profile.");Erase eraseXbox{xbox};
        body=json::Value::Object();body["Properties"]=json::Value::Object();body["Properties"]["SandboxId"]="RETAIL";auto users=json::Value::Array();users.Push(xbox);body["Properties"]["UserTokens"]=std::move(users);body["RelyingParty"]="rp://api.minecraftservices.com/";body["TokenType"]="JWT";
        auto xsts=Post(L"xsts.auth.xboxlive.com",L"/xsts/authorize",body.Dump(0),false,cancel);
        auto token=RequireToken(xsts,"Token","Xbox authorization failed. Check age/family settings and Xbox profile.");Erase eraseToken{token};
        const auto* claims=xsts.value.Find("DisplayClaims");const auto* xui=claims?claims->Find("xui"):nullptr;
        if(!xui||xui->Items().empty())throw std::string("Invalid Xbox identity.");const auto uhs=Text(xui->Items()[0],"uhs");if(uhs.empty())throw std::string("Invalid Xbox identity.");
        progress("Signing in to Minecraft...");body=json::Value::Object();body["identityToken"]="XBL3.0 x="+uhs+";"+token;
        auto mc=Post(L"api.minecraftservices.com",L"/authentication/login_with_xbox",body.Dump(0),false,cancel);
        result.access=RequireToken(mc,"access_token","Minecraft sign-in failed. The app may need Minecraft API approval.");result.expires=std::clamp(Number(mc.value,"expires_in",3600),60,86400);
        Check(cancel);return result;
    }catch(...){Wipe(result.refresh);Wipe(result.access);throw;}
}
}
