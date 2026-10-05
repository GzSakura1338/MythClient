#include "minecraft/accounts/AccountPortraits.h"
#include "minecraft/accounts/AccountValidation.h"
#include "ui/skin/SkinTextures.h"
#include "core/Instance.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <atomic>
#include <thread>
#include <unordered_map>

namespace nova::minecraft::accounts::portraits {
namespace {
using Microsoft::WRL::ComPtr;
struct Internet { HINTERNET value{}; ~Internet(){if(value)WinHttpCloseHandle(value);} };
struct Image { UINT width{},height{};std::vector<std::uint32_t> pixels; };
struct State {
    std::atomic<bool> stop{false},done{true};
    std::thread worker;
    std::string uuid;
    Image result;
    std::unordered_map<std::string,ULONGLONG> retry;
};
State& Data(){return Instance<State>();}
std::vector<BYTE> Download(const std::string& uuid,const std::atomic<bool>& stop) {
    // Same UUID-based face service used by Lunar's account manager. No session
    // credentials or usernames are sent; redirects cannot change the endpoint.
    Internet session{WinHttpOpen(L"Nova Accounts/1.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,nullptr,nullptr,0)};
    if(!session.value)return {};
    WinHttpSetTimeouts(session.value,1500,1500,1500,1500);
    Internet connection{WinHttpConnect(session.value,L"skins.mcstats.com",INTERNET_DEFAULT_HTTPS_PORT,0)};
    if(!connection.value)return {};
    const auto id=DashedUuid(uuid);const std::wstring path=L"/face/"+std::wstring(id.begin(),id.end());
    Internet request{WinHttpOpenRequest(connection.value,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)};
    if(!request.value)return {};
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if(!WinHttpSetOption(request.value,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects)))return {};
    if(stop.load()||!WinHttpSendRequest(request.value,WINHTTP_NO_ADDITIONAL_HEADERS,0,nullptr,0,0,0)||stop.load()||!WinHttpReceiveResponse(request.value,nullptr))return {};
    DWORD status{},size=sizeof(status);
    if(!WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&size,nullptr)||status!=200)return {};
    const auto deadline=GetTickCount64()+5000;std::vector<BYTE> bytes;
    while(!stop.load()&&GetTickCount64()<deadline) {
        BYTE block[4096];DWORD count{};
        if(!WinHttpReadData(request.value,block,sizeof(block),&count))return {};
        if(!count)return bytes;
        if(bytes.size()+count>1024*1024)return {};
        bytes.insert(bytes.end(),block,block+count);
    }
    return {};
}
Image Decode(std::vector<BYTE>& bytes) {
    Image image;if(bytes.empty())return image;
    ComPtr<IWICImagingFactory> factory;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))))return {};
    ComPtr<IWICStream> stream;
    if(FAILED(factory->CreateStream(&stream))||FAILED(stream->InitializeFromMemory(bytes.data(),static_cast<DWORD>(bytes.size()))))return {};
    ComPtr<IWICBitmapDecoder> decoder;
    if(FAILED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnDemand,&decoder)))return {};
    ComPtr<IWICBitmapFrameDecode> frame;
    if(FAILED(decoder->GetFrame(0,&frame))||FAILED(frame->GetSize(&image.width,&image.height))||
       !image.width||image.width>512||image.height!=image.width)return {};
    ComPtr<IWICFormatConverter> converter;
    if(FAILED(factory->CreateFormatConverter(&converter))||FAILED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,
       WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return {};
    image.pixels.resize(static_cast<size_t>(image.width)*image.height);
    if(FAILED(converter->CopyPixels(nullptr,image.width*4,static_cast<UINT>(image.pixels.size()*4),reinterpret_cast<BYTE*>(image.pixels.data()))))return {};
    return image;
}
}
void Update(nova::accounts::Snapshot& view) {
    auto& s=Data();if(s.stop.load())return;
    if(s.worker.joinable()&&s.done.load(std::memory_order_acquire)) {
        s.worker.join();
        if(!s.result.pixels.empty())ui::skins::Store("account-face:"+s.uuid,s.result.width,s.result.height,std::move(s.result.pixels));
        s.result={};
    }
    const auto now=GetTickCount64();
    for(auto& row:view.rows) {
        const auto uuid=NormalizeUuid(row.uuid);if(uuid.empty())continue;
        row.portrait=ui::skins::Find("account-face:"+uuid);
        if(row.portrait||s.worker.joinable())continue;
        const auto it=s.retry.find(uuid);
        if(it!=s.retry.end()&&now<it->second)continue;
        if(it==s.retry.end()&&s.retry.size()>=256)continue;
        s.retry[uuid]=now+60000;s.uuid=uuid;s.done=false;
        try{s.worker=std::thread([&s,uuid]{
            const HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
            try{auto bytes=Download(uuid,s.stop);if(!s.stop.load())s.result=Decode(bytes);}catch(...){s.result={};}
            if(SUCCEEDED(initialized))CoUninitialize();
            s.done.store(true,std::memory_order_release);
        });}catch(...){s.done=true;}
    }
}
void Stop(){Data().stop=true;}
bool WaitStopped(unsigned milliseconds) {
    auto& s=Data();const auto deadline=GetTickCount64()+milliseconds;
    while(!s.done.load(std::memory_order_acquire)&&GetTickCount64()<deadline)Sleep(10);
    if(!s.done.load(std::memory_order_acquire))return false;
    if(s.worker.joinable())s.worker.join();s.result={};s.retry.clear();return true;
}
}
