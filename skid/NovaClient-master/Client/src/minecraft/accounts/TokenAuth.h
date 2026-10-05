#pragma once
#include <string>
#include <atomic>
namespace nova::minecraft::accounts {
struct TokenProfile { std::string name,uuid,error; };
TokenProfile VerifyToken(const std::string& token,const std::atomic<bool>& cancelled);
}
