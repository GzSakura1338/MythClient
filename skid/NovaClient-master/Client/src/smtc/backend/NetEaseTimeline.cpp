#include "smtc/backend/NetEaseTimeline.h"
#include "smtc/backend/TimelineSample.h"
#include "smtc/backend/NetEaseLayout.h"
#include "utils/Json.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <array>
#include <cstring>
#include <utility>

namespace nova::smtc::backend {
namespace {
struct Handle {
    HANDLE value{};
    explicit Handle(HANDLE h = nullptr) : value(h == INVALID_HANDLE_VALUE ? nullptr : h) {}
    ~Handle() { if (value) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    void Reset(HANDLE h = nullptr) { if (value) CloseHandle(value); value = h; }
};
// cloudmusic.dll, NetEase 3.1.40.205461 x64. Gate these RVAs by PE identity AND
// accessor/publisher code bytes; never apply offsets to another build.
constexpr DWORD BuildTimestamp = 0x6a9e91a6, ImageSize = 0x2711000;
bool Read(HANDLE process, std::uintptr_t address, void* data, std::size_t size) {
    if (address < 0x10000 || address > 0x00007fffffffffffULL - size) return false;
    SIZE_T read{};
    return ReadProcessMemory(process, reinterpret_cast<void*>(address), data, size, &read) && read == size;
}
template<class T> bool Read(HANDLE process, std::uintptr_t address, T& data) {
    return Read(process, address, &data, sizeof(data));
}
bool String(HANDLE process, std::uintptr_t address, std::string& result) {
    struct StringLayout { char storage[16]; std::uint64_t size, capacity; } value{}, after{};
    if (!Read(process, address, value) || !value.size || value.size > 128 ||
        value.capacity < value.size || value.capacity > 4096) return false;
    if (value.capacity < 16) result.assign(value.storage, static_cast<std::size_t>(value.size));
    else {
        std::uintptr_t pointer{}; std::memcpy(&pointer, value.storage, sizeof(pointer));
        result.resize(static_cast<std::size_t>(value.size));
        if (!Read(process, pointer, result.data(), result.size())) return false;
    }
    return Read(process, address, after) && std::memcmp(&value, &after, sizeof(value)) == 0 &&
        std::all_of(result.begin(), result.end(), [](unsigned char c) { return c >= 32 && c < 127; });
}
std::optional<NetEaseLayout> FindLayout(HANDLE process, std::uintptr_t base) {
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS64 nt{};
    if (!Read(process, base, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 || dos.e_lfanew > 4096 ||
        !Read(process, base + dos.e_lfanew, nt) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt.OptionalHeader.SizeOfImage < 0x1000 || nt.OptionalHeader.SizeOfImage > 128 * 1024 * 1024 ||
        !nt.FileHeader.NumberOfSections || nt.FileHeader.NumberOfSections > 96) return {};
    constexpr unsigned char getter[] = {0x40,0x53,0x48,0x83,0xec,0x30,0x48,0x8b,0xda,0x48,0x8d,0x51,0x70,0x48,0x8b,0xcb};
    constexpr unsigned char publisher[] = {0x48,0x8b,0xc4,0x53,0x48,0x81,0xec,0xa0,0,0,0,0x0f,0x29,0x70,0xe8,0x0f,0x29,0x78,0xd8};
    unsigned char actualGetter[sizeof(getter)]{}, actualPublisher[sizeof(publisher)]{};
    if (nt.FileHeader.TimeDateStamp == BuildTimestamp && nt.OptionalHeader.SizeOfImage == ImageSize &&
        Read(process, base + 0xbe770, actualGetter) && Read(process, base + 0xbe510, actualPublisher) &&
        std::memcmp(getter, actualGetter, sizeof(getter)) == 0 && std::memcmp(publisher, actualPublisher, sizeof(publisher)) == 0)
        return NetEaseLayout{0x2200330, 0x2193468, 0x1d977c0};

    std::vector<IMAGE_SECTION_HEADER> sections(nt.FileHeader.NumberOfSections);
    const auto sectionTable = base + dos.e_lfanew + 24 + nt.FileHeader.SizeOfOptionalHeader;
    if (!Read(process, sectionTable, sections.data(), sections.size() * sizeof(IMAGE_SECTION_HEADER))) return {};
    std::optional<NetEaseLayout> resolved;
    for (const auto& section : sections) {
        if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        const auto size = section.Misc.VirtualSize;
        if (!size || section.VirtualAddress >= nt.OptionalHeader.SizeOfImage ||
            size > nt.OptionalHeader.SizeOfImage - section.VirtualAddress) return {};
        std::vector<unsigned char> code(size);
        if (!Read(process, base + section.VirtualAddress, code.data(), code.size())) return {};
        const auto layout = ResolveNetEaseLayout(code, section.VirtualAddress, base, nt.OptionalHeader.SizeOfImage,
            [&](std::uint64_t address, void* data, std::size_t count) { return Read(process, address, data, count); });
        if (!layout) continue;
        if (resolved) return {};
        resolved = layout;
    }
    if (!resolved) return {};
    // Resolved globals must be writable data; the object's vtable must be read-only.
    const auto inSection = [&](std::uint32_t rva, std::uint32_t size, bool writable) {
        for (const auto& section : sections) {
            if (!(section.Characteristics & IMAGE_SCN_MEM_READ) || (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
                !!(section.Characteristics & IMAGE_SCN_MEM_WRITE) != writable) continue;
            if (rva >= section.VirtualAddress && rva - section.VirtualAddress <= section.Misc.VirtualSize &&
                size <= section.Misc.VirtualSize - (rva - section.VirtualAddress)) return true;
        }
        return false;
    };
    if (!inSection(resolved->engine, 0xb0, true) || !inSection(resolved->progress, 16, true) ||
        !inSection(resolved->infoVtable, 32, false)) return {};
    return resolved;
}
std::string Field(const json::Value& value, const char* name) {
    const auto* field = value.Find(name); return field ? field->asString() : "";
}
}

struct NetEaseTimeline::Impl {
    Handle process;
    std::uintptr_t base{};
    NetEaseLayout layout{};
    Clock::time_point nextDiscovery{}, nextPlaylist{};
    TimelineGate gate;
    std::string cachedSong, title, artist;
    double catalogDuration{};
    void Clear() {
        process.Reset(); base = 0; layout = {}; gate.Reset(); cachedSong.clear(); title.clear(); artist.clear();
        catalogDuration = 0; nextPlaylist = {};
    }
    bool Discover() {
        if (process.value) {
            DWORD code{};
            if (GetExitCodeProcess(process.value, &code) && code == STILL_ACTIVE) return true;
            Clear();
        }
        const auto now = Clock::now();
        if (now < nextDiscovery) return false;
        nextDiscovery = now + std::chrono::seconds(5);
        Handle processes(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        PROCESSENTRY32W entry{}; entry.dwSize = sizeof(entry);
        if (!processes.value || !Process32FirstW(processes.value, &entry)) return false;
        do {
            if (_wcsicmp(entry.szExeFile, L"cloudmusic.exe") != 0) continue;
            Handle candidate(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID));
            if (!candidate.value) continue;
            Handle modules(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, entry.th32ProcessID));
            MODULEENTRY32W module{}; module.dwSize = sizeof(module);
            if (!modules.value || !Module32FirstW(modules.value, &module)) continue;
            do {
                if (_wcsicmp(module.szModule, L"cloudmusic.dll") != 0) continue;
                const auto moduleBase = reinterpret_cast<std::uintptr_t>(module.modBaseAddr);
                std::uintptr_t info{};
                const auto candidateLayout = FindLayout(candidate.value, moduleBase);
                if (candidateLayout && Read(candidate.value, moduleBase + candidateLayout->engine + 80, info) && info) {
                    process.Reset(std::exchange(candidate.value, nullptr)); base = moduleBase; layout = *candidateLayout; return true;
                }
            } while (Module32NextW(modules.value, &module));
        } while (Process32NextW(processes.value, &entry));
        return false;
    }
    bool Sample(TimelineSample& result) {
        TimelineSample after; std::uintptr_t vtable{};
        const auto engine = base + layout.engine;
        if (!Read(process.value, engine + 16, result.generation) || !Read(process.value, engine + 80, result.object) ||
            !Read(process.value, result.object, vtable) || vtable != base + layout.infoVtable ||
            !String(process.value, result.object + 16, result.playId) || !String(process.value, result.object + 112, result.songId) ||
            !Read(process.value, engine + 168, result.duration) || !Read(process.value, base + layout.progress, result.position) ||
            !Read(process.value, engine + 16, after.generation) || !Read(process.value, engine + 80, after.object) ||
            !String(process.value, after.object + 16, after.playId) || !String(process.value, after.object + 112, after.songId) ||
            !Read(process.value, engine + 168, after.duration)) return false;
        return result.SameTrack(after) && result.duration == after.duration && result.Valid();
    }
    bool MetadataMatches(const TimelineSample& sample, const MediaSnapshot& snapshot) {
        const auto now = Clock::now();
        if (cachedSong != sample.songId) {
            cachedSong = sample.songId; title.clear(); artist.clear(); catalogDuration = 0; nextPlaylist = {};
        }
        if (title.empty() && now >= nextPlaylist) {
            nextPlaylist = now + std::chrono::seconds(5);
            PWSTR folder{};
            if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DONT_VERIFY, nullptr, &folder))) return false;
            const std::wstring path = std::wstring(folder) + L"\\NetEase\\CloudMusic\\webdata\\file\\playingList";
            CoTaskMemFree(folder);
            Handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
            LARGE_INTEGER size{};
            if (!file.value || !GetFileSizeEx(file.value, &size) || size.QuadPart <= 0 || size.QuadPart > 8 * 1024 * 1024) return false;
            std::string bytes(static_cast<std::size_t>(size.QuadPart), '\0'); DWORD read{};
            if (!ReadFile(file.value, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) || read != bytes.size()) return false;
            const auto root = json::Parse(bytes); const auto* list = root ? root->Find("list") : nullptr;
            if (!list || !list->isArray()) return false;
            for (const auto& item : list->Items()) {
                const auto* track = item.Find("track");
                if (!track || Field(*track, "id") != sample.songId) continue;
                title = Field(*track, "name");
                const auto* duration = track->Find("duration");
                catalogDuration = duration ? duration->asNumber(0) / 1000.0 : 0;
                const auto* artists = track->Find("artists");
                if (artists && artists->isArray() && !artists->Items().empty()) artist = Field(artists->Items().front(), "name");
                break;
            }
        }
        // ID binds the native player to this metadata; title/artist bind it to SMTC.
        // A duration mismatch (e.g. preview playback) stays unavailable, not guessed.
        return !title.empty() && title == snapshot.title && !artist.empty() && snapshot.artist.find(artist) != std::string::npos &&
            std::isfinite(catalogDuration) && catalogDuration > 0 && std::abs(catalogDuration - sample.duration) <= 2;
    }
};

NetEaseTimeline::NetEaseTimeline() : impl_(std::make_unique<Impl>()) {}
NetEaseTimeline::~NetEaseTimeline() = default;
void NetEaseTimeline::Reset() { impl_->Clear(); impl_->nextDiscovery = {}; }
bool NetEaseTimeline::Apply(MediaSnapshot& snapshot) {
    if (_stricmp(snapshot.source.c_str(), "cloudmusic.exe") != 0 || snapshot.HasDuration()) { Reset(); return false; }
    auto& p = *impl_;
    try {
        if (!p.Discover()) return false;
        TimelineSample sample;
        if (!p.Sample(sample)) { p.Clear(); return false; }
        if (!p.MetadataMatches(sample, snapshot)) { p.gate.Reset(); return false; }
        if (!p.gate.Accept(sample, Clock::now())) return false;
        snapshot.start = snapshot.minSeek = 0; snapshot.end = snapshot.maxSeek = sample.duration;
        snapshot.position = std::clamp(sample.position, 0.0, sample.duration);
        snapshot.sampled = Clock::now();
        // Only measured progress. Do not extrapolate through pauses, buffering or stale events.
        snapshot.rate = 0; snapshot.canSeek = false; snapshot.measuredTimeline = true;
        snapshot.songId = sample.songId;
        return true;
    } catch (...) { p.Clear(); return false; }
}
}
