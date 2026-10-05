#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../../shared/Bootstrap.h"

extern "C" BOOL DoInjectRemoteThread(DWORD,const char*,uint32_t,char*,int);
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"Remote-thread test failed at %d: %s (error=%lu)\n",__LINE__,#x,GetLastError());std::exit(1);}}while(false)

int main(int argc,char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    std::setvbuf(stdout,nullptr,_IONBF,0);
    if(argc==4 && std::strcmp(argv[1],"--child")==0) {
        const std::string old=argv[2];
        for(const char* name:{"vcruntime140.dll","vcruntime140_1.dll","msvcp140.dll"})CHECK(LoadLibraryA((old+"/"+name).c_str()));
        CHECK(LoadLibraryA("jvm.dll")); // Native fixture returns zero VMs; never starts Minecraft.
        char ready[64],unload[64],stop[64];
        std::snprintf(ready,sizeof(ready),"Local\\NovaRemoteTestReady-%lu",GetCurrentProcessId());
        std::snprintf(unload,sizeof(unload),"Local\\NovaRemoteTestUnload-%lu",GetCurrentProcessId());
        std::snprintf(stop,sizeof(stop),"Local\\NovaRemoteTestStop-%lu",GetCurrentProcessId());
        HANDLE events[]{CreateEventA(nullptr,TRUE,FALSE,stop),CreateEventA(nullptr,FALSE,FALSE,unload)};
        HANDLE announce=CreateEventA(nullptr,TRUE,TRUE,ready);CHECK(events[0]&&events[1]&&announce);
        for(;;){DWORD wait=WaitForMultipleObjects(2,events,FALSE,30000);if(wait!=WAIT_OBJECT_0+1)break;
            HMODULE image=GetModuleHandleA(argv[3]);CHECK(image&&FreeLibrary(image));}
        CloseHandle(announce);CloseHandle(events[0]);CloseHandle(events[1]);return 0;
    }
    CHECK(argc==3 || argc==4);
    char executable[MAX_PATH];CHECK(GetModuleFileNameA(nullptr,executable,MAX_PATH));
    std::string command="\""+std::string(executable)+"\" --child \""+argv[1]+"\" \""+argv[2]+"\"";
    STARTUPINFOA startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    CHECK(CreateProcessA(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child));
    CloseHandle(child.hThread);
    char name[64];HANDLE ready=nullptr;
    std::snprintf(name,sizeof(name),"Local\\NovaRemoteTestReady-%lu",child.dwProcessId);
    for(int i=0;i<500&&!ready;++i){ready=OpenEventA(SYNCHRONIZE,FALSE,name);Sleep(10);}
    CHECK(ready&&WaitForSingleObject(ready,1000)==WAIT_OBJECT_0);CloseHandle(ready);
    std::snprintf(name,sizeof(name),"Local\\NovaRemoteTestUnload-%lu",child.dwProcessId);
    HANDLE unload=OpenEventA(EVENT_MODIFY_STATE,FALSE,name);CHECK(unload);
    char message[1024];
    CHECK(!DoInjectRemoteThread(child.dwProcessId,argv[2],NovaClient_Lunar189,message,sizeof(message)));
    for(int cycle=0;cycle<3;++cycle) {
        const BOOL ok=DoInjectRemoteThread(child.dwProcessId,argv[2],NovaClient_Forge1201,message,sizeof(message));
        std::puts(message);CHECK(ok);
        CHECK(std::strstr(message,"compatible CRT"));
        CHECK(!DoInjectRemoteThread(child.dwProcessId,argv[2],NovaClient_Forge1201,message,sizeof(message)));
        CHECK(std::strstr(message,"already loaded"));
        CHECK(SetEvent(unload));
        // Poll the actual loader list, so a repeated injection cannot race unload.
        bool present=true;
        for(int i=0;i<500&&present;++i) {
            HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,child.dwProcessId);CHECK(snapshot!=INVALID_HANDLE_VALUE);
            MODULEENTRY32 entry{};entry.dwSize=sizeof(entry);present=false;
            for(BOOL more=Module32First(snapshot,&entry);more;more=Module32Next(snapshot,&entry))
                if(_stricmp(entry.szExePath,argv[2])==0)present=true;
            CloseHandle(snapshot);Sleep(10);
        }
        CHECK(!present);
    }
    if(argc==4) {
        CHECK(!DoInjectRemoteThread(child.dwProcessId,argv[3],NovaClient_Forge1201,message,sizeof(message)));
        std::puts(message);CHECK(std::strstr(message,"Could not resolve the JVM"));
        bool present=true;
        for(int i=0;i<500&&present;++i) {
            HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,child.dwProcessId);CHECK(snapshot!=INVALID_HANDLE_VALUE);
            MODULEENTRY32 entry{};entry.dwSize=sizeof(entry);present=false;
            for(BOOL more=Module32First(snapshot,&entry);more;more=Module32Next(snapshot,&entry))
                if(_stricmp(entry.szExePath,argv[3])==0)present=true;
            CloseHandle(snapshot);Sleep(10);
        }
        CHECK(!present);std::puts("PASS: production DLL normal-load initialization, old CRT compatibility, wrong-host rejection and self-unload");
    }
    std::snprintf(name,sizeof(name),"Local\\NovaRemoteTestStop-%lu",child.dwProcessId);
    HANDLE stop=OpenEventA(EVENT_MODIFY_STATE,FALSE,name);CHECK(stop&&SetEvent(stop));
    CHECK(WaitForSingleObject(child.hProcess,5000)==WAIT_OBJECT_0);
    DWORD exit=1;CHECK(GetExitCodeProcess(child.hProcess,&exit)&&exit==0);
    CloseHandle(stop);CloseHandle(unload);CloseHandle(child.hProcess);
    std::puts("PASS: remote loading into separate x64 process with old host CRT; duplicate rejection; three unload/reload cycles; profile gate");
}
