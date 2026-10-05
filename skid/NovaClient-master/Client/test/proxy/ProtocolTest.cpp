#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "proxy/ProxyService.h"
#include "proxy/TargetAddress.h"
#include <cassert>
#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <functional>
using namespace nova::proxy;
std::string Read(SOCKET s,int size){std::string out(size,'\0');int used=0;while(used<size){const int n=recv(s,out.data()+used,size-used,0);assert(n>0);used+=n;}return out;}
void Write(SOCKET s,std::string_view text){while(!text.empty()){const int n=send(s,text.data(),int(text.size()),0);assert(n>0);text.remove_prefix(n);}}
struct Server {
    SOCKET listener;int port;std::thread worker;
    explicit Server(std::function<void(SOCKET)> f){listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET);
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        assert(bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);assert(listen(listener,1)==0);
        int len=sizeof(address);getsockname(listener,reinterpret_cast<sockaddr*>(&address),&len);port=ntohs(address.sin_port);
        worker=std::thread([this,f]{SOCKET client=accept(listener,nullptr,nullptr);assert(client!=INVALID_SOCKET);
            DWORD timeout=5000;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&timeout),sizeof(timeout));
            f(client);closesocket(client);
        });
    }
    ~Server(){worker.join();closesocket(listener);}
};
std::string Header(SOCKET s){std::string header;while(!header.ends_with("\r\n\r\n")){assert(header.size()<32768);header+=Read(s,1);}return header;}
void SocksReply(SOCKET s,bool authentication,std::string target,int port,int reply=0){
    const auto greeting=Read(s,3);assert(greeting==std::string(authentication?"\5\1\2":"\5\1\0",3));
    Write(s,std::string(authentication?"\5\2":"\5\0",2));
    if(authentication){assert(Read(s,1)==std::string("\1",1));auto length=static_cast<unsigned char>(Read(s,1)[0]);
        assert(Read(s,length)=="name");length=static_cast<unsigned char>(Read(s,1)[0]);assert(Read(s,length)=="secret");Write(s,std::string("\1\0",2));}
    const auto prefix=Read(s,4);assert(prefix.substr(0,3)==std::string("\5\1\0",3));
    const int type=static_cast<unsigned char>(prefix[3]);
    if(type==1){auto ip=Read(s,4);IN_ADDR address;memcpy(&address,ip.data(),4);char text[64];InetNtopA(AF_INET,&address,text,sizeof(text));assert(target==text);}
    else if(type==4){auto ip=Read(s,16);assert(target=="::1");}
    else {assert(type==3);auto len=static_cast<unsigned char>(Read(s,1)[0]);assert(Read(s,len)==target);}
    const auto encoded=Read(s,2);assert((static_cast<unsigned char>(encoded[0])*256+static_cast<unsigned char>(encoded[1]))==port);
    std::string answer("\5\0\0\1\0\0\0\0\0\0",10);answer[1]=char(reply);
    for(char ch:answer){Write(s,std::string(1,ch));Sleep(1);}
}
SOCKET Downstream(int port){
    SOCKET socketValue=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(static_cast<u_short>(port));
    assert(connect(socketValue,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);
    DWORD timeout=5000;setsockopt(socketValue,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&timeout),sizeof(timeout));return socketValue;
}
int main(){
    WSADATA data{};assert(WSAStartup(MAKEWORD(2,2),&data)==0);SetSupported(true);
    assert(nova::proxy::FakeAddress("198.18.1.6")&&nova::proxy::FakeAddress("198.19.255.255"));
    assert(nova::proxy::FakeAddress("2001:2::105")&&!nova::proxy::FakeAddress("198.20.1.6"));
    assert(nova::proxy::SelectTarget("198.18.1.6","","mc.loyisa.cn")=="mc.loyisa.cn");
    assert(nova::proxy::SelectTarget("2001:2::105","srv.example.test","original.example.test")=="srv.example.test");
    assert(nova::proxy::SelectTarget("203.0.113.20","server.test","server.test")=="203.0.113.20");
    for(bool auth:{false,true}){
        const std::string payload(1024*1024,'x');
        Server server([&](SOCKET s){SocksReply(s,auth,"127.0.0.1",25565);Write(s,"first");
            for(size_t i=0;i<payload.size();i+=8192){auto bytes=Read(s,8192);Write(s,bytes);}
            assert(recv(s,const_cast<char*>(payload.data()),1,0)==0);shutdown(s,SD_SEND);
        });
        Config cfg;cfg.enabled=true;cfg.host="127.0.0.1";cfg.port=server.port;cfg.authentication=auth;cfg.username="name";cfg.password="secret";
        auto prepared=Prepare(cfg,"127.0.0.1",25565,[] {return false;});SOCKET local=Downstream(prepared.port);
        assert(Read(local,5)=="first");
        for(size_t i=0;i<payload.size();i+=8192){Write(local,std::string_view(payload).substr(i,8192));assert(Read(local,8192)==payload.substr(i,8192));}
        shutdown(local,SD_SEND);char byte;assert(recv(local,&byte,1,0)==0);closesocket(local);
    }
    for(std::string target:{"example.test","::1"}){
        Server server([&](SOCKET s){SocksReply(s,false,target,30100);});
        Config cfg;cfg.host="127.0.0.1";cfg.port=server.port;auto prepared=Prepare(cfg,target,30100,[]{return false;});Abandon(prepared);
    }
    for(bool auth:{false,true}){
        Server server([&](SOCKET s){const auto request=Header(s);assert(request.starts_with("CONNECT [::1]:25565 HTTP/1.1\r\n"));
            assert(request.find("Host: [::1]:25565")!=request.npos);
            assert((request.find("Proxy-Authorization: Basic bmFtZTpzZWNyZXQ=")!=request.npos)==auth);
            for(char c:std::string("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 Connection established\r\n\r\nHELLO"))Write(s,std::string(1,c));
            assert(Read(s,4)=="PING");Write(s,"PONG");
        });
        Config cfg;cfg.protocol=Protocol::Http;cfg.host="127.0.0.1";cfg.port=server.port;cfg.authentication=auth;cfg.username="name";cfg.password="secret";
        auto prepared=Prepare(cfg,"::1",25565,[]{return false;});auto local=Downstream(prepared.port);
        assert(Read(local,5)=="HELLO");Write(local,"PING");assert(Read(local,4)=="PONG");closesocket(local);
    }
    for(int code:{403,407,502}){
        Server server([&](SOCKET s){Header(s);Write(s,"HTTP/1.1 "+std::to_string(code)+" Failed\r\n\r\n");});
        Config cfg;cfg.protocol=Protocol::Http;cfg.host="127.0.0.1";cfg.port=server.port;
        bool rejected=false;try{Prepare(cfg,"127.0.0.1",25565,[]{return false;});}catch(const std::exception&){rejected=true;}assert(rejected);
    }
    {
        Server server([](SOCKET s){const auto prefix=Read(s,3);assert(prefix==std::string("\5\1\0",3));Write(s,std::string("\5\xff",2));});
        Config cfg;cfg.host="127.0.0.1";cfg.port=server.port;bool rejected=false;
        try{Prepare(cfg,"127.0.0.1",25565,[]{return false;});}catch(...){rejected=true;}assert(rejected);
    }
    {
        Server server([](SOCKET s){Read(s,3);char byte;assert(recv(s,&byte,1,0)==0);});
        Config cfg;cfg.host="127.0.0.1";cfg.port=server.port;const auto start=std::chrono::steady_clock::now();bool cancelled=false;
        try{Prepare(cfg,"127.0.0.1",25565,[&]{return std::chrono::steady_clock::now()-start>std::chrono::milliseconds(150);});}catch(...){cancelled=true;}
        assert(cancelled&&std::chrono::steady_clock::now()-start<std::chrono::seconds(2));
    }
    Config cfg;cfg.enabled=true;cfg.host="127.0.0.1";cfg.authentication=true;cfg.username="name";cfg.password="secret";
    std::string error;assert(Apply(cfg,error));const auto saved=Save();assert(saved.Dump().find("secret")==std::string::npos);
    Load(&saved);const auto restored=ReadConfig();assert(restored.username==cfg.username&&restored.password==cfg.password&&restored.enabled);
    cfg.host="host\r\nInjected";assert(!Validate(cfg).empty());cfg.host="127.0.0.1";cfg.password="secret\r\n";assert(!Validate(cfg).empty());
    BeginShutdown();assert(WaitStopped(5000));assert(ReadStatus().active==0);WSACleanup();
    std::cout<<"PASS: HTTP/SOCKS5/authentication, IPv4/IPv6/domain, split replies, 1 MiB duplex relay, half-close, residual data, refusal, cancellation and DPAPI persistence\n";
}
