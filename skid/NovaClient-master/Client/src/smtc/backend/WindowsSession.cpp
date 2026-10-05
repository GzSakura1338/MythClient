#include "smtc/backend/WindowsSession.h"
#include "smtc/backend/ArtworkDecoder.h"
#include "smtc/backend/NetEaseTimeline.h"
#include "smtc/backend/NetEaseLyrics.h"
#include <windows.media.control.h>
#include <roapi.h>
#include <wrl/wrappers/corewrappers.h>
#include <functional>

namespace nova::smtc::backend {
namespace {
namespace M = ABI::Windows::Media::Control;
using Session = M::IGlobalSystemMediaTransportControlsSession;
using Manager = M::IGlobalSystemMediaTransportControlsSessionManager;
using Microsoft::WRL::Wrappers::HString;
std::atomic<std::uint64_t> sequence{0};
std::string Text(HSTRING value) {
    UINT32 size{}; const auto* text = WindowsGetStringRawBuffer(value, &size);
    if (!text || !size) return {};
    size = std::min(size, 1024u);
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, text, size, nullptr, 0, nullptr, nullptr);
    std::string result(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, size, result.data(), bytes, nullptr, nullptr);
    for (auto& c : result) if (static_cast<unsigned char>(c) < 32) c = ' ';
    return result;
}
std::string SourceId(Session* session) { HString id; return session && SUCCEEDED(session->get_SourceAppUserModelId(id.GetAddressOf())) ? Text(id.Get()) : ""; }
template <typename T, typename Method> std::string Property(T* object, Method method) {
    HString value; return SUCCEEDED((object->*method)(value.GetAddressOf())) ? Text(value.Get()) : "";
}
struct Revokers {
    std::vector<std::function<void()>> items;
    void Clear() { for (auto& remove : items) remove(); items.clear(); }
    ~Revokers() { Clear(); }
};
void Revoke(Revokers& items) { items.Clear(); }
template <typename Handler, typename T, typename Add, typename Remove>
void Subscribe(const ComPtr<T>& object, Add add, Remove remove, const std::shared_ptr<Signal>& signal, unsigned mask, Revokers& revokers) {
    auto lease = std::make_shared<DelegateLease>(signal);
    auto handler = Microsoft::WRL::Callback<Handler>([lease, mask](auto*, auto*) -> HRESULT {
        if (!lease->signal->stop.load()) lease->signal->Notify(mask);
        return S_OK;
    });
    EventRegistrationToken token{};
    if (SUCCEEDED((object.Get()->*add)(handler.Get(), &token))) {
        try { revokers.items.emplace_back([object, remove, token] { (object.Get()->*remove)(token); }); }
        catch (...) { (object.Get()->*remove)(token); throw; }
    }
}
}

struct WindowsSession::Impl {
    std::shared_ptr<Signal> signal;
    ComPtr<Manager> manager;
    ComPtr<Session> selected;
    ComPtr<IUnknown> identity;
    Revokers managerEvents, sessionEvents;
    MediaSnapshot snapshot;
    NetEaseTimeline netEaseTimeline;
    NetEaseLyrics netEaseLyrics;
    Clock::time_point nextMetadata{};
    explicit Impl(std::shared_ptr<Signal> s) : signal(std::move(s)) {
        ComPtr<M::IGlobalSystemMediaTransportControlsSessionManagerStatics> factory;
        Microsoft::WRL::Wrappers::HStringReference name(RuntimeClass_Windows_Media_Control_GlobalSystemMediaTransportControlsSessionManager);
        Require(RoGetActivationFactory(name.Get(), IID_PPV_ARGS(&factory)));
        ComPtr<__FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager> operation;
        Require(factory->RequestAsync(&operation));
        if (!Await(operation.Get(), signal)) throw std::runtime_error("Media session manager unavailable");
        Require(operation->GetResults(&manager));
        Require(manager ? S_OK : E_POINTER);
        Subscribe<__FITypedEventHandler_2_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager_Windows__CMedia__CControl__CCurrentSessionChangedEventArgs>(
            manager, &Manager::add_CurrentSessionChanged, &Manager::remove_CurrentSessionChanged, signal, 7, managerEvents);
        Subscribe<__FITypedEventHandler_2_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager_Windows__CMedia__CControl__CSessionsChangedEventArgs>(
            manager, &Manager::add_SessionsChanged, &Manager::remove_SessionsChanged, signal, 7, managerEvents);
    }
    ~Impl() { Revoke(sessionEvents); Revoke(managerEvents); }
    void Switch(ComPtr<Session> next) {
        ComPtr<IUnknown> nextIdentity; if (next) Require(next.As(&nextIdentity));
        if (nextIdentity.Get() == identity.Get()) return;
        Revoke(sessionEvents); selected = next; identity = nextIdentity;
        netEaseTimeline.Reset();
        netEaseLyrics.Reset();
        auto sources = std::move(snapshot.sources);
        snapshot = {}; snapshot.sources = std::move(sources); snapshot.session = ++sequence;
        nextMetadata = {}; signal->Notify(6);
        if (!selected) return;
        snapshot.source = SourceId(selected.Get());
        Subscribe<__FITypedEventHandler_2_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_Windows__CMedia__CControl__CMediaPropertiesChangedEventArgs>(
            selected, &Session::add_MediaPropertiesChanged, &Session::remove_MediaPropertiesChanged, signal, 2, sessionEvents);
        Subscribe<__FITypedEventHandler_2_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_Windows__CMedia__CControl__CPlaybackInfoChangedEventArgs>(
            selected, &Session::add_PlaybackInfoChanged, &Session::remove_PlaybackInfoChanged, signal, 4, sessionEvents);
        Subscribe<__FITypedEventHandler_2_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_Windows__CMedia__CControl__CTimelinePropertiesChangedEventArgs>(
            selected, &Session::add_TimelinePropertiesChanged, &Session::remove_TimelinePropertiesChanged, signal, 4, sessionEvents);
    }
    void Metadata() {
        const auto revision = signal->mediaRevision.load();
        ComPtr<__FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionMediaProperties> operation;
        Require(selected->TryGetMediaPropertiesAsync(&operation));
        if (!Await(operation.Get(), signal)) return;
        ComPtr<M::IGlobalSystemMediaTransportControlsSessionMediaProperties> props; Require(operation->GetResults(&props));
        Require(props ? S_OK : E_POINTER);
        const auto title = Property(props.Get(), &M::IGlobalSystemMediaTransportControlsSessionMediaProperties::get_Title);
        const auto artist = Property(props.Get(), &M::IGlobalSystemMediaTransportControlsSessionMediaProperties::get_Artist);
        const auto album = Property(props.Get(), &M::IGlobalSystemMediaTransportControlsSessionMediaProperties::get_AlbumTitle);
        const bool changed = title != snapshot.title || artist != snapshot.artist || album != snapshot.album || !snapshot.track;
        std::shared_ptr<Artwork> artwork;
        // An event may replace a cover even when title/artist do not change.
        ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStreamReference> thumbnail;
        if (SUCCEEDED(props->get_Thumbnail(&thumbnail)) && thumbnail) {
            try { artwork = DecodeArtwork(thumbnail.Get(), signal); } catch (...) { /* Optional artwork. */ }
        }
        if (signal->stop.load() || revision != signal->mediaRevision.load()) { signal->dirty.fetch_or(2); return; }
        if (changed) snapshot.track = ++sequence;
        snapshot.title = title; snapshot.artist = artist; snapshot.album = album;
        if (artwork && snapshot.artwork && artwork->width == snapshot.artwork->width &&
            artwork->height == snapshot.artwork->height && artwork->rgba == snapshot.artwork->rgba) return;
        if (artwork) artwork->version = ++sequence;
        snapshot.artwork = std::move(artwork);
    }
    void Playback() {
        snapshot.measuredTimeline = false;
        snapshot.songId.clear(); snapshot.lyrics.reset();
        ComPtr<M::IGlobalSystemMediaTransportControlsSessionPlaybackInfo> info;
        Require(selected->GetPlaybackInfo(&info));
        // A disappearing media session can succeed with a null result. Route
        // this through the worker's retry path rather than dereferencing it.
        Require(info ? S_OK : E_POINTER);
        M::GlobalSystemMediaTransportControlsSessionPlaybackStatus status{};
        Require(info->get_PlaybackStatus(&status));
        snapshot.playing = status == M::GlobalSystemMediaTransportControlsSessionPlaybackStatus_Playing;
        snapshot.rate = 1;
        ComPtr<__FIReference_1_double> rate;
        if (SUCCEEDED(info->get_PlaybackRate(&rate)) && rate) rate->get_Value(&snapshot.rate);
        ComPtr<M::IGlobalSystemMediaTransportControlsSessionPlaybackControls> controls;
        Require(info->get_Controls(&controls));
        Require(controls ? S_OK : E_POINTER);
        boolean value{};
#define CAP(field, name) value = false; controls->get_##name(&value); snapshot.field = value != 0
        CAP(canPlay, IsPlayEnabled); CAP(canPause, IsPauseEnabled); CAP(canToggle, IsPlayPauseToggleEnabled);
        CAP(canPrevious, IsPreviousEnabled); CAP(canNext, IsNextEnabled); CAP(canSeek, IsPlaybackPositionEnabled);
#undef CAP
        ComPtr<M::IGlobalSystemMediaTransportControlsSessionTimelineProperties> timeline;
        Require(selected->GetTimelineProperties(&timeline));
        Require(timeline ? S_OK : E_POINTER);
        ABI::Windows::Foundation::TimeSpan time{};
#define TIME(field, name) time = {}; timeline->get_##name(&time); snapshot.field = time.Duration / 10000000.0
        TIME(start, StartTime); TIME(end, EndTime); TIME(position, Position); TIME(minSeek, MinSeekTime); TIME(maxSeek, MaxSeekTime);
#undef TIME
        ABI::Windows::Foundation::DateTime updated{}; timeline->get_LastUpdatedTime(&updated);
        FILETIME now{}; GetSystemTimeAsFileTime(&now);
        const auto utc = (static_cast<std::uint64_t>(now.dwHighDateTime) << 32) | now.dwLowDateTime;
        if (snapshot.playing && updated.UniversalTime > 0 && utc >= static_cast<std::uint64_t>(updated.UniversalTime))
            snapshot.position += (utc - updated.UniversalTime) / 10000000.0 * snapshot.rate;
        snapshot.sampled = Clock::now(); snapshot.position = snapshot.Position(snapshot.sampled);
        if (snapshot.maxSeek <= snapshot.minSeek) { snapshot.minSeek = snapshot.start; snapshot.maxSeek = snapshot.end; }
        netEaseTimeline.Apply(snapshot);
    }
};

WindowsSession::WindowsSession(std::shared_ptr<Signal> signal) : impl_(std::make_unique<Impl>(std::move(signal))) {}
WindowsSession::~WindowsSession() = default;
MediaSnapshot WindowsSession::Refresh(const std::string& source, bool lyricsEnabled) {
    auto& p = *impl_;
    ComPtr<__FIVectorView_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession> sessions;
    Require(p.manager->GetSessions(&sessions));
    Require(sessions ? S_OK : E_POINTER);
    unsigned count{}; Require(sessions->get_Size(&count));
    p.snapshot.sources.clear(); ComPtr<Session> next;
    const bool automatic = source.empty() || source == "Auto";
    if (automatic) p.manager->GetCurrentSession(&next);
    for (unsigned i = 0; i < std::min(count, 64u); ++i) {
        ComPtr<Session> item; if (FAILED(sessions->GetAt(i, &item)) || !item) continue;
        const auto id = SourceId(item.Get());
        p.snapshot.sources.push_back({id, _stricmp(id.c_str(), "cloudmusic.exe") == 0 ? "NetEase Cloud Music" : id});
        if (!automatic && !next && id == source) next = item;
    }
    p.Switch(next);
    if (!p.selected) { p.signal->dirty.exchange(0); p.snapshot.state = State::NoSession; return p.snapshot; }
    p.snapshot.state = State::Ready;
    const auto dirty = p.signal->dirty.exchange(0);
    if ((dirty & 2) || Clock::now() >= p.nextMetadata) {
        p.nextMetadata = Clock::now() + std::chrono::seconds(5);
        p.Metadata();
    }
    if (!p.signal->stop.load()) p.Playback();
    if (lyricsEnabled && !p.signal->stop.load() && !p.snapshot.songId.empty()) {
        const auto revision = p.signal->mediaRevision.load(); const auto song = p.snapshot.songId;
        auto lyrics = p.netEaseLyrics.Get(song, p.signal);
        // Download can span a seek or a track change. Resample before publishing.
        if (!p.signal->stop.load() && revision == p.signal->mediaRevision.load()) {
            p.Playback();
            if (song == p.snapshot.songId) p.snapshot.lyrics = std::move(lyrics);
        }
    }
    return p.snapshot;
}
bool WindowsSession::Execute(const MediaCommand& command) {
    auto& p = *impl_;
    if (!p.selected || command.session != p.snapshot.session || command.track != p.snapshot.track ||
        p.signal->stop.load() || (p.signal->dirty.load() & 3)) return false;
    p.Playback(); // Recheck support against the current player state.
    ComPtr<__FIAsyncOperation_1_boolean> operation; HRESULT hr = E_NOTIMPL;
    if (command.action == Action::Toggle) {
        if (p.snapshot.canToggle) hr = p.selected->TryTogglePlayPauseAsync(&operation);
        else if (p.snapshot.playing && p.snapshot.canPause) hr = p.selected->TryPauseAsync(&operation);
        else if (!p.snapshot.playing && p.snapshot.canPlay) hr = p.selected->TryPlayAsync(&operation);
    } else if (command.action == Action::Previous && p.snapshot.canPrevious) hr = p.selected->TrySkipPreviousAsync(&operation);
    else if (command.action == Action::Next && p.snapshot.canNext) hr = p.selected->TrySkipNextAsync(&operation);
    else if (command.action == Action::Seek && p.snapshot.canSeek && p.snapshot.HasDuration()) {
        const double target = std::clamp(command.position, p.snapshot.minSeek, p.snapshot.maxSeek);
        hr = p.selected->TryChangePlaybackPositionAsync(static_cast<INT64>(target * 10000000.0), &operation);
    }
    if (FAILED(hr) || !Await(operation.Get(), p.signal)) return false;
    boolean success{}; const bool result = SUCCEEDED(operation->GetResults(&success)) && success;
    p.signal->Notify(6); return result;
}
}
