#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace nova::smtc::backend {
struct NetEaseLayout {
    std::uint32_t engine{}, progress{}, infoVtable{};
};

namespace layout_detail {
template<std::size_t N>
bool Matches(std::span<const unsigned char> bytes, std::size_t at, const int (&pattern)[N]) {
    if (at > bytes.size() || N > bytes.size() - at) return false;
    for (std::size_t i = 0; i < N; ++i)
        if (pattern[i] >= 0 && bytes[at + i] != pattern[i]) return false;
    return true;
}
template<std::size_t N>
std::optional<std::size_t> Unique(std::span<const unsigned char> bytes, const int (&pattern)[N]) {
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i + N <= bytes.size(); ++i) {
        if (bytes[i] != pattern[0] || !Matches(bytes, i, pattern)) continue;
        if (found) return {}; // Ambiguous code is not a compatible layout.
        found = i;
    }
    return found;
}
inline std::optional<std::uint32_t> Relative(std::span<const unsigned char> bytes,
    std::size_t displacement, std::uint32_t rva, std::uint32_t imageSize) {
    if (displacement > bytes.size() || 4 > bytes.size() - displacement) return {};
    std::int32_t delta{};
    std::memcpy(&delta, bytes.data() + displacement, sizeof(delta));
    const auto target = static_cast<std::int64_t>(rva) + displacement + 4 + delta;
    if (target < 0x1000 || target >= imageSize) return {};
    return static_cast<std::uint32_t>(target);
}
}

// Resolve relationships in compatible x64 player code, not version-specific RVAs.
// Code shape, object size, string accessor, singleton and publisher must all agree.
// ASLR and linker shifts are supported; a changed structure must fail closed.
template<class Reader>
std::optional<NetEaseLayout> ResolveNetEaseLayout(std::span<const unsigned char> code,
    std::uint32_t codeRva, std::uint64_t base, std::uint32_t imageSize, Reader read) {
    using namespace layout_detail;
    constexpr int constructor[] = {
        0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,
        0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xfa,0x48,0x8b,0xe9,0xe8,-1,-1,-1,-1,
        0x48,0x8b,0xc8,0xba,1,0,0,0,0xe8,-1,-1,-1,-1,0xb9,0x10,2,0,0,0xe8,-1,-1,-1,-1,
        0x48,0x8b,0xf0,0x48,0x89,0x44,0x24,0x30,0x48,0x8d,5,-1,-1,-1,-1,0x48,0x89,6};
    constexpr int publisher[] = {
        0x48,0x8b,0xc4,0x53,0x48,0x81,0xec,0xa0,0,0,0,0x0f,0x29,0x70,0xe8,0x0f,0x29,0x78,0xd8,
        0x48,0x8b,5,-1,-1,-1,-1,0x48,0x33,0xc4,0x48,0x89,0x44,0x24,0x70,
        0x0f,0x28,0xf2,0x0f,0x28,0xf9,0x48,0x8b,0xd9,0x66,0x0f,0x2e,0x0d,-1,-1,-1,-1,
        0x7a,0x12,0x75,0x10,0x66,0x0f,0x2e,0x15,-1,-1,-1,-1,0x7a,6,0x0f,0x84,-1,-1,-1,-1,
        0xf2,0x0f,0x11,0x3d,-1,-1,-1,-1,0xf2,0x0f,0x11,0x35,-1,-1,-1,-1};
    const auto ctor = Unique(code, constructor), pub = Unique(code, publisher);
    if (!ctor || !pub) return {};
    const auto singleton = Relative(code, *ctor + 0x1b, codeRva, imageSize);
    const auto vtable = Relative(code, *ctor + 0x41, codeRva, imageSize);
    const auto progress = Relative(code, *pub + 0x4b, codeRva, imageSize);
    const auto compared = Relative(code, *pub + 0x2f, codeRva, imageSize);
    const auto second = Relative(code, *pub + 0x53, codeRva, imageSize);
    const auto secondCompared = Relative(code, *pub + 0x3b, codeRva, imageSize);
    if (!singleton || !vtable || !progress || progress != compared || !second || second != secondCompared ||
        *second != *progress + 8 || *singleton < codeRva || *singleton - codeRva >= code.size()) return {};
    const auto accessor = code.subspan(*singleton - codeRva);
    constexpr int prologue[] = {0x40,0x55,0x57,0x48,0x81,0xec,0xe8,0,0,0,0x48,0x8d,0x6c,0x24,0x20};
    constexpr int initialize[] = {0x48,0x8d,0x0d,-1,-1,-1,-1,0xe8};
    constexpr int result[] = {0x48,0x8d,5,-1,-1,-1,-1,0x48,0x8d,0xa5,0xc8,0,0,0,0x5f,0x5d,0xc3};
    if (!Matches(accessor, 0, prologue) || !Matches(accessor, 0x55, initialize) || !Matches(accessor, 0x7a, result)) return {};
    const auto engine = Relative(accessor, 0x7d, *singleton, imageSize);
    if (!engine || engine != Relative(accessor, 0x58, *singleton, imageSize) || *engine > imageSize - 0xb0 ||
        *vtable > imageSize - 32 || *progress > imageSize - 16) return {};
    std::uint64_t getter{};
    if (!read(base + *vtable + 24, &getter, sizeof(getter)) || getter < base + codeRva ||
        getter - base - codeRva >= code.size()) return {};
    constexpr int songAccessor[] = {0x40,0x53,0x48,0x83,0xec,0x30,0x48,0x8b,0xda,0x48,0x8d,0x51,0x70,0x48,0x8b,0xcb,
        0xe8,-1,-1,-1,-1,0x48,0x8b,0xc3,0x48,0x83,0xc4,0x30,0x5b,0xc3};
    if (!Matches(code, static_cast<std::size_t>(getter - base - codeRva), songAccessor)) return {};
    return NetEaseLayout{*engine, *progress, *vtable};
}
}
