#include "smtc/backend/NetEaseLyrics.h"
#include "utils/Json.h"
#include <winhttp.h>

namespace nova::smtc::backend {
namespace {
struct InternetHandle {
    HINTERNET value{};
    explicit InternetHandle(HINTERNET handle) : value(handle) {}
    ~InternetHandle() { if (value) WinHttpCloseHandle(value); }
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
};
std::string Download(const std::string& song, const std::shared_ptr<Signal>& signal) {
    // No cookies, account credentials, song-title search or third-party relay.
    // Only the verified numeric NetEase song ID is sent to its own HTTPS endpoint.
    if (song.empty() || song.size() > 20 || !std::all_of(song.begin(), song.end(), [](char c) { return c >= '0' && c <= '9'; })) return {};
    const auto revision = signal->mediaRevision.load();
    const auto deadline = Clock::now() + std::chrono::seconds(3);
    auto cancelled = [&] { return signal->stop.load() || revision != signal->mediaRevision.load() || Clock::now() >= deadline; };
    InternetHandle session(WinHttpOpen(L"Nova MusicHUD/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0));
    if (!session.value || cancelled()) return {};
    WinHttpSetTimeouts(session.value, 800, 800, 800, 800);
    InternetHandle connection(WinHttpConnect(session.value, L"music.163.com", INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection.value || cancelled()) return {};
    const auto path = L"/api/song/lyric/v1?id=" + std::wstring(song.begin(), song.end()) + L"&lv=-1&kv=-1&tv=-1&rv=-1&yv=-1&ytv=-1&yrv=-1";
    InternetHandle request(WinHttpOpenRequest(connection.value, L"GET", path.c_str(), nullptr, nullptr,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request.value) return {};
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    WinHttpSetOption(request.value, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));
    if (cancelled() || !WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, 0) ||
        cancelled() || !WinHttpReceiveResponse(request.value, nullptr)) return {};
    DWORD status{}, size = sizeof(status);
    if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        nullptr, &status, &size, nullptr) || status != 200) return {};
    std::string body; char block[8192];
    while (!cancelled()) {
        DWORD count{};
        if (!WinHttpReadData(request.value, block, sizeof(block), &count)) return {};
        if (!count) return body;
        if (body.size() + count > 1024 * 1024) return {};
        body.append(block, count);
    }
    return {};
}
std::string Lyric(const json::Value& root, const char* key) {
    const auto* section = root.Find(key); const auto* value = section ? section->Find("lyric") : nullptr;
    return value ? value->asString() : "";
}
}
void NetEaseLyrics::Reset() { song_.clear(); lyrics_.reset(); retry_ = {}; }
std::shared_ptr<const Lyrics> NetEaseLyrics::Get(const std::string& song, const std::shared_ptr<Signal>& signal) {
    if (song.empty()) return {};
    if (song != song_) { Reset(); song_ = song; }
    if (lyrics_ || Clock::now() < retry_ || signal->stop.load()) return lyrics_;
    retry_ = Clock::now() + std::chrono::seconds(30);
    try {
        const auto body = Download(song, signal); if (body.empty() || signal->stop.load()) return {};
        const auto root = json::Parse(body); if (!root) return {};
        const auto* code = root->Find("code"); if (!code || code->asNumber(0) != 200) return {};
        const auto* pure = root->Find("pureMusic"); const auto* missing = root->Find("nolyric");
        if ((pure && pure->asBool(false)) || (missing && missing->asBool(false))) lyrics_ = std::make_shared<Lyrics>();
        else {
            lyrics_ = ParseYrcLyrics(Lyric(*root, "yrc"), Lyric(*root, "ytlrc"));
            if (lyrics_->lines.empty()) lyrics_ = ParseLyrics(Lyric(*root, "lrc"), Lyric(*root, "tlyric"));
        }
    } catch (...) { return {}; } // Optional lyrics must not take down SMTC or its cover.
    return lyrics_;
}
}
