#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "core/Runtime.h"
#include "core/RuntimeControl.h"

namespace {
std::atomic<bool> circuitOpen{false};
SRWLOCK logLock=SRWLOCK_INIT;
char preferredLog[MAX_PATH]{};
HANDLE logFile=INVALID_HANDLE_VALUE;
bool triedLog=false;
}
bool IsCircuitOpen() {return circuitOpen.load(std::memory_order_acquire);}
void OpenCircuit() {circuitOpen.store(true,std::memory_order_release);}
namespace nova::runtime {
void SetLogPath(const char* path) {
    AcquireSRWLockExclusive(&logLock);
    if(!triedLog && path)lstrcpynA(preferredLog,path,MAX_PATH);
    ReleaseSRWLockExclusive(&logLock);
}
}
void Dbg(const char* fmt,...) {
    char buffer[1024];va_list args;va_start(args,fmt);vsnprintf(buffer,sizeof(buffer),fmt,args);va_end(args);
    MessageBoxA(nullptr,buffer,"Nova Base",MB_OK);LogTo("[DBG] %s",buffer);
}
void LogTo(const char* fmt,...) {
    AcquireSRWLockExclusive(&logLock);
    if(!triedLog) {
        triedLog=true;
        const auto open=[](const char* path) {
            return path && *path?CreateFileA(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,
                nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr):INVALID_HANDLE_VALUE;
        };
        logFile=open(preferredLog);
        if(logFile==INVALID_HANDLE_VALUE) {
            char temporary[MAX_PATH];const DWORD count=GetTempPathA(MAX_PATH,temporary);
            if(count>0 && count<MAX_PATH-24) {lstrcatA(temporary,"NovaBase.log");logFile=open(temporary);}
        }
        if(logFile==INVALID_HANDLE_VALUE)logFile=open("C:\\NovaBase.log");
    }
    if(logFile!=INVALID_HANDLE_VALUE) {
        char message[1200];
        const int prefix=snprintf(message,sizeof(message),"[%08lu] ",static_cast<unsigned long>(GetTickCount()));
        va_list args;va_start(args,fmt);vsnprintf(message+prefix,sizeof(message)-prefix-2,fmt,args);va_end(args);
        size_t length=strlen(message);
        if(length<sizeof(message)-2) {message[length++]='\r';message[length++]='\n';}
        DWORD written=0;WriteFile(logFile,message,static_cast<DWORD>(length),&written,nullptr);
    }
    ReleaseSRWLockExclusive(&logLock);
}
