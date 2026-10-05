#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <string_view>

namespace nova::minecraft::accounts::microsoft {
inline constexpr unsigned short CallbackPort=8247;
inline constexpr char RedirectUri[]="http://localhost:8247";
// Public application identifier and registered callback used by IAS 9.0.8.
inline constexpr char DefaultClientId[]="54fd49e4-2103-4044-9603-2b028c814ec3";
inline constexpr char DefaultCallbackPath[]="/in_game_account_switcher_long_enough_uri_to_prevent_accidental_leaks_on_screensharing_even_if_you_have_like_extremely_big_screen_though_it_might_not_mork_but_we_will_try_it_anyway_to_prevent_funny_things_from_happening_or_something";
inline constexpr unsigned short DefaultPorts[]={59125,59126,59127,59128,59129,59130,59131,59132,59133,59134,59135,1234,1235,1236,1237,80,8080,19364,19365,19366,27930,27931,27932,27933,27934,42069};
inline constexpr char AuthorizeEndpoint[]="https://login.live.com/oauth20_authorize.srf";
inline constexpr wchar_t TokenHost[]=L"login.live.com";
inline constexpr wchar_t TokenPath[]=L"/oauth20_token.srf";
struct Tokens { std::string access,refresh; int expires=0; };
using Progress=std::function<void(const char*)>;
bool ValidClientId(std::string_view);
std::string ResolveClientId(std::string_view configured);
struct OAuthProfile { std::string clientId,redirectUri,callbackPath,scope; bool pkce=false; };
OAuthProfile Profile(std::string_view configured,unsigned short port=0);
std::string AuthorizationUrl(const OAuthProfile&,std::string_view state,const std::string& verifier);
std::string TokenRequest(const OAuthProfile&,std::string_view code,std::string_view verifier,std::string_view refresh);
std::string Encode(std::string_view);
// Returns only a matching callback's authorization code, never accepts another state.
std::string CallbackCode(std::string_view request,std::string_view state,std::string_view path="/");
Tokens Login(const std::string& clientId,const std::string& refresh,const std::atomic<bool>& cancel,const Progress&);
void Wipe(std::string&);
}
