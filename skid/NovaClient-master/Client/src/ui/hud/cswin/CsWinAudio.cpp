#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <cstring>
#include "ui/hud/cswin/CsWinAudio.h"

namespace nova::ui::cswin {
extern const unsigned char MusicInhuman[],MusicEZ4ENCE[];
extern const std::size_t MusicInhumanSize,MusicEZ4ENCESize;
struct Audio::Impl { HWAVEOUT device=nullptr; WAVEHDR header{}; bool prepared=false; double duration=0; };
Audio::Audio():impl_(std::make_unique<Impl>()) {}
Audio::~Audio(){Stop();}
void Audio::Stop() {
    if(!impl_->device)return;
    // No callback/thread can jump into an unloaded DLL. Reset returns buffers.
    waveOutReset(impl_->device);
    if(impl_->prepared)waveOutUnprepareHeader(impl_->device,&impl_->header,sizeof(WAVEHDR));
    waveOutClose(impl_->device);*impl_=Impl{};
}
double Audio::DurationSeconds() const {return impl_->duration;}
bool Audio::Play(Track track) {
    Stop();
    const auto* Music=track==Track::EZ4ENCE?MusicEZ4ENCE:MusicInhuman;
    const auto MusicSize=track==Track::EZ4ENCE?MusicEZ4ENCESize:MusicInhumanSize;
    if(MusicSize<12 || std::memcmp(Music,"RIFF",4) || std::memcmp(Music+8,"WAVE",4))return false;
    WAVEFORMATEX format{};const unsigned char* pcm=nullptr;DWORD size=0;
    for(std::size_t offset=12;offset+8<=MusicSize;) {
        DWORD length=0;std::memcpy(&length,Music+offset+4,4);
        const auto start=offset+8;if(length>MusicSize-start)return false;
        if(!std::memcmp(Music+offset,"fmt ",4) && length>=16)std::memcpy(&format,Music+start,16);
        if(!std::memcmp(Music+offset,"data",4)){pcm=Music+start;size=length;}
        offset=start+length+(length&1);
    }
    if(!pcm || !size || format.wFormatTag!=WAVE_FORMAT_PCM || !format.nBlockAlign || !format.nAvgBytesPerSec || size%format.nBlockAlign)return false;
    if(waveOutOpen(&impl_->device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR)return false;
    impl_->header.lpData=reinterpret_cast<LPSTR>(const_cast<unsigned char*>(pcm));
    impl_->header.dwBufferLength=size;
    if(waveOutPrepareHeader(impl_->device,&impl_->header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR){Stop();return false;}
    impl_->prepared=true;
    if(waveOutWrite(impl_->device,&impl_->header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR){Stop();return false;}
    impl_->duration=double(size)/format.nAvgBytesPerSec;
    return true;
}
}
