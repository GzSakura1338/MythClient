#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "core/Runtime.h"
#include "core/RuntimeControl.h"
#include "core/Hosts.h"
#include "minecraft/host/MinecraftHost.h"
#include "cs2/host/Cs2Host.h"
#include "../../../shared/Bootstrap.h"

extern "C" void InitializeBase(void*);
extern "C" unsigned NovaBootstrapVersion();
extern "C" unsigned NovaHeyPixelProxyABI_v1();

#define CHECK(expression) do { if (!(expression)) { \
    std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#expression); std::exit(1); \
} } while (false)

namespace {
HANDLE finished;
nova::HostContext received;
std::atomic<unsigned> calls{0};
bool failHost=false;
}

namespace nova::minecraft {
unsigned long RunHost(HostContext context) {
    received=context;
    ++calls;
    if (failHost) {
        ReportBootstrapFailed("test host failure");
        ReportBootstrapReady();
    } else {
        ReportBootstrapReady();
        ReportBootstrapFailed("late result must not replace ready");
    }
    SetEvent(finished);
    return 0;
}
}
namespace nova::cs2 {
unsigned long RunHost(HostContext context) {
    received=context; ++calls;
    ReportBootstrapReady();
    SetEvent(finished);
    return 0;
}
}

int main(int argc,char** argv) {
    CHECK(argc==2);
    const char* scenario=argv[1];
    CHECK(NovaBootstrapVersion()==NOVA_BOOTSTRAP_ABI);
    CHECK(NovaHeyPixelProxyABI_v1()==1);
    NovaBootstrap boot{NOVA_BOOTSTRAP_MAGIC,sizeof(NovaBootstrap),NOVA_BOOTSTRAP_ABI,
                       NovaClient_Forge1201,NovaBoot_Pending,{}};
    finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    CHECK(finished);
    if (!std::strcmp(scenario,"abi")) {
        ++boot.abi;
        InitializeBase(&boot);
        CHECK(boot.state==NovaBoot_Failed);
        CHECK(std::strstr(boot.message,"do not match"));
        CHECK(calls==0);
    } else if (!std::strcmp(scenario,"size")) {
        --boot.size;
        InitializeBase(&boot);
        CHECK(boot.state==NovaBoot_Failed && calls==0);
    } else if (!std::strcmp(scenario,"magic")) {
        boot.magic=0;
        InitializeBase(&boot);
        CHECK(boot.state==NovaBoot_Pending && calls==0);
    } else if (!std::strcmp(scenario,"unknown")) {
        boot.client=0xffffffffu;
        InitializeBase(&boot);
        const auto start=GetTickCount64();
        while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&boot.state),0,0)==NovaBoot_Pending
               && GetTickCount64()-start<3000) Sleep(1);
        CHECK(boot.state==NovaBoot_Failed && calls==0);
        CHECK(std::strstr(boot.message,"Unknown client profile"));
    } else if (!std::strcmp(scenario,"services")) {
        CHECK(nova::runtime::AcquireInstance());
        CHECK(!nova::runtime::AcquireInstance());
        nova::runtime::ReleaseInstance();
        CHECK(nova::runtime::AcquireInstance());
        nova::runtime::ReleaseInstance();
        RequestUnload();
        HANDLE wake=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        CHECK(wake);
        nova::runtime::SetUnloadEvent(wake);
        RequestUnload();
        CHECK(WaitForSingleObject(wake,0)==WAIT_OBJECT_0);
        nova::runtime::SetUnloadEvent(nullptr);
        ResetEvent(wake);
        RequestUnload();
        CHECK(WaitForSingleObject(wake,0)==WAIT_TIMEOUT);
        CloseHandle(wake);
        std::atomic<bool> stop{false};
        std::thread requests([&] {while (!stop.load()) RequestUnload();});
        for (unsigned i=0;i<1000;++i) {
            wake=CreateEventW(nullptr,TRUE,FALSE,nullptr);
            CHECK(wake);
            nova::runtime::SetUnloadEvent(wake);
            nova::runtime::SetUnloadEvent(nullptr);
            CloseHandle(wake);
        }
        stop=true;
        requests.join();
        CHECK(!IsCircuitOpen());
        OpenCircuit();
        CHECK(IsCircuitOpen());
    } else {
        failHost=!std::strcmp(scenario,"failed");
        const bool legacy=!std::strcmp(scenario,"legacy");
        if (!failHost && !legacy) boot.client=static_cast<unsigned>(std::strtoul(scenario,nullptr,10));
        const auto* host=nova::SelectGameHost(boot.client);
        CHECK(host && !std::strcmp(host->game,boot.client==NovaClient_Cs2?"cs2":"minecraft"));
        const DWORD caller=GetCurrentThreadId();
        InitializeBase(legacy?nullptr:&boot);
        CHECK(WaitForSingleObject(finished,3000)==WAIT_OBJECT_0);
        CHECK(calls==1);
        CHECK(received.profile==boot.client);
        CHECK(received.loaderThread==caller);
        CHECK(received.reflective==!legacy);
        if (legacy) CHECK(boot.state==NovaBoot_Pending);
        else if (failHost) {
            CHECK(boot.state==NovaBoot_Failed);
            CHECK(!std::strcmp(boot.message,"test host failure"));
        } else {
            CHECK(boot.state==NovaBoot_Ready);
            CHECK(!std::strcmp(boot.message,host->label));
        }
    }
    CloseHandle(finished);
    std::printf("PASS: native runtime %s (no JVM)\n",scenario);
}
