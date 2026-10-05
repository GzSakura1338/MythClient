#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#include <cassert>
#include <cstdio>
#include <cmath>
namespace {
int opens=0,resets=0,closes=0,writes=0;double seconds=0;
MMRESULT TestOpen(LPHWAVEOUT out,UINT,LPCWAVEFORMATEX format,DWORD_PTR,DWORD_PTR,DWORD flags) {
    assert(flags==CALLBACK_NULL && format->wFormatTag==WAVE_FORMAT_PCM);
    assert(format->nSamplesPerSec==44100 && format->nChannels==2 && format->wBitsPerSample==16);
    *out=reinterpret_cast<HWAVEOUT>(1);++opens;return MMSYSERR_NOERROR;
}
MMRESULT TestReset(HWAVEOUT){++resets;return MMSYSERR_NOERROR;}
MMRESULT TestClose(HWAVEOUT){++closes;return MMSYSERR_NOERROR;}
MMRESULT TestHeader(HWAVEOUT,LPWAVEHDR,UINT){return MMSYSERR_NOERROR;}
MMRESULT TestWrite(HWAVEOUT,LPWAVEHDR header,UINT) {
    assert(header->lpData && header->dwBufferLength>100000);
    seconds=header->dwBufferLength/176400.;++writes;return MMSYSERR_NOERROR;
}
}
#define waveOutOpen TestOpen
#define waveOutReset TestReset
#define waveOutClose TestClose
#define waveOutPrepareHeader TestHeader
#define waveOutUnprepareHeader TestHeader
#define waveOutWrite TestWrite
#include "ui/hud/cswin/CsWinAudio.cpp"
int main() {
    using namespace nova::ui::cswin;
    {
        Audio audio;
        assert(audio.Play(Track::Inhuman));assert(seconds>11.9 && seconds<12.1);
        assert(std::abs(audio.DurationSeconds()-seconds)<1e-8);
        assert(audio.Play(Track::EZ4ENCE));assert(seconds>6.2 && seconds<6.4);
        assert(std::abs(audio.DurationSeconds()-seconds)<1e-8);
        assert(opens==2 && writes==2 && closes==1 && resets==1);
        audio.Stop();audio.Stop();assert(closes==2 && resets==2);
        assert(audio.Play(Track::Inhuman));
    }
    assert(opens==3 && closes==3 && resets==3);
    std::puts("PASS: both embedded music assets, PCM format/duration, track switch and device cleanup (muted test)");
}
