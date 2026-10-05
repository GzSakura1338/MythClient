#pragma once
#include "contracts/AccountService.h"
#include "minecraft/java/api/base.h"
#include <atomic>

namespace nova::minecraft::accounts {
struct RuntimeView { nova::accounts::Snapshot snapshot; std::string currentUuid; bool lunarBusy=false; };
struct Login {
    nova::accounts::Account account;
    std::string token;
    // Expected original identity prevents late worker results replacing a new login.
    std::string expectedUuid;
    std::string clientId,refresh;
};
struct Prepared {
    Login login;
    jobject user=nullptr, api=nullptr, properties=nullptr, profile=nullptr, expectedUser=nullptr;
    std::string error;
    void Release(JNIEnv*);
};
RuntimeView Inspect(JNIEnv*);
jobject CaptureSession(JNIEnv*);
Login ReadLunarLogin(JNIEnv*,const std::string& id,const std::string& expectedUuid);
void Prepare(Prepared&,const std::atomic<bool>& cancelled);
// Returns false before mutation for stale/in-world jobs. Exceptions roll back fields.
bool Commit(JNIEnv*,Prepared&);
}
