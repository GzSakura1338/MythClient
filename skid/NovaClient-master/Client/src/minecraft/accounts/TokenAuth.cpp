#include "minecraft/accounts/TokenAuth.h"
#include "minecraft/accounts/AccountValidation.h"
#include "utils/Json.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <chrono>

namespace nova::minecraft::accounts {
namespace {
struct Handle { HINTERNET value{}; ~Handle(){if(value)WinHttpCloseHandle(value);} };
}
TokenProfile VerifyToken(const std::string& token,const std::atomic<bool>& cancelled) {
    TokenProfile result;
    auto fail=[&](const char* error){result.error=error;return result;};
    if(!ValidToken(token))return fail("Invalid token format.");
    Handle session{WinHttpOpen(L"Nova Accounts/1.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,nullptr,nullptr,0)};
    if(!session.value)return fail("Unable to initialize authentication.");
    WinHttpSetTimeouts(session.value,5000,5000,5000,5000);
    Handle connection{WinHttpConnect(session.value,L"api.minecraftservices.com",INTERNET_DEFAULT_HTTPS_PORT,0)};
    if(!connection.value)return fail("Unable to connect to Minecraft services.");
    Handle request{WinHttpOpenRequest(connection.value,L"GET",L"/minecraft/profile",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)};
    if(!request.value)return fail("Unable to create authentication request.");
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if(!WinHttpSetOption(request.value,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects)))return fail("Unable to secure authentication request.");
    std::wstring headers=L"Authorization: Bearer ";headers.append(token.begin(),token.end());headers+=L"\r\n";
    const bool sent=!cancelled.load() && WinHttpSendRequest(request.value,headers.c_str(),DWORD(headers.size()),nullptr,0,0,0);
    SecureZeroMemory(headers.data(),headers.size()*sizeof(wchar_t));
    if(!sent||!WinHttpReceiveResponse(request.value,nullptr))return fail("Authentication request failed or timed out.");
    DWORD status=0,size=sizeof(status);
    if(!WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&size,nullptr)||status!=200)
        return fail(status==401||status==403?"Token expired or access denied.":status==404?"No Minecraft Java profile for this token.":"Minecraft services rejected the request.");
    std::string body; const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
    for(;;) {
        if(cancelled.load()||std::chrono::steady_clock::now()>deadline)return fail("Authentication cancelled or timed out.");
        char buffer[4096];DWORD read=0;
        if(!WinHttpReadData(request.value,buffer,sizeof(buffer),&read))return fail("Unable to read authentication response.");
        if(!read)break;
        if(body.size()+read>65536)return fail("Authentication response exceeds the size limit.");
        body.append(buffer,read);
    }
    const auto data=json::Parse(body);
    if(!data||!data->isObject())return fail("Invalid profile response.");
    const auto* name=data->Find("name");const auto* id=data->Find("id");
    if(!name||!id||!ValidName(name->asString())||NormalizeUuid(id->asString()).empty())return fail("Invalid Minecraft profile.");
    result.name=name->asString();result.uuid=NormalizeUuid(id->asString());return result;
}
}
