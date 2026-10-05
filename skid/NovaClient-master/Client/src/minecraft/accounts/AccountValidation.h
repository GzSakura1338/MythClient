#pragma once
#include <algorithm>
#include <string>
#include <string_view>
namespace nova::minecraft::accounts {
inline bool ValidName(std::string_view name) {
    return name.size()>=3 && name.size()<=16 && std::all_of(name.begin(),name.end(),[](unsigned char c){
        return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';
    });
}
inline bool ValidToken(std::string_view token) {
    return token.size()>=16 && token.size()<=8192 && std::all_of(token.begin(),token.end(),[](unsigned char c){return c>=33&&c<=126;});
}
inline std::string NormalizeUuid(std::string_view uuid) {
    std::string out;
    for(unsigned char c:uuid) {
        if(c=='-')continue;
        if(c>='A'&&c<='F')c+=32;
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return {};
        out+=char(c);
    }
    if(out.size()!=32)return {};
    return out;
}
inline std::string DashedUuid(std::string_view uuid) {
    auto s=NormalizeUuid(uuid);if(s.empty())return {};
    s.insert(20,"-");s.insert(16,"-");s.insert(12,"-");s.insert(8,"-");return s;
}
}
