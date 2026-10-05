#pragma once
#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>
#include <cstdio>
namespace nova::proxy {
// RFC2544 benchmarking ranges are used by Fake-IP DNS engines and are not
// publicly routable server endpoints. Never silently fall back to direct TCP.
inline bool FakeAddress(std::string_view address){
    unsigned a=0,b=0,c=0,d=0;char tail=0;
    const std::string text(address);
    if(std::sscanf(text.c_str(),"%u.%u.%u.%u%c",&a,&b,&c,&d,&tail)==4)
        return a==198&&(b==18||b==19)&&c<=255&&d<=255;
    auto lower=text;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char ch){return char(std::tolower(ch));});
    return lower.starts_with("2001:2:")||lower.starts_with("2001:0002:");
}
inline std::string SelectTarget(std::string_view numeric,std::string_view named,std::string_view requested){
    if(!FakeAddress(numeric))return std::string(numeric);
    // Prefer the endpoint's hostname (which can be an SRV redirect). Otherwise
    // use the connector's original hostname, captured before local resolution.
    for(auto host:{named,requested})if(!host.empty()&&host!=numeric&&!FakeAddress(host))return std::string(host);
    return std::string(numeric);
}
}
