#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include "utils/Json.h"

namespace nova::proxy {
enum class Protocol { Http, Socks5 };
struct Config {
    bool enabled=false,authentication=false;
    Protocol protocol=Protocol::Socks5;
    std::string host,username,password;
    int port=1080;
};
struct Status {std::uint64_t revision=0;bool supported=false,testing=false;std::string message;int active=0;};
using Cancel=std::function<bool()>;
struct Tunnel;
struct Prepared {std::shared_ptr<Tunnel> tunnel;int port=0;};
Config ReadConfig();
Status ReadStatus();
std::string Validate(const Config&);
bool Apply(Config,std::string& error);
void SetSupported(bool supported);
void ReportError(const std::string&);
Prepared Prepare(const Config&,const std::string& target,int port,const Cancel& cancel);
void Abandon(const Prepared&);
void Test(const std::string& target,int port);
void CancelTest();
void BeginShutdown();
bool WaitStopped(unsigned milliseconds);
json::Value Save();
void Load(const json::Value*);
}
