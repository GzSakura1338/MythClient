#include "minecraft/java/api/game_beds.h"
#include "minecraft/java/game/BedScanRules.h"
#include "minecraft/java/api/base.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/BedMapping.h"
#include "minecraft/compat/lunar1710/Access.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <limits>

#define NOVA_BEDPLATES_MAPPING
#include "minecraft/compat/mappings/Lunar189.inc"
#include "minecraft/compat/mappings/forge1201.inc"
#include "minecraft/compat/mappings/minecraft263.inc"
#undef NOVA_BEDPLATES_MAPPING

namespace nova::java {
namespace {
bool Good(JNIEnv* e) { if (!e->ExceptionCheck()) return true; e->ExceptionClear(); return false; }
struct LocalScope {
    JNIEnv* e; bool ok;
    explicit LocalScope(JNIEnv* env) : e(env), ok(e->PushLocalFrame(128) == 0) { if (!ok) Good(e); }
    ~LocalScope() { if (ok) e->PopLocalFrame(nullptr); }
};
double Distance(const rotation::Vec3& a, const rotation::Vec3& b) {
    const auto x = a.x - b.x, y = a.y - b.y, z = a.z - b.z; return x*x + y*y + z*z;
}
std::string HumanName(std::string value) {
    const auto colon = value.find(':');
    if (colon != std::string::npos) value.erase(0, colon + 1);
    const auto end = value.find_first_of("}["); if (end != std::string::npos) value.resize(end);
    bool upper = true;
    for (auto& c : value) { if (c == '_') { c = ' '; upper = true; } else if (upper) { if (c >= 'a' && c <= 'z') c -= 'a' - 'A'; upper = false; } }
    return value.empty() || value.size() > 64 ? "Defense" : value;
}
struct Reader {
    JNIEnv* e;
    bool legacy = compat::IsLegacyLunar();
    const compat::BedMapping& m = legacy ? compat::Bed189Mapping : compat::UsesModernRuntime() ? compat::Bed263Mapping : compat::Bed1201Mapping;
    jobject world{}; jclass posClass{}, bedClass{};
    jmethodID ctor{}, state{}, chunk{}, stringMethod{};
    bool valid{};
    jmethodID Method(jobject object, const char* name, const std::string& desc) {
        if (!object) return nullptr;
        jclass cls = e->GetObjectClass(object); if (!Good(e) || !cls) return nullptr;
        auto id = e->GetMethodID(cls, name, desc.c_str()); e->DeleteLocalRef(cls);
        return Good(e) ? id : nullptr;
    }
    jobject Call(jobject object, const char* name, const char* desc) {
        auto id = Method(object, name, desc); if (!id) return nullptr;
        auto value = e->CallObjectMethod(object, id); return Good(e) ? value : nullptr;
    }
    explicit Reader(JNIEnv* env) : e(env) {
        auto client = compat::Minecraft(e); if (!client) return;
        auto cls = e->GetObjectClass(client);
        const auto* mapping = compat::Current().mapping;
        if (!cls || !mapping) return;
        const auto field = e->GetFieldID(cls, mapping->world, mapping->worldType);
        if (!Good(e) || !field) return;
        world = e->GetObjectField(client, field); if (!Good(e) || !world) return;
        if (compat::IsLunar1710()) {
            bedClass=compat::mc263::access::Class(e,"net.minecraft.block.BlockBed");
            chunk=Method(world,m.getChunk,std::string("(II)")+m.chunkSig);
            valid=Good(e) && bedClass && chunk;
            return;
        }
        auto loader = compat::ClassLoader(e);
        posClass = LoadClassInLoader(e, loader, m.posClass);
        if (Good(e) && posClass) bedClass = LoadClassInLoader(e, loader, m.bedClass);
        if (loader) e->DeleteGlobalRef(loader);
        if (!Good(e) || !posClass || !bedClass) return;
        ctor = e->GetMethodID(posClass, "<init>", "(III)V"); if (!Good(e) || !ctor) return;
        state = Method(world, m.getState, std::string("(") + m.posSig + ")" + m.stateSig);
        chunk = Method(world, m.getChunk, std::string("(II)") + m.chunkSig);
        auto objectClass = e->FindClass("java/lang/Object");
        stringMethod = objectClass ? e->GetMethodID(objectClass, "toString", "()Ljava/lang/String;") : nullptr;
        valid = Good(e) && state && chunk && stringMethod;
    }
    std::string Text(jobject object) {
        auto string = static_cast<jstring>(e->CallObjectMethod(object, stringMethod));
        if (!Good(e) || !string) return {};
        const char* data = e->GetStringUTFChars(string, nullptr); std::string result;
        if (data) { result = data; e->ReleaseStringUTFChars(string, data); }
        e->DeleteLocalRef(string); Good(e); return result;
    }
    jobject Pos(const BlockPos& p) { auto value = e->NewObject(posClass, ctor, p.x,p.y,p.z); return Good(e) ? value : nullptr; }
    jobject State(jobject p) { if (!p) return nullptr; auto value = e->CallObjectMethod(world, state, p); return Good(e) ? value : nullptr; }
    jobject Block(jobject s) { const std::string desc = std::string("()") + m.blockSig; return Call(s, m.getBlock, desc.c_str()); }
    bool Bed(jobject s, jobject b, BlockPos& foot, const BlockPos& head) {
        if (!s || !b || !e->IsInstanceOf(b, bedClass)) return false;
        return beds::Foot(Text(s),head,foot);
    }
    bool Read(const BlockPos& head, BedPlate& result) {
        if (compat::IsLunar1710()) return Read17(head,result);
        LocalScope scope(e); if (!scope.ok) return false;
        auto pos = Pos(head), s = State(pos), b = s ? Block(s) : nullptr; BlockPos foot;
        if (!Bed(s,b,foot,head)) return false;
        result = {}; result.head = head;
        result.center = {(head.x + foot.x + 1.) / 2, head.y + .5, (head.z + foot.z + 1.) / 2};
        constexpr int faces[][3] = {{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}};
        for (const auto half : {head,foot}) for (const auto& f : faces) {
            LocalScope neighbour(e); if (!neighbour.ok) return false;
            const BlockPos at{half.x+f[0], half.y+f[1], half.z+f[2]};
            auto p = Pos(at), bs = State(p), block = bs ? Block(bs) : nullptr;
            if (!block || e->IsInstanceOf(block, bedClass)) continue;
            auto solidObject = legacy ? Call(block,m.material,m.materialDesc) : bs;
            auto solidMethod = Method(solidObject, m.solid, "()Z");
            if (!solidMethod) return false;
            const bool solid = e->CallBooleanMethod(solidObject, solidMethod) == JNI_TRUE;
            if (!Good(e)) return false;
            if (!solid) continue;
            auto hardObject = legacy ? block : bs;
            auto hardness = Method(hardObject, m.hardness, m.hardnessDesc);
            if (!hardness) return false;
            const float value = e->CallFloatMethod(hardObject, hardness, world, p);
            if (!Good(e)) return false;
            if (beds::Defense(value,true,false,result)) result.material = HumanName(Text(bs));
        }
        if (!result.covered) result.material = "Bed";
        return true;
    }
    bool Read17(const BlockPos& head, BedPlate& result) {
        namespace n=compat::lunar1710::names;
        compat::lunar1710::Frame f(e);
        auto block=compat::lunar1710::Block(f,world,head.x,head.y,head.z);
        if (!block || !e->IsInstanceOf(block,bedClass)) return false;
        const int meta=f.call<int>(world,n::BlockMetadata,head.x,head.y,head.z);
        if (!f || !(meta&8)) return false;
        // Bed metadata: south, west, north, east; the head is one step from foot.
        constexpr int dx[]{0,-1,0,1},dz[]{1,0,-1,0};
        BlockPos foot{head.x-dx[meta&3],head.y,head.z-dz[meta&3]};
        result={};result.head=head;
        result.center={(head.x+foot.x+1.)/2,head.y+.5,(head.z+foot.z+1.)/2};
        constexpr int faces[][3]={{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}};
        for (auto half:{head,foot}) for (auto& face:faces) {
            compat::lunar1710::Frame cell(e);
            const BlockPos p{half.x+face[0],half.y+face[1],half.z+face[2]};
            auto b=compat::lunar1710::Block(cell,world,p.x,p.y,p.z);
            if (!b || e->IsInstanceOf(b,bedClass)) continue;
            const bool solid=cell.call<bool>(cell.call(b,n::Material),{"net.minecraft.block.material.Material","isSolid","()Z"});
            const float hardness=cell.call<float>(b,n::Hardness,world,p.x,p.y,p.z);
            if (!cell) return false;
            if (beds::Defense(hardness,solid,false,result)) {
                auto registry=cell.stat({"net.minecraft.block.Block","blockRegistry","Lnet/minecraft/util/RegistryNamespaced;"});
                auto name=cell.call(registry,{"net.minecraft.util.RegistryNamespaced","getNameForObject","(Ljava/lang/Object;)Ljava/lang/String;"},b);
                result.material=HumanName(cell.text(name));
            }
        }
        if (!result.covered) result.material="Bed";
        return f.check();
    }
    bool Scan(int x, int z, std::vector<BlockPos>& heads) {
        LocalScope scope(e); if (!scope.ok) return false;
        auto c = e->CallObjectMethod(world, chunk, x,z); if (!Good(e)) return false;
        if (!c) return true;
        if (legacy) {
            auto sections = static_cast<jobjectArray>(Call(c,m.sections,m.sectionsDesc));
            if (!sections) return false;
            const int count = std::min(16, static_cast<int>(e->GetArrayLength(sections)));
            for (int sy = 0; sy < count; ++sy) {
                LocalScope sectionScope(e); if (!sectionScope.ok) return false;
                auto section = e->GetObjectArrayElement(sections,sy); if (!Good(e)) return false;
                if (!section) continue;
                if (compat::IsLunar1710()) {
                    auto data=static_cast<jbyteArray>(Call(section,"getBlockLSBArray","()[B"));
                    if (!data || e->GetArrayLength(data)!=4096) return false;
                    std::array<jbyte,4096> cells{};
                    e->GetByteArrayRegion(data,0,4096,cells.data());
                    auto block=Method(section,"getBlockByExtId","(III)Lnet/minecraft/block/Block;");
                    auto metadata=Method(section,"getExtBlockMetadata","(III)I");
                    if (!Good(e) || !block || !metadata) return false;
                    for(int i=0;i<4096 && heads.size()<512;++i) {
                        if (static_cast<unsigned char>(cells[i])!=26) continue;
                        LocalScope cell(e); if (!cell.ok) return false;
                        auto b=e->CallObjectMethod(section,block,i&15,i>>8,(i>>4)&15);
                        const int meta=e->CallIntMethod(section,metadata,i&15,i>>8,(i>>4)&15);
                        if (!Good(e)) return false;
                        if (b && e->IsInstanceOf(b,bedClass) && (meta&8)) heads.push_back(beds::Cell189(x,z,sy,i));
                    }
                    continue;
                }
                auto data = static_cast<jcharArray>(Call(section,m.sectionData,"()[C"));
                if (!data || e->GetArrayLength(data) != 4096) return false;
                std::array<jchar,4096> cells{}; e->GetCharArrayRegion(data,0,4096,cells.data()); if (!Good(e)) return false;
                for (int i=0; i<4096 && heads.size()<512; ++i)
                    if (beds::Head189(cells[i])) heads.push_back(beds::Cell189(x,z,sy,i));
            }
        } else {
            auto map = Call(c,m.chunkMap,"()Ljava/util/Map;");
            auto keys = map ? Call(map,"keySet","()Ljava/util/Set;") : nullptr;
            auto array = static_cast<jobjectArray>(keys ? Call(keys,"toArray","()[Ljava/lang/Object;") : nullptr);
            if (!array) return false;
            const int count = std::min(4096, static_cast<int>(e->GetArrayLength(array)));
            for (int i=0; i<count && heads.size()<512; ++i) {
                LocalScope entry(e); if (!entry.ok) return false;
                auto pos = e->GetObjectArrayElement(array,i); if (!Good(e) || !pos) continue;
                auto gx = Method(pos,m.getX,"()I"), gy = Method(pos,m.getY,"()I"), gz = Method(pos,m.getZ,"()I");
                if (!gx || !gy || !gz) return false;
                BlockPos p, foot;
                p.x=e->CallIntMethod(pos,gx); if (!Good(e)) return false;
                p.y=e->CallIntMethod(pos,gy); if (!Good(e)) return false;
                p.z=e->CallIntMethod(pos,gz); if (!Good(e)) return false;
                auto s = State(pos), b = s ? Block(s) : nullptr;
                if (Bed(s,b,foot,p)) heads.push_back(p);
            }
        }
        return true;
    }
};
}
struct BedScanner::Impl {
    jobject world{};
    rotation::Vec3 home{}, origin{}, previous{};
    std::vector<BlockPos> heads;
    std::vector<std::pair<int,int>> chunks;
    std::vector<BedPlate> entries;
    std::size_t cursor{};
    int radius{}, ticks{};
    std::chrono::steady_clock::time_point next{};
    void Clear(JNIEnv* e) {
        if (world) e->DeleteGlobalRef(world);
        world = nullptr; heads.clear(); chunks.clear(); entries.clear(); cursor = 0; radius = ticks = 0; next = {};
    }
};
BedScanner::BedScanner() : impl_(std::make_unique<Impl>()) {}
BedScanner::~BedScanner() { Reset(); }
void BedScanner::Reset() { JniAttach attach; if (attach) impl_->Clear(attach.env); }
bool BedScanner::Tick(const rotation::Vec3& player, int range, int refreshTicks, std::vector<BedPlate>& out) {
    out.clear(); JniAttach attach; if (!attach) return false;
    auto* e = attach.env; LocalScope scope(e); if (!scope.ok) return false;
    Reader r(e); auto& p = *impl_;
    if (!r.valid) { p.Clear(e); return false; }
    if (!p.world || !e->IsSameObject(p.world,r.world)) {
        p.Clear(e); p.world = e->NewGlobalRef(r.world); if (!p.world || !Good(e)) return false;
        p.home = p.previous = player;
    }
    // Rise's Breaker updates its home anchor after a teleport farther than 40 blocks.
    if (Distance(player,p.previous)>40*40) p.home=player;
    p.previous=player;
    range = std::clamp(range,20,200);
    const auto now = std::chrono::steady_clock::now();
    if (p.radius != range || Distance(player,p.origin) > 32*32 || (p.cursor == p.chunks.size() && now >= p.next)) {
        p.chunks.clear(); p.cursor=0; p.radius=range; p.origin=player;
        const int cx = static_cast<int>(std::floor(player.x/16)), cz = static_cast<int>(std::floor(player.z/16));
        const int radius = (range+15)/16;
        for (int x=-radius; x<=radius; ++x) for (int z=-radius; z<=radius; ++z) p.chunks.emplace_back(cx+x,cz+z);
        std::stable_sort(p.chunks.begin(),p.chunks.end(),[&](auto a,auto b) {
            return (a.first-cx)*(a.first-cx)+(a.second-cz)*(a.second-cz) < (b.first-cx)*(b.first-cx)+(b.second-cz)*(b.second-cz);
        });
        p.next=now+std::chrono::seconds(5);
    }
    const auto deadline=now+std::chrono::milliseconds(3);
    for (int n=0; n<8 && p.cursor<p.chunks.size(); ++n) {
        const auto [x,z]=p.chunks[p.cursor++]; std::vector<BlockPos> found;
        if (!r.Scan(x,z,found)) { p.Clear(e); return false; }
        std::erase_if(p.heads,[&](auto h){return static_cast<int>(std::floor(h.x/16.))==x && static_cast<int>(std::floor(h.z/16.))==z;});
        for (auto h:found) if (p.heads.size()<512) p.heads.push_back(h);
        if (std::chrono::steady_clock::now()>=deadline) break;
    }
    if (++p.ticks>=std::clamp(refreshTicks,1,20) || p.entries.empty()) {
        p.ticks=0; p.entries.clear();
        std::erase_if(p.heads,[&](auto h) { return Distance({h.x+.5,h.y+.5,h.z+.5},player)> (range+16.)*(range+16.); });
        for (const auto head:p.heads) {
            if (Distance({head.x+.5,head.y+.5,head.z+.5},player)>range*range ||
                std::abs(head.y-player.y)>std::min(100,std::max(4,range/2))) continue;
            BedPlate plate;
            if (!r.Read(head,plate)) continue;
            plate.own = Distance(plate.center,p.home)<=4.5*4.5;
            p.entries.push_back(std::move(plate));
        }
    }
    out=p.entries; return true;
}
}
