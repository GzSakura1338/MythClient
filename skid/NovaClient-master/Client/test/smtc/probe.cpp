#include "smtc/SmtcService.h"
#include "smtc/backend/NetEaseLyrics.h"
#include <windows.h>
#include <cstdio>
#include <stdexcept>
#include <cstring>
int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    if (argc > 2 && std::strcmp(argv[2], "--yrc") == 0) {
        nova::smtc::backend::NetEaseLyrics provider;
        const auto lyrics = provider.Get(argv[1], std::make_shared<nova::smtc::backend::Signal>());
        std::size_t words = 0;
        if (lyrics) for (const auto& line : lyrics->lines) words += line.words.size();
        printf("YRC read-only endpoint: lines=%zu words=%zu\n", lyrics ? lyrics->lines.size() : 0, words);
        return words ? 0 : 8;
    }
    if (argc > 1) nova::smtc::SelectSource(argv[1]);
    const bool requireLyrics = argc > 2 && std::strcmp(argv[2], "--lyrics") == 0;
    nova::smtc::SetLyricsEnabled(requireLyrics);
    nova::smtc::Start();
    bool refreshed = false;
    const bool requireTimeline = requireLyrics || (argc > 2 && std::strcmp(argv[2], "--timeline") == 0);
    int samples = 0;
    bool measured = false;
    bool gotLyrics = false;
    for (int i = 0; i < (requireTimeline ? 240 : 100); ++i) {
        Sleep(50);
        const auto data = nova::smtc::ReadSnapshot();
        if (data->state == nova::smtc::State::Starting) continue;
        printf("state=%d sources=%zu source=%s title=%s artist=%s cover=%dx%d error=%s\n",
            static_cast<int>(data->state), data->sources.size(), data->source.c_str(), data->title.c_str(), data->artist.c_str(),
            data->artwork ? data->artwork->width : 0, data->artwork ? data->artwork->height : 0, data->error.c_str());
        printf("timeline start=%.3f end=%.3f position=%.3f seekMin=%.3f seekMax=%.3f rate=%.3f playing=%d seek=%d measured=%d\n",
            data->start, data->end, data->position, data->minSeek, data->maxSeek, data->rate, data->playing, data->canSeek, data->measuredTimeline);
        if (data->state == nova::smtc::State::Unavailable) { nova::smtc::RequestStop(); nova::smtc::WaitStopped(5000); return 1; }
        refreshed = true;
        if (!requireTimeline) break;
        measured |= data->measuredTimeline && data->HasDuration();
        if (requireLyrics) {
            gotLyrics |= data->lyrics != nullptr;
            const int index = data->lyrics ? data->lyrics->Index(data->Position()) : -1;
            printf("lyrics fetched=%d lines=%zu current=%d song=%s\n", data->lyrics != nullptr,
                data->lyrics ? data->lyrics->lines.size() : 0, index, data->songId.c_str());
        }
        if (++samples >= 10) break;
        Sleep(950);
    }
    nova::smtc::RequestStop();
    if (!nova::smtc::WaitStopped(5000)) { puts("FAIL: callbacks did not drain"); return 2; }
    if (!refreshed) { puts("FAIL: no initial snapshot within deadline"); return 5; }
    if (requireTimeline && !measured) { puts("FAIL: native timeline was not acquired"); return 6; }
    if (requireLyrics && !gotLyrics) { puts("FAIL: lyrics response was not acquired"); return 7; }
    for (int i = 0; i < 5; ++i) {
        nova::smtc::Start(); Sleep(i * 5); nova::smtc::RequestStop();
        if (!nova::smtc::WaitStopped(5000)) return 3;
    }
    nova::smtc::BeginShutdown(); nova::smtc::Start();
    if (!nova::smtc::WaitStopped(100)) return 4;
    puts("PASS: read-only Windows SMTC, cancellation, delegate drain, restart and shutdown gate");
}
