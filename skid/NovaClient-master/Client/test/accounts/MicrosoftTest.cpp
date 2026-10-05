#include "minecraft/accounts/MicrosoftAuth.h"
#include "minecraft/accounts/CredentialStore.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace nova::minecraft::accounts;
    using namespace microsoft;
    assert(std::string(AuthorizeEndpoint)=="https://login.live.com/oauth20_authorize.srf");
    assert(std::wstring(TokenHost)==L"login.live.com"&&std::wstring(TokenPath)==L"/oauth20_token.srf");
    assert(CallbackPort==8247&&std::string(RedirectUri)=="http://localhost:8247");
    const auto builtin=Profile("");
    assert(builtin.clientId==DefaultClientId&&!builtin.pkce);
    assert(builtin.scope=="XboxLive.signin XboxLive.offline_access");
    assert(builtin.redirectUri=="http://localhost:59125"+std::string(DefaultCallbackPath));
    const auto fallback=Profile("",59126);
    const auto url=AuthorizationUrl(fallback,"test-state","");
    assert(url.find("client_id="+std::string(DefaultClientId))!=url.npos);
    assert(url.find("redirect_uri="+Encode(fallback.redirectUri))!=url.npos);
    assert(url.find("scope=XboxLive.signin%20XboxLive.offline_access")!=url.npos);
    assert(url.find("code_challenge")==url.npos);
    const auto grant=TokenRequest(fallback,"example-code","","");
    assert(grant.find("redirect_uri="+Encode(fallback.redirectUri))!=grant.npos);
    assert(grant.find("code_verifier")==grant.npos&&grant.find("client_secret")==grant.npos);
    const auto refreshGrant=TokenRequest(fallback,"","","example-refresh");
    assert(refreshGrant.find("grant_type=refresh_token")!=refreshGrant.npos&&refreshGrant.find("redirect_uri")==refreshGrant.npos);
    const auto custom=Profile("069A79F4-44E9-4726-A5BE-FCA90E38AAF5");
    assert(custom.pkce&&custom.clientId=="069A79F4-44E9-4726-A5BE-FCA90E38AAF5"&&custom.redirectUri==RedirectUri);
    const auto customUrl=AuthorizationUrl(custom,"state with space","dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");
    assert(customUrl.find("state=state%20with%20space")!=customUrl.npos);
    assert(customUrl.find("code_challenge=E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM")!=customUrl.npos);
    assert(TokenRequest(custom,"code","verifier","").find("code_verifier=verifier")!=std::string::npos);
    const auto iasCallback="GET "+std::string(DefaultCallbackPath)+"?code=abc&state=test-state HTTP/1.1\r\n\r\n";
    assert(CallbackCode(iasCallback,"test-state",builtin.callbackPath)=="abc");
    assert(CallbackCode(iasCallback,"wrong-state",builtin.callbackPath).empty());
    assert(CallbackCode(iasCallback,"test-state").empty());
    assert(CallbackCode("GET /?code=abc&state=test-state HTTP/1.1\r\n\r\n","test-state",builtin.callbackPath).empty());
    assert(ValidClientId("069A79F4-44E9-4726-A5BE-FCA90E38AAF5"));
    for(auto id:{"", "069a79f444e94726a5befca90e38aaf5", "069a79f4-44e9-4726-a5be-fca90e38aafg"})assert(!ValidClientId(id));
    assert(Encode("a+b&c= /?")=="a%2Bb%26c%3D%20%2F%3F");
    const auto callback=[](const std::string& query){return CallbackCode("GET /?"+query+" HTTP/1.1\r\nHost: localhost:8247\r\n\r\n","test-state");};
    assert(callback("code=abc%2B123&state=test-state")=="abc+123");
    assert(callback("state=test-state&error=access_denied")=="!denied");
    for(auto query:{"code=abc", "state=other&code=abc", "state=test-state&state=test-state&code=abc",
        "state=test-state&code=a&code=b", "state=test-state&code=a&error=denied",
        "state=test-state&code=%00", "state=test-state&code=%XX", "state=test-state&code=%1",
        "state=test-state&code=!denied"})assert(callback(query).empty());
    assert(CallbackCode("POST /?state=test-state&code=abc HTTP/1.1\r\n\r\n","test-state").empty());
    assert(CallbackCode("GET /other?state=test-state&code=abc HTTP/1.1\r\n\r\n","test-state").empty());
    assert(callback("state=test-state&code="+std::string(8193,'a')).empty());
    assert(CallbackCode(std::string(16385,'a'),"test-state").empty());
    const std::string fixture="test-only-refresh-value";
    auto encrypted=ProtectCredentials(fixture);assert(!encrypted.empty()&&encrypted.find(fixture)==std::string::npos);
    assert(UnprotectCredentials(encrypted)==fixture);
    encrypted[encrypted.size()/2]=encrypted[encrypted.size()/2]=='0'?'1':'0';
    assert(UnprotectCredentials(encrypted).empty());
    assert(UnprotectCredentials("zz").empty());assert(UnprotectCredentials("0").empty());
    std::atomic<bool> cancel=true;bool cancelled=false;
    try{Login("069a79f4-44e9-4726-a5be-fca90e38aaf5",fixture,cancel,[](const char*){});}catch(const std::string&){cancelled=true;}
    assert(cancelled);
    std::puts("PASS: OAuth callback state/encoding/duplicates/bounds, DPAPI roundtrip/tampering, cancelled request");
}
