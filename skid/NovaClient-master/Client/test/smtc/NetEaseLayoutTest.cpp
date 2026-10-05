#include "smtc/backend/NetEaseLayout.h"
#include <cassert>
#include <cstdio>
#include <sstream>
#include <vector>

// Instruction fixtures from the updated x64 player, relocated independently of
// their original PE addresses. Never requires an installed/running music client.
namespace {
struct Fixture {
    std::uint64_t base;
    std::uint32_t codeRva, imageSize, ctor, publisher, singleton, getter, engine, progress, vtable;
    std::vector<unsigned char> code = std::vector<unsigned char>(0x1000, 0xcc);
    bool readable = true;
    Fixture(std::uint64_t module, std::uint32_t shift)
        : base(module), codeRva(0x1000 + shift), imageSize(0x20000 + shift),
          ctor(0x40), publisher(0x180), singleton(0x280), getter(0x380),
          engine(0x18000 + shift), progress(0x19000 + shift), vtable(0x12000 + shift) {
        Put(ctor, "48 89 5c 24 10 48 89 6c 24 18 48 89 74 24 20 57 48 83 ec 20 48 8b fa 48 8b e9 e8 e1 b8 f5 00 48 8b c8 ba 01 00 00 00 e8 f4 f5 f5 00 b9 10 02 00 00 e8 7e 47 b9 00 48 8b f0 48 89 44 24 30 48 8d 05 2b ec ce 01 48 89 06");
        Put(publisher, "48 8b c4 53 48 81 ec a0 00 00 00 0f 29 70 e8 0f 29 78 d8 48 8b 05 5e c7 0d 02 48 33 c4 48 89 44 24 70 0f 28 f2 0f 28 f9 48 8b d9 66 0f 2e 0d 95 8f 0e 02 7a 12 75 10 66 0f 2e 15 91 8f 0e 02 7a 06 0f 84 c9 00 00 00 f2 0f 11 3d 79 8f 0e 02 f2 0f 11 35 79 8f 0e 02");
        Put(singleton, "40 55 57 48 81 ec e8 00 00 00 48 8d 6c 24 20 48 8d 0d 2f 38 6e 01 e8 c1 89 c3 ff b8 04 00 00 00 8b c0 8b 0d 20 c9 1f 01 65 48 8b 14 25 58 00 00 00 48 8b 0c ca 8b 04 08 39 05 22 ff 1f 01 7e 3a 48 8d 0d 19 ff 1f 01 e8 38 95 c3 ff 83 3d 0d ff 1f 01 ff 75 25 48 8d 0d 14 fe 1f 01 e8 3f e7 ff ff 48 8d 0d c8 7b 9d 00 e8 bf 92 c3 ff 90 48 8d 0d eb fe 1f 01 e8 aa 94 c3 ff 48 8d 05 ef fd 1f 01 48 8d a5 c8 00 00 00 5f 5d c3");
        Put(getter, "40 53 48 83 ec 30 48 8b da 48 8d 51 70 48 8b cb e8 db 17 fb ff 48 8b c3 48 83 c4 30 5b c3");
        Rel(ctor + 0x1b, codeRva + singleton); Rel(ctor + 0x41, vtable);
        Rel(publisher + 0x2f, progress); Rel(publisher + 0x4b, progress);
        Rel(publisher + 0x3b, progress + 8); Rel(publisher + 0x53, progress + 8);
        Rel(singleton + 0x58, engine); Rel(singleton + 0x7d, engine);
    }
    void Put(std::size_t at, const char* hex) {
        std::istringstream stream(hex); unsigned value{};
        while (stream >> std::hex >> value) code.at(at++) = static_cast<unsigned char>(value);
    }
    void Rel(std::size_t at, std::uint32_t target) {
        const auto delta = static_cast<std::int32_t>(target - codeRva - at - 4);
        std::memcpy(code.data() + at, &delta, sizeof(delta));
    }
    auto Resolve() {
        return nova::smtc::backend::ResolveNetEaseLayout(code, codeRva, base, imageSize,
            [&](std::uint64_t address, void* data, std::size_t size) {
                if (!readable || address != base + vtable + 24 || size != 8) return false;
                const std::uint64_t target = base + codeRva + getter;
                std::memcpy(data, &target, size); return true;
            });
    }
};
}
int main() {
    for (const auto base : {0x180000000ULL, 0x7ff800000000ULL}) {
        for (const auto shift : {0u, 0x13000u}) {
            Fixture f(base, shift);
            const auto layout = f.Resolve();
            assert(layout && layout->engine == f.engine && layout->progress == f.progress && layout->infoVtable == f.vtable);
            f.Rel(f.singleton + 0x58, f.engine + 16); assert(!f.Resolve()); // Different initialized singleton.
        }
    }
    Fixture f(0x180000000ULL, 0);
    f.Rel(f.publisher + 0x4b, f.progress + 16); assert(!f.Resolve());
    f.Rel(f.publisher + 0x4b, f.progress);
    f.code[f.getter + 12] = 0x78; assert(!f.Resolve()); // Changed song ID member offset.
    f.code[f.getter + 12] = 0x70;
    f.readable = false; assert(!f.Resolve()); f.readable = true;
    std::memcpy(f.code.data() + 0x600, f.code.data() + f.publisher, 0x57);
    assert(!f.Resolve()); // Two matching publishers cannot be guessed between.
    f.code.resize(0x600); assert(f.Resolve());
    f.code.resize(f.getter + 10); assert(!f.Resolve()); // Truncated accessor.
    puts("PASS: NetEase relocation, ASLR, identity relationships, changed layout, ambiguity and read failure");
}
