#include "minecraft/accounts/CredentialStore.h"
#include <windows.h>
#include <wincrypt.h>
#include <vector>
namespace nova::minecraft::accounts {
std::string ProtectCredentials(const std::string& text) {
    if(text.empty()||text.size()>1024*1024)return {};
    DATA_BLOB in{DWORD(text.size()),reinterpret_cast<BYTE*>(const_cast<char*>(text.data()))},out{};
    if(!CryptProtectData(&in,L"Nova Microsoft accounts",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))return {};
    constexpr char hex[]="0123456789abcdef";std::string encoded;encoded.reserve(out.cbData*2);
    for(DWORD i=0;i<out.cbData;++i){encoded+=hex[out.pbData[i]>>4];encoded+=hex[out.pbData[i]&15];}
    SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);return encoded;
}
std::string UnprotectCredentials(const std::string& text) {
    if(text.empty()||text.size()%2||text.size()>3*1024*1024)return {};
    auto hex=[](char c)->int {return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    std::vector<BYTE> data(text.size()/2);for(size_t i=0;i<data.size();++i){int a=hex(text[i*2]),b=hex(text[i*2+1]);if(a<0||b<0)return {};data[i]=BYTE(a*16+b);}
    DATA_BLOB in{DWORD(data.size()),data.data()},out{};if(!CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))return {};
    std::string plain(reinterpret_cast<char*>(out.pbData),out.cbData);SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);return plain;
}
}
