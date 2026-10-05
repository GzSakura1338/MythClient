#include "minecraft/java/api/class_edit.h"

#include <cstring>
#include <stdexcept>
#include <string>

u2 ClassFileEditor::rd16(const u1*& p) {
    u2 v = (u2(p[0]) << 8) | p[1];
    p += 2;
    return v;
}

u4 ClassFileEditor::rd32(const u1*& p) {
    u4 v = (u4(p[0]) << 24) | (u4(p[1]) << 16) | (u4(p[2]) << 8) | p[3];
    p += 4;
    return v;
}

void ClassFileEditor::wr16(std::vector<u1>& o, u2 v) {
    o.push_back(u1(v >> 8));
    o.push_back(u1(v & 0xFF));
}

void ClassFileEditor::wr32(std::vector<u1>& o, u4 v) {
    o.push_back(u1((v >> 24) & 0xFF));
    o.push_back(u1((v >> 16) & 0xFF));
    o.push_back(u1((v >>  8) & 0xFF));
    o.push_back(u1( v        & 0xFF));
}

void ClassFileEditor::copyN(std::vector<u1>& out, const u1*& p, size_t n) {
    out.insert(out.end(), p, p + n);
    p += n;
}

size_t ClassFileEditor::cpBodySize(u1 tag, const u1* p) {
    switch (tag) {
        case CP_Utf8: {
            u2 len = (u2(p[0]) << 8) | p[1];
            return 2 + len;
        }
        case CP_Class:
        case CP_String:
        case 16 :
        case 19 :
        case 20 :
            return 2;
        case CP_Fieldref:
        case CP_Methodref:
        case CP_InterfaceMethodref:
        case CP_NameAndType:
        case 3  :
        case 4  :
        case 18 :
        case 17 :
            return 4;
        case 5  :
        case 6  :
            return 8;
        case 15 :
            return 3;
        default:
            throw std::runtime_error(
                "class_edit: unknown constant pool tag " + std::to_string((int)tag));
    }
}

ClassFileEditor::ClassFileEditor(const u1* bytes, size_t length) {
    parse(bytes, bytes + length);
}

void ClassFileEditor::parse(const u1* p, const u1* end) {
    if (end - p < 10) throw std::runtime_error("class_edit: too short");
    magic_ = rd32(p);
    if (magic_ != 0xCAFEBABE) throw std::runtime_error("class_edit: bad magic");
    minor_version_ = rd16(p);
    major_version_ = rd16(p);
    u2 cpCount = rd16(p);
    cp_.clear();
    cp_.push_back(CpEntry{});

    for (u2 i = 1; i < cpCount; ++i) {
        u1 tag = *p++;
        size_t bodyLen = cpBodySize(tag, p);
        CpEntry e;
        e.tag = tag;
        e.body.assign(p, p + bodyLen);
        e.wide = (tag == 5 || tag == 6);
        p += bodyLen;
        cp_.push_back(std::move(e));
        if (e.wide) {
            cp_.push_back(CpEntry{});
            ++i;
        }
    }

    access_flags_ = rd16(p);
    this_class_   = rd16(p);
    super_class_  = rd16(p);

    u2 ifCount = rd16(p);
    interfaces_.clear();
    interfaces_.reserve(ifCount);
    for (u2 i = 0; i < ifCount; ++i) interfaces_.push_back(rd16(p));

    if (this_class_ && cp_[this_class_].tag == CP_Class) {
        u2 nameIdx = (u2(cp_[this_class_].body[0]) << 8) | cp_[this_class_].body[1];
        this_name_ = cpUtf8Value(nameIdx);
    }

    const u1* fieldsStart = p;
    u2 fieldCount = rd16(p);
    for (u2 i = 0; i < fieldCount; ++i) {
        p += 6;
        u2 attrCount = rd16(p);
        for (u2 j = 0; j < attrCount; ++j) {
            p += 2;
            u4 attrLen = rd32(p);
            p += attrLen;
        }
    }
    fields_raw_.assign(fieldsStart, p);

    u2 methodCount = rd16(p);
    methods_.clear();
    methods_.reserve(methodCount);
    for (u2 i = 0; i < methodCount; ++i) {
        MethodInfo m;
        m.access_flags     = rd16(p);
        m.name_index       = rd16(p);
        m.descriptor_index = rd16(p);
        const u1* attrsStart = p;
        u2 attrCount = rd16(p);
        for (u2 j = 0; j < attrCount; ++j) {
            p += 2;
            u4 attrLen = rd32(p);
            p += attrLen;
        }
        m.attributes_raw_.assign(attrsStart, p);
        methods_.push_back(std::move(m));
    }

    class_attrs_raw_.assign(p, end);
}

std::string ClassFileEditor::cpUtf8Value(u2 idx) const {
    if (idx == 0 || idx >= cp_.size()) return {};
    const CpEntry& e = cp_[idx];
    if (e.tag != CP_Utf8) return {};
    if (e.body.size() < 2) return {};
    u2 len = (u2(e.body[0]) << 8) | e.body[1];
    return std::string(reinterpret_cast<const char*>(e.body.data() + 2), len);
}

u2 ClassFileEditor::findUtf8(std::string_view s) const {
    for (u2 i = 1; i < cp_.size(); ++i) {
        if (cp_[i].tag != CP_Utf8) continue;
        if (cp_[i].body.size() < 2) continue;
        u2 len = (u2(cp_[i].body[0]) << 8) | cp_[i].body[1];
        if (len != s.size()) continue;
        if (std::memcmp(cp_[i].body.data() + 2, s.data(), len) == 0) {
            return i;
        }
    }
    return 0;
}

u2 ClassFileEditor::addRawEntry(u1 tag, std::vector<u1> body, bool wide) {
    CpEntry e;
    e.tag = tag;
    e.body = std::move(body);
    e.wide = wide;
    u2 idx = static_cast<u2>(cp_.size());
    cp_.push_back(std::move(e));
    if (wide) cp_.push_back(CpEntry{});
    return idx;
}

u2 ClassFileEditor::addUtf8(std::string_view s) {
    if (u2 e = findUtf8(s)) return e;
    std::vector<u1> body;
    body.reserve(2 + s.size());
    wr16(body, static_cast<u2>(s.size()));
    body.insert(body.end(), s.begin(), s.end());
    return addRawEntry(CP_Utf8, std::move(body));
}

u2 ClassFileEditor::addClass(std::string_view internalName) {
    u2 nameIdx = addUtf8(internalName);

    for (u2 i = 1; i < cp_.size(); ++i) {
        if (cp_[i].tag == CP_Class &&
            cp_[i].body.size() == 2 &&
            ((u2(cp_[i].body[0]) << 8) | cp_[i].body[1]) == nameIdx) {
            return i;
        }
    }
    std::vector<u1> body;
    wr16(body, nameIdx);
    return addRawEntry(CP_Class, std::move(body));
}

u2 ClassFileEditor::addNameAndType(std::string_view name, std::string_view desc) {
    u2 nameIdx = addUtf8(name);
    u2 descIdx = addUtf8(desc);
    for (u2 i = 1; i < cp_.size(); ++i) {
        if (cp_[i].tag == CP_NameAndType && cp_[i].body.size() == 4) {
            u2 n = (u2(cp_[i].body[0]) << 8) | cp_[i].body[1];
            u2 d = (u2(cp_[i].body[2]) << 8) | cp_[i].body[3];
            if (n == nameIdx && d == descIdx) return i;
        }
    }
    std::vector<u1> body;
    wr16(body, nameIdx);
    wr16(body, descIdx);
    return addRawEntry(CP_NameAndType, std::move(body));
}

u2 ClassFileEditor::addMethodRef(std::string_view klass,
                                 std::string_view name,
                                 std::string_view desc) {
    u2 clsIdx = addClass(klass);
    u2 natIdx = addNameAndType(name, desc);
    for (u2 i = 1; i < cp_.size(); ++i) {
        if (cp_[i].tag == CP_Methodref && cp_[i].body.size() == 4) {
            u2 c = (u2(cp_[i].body[0]) << 8) | cp_[i].body[1];
            u2 n = (u2(cp_[i].body[2]) << 8) | cp_[i].body[3];
            if (c == clsIdx && n == natIdx) return i;
        }
    }
    std::vector<u1> body;
    wr16(body, clsIdx);
    wr16(body, natIdx);
    return addRawEntry(CP_Methodref, std::move(body));
}

namespace {

u4 Be32(const u1* p) {
    return (u4(p[0]) << 24) | (u4(p[1]) << 16) | (u4(p[2]) << 8) | p[3];
}

void PutBe32(std::vector<u1>& c, u4 at, u4 v) {
    c[at + 0] = u1((v >> 24) & 0xFF);
    c[at + 1] = u1((v >> 16) & 0xFF);
    c[at + 2] = u1((v >> 8) & 0xFF);
    c[at + 3] = u1(v & 0xFF);
}

void PutBe16(std::vector<u1>& c, u4 at, u2 v) {
    c[at + 0] = u1((v >> 8) & 0xFF);
    c[at + 1] = u1(v & 0xFF);
}

}

bool ClassFileEditor::nextPc(const u1* code, u4 len, u4 pc, u4& next) {
    if (pc >= len) return false;
    const u1 op = code[pc];

    switch (op) {
        case 0x10: case 0x12:
        case 0x15: case 0x16: case 0x17: case 0x18: case 0x19:
        case 0x36: case 0x37: case 0x38: case 0x39: case 0x3A:
        case 0xA9: case 0xBC:
            next = pc + 2;
            return next <= len;

        case 0x11: case 0x13: case 0x14: case 0x84:
        case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E:
        case 0x9F: case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4:
        case 0xA5: case 0xA6: case 0xA7: case 0xA8:
        case 0xB2: case 0xB3: case 0xB4: case 0xB5:
        case 0xB6: case 0xB7: case 0xB8: case 0xB9:
        case 0xBB: case 0xBD: case 0xC0: case 0xC1:
        case 0xC6: case 0xC7:
            next = pc + 3;
            return next <= len;

        case 0xBA: case 0xC8: case 0xC9:
            next = pc + 5;
            return next <= len;

        case 0xC5:
            next = pc + 4;
            return next <= len;

        case 0xAA: {
            u4 q = (pc + 4) & ~3u;
            if (q + 12 > len) return false;
            const i8 lo = static_cast<i4>(Be32(code + q + 4));
            const i8 hi = static_cast<i4>(Be32(code + q + 8));
            if (hi < lo) return false;
            const i8 n = hi - lo + 1;
            if (n > (len - q - 12) / 4) return false;
            next = q + 12 + 4 * n;
            return next <= len;
        }
        case 0xAB: {
            u4 q = (pc + 4) & ~3u;
            if (q + 8 > len) return false;
            const u4 n = Be32(code + q + 4);
            if (n > (len - q - 8) / 8) return false;
            next = q + 8 + 8 * n;
            return next <= len;
        }

        case 0xC4:
            if (pc + 1 >= len) return false;
            next = pc + (code[pc + 1] == 0x84 ? 6 : 4);
            return next <= len;

        case 0xCA: case 0xFE: case 0xFF:
            next = pc + 1;
            return true;

        default:
            if (op <= 0x0F ||
                (op >= 0x1A && op <= 0x35) ||
                (op >= 0x3B && op <= 0x83) ||
                (op >= 0x85 && op <= 0x98) ||
                (op >= 0xAC && op <= 0xB1) ||
                op == 0xBE || op == 0xBF || op == 0xC2 || op == 0xC3) {
                next = pc + 1;
                return true;
            }
            return false;
    }
}

bool ClassFileEditor::instructionOffsets(const u1* code, u4 codeLen,
                                         std::vector<u4>& out) const {
    out.clear();
    u4 pc = 0;
    while (pc < codeLen) {
        out.push_back(pc);
        u4 next = 0;
        if (!nextPc(code, codeLen, pc, next)) return false;
        pc = next;
    }
    return pc == codeLen;
}

ClassFileEditor::MethodInfo* ClassFileEditor::findMethod(std::string_view name,
                                                         std::string_view desc) {
    const u2 nameIdx = findUtf8(name);
    const u2 descIdx = findUtf8(desc);
    if (!nameIdx || !descIdx) return nullptr;
    for (MethodInfo& m : methods_) {
        if (m.name_index == nameIdx && m.descriptor_index == descIdx) return &m;
    }
    return nullptr;
}

const ClassFileEditor::MethodInfo* ClassFileEditor::findMethod(
        std::string_view name, std::string_view desc) const {
    const u2 nameIdx = findUtf8(name);
    const u2 descIdx = findUtf8(desc);
    if (!nameIdx || !descIdx) return nullptr;
    for (const MethodInfo& m : methods_) {
        if (m.name_index == nameIdx && m.descriptor_index == descIdx) return &m;
    }
    return nullptr;
}

bool ClassFileEditor::codeOf(const MethodInfo& method, const u1*& code,
                             u4& codeLen, std::vector<u4>& offsets) const {
    const u2 codeUtf8 = findUtf8("Code");
    if (!codeUtf8) return false;

    const u1* p = method.attributes_raw_.data();
    const u1* end = p + method.attributes_raw_.size();
    if (end - p < 2) return false;
    const u2 attrCount = (u2(p[0]) << 8) | p[1];
    p += 2;

    for (u2 i = 0; i < attrCount; ++i) {
        if (end - p < 6) return false;
        const u2 nameIdx = (u2(p[0]) << 8) | p[1];
        const u4 attrLen = Be32(p + 2);
        const u1* attrEnd = p + 6 + attrLen;
        if (attrEnd > end) return false;
        if (nameIdx != codeUtf8) { p = attrEnd; continue; }
        if (attrLen < 8) return false;

        codeLen = Be32(p + 10);
        code = p + 14;
        if (codeLen > u4(attrEnd - code)) return false;
        return instructionOffsets(code, codeLen, offsets);
    }
    return false;
}

bool ClassFileEditor::hasMethod(std::string_view name, std::string_view desc) const {
    const u2 nameIdx = findUtf8(name);
    const u2 descIdx = findUtf8(desc);
    if (!nameIdx || !descIdx) return false;
    for (const MethodInfo& m : methods_) {
        if (m.name_index == nameIdx && m.descriptor_index == descIdx) return true;
    }
    return false;
}

bool ClassFileEditor::callTarget(const u1* code, u4 codeLen, const CallSite& site,
                                 std::string& owner, std::string& calleeName,
                                 std::string& calleeDesc, bool& isStatic) const {
    if (site.opcode != 0xB4 && site.opcode != 0xB5 && site.opcode != 0xB6 && site.opcode != 0xB7 &&
        site.opcode != 0xB8 && site.opcode != 0xB9) {
        return false;
    }
    if (site.offset + 3 > codeLen) return false;
    const u2 cpIdx = (u2(code[site.offset + 1]) << 8) | code[site.offset + 2];
    if (cpIdx == 0 || cpIdx >= cp_.size()) return false;

    u1 wantTag = CP_Methodref;
    if (site.opcode == 0xB4 || site.opcode == 0xB5) wantTag = CP_Fieldref;
    if (site.opcode == 0xB9) wantTag = CP_InterfaceMethodref;
    const CpEntry& ref = cp_[cpIdx];
    if ((site.opcode == 0xB4 || site.opcode == 0xB5) ? ref.tag != CP_Fieldref :
        (ref.tag != wantTag && ref.tag != CP_Methodref && ref.tag != CP_InterfaceMethodref)) {
        return false;
    }
    if (ref.body.size() != 4) return false;

    const u2 classIdx = (u2(ref.body[0]) << 8) | ref.body[1];
    const u2 natIdx   = (u2(ref.body[2]) << 8) | ref.body[3];
    if (classIdx >= cp_.size() || natIdx >= cp_.size()) return false;
    if (cp_[classIdx].tag != CP_Class || cp_[natIdx].tag != CP_NameAndType) return false;
    if (cp_[classIdx].body.size() != 2 || cp_[natIdx].body.size() != 4) return false;

    owner = cpUtf8Value((u2(cp_[classIdx].body[0]) << 8) | cp_[classIdx].body[1]);
    calleeName = cpUtf8Value((u2(cp_[natIdx].body[0]) << 8) | cp_[natIdx].body[1]);
    calleeDesc = cpUtf8Value((u2(cp_[natIdx].body[2]) << 8) | cp_[natIdx].body[3]);
    isStatic = (site.opcode == 0xB8);
    return true;
}

std::vector<ClassFileEditor::CallSite> ClassFileEditor::findCalls(
        std::string_view name, std::string_view desc,
        std::string_view calleeOwner, std::string_view calleeName,
        std::string_view calleeDesc, u1 opcodeMask) const {

    std::vector<CallSite> hits;
    const MethodInfo* method = findMethod(name, desc);
    if (!method) return hits;

    const u2 codeUtf8 = findUtf8("Code");
    if (!codeUtf8) return hits;

    const u1* p = method->attributes_raw_.data();
    const u1* end = p + method->attributes_raw_.size();
    if (end - p < 2) return hits;
    const u2 attrCount = (u2(p[0]) << 8) | p[1];
    p += 2;

    for (u2 i = 0; i < attrCount; ++i) {
        if (end - p < 6) return hits;
        const u2 nameIdx = rd16(p);
        const u4 attrLen = rd32(p);
        const u1* attrEnd = p + attrLen;
        if (attrEnd > end) return hits;
        if (nameIdx != codeUtf8) { p = attrEnd; continue; }
        if (attrEnd - p < 6) return hits;

        p += 4;
        const u4 codeLen = rd32(p);
        const u1* code = p;
        if (codeLen > u4(attrEnd - code)) return hits;

        u4 pc = 0;
        while (pc < codeLen) {
            u4 next = 0;
            if (!nextPc(code, codeLen, pc, next)) break;
            const u1 op = code[pc];
            if ((op == 0xB6 || op == 0xB7 || op == 0xB8 || op == 0xB9 ||
                 ((op == 0xB4 || op == 0xB5) && opcodeMask == op)) &&
                (opcodeMask == 0 || opcodeMask == op)) {
                CallSite site;
                site.offset = pc;
                site.opcode = op;
                std::string o, n, d;
                bool isStatic = false;
                if (callTarget(code, codeLen, site, o, n, d, isStatic) &&
                    o == calleeOwner && n == calleeName && d == calleeDesc) {
                    hits.push_back(site);
                }
            }
            pc = next;
        }
        return hits;
    }
    return hits;
}

namespace {

bool OneSlotWidth(std::string_view d, size_t& i, int& slots) {
    slots = 1;
    if (i >= d.size()) return false;

    while (i < d.size() && d[i] == '[') ++i;
    if (i >= d.size()) return false;

    switch (d[i]) {
        case 'J': case 'D':
            slots = 2;
            ++i;
            return true;
        case 'Z': case 'B': case 'C': case 'S': case 'I': case 'F':
            ++i;
            return true;
        case 'L': {
            const size_t semi = d.find(';', i);
            if (semi == std::string_view::npos) return false;
            i = semi + 1;
            return true;
        }
        default:
            return false;
    }
}

}

bool ClassFileEditor::stackEffect(std::string_view descriptor, bool isStatic,
                                  int& popped, int& pushed) {
    popped = pushed = 0;
    size_t i = 0;
    if (descriptor.empty() || descriptor[0] != '(') return false;
    ++i;

    while (i < descriptor.size() && descriptor[i] != ')') {
        int slots = 0;
        if (!OneSlotWidth(descriptor, i, slots)) return false;
        popped += slots;
    }
    if (i >= descriptor.size() || descriptor[i] != ')') return false;
    ++i;

    if (i >= descriptor.size()) return false;
    if (descriptor[i] == 'V') {
        if (i + 1 != descriptor.size()) return false;
    } else {
        int slots = 0;
        if (!OneSlotWidth(descriptor, i, slots)) return false;
        if (i != descriptor.size()) return false;
        pushed = slots;
    }

    if (!isStatic) popped += 1;
    return true;
}

bool ClassFileEditor::redirectCalls(std::string_view name, std::string_view desc,
                                    std::string_view calleeOwner,
                                    std::string_view calleeName,
                                    std::string_view calleeDesc,
                                    std::string_view newOwner,
                                    std::string_view newName,
                                    std::string_view newDesc,
                                    u1 opcodeMask, int limit, int ordinal) {

    if (name == "*") {
        if (!desc.empty() || limit != -1 || ordinal != -1) return false;
        // MixinExtras moves vanilla calls into generated bridge methods whose
        // suffixes change between Lunar builds. Match the stable callee instead.
        auto staged = *this;
        bool changed = false;
        for (const auto& method : staged.methods_) {
            const auto methodName = staged.cpUtf8Value(method.name_index);
            const auto methodDesc = staged.cpUtf8Value(method.descriptor_index);
            if (staged.findCalls(methodName, methodDesc, calleeOwner, calleeName, calleeDesc, opcodeMask).empty()) continue;
            if (!staged.redirectCalls(methodName, methodDesc, calleeOwner, calleeName, calleeDesc,
                                      newOwner, newName, newDesc, opcodeMask)) return false;
            changed = true;
        }
        if (changed) *this = std::move(staged);
        return changed;
    }

    MethodInfo* method = findMethod(name, desc);
    if (!method) return false;

    std::vector<CallSite> hits =
        findCalls(name, desc, calleeOwner, calleeName, calleeDesc, opcodeMask);
    if (hits.empty()) return false;
    if (ordinal >= 0) {
        if (hits.size() <= size_t(ordinal)) return false;
        const CallSite chosen = hits[size_t(ordinal)];
        hits.clear();
        hits.push_back(chosen);
    }
    if (limit >= 0 && hits.size() > size_t(limit)) hits.resize(size_t(limit));
    if (hits.empty()) return false;

    const bool oldStatic = (hits.front().opcode == 0xB8);
    for (const CallSite& site : hits) {
        if ((site.opcode == 0xB8) != oldStatic) return false;
    }
    int oldPop = 0, oldPush = 0, newPop = 0, newPush = 0;
    const bool fieldRead = hits.front().opcode == 0xB4;
    const bool fieldWrite = hits.front().opcode == 0xB5;
    if (fieldRead || fieldWrite) {
        const std::string expected = "(L" + std::string(calleeOwner) + ";" +
            (fieldWrite ? std::string(calleeDesc) + ")V" : ")" + std::string(calleeDesc));
        if (newDesc != expected) return false;
    }
    const std::string oldDescriptor = fieldRead ? "()" + std::string(calleeDesc) :
        fieldWrite ? "(" + std::string(calleeDesc) + ")V" : std::string(calleeDesc);
    if (!stackEffect(oldDescriptor, oldStatic, oldPop, oldPush)) return false;
    if (!stackEffect(newDesc, true, newPop, newPush)) return false;
    if (oldPop != newPop || oldPush != newPush) return false;

    const u2 codeUtf8 = findUtf8("Code");
    if (!codeUtf8) return false;

    u1* p = method->attributes_raw_.data();
    u1* end = p + method->attributes_raw_.size();
    if (end - p < 2) return false;
    const u2 attrCount = (u2(p[0]) << 8) | p[1];
    p += 2;

    for (u2 i = 0; i < attrCount; ++i) {
        if (end - p < 6) return false;
        const u2 nameIdx = (u2(p[0]) << 8) | p[1];
        const u4 attrLen = Be32(p + 2);
        u1* attrEnd = p + 6 + attrLen;
        if (attrEnd > end) return false;
        if (nameIdx != codeUtf8) { p = attrEnd; continue; }

        if (attrEnd - p < 14) return false;

        const u4 codeLen = Be32(p + 10);
        if (u4(attrEnd - p) - 14 < codeLen) return false;
        for (const CallSite& site : hits) {
            if (site.end() > codeLen) return false;
        }

        const u2 newRef = addMethodRef(newOwner, newName, newDesc);
        if (!newRef) return false;
        u1* code = p + 14;
        for (const CallSite& site : hits) {
            code[site.offset]     = 0xB8;
            code[site.offset + 1] = u1((newRef >> 8) & 0xFF);
            code[site.offset + 2] = u1(newRef & 0xFF);
            if (site.opcode == 0xB9) {
                code[site.offset + 3] = 0;
                code[site.offset + 4] = 0;
            }
        }
        return true;
    }
    return false;
}

namespace {

struct VtiWalker {
    const u1* p;
    const u1* end;

    bool skip() {
        if (p >= end) return false;
        switch (*p) {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6:
                p += 1;
                return true;
            case 7: case 8:
                if (end - p < 3) return false;
                p += 3;
                return true;
            default:
                return false;
        }
    }

    bool skipN(u2 n) {
        for (u2 i = 0; i < n; ++i) if (!skip()) return false;
        return true;
    }
};

}

std::vector<u1> ClassFileEditor::shiftStackMap(const std::vector<u1>& body,
                                               u4 delta, u4 from) {
    if (body.size() < 2) return body;
    const u2 numEntries = (u2(body[0]) << 8) | body[1];
    if (numEntries == 0) return body;

    const u1* p = body.data() + 2;
    const u1* end = body.data() + body.size();

    std::vector<u1> out;
    out.reserve(body.size() + 6);
    out.push_back(body[0]);
    out.push_back(body[1]);

    u4 abs = 0;
    bool adjusted = false;

    for (u2 i = 0; i < numEntries; ++i) {
        if (p >= end) return body;
        const u1 tag = *p++;

        u4 frameDelta = 0;
        bool narrow = false;

        if (tag <= 63) {
            frameDelta = tag;
            narrow = true;
        } else if (tag <= 127) {
            frameDelta = tag - 64;
            narrow = true;
        } else {
            if (end - p < 2) return body;
            frameDelta = (u4(p[0]) << 8) | p[1];
            p += 2;
        }

        abs = (i == 0) ? frameDelta : abs + frameDelta + 1;

        if (abs > from && !adjusted) {
            frameDelta += delta;
            adjusted = true;
        }

        if (narrow) {
            if (tag <= 63) {
                if (frameDelta <= 63) {
                    out.push_back(u1(frameDelta));
                } else {
                    out.push_back(251);
                    out.push_back(u1((frameDelta >> 8) & 0xFF));
                    out.push_back(u1(frameDelta & 0xFF));
                }
            } else {
                if (frameDelta <= 63) {
                    out.push_back(u1(64 + frameDelta));
                } else {
                    out.push_back(247);
                    out.push_back(u1((frameDelta >> 8) & 0xFF));
                    out.push_back(u1(frameDelta & 0xFF));
                }
                VtiWalker w{p, end};
                if (!w.skip()) return body;
                out.insert(out.end(), p, w.p);
                p = w.p;
            }
            continue;
        }

        out.push_back(tag);
        out.push_back(u1((frameDelta >> 8) & 0xFF));
        out.push_back(u1(frameDelta & 0xFF));

        VtiWalker w{p, end};
        if (tag == 247) {
            if (!w.skip()) return body;
        } else if (tag >= 252 && tag <= 254) {
            if (!w.skipN(u2(tag - 251))) return body;
        } else if (tag == 255) {
            if (end - w.p < 2) return body;
            const u2 nLocals = (u2(w.p[0]) << 8) | w.p[1];
            w.p += 2;
            if (!w.skipN(nLocals)) return body;
            if (end - w.p < 2) return body;
            const u2 nStack = (u2(w.p[0]) << 8) | w.p[1];
            w.p += 2;
            if (!w.skipN(nStack)) return body;
        }
        out.insert(out.end(), p, w.p);
        p = w.p;
    }

    out.insert(out.end(), p, end);
    return out;
}

std::vector<u1> ClassFileEditor::shiftLineNumberTable(const std::vector<u1>& body,
                                                      u4 delta, u4 from) {
    if (body.size() < 2) return body;
    std::vector<u1> out = body;
    const u2 count = (u2(body[0]) << 8) | body[1];
    if (2 + size_t(count) * 4 > body.size()) return body;
    for (u2 i = 0; i < count; ++i) {
        const size_t off = 2 + i * 4;
        u4 startPc = (u4(out[off]) << 8) | out[off + 1];
        if (startPc >= from) {
            startPc += delta;
            out[off]     = u1((startPc >> 8) & 0xFF);
            out[off + 1] = u1(startPc & 0xFF);
        }
    }
    return out;
}

std::vector<u1> ClassFileEditor::shiftLocalVarTable(const std::vector<u1>& body,
                                                    u4 delta, u4 from) {
    if (body.size() < 2) return body;
    std::vector<u1> out = body;
    const u2 count = (u2(body[0]) << 8) | body[1];
    const size_t entrySize = 10;
    if (2 + size_t(count) * entrySize > body.size()) return body;
    for (u2 i = 0; i < count; ++i) {
        const size_t off = 2 + i * entrySize;
        const u4 startPc = (u4(out[off]) << 8) | out[off + 1];
        const u4 length  = (u4(out[off + 2]) << 8) | out[off + 3];
        const u4 rangeEnd = startPc + length;

        const u4 newStart = (startPc > from) ? startPc + delta : startPc;
        const u4 newEnd   = (rangeEnd > from) ? rangeEnd + delta : rangeEnd;
        const u4 newLen   = newEnd - newStart;

        out[off]     = u1((newStart >> 8) & 0xFF);
        out[off + 1] = u1(newStart & 0xFF);
        out[off + 2] = u1((newLen >> 8) & 0xFF);
        out[off + 3] = u1(newLen & 0xFF);
    }
    return out;
}

bool ClassFileEditor::shiftBranches(std::vector<u1>& code, u4 insertAt, u4 delta) {
    const u4 len = static_cast<u4>(code.size());
    u4 pc = 0;
    while (pc < len) {
        u4 next = 0;
        if (!nextPc(code.data(), len, pc, next)) return false;
        const u1 op = code[pc];

        const u4 newPc = (pc >= insertAt) ? pc + delta : pc;

        bool overflow = false;
        auto fix = [&](u4 fieldPos, u4 fieldSize, i4 off) {
            const u4 target = u4(i8(pc) + i8(off));
            u4 newTarget = target;
            if (target == insertAt) {
                newTarget = insertAt;
            } else if (target > insertAt) {
                newTarget = target + delta;
            }
            const i8 newOff = i8(newTarget) - i8(newPc);
            if (fieldSize == 2) {
                if (newOff < -32768 || newOff > 32767) { overflow = true; return; }
                PutBe16(code, fieldPos, u2(i2(newOff)));
            } else {
                PutBe32(code, fieldPos, u4(i4(newOff)));
            }
        };

        if ((op >= 0x99 && op <= 0xA8) || op == 0xC6 || op == 0xC7) {
            const i2 off = i2((u2(code[pc + 1]) << 8) | code[pc + 2]);
            fix(pc + 1, 2, off);
        } else if (op == 0xC8 || op == 0xC9) {
            fix(pc + 1, 4, i4(Be32(code.data() + pc + 1)));
        } else if (op == 0xAA || op == 0xAB) {
            const u4 q = (pc + 4) & ~3u;
            fix(q, 4, i4(Be32(code.data() + q)));
            if (op == 0xAA) {
                const i8 lo = static_cast<i4>(Be32(code.data() + q + 4));
                const i8 hi = static_cast<i4>(Be32(code.data() + q + 8));
                if (hi < lo) return false;
                for (u4 k = 0; k <= hi - lo; ++k) {
                    const u4 at = q + 12 + 4 * k;
                    if (at + 4 > len) return false;
                    fix(at, 4, i4(Be32(code.data() + at)));
                }
            } else {
                const u4 n = Be32(code.data() + q + 4);
                for (u4 k = 0; k < n; ++k) {
                    const u4 at = q + 8 + 8 * k + 4;
                    if (at + 4 > len) return false;
                    fix(at, 4, i4(Be32(code.data() + at)));
                }
            }
        }
        if (overflow) return false;
        pc = next;
    }
    return true;
}

bool ClassFileEditor::insertAt(std::string_view name, std::string_view desc,
                               u4 at, const std::vector<u1>& insertion,
                               u2 extraStack) {
    MethodInfo* method = findMethod(name, desc);
    if (!method) return false;

    u2 codeUtf8 = findUtf8("Code");
    if (!codeUtf8) codeUtf8 = addUtf8("Code");

    std::vector<u1> padded = insertion;
    while ((padded.size() & 3u) != 0) padded.push_back(0x00);

    std::vector<u1> rebuilt = rebuildCodeAttribute(
        method->attributes_raw_, codeUtf8, at, padded, extraStack);
    if (rebuilt.empty()) return false;
    method->attributes_raw_ = std::move(rebuilt);
    return true;
}

bool ClassFileEditor::branchesTo(const u1* code, u4 codeLen, u4 target) {
    u4 pc = 0;
    while (pc < codeLen) {
        u4 next = 0;
        if (!nextPc(code, codeLen, pc, next)) return false;
        const u1 op = code[pc];

        auto hits = [&](u4 at) { return at == target; };
        auto rel16 = [&]() { return u4(i8(pc) + i8(i2((u2(code[pc + 1]) << 8) | code[pc + 2]))); };
        auto rel32 = [&](u4 at) { return u4(i8(pc) + i8(i4(Be32(code + at)))); };

        if ((op >= 0x99 && op <= 0xA8) || op == 0xC6 || op == 0xC7) {
            if (hits(rel16())) return true;
        } else if (op == 0xC8 || op == 0xC9) {
            if (hits(rel32(pc + 1))) return true;
        } else if (op == 0xAA || op == 0xAB) {
            const u4 q = (pc + 4) & ~3u;
            if (hits(rel32(q))) return true;
            if (op == 0xAA) {
                const i8 lo = static_cast<i4>(Be32(code + q + 4));
                const i8 hi = static_cast<i4>(Be32(code + q + 8));
                if (hi < lo) return false;
                for (u4 k = 0; k <= hi - lo; ++k) {
                    const u4 at = q + 12 + 4 * k;
                    if (at + 4 > codeLen) return false;
                    if (hits(rel32(at))) return true;
                }
            } else {
                const u4 n = Be32(code + q + 4);
                for (u4 k = 0; k < n; ++k) {
                    const u4 at = q + 8 + 8 * k + 4;
                    if (at + 4 > codeLen) return false;
                    if (hits(rel32(at))) return true;
                }
            }
        }
        pc = next;
    }
    return false;
}

bool ClassFileEditor::frameHasLocalFrom(const MethodInfo& method, u4 at,
                                        const std::vector<int>& paramWidths) const {
    const u2 codeUtf8 = findUtf8("Code");
    const u2 smtUtf8  = findUtf8("StackMapTable");
    if (!codeUtf8 || !smtUtf8) return true;

    const u1* p = method.attributes_raw_.data();
    const u1* end = p + method.attributes_raw_.size();
    if (end - p < 2) return true;
    const u2 attrCount = (u2(p[0]) << 8) | p[1];
    p += 2;

    const u1* table = nullptr;
    const u1* tableEnd = nullptr;

    for (u2 i = 0; i < attrCount && !table; ++i) {
        if (end - p < 6) return true;
        const u2 nameIdx = (u2(p[0]) << 8) | p[1];
        const u4 attrLen = Be32(p + 2);
        const u1* attrEnd = p + 6 + attrLen;
        if (attrEnd > end) return true;
        if (nameIdx != codeUtf8) {
            p = attrEnd;
            continue;
        }
        if (attrLen < 14) return true;

        const u4 codeLen = Be32(p + 10);
        const u1* code = p + 14;
        if (u4(attrEnd - code) < codeLen) return true;

        const u1* q = code + codeLen;
        if (attrEnd - q < 2) return true;
        const u2 excCount = (u2(q[0]) << 8) | q[1];
        q += 2;
        if (u4(attrEnd - q) < size_t(excCount) * 8 + 2) return true;
        q += size_t(excCount) * 8;

        const u2 innerCount = (u2(q[0]) << 8) | q[1];
        q += 2;
        for (u2 j = 0; j < innerCount; ++j) {
            if (attrEnd - q < 6) return true;
            const u2 innerName = (u2(q[0]) << 8) | q[1];
            const u4 innerLen = Be32(q + 2);
            const u1* innerEnd = q + 6 + innerLen;
            if (innerEnd > attrEnd) return true;
            if (innerName == smtUtf8) {
                table = q + 6;
                tableEnd = innerEnd;
            }
            q = innerEnd;
        }
    }

    if (!table) return true;
    if (tableEnd - table < 2) return true;

    const u2 entries = (u2(table[0]) << 8) | table[1];
    const u1* f = table + 2;

    struct Entry {
        u2  slot;
        u1  width;
        bool live;
    };

    std::vector<Entry> locals;
    u2 slot = 0;
    locals.push_back(Entry{slot, 1, true});
    slot = u2(slot + 1);
    for (int width : paramWidths) {
        locals.push_back(Entry{slot, u1(width), true});
        slot = u2(slot + width);
    }
    const u2 firstFree = slot;

    u4 abs = 0;
    for (u2 i = 0; i < entries; ++i) {
        if (f >= tableEnd) return true;
        const u1 tag = *f++;

        u4 delta = 0;
        if (tag <= 127) {
            delta = (tag <= 63) ? tag : u4(tag - 64);
        } else {
            if (tableEnd - f < 2) return true;
            delta = (u4(f[0]) << 8) | f[1];
            f += 2;
        }
        abs = (i == 0) ? delta : abs + delta + 1;

        auto readOne = [&](Entry& out) -> bool {
            if (f >= tableEnd) return false;
            const u1 vti = *f++;
            out.slot = 0;
            out.width = 1;
            out.live = true;
            switch (vti) {
                case 0: out.live = false; break;
                case 3:
                case 4: out.width = 2; break;
                case 1: case 2: case 5: case 6: break;
                case 7:
                case 8:
                    if (tableEnd - f < 2) return false;
                    f += 2;
                    break;
                default:
                    return false;
            }
            return true;
        };

        if (tag >= 248 && tag <= 250) {
            const u2 chop = u2(251 - tag);
            if (chop > locals.size()) return true;
            locals.resize(locals.size() - chop);
            slot = locals.empty() ? 0 : u2(locals.back().slot + locals.back().width);
        } else if (tag >= 252 && tag <= 254) {
            for (u2 k = 0; k < u2(tag - 251); ++k) {
                Entry e{};
                if (!readOne(e)) return true;
                e.slot = slot;
                slot = u2(slot + e.width);
                locals.push_back(e);
            }
        } else if (tag == 255) {
            if (tableEnd - f < 2) return true;
            const u2 nLocals = (u2(f[0]) << 8) | f[1];
            f += 2;
            locals.clear();
            slot = 0;
            for (u2 k = 0; k < nLocals; ++k) {
                Entry e{};
                if (!readOne(e)) return true;
                e.slot = slot;
                slot = u2(slot + e.width);
                locals.push_back(e);
            }
            if (tableEnd - f < 2) return true;
            const u2 nStack = (u2(f[0]) << 8) | f[1];
            f += 2;
            for (u2 k = 0; k < nStack; ++k) {
                Entry ignored{};
                if (!readOne(ignored)) return true;
            }
        } else if ((tag >= 64 && tag <= 127) || tag == 247) {
            Entry ignored{};
            if (!readOne(ignored)) return true;
        }

        if (abs != at) continue;

        for (const Entry& e : locals)
            if (e.live && e.slot >= firstFree) return true;
        return false;
    }

    return true;
}

bool ClassFileEditor::prependToMethodCode(std::string_view name,
                                          std::string_view desc,
                                          const std::vector<u1>& insertion,
                                          u2 extraStack) {
    return insertAt(name, desc, 0, insertion, extraStack);
}

bool ClassFileEditor::insertBeforeCall(std::string_view name, std::string_view desc,
                                       std::string_view calleeOwner,
                                       std::string_view calleeName,
                                       std::string_view calleeDesc,
                                       int ordinal, const std::vector<u1>& insertion,
                                       u2 extraStack, u1 opcodeMask, bool after) {
    const std::vector<CallSite> hits =
        findCalls(name, desc, calleeOwner, calleeName, calleeDesc, opcodeMask);
    if (ordinal < 0 || ordinal >= int(hits.size())) return false;
    const u4 at = after ? hits[ordinal].end() : hits[ordinal].offset;
    return insertAt(name, desc, at, insertion, extraStack);
}

namespace {

bool ParameterTypes(std::string_view descriptor,
                    std::vector<char>& types, std::vector<int>& widths) {
    types.clear();
    widths.clear();
    if (descriptor.size() < 3 || descriptor[0] != '(') return false;

    size_t i = 1;
    while (i < descriptor.size() && descriptor[i] != ')') {
        const size_t start = i;
        int width = 0;
        if (!OneSlotWidth(descriptor, i, width)) return false;

        const char first = descriptor[start];
        if (first == 'L' || first == '[')      types.push_back('L');
        else if (first == 'J')                 types.push_back('J');
        else if (first == 'D')                 types.push_back('D');
        else if (first == 'F')                 types.push_back('F');
        else                                   types.push_back('I');
        widths.push_back(width);
    }
    return i < descriptor.size() && descriptor[i] == ')';
}

bool LocalOpcode(char type, int slot, u1 base, u1 firstShort,
                 std::vector<u1>& out) {
    if (slot < 0 || slot > 0xFF) return false;
    if (slot <= 3) {
        out.push_back(u1(firstShort + slot));
    } else {
        out.push_back(base);
        out.push_back(u1(slot));
    }
    return true;
}

bool LoadOpcode(char type, int slot, std::vector<u1>& out) {
    switch (type) {
        case 'J': return LocalOpcode(type, slot, 0x16, 0x1E, out);
        case 'F': return LocalOpcode(type, slot, 0x17, 0x22, out);
        case 'D': return LocalOpcode(type, slot, 0x18, 0x26, out);
        case 'L': return LocalOpcode(type, slot, 0x19, 0x2A, out);
        default:  return LocalOpcode(type, slot, 0x15, 0x1A, out);
    }
}

bool StoreOpcode(char type, int slot, std::vector<u1>& out) {
    switch (type) {
        case 'J': return LocalOpcode(type, slot, 0x37, 0x3F, out);
        case 'F': return LocalOpcode(type, slot, 0x38, 0x43, out);
        case 'D': return LocalOpcode(type, slot, 0x39, 0x47, out);
        case 'L': return LocalOpcode(type, slot, 0x3A, 0x4B, out);
        default:  return LocalOpcode(type, slot, 0x36, 0x3B, out);
    }
}

char LocalTypeOf(std::string_view fieldDesc) {
    if (fieldDesc.empty()) return 'I';
    switch (fieldDesc[0]) {
        case 'L': case '[': return 'L';
        case 'J': return 'J';
        case 'D': return 'D';
        case 'F': return 'F';
        default:            return 'I';
    }
}

bool ReturnDescriptor(std::string_view methodDesc, std::string_view& out) {
    const size_t close = methodDesc.rfind(')');
    if (close == std::string_view::npos || close + 1 >= methodDesc.size())
        return false;
    out = methodDesc.substr(close + 1);
    return true;
}

}

bool ClassFileEditor::insertHookCall(std::string_view name, std::string_view desc,
                                     std::string_view calleeOwner,
                                     std::string_view calleeName,
                                     std::string_view calleeDesc,
                                     bool calleeIsStatic, int ordinal,
                                     std::string_view hookOwner,
                                     std::string_view hookName,
                                     std::string_view hookDesc,
                                     int argCount, int storeSlot,
                                     u1 opcodeMask) {

    std::vector<char> types;
    std::vector<int> widths;
    if (!ParameterTypes(desc, types, widths)) return false;

    std::vector<char> localType{'L'};
    std::vector<int> localSlot{0};
    int next = 1;
    for (size_t k = 0; k < types.size(); ++k) {
        localType.push_back(types[k]);
        localSlot.push_back(next);
        next += widths[k];
    }
    if (argCount < 1 || argCount > int(localType.size())) return false;

    int calleeArgs = 0, calleePushed = 0;
    if (!stackEffect(calleeDesc, calleeIsStatic, calleeArgs, calleePushed))
        return false;

    int hookArgs = 0, hookPushed = 0;
    if (!stackEffect(hookDesc, true, hookArgs, hookPushed))
        return false;

    if (hookArgs != argCount) return false;

    const bool storing = storeSlot >= 0;
    if (storing != (hookPushed > 0)) return false;

    std::vector<u1> insertion;
    for (int i = 0; i < argCount; ++i) {
        if (!LoadOpcode(localType[size_t(i)], localSlot[size_t(i)], insertion))
            return false;
    }

    const u2 hookRef = addMethodRef(hookOwner, hookName, hookDesc);
    insertion.push_back(0xB8);
    insertion.push_back(u1(hookRef >> 8));
    insertion.push_back(u1(hookRef & 0xFF));

    if (storing) {
        std::string_view returns;
        if (!ReturnDescriptor(hookDesc, returns) || returns == "V") return false;
        if (!StoreOpcode(LocalTypeOf(returns), storeSlot, insertion)) return false;
    }

    return insertBeforeCall(name, desc, calleeOwner, calleeName, calleeDesc,
                            ordinal, insertion,
                            u2(calleeArgs + hookArgs), opcodeMask);
}

bool ClassFileEditor::addStraightLineReturnFrame(std::string_view name, std::string_view desc) {
    if (name == "<init>" || name == "<clinit>") return false;
    auto* method = findMethod(name, desc);
    if (!method) return false;
    const u1* code = nullptr;
    u4 length = 0;
    std::vector<u4> offsets;
    if (!codeOf(*method, code, length, offsets) || offsets.empty()) return false;
    const u4 last = offsets.back();
    if (code[last] != 0xB1) return false;
    // A same_frame at RETURN is valid only when the initial locals survive.
    // Restrict this fallback to straight-line methods with no stores, jumps,
    // early exits, monitors or exception handlers (e.g. PacketThreadUtil$1).
    for (const auto at : offsets) {
        const u1 op = code[at];
        if ((op >= 0x36 && op <= 0x4E) || op == 0x84 || op == 0xA9 ||
            (op >= 0x99 && op <= 0xAB) || (op >= 0xC2 && op <= 0xC9) ||
            op == 0xBF || (op >= 0xAC && op <= 0xB1 && at != last)) return false;
    }
    const u1* p = method->attributes_raw_.data();
    const u1* end = p + method->attributes_raw_.size();
    if (end - p < 2) return false;
    const u2 count = rd16(p);
    std::vector<u1> result;
    wr16(result, count);
    bool added = false;
    for (u2 i = 0; i < count; ++i) {
        if (end - p < 6) return false;
        const u2 attr = rd16(p);
        const u4 size = rd32(p);
        if (size > size_t(end - p)) return false;
        const u1* attrEnd = p + size;
        wr16(result, attr);
        if (cpUtf8Value(attr) != "Code") {
            wr32(result, size); result.insert(result.end(), p, attrEnd); p = attrEnd; continue;
        }
        if (size < 12 + length) return false;
        const u1* inner = p + 8 + length;
        if (rd16(inner) != 0) return false;
        const u2 innerCount = rd16(inner);
        if (innerCount == 65535) return false;
        const u1* cursor = inner;
        for (u2 j = 0; j < innerCount; ++j) {
            if (attrEnd - cursor < 6) return false;
            const u2 nameIndex = rd16(cursor);
            const u4 innerLength = rd32(cursor);
            if (innerLength > size_t(attrEnd - cursor) || cpUtf8Value(nameIndex) == "StackMapTable")
                return false;
            cursor += innerLength;
        }
        if (cursor != attrEnd) return false;
        wr32(result, size + 11);
        result.insert(result.end(), p, inner - 2);
        wr16(result, innerCount + 1);
        result.insert(result.end(), inner, attrEnd);
        wr16(result, addUtf8("StackMapTable")); wr32(result, 5);
        wr16(result, 1); result.push_back(251); wr16(result, static_cast<u2>(last));
        added = true; p = attrEnd;
    }
    if (!added) return false;
    method->attributes_raw_ = std::move(result);
    return true;
}

bool ClassFileEditor::addEntryFrame(std::string_view name, std::string_view desc, u2 at) {
    auto* method = findMethod(name, desc);
    if (!method) return false;
    const u1* p = method->attributes_raw_.data();
    const u1* end = p + method->attributes_raw_.size();
    if (end - p < 2) return false;
    const u2 count = rd16(p);
    std::vector<u1> result;
    wr16(result, count);
    bool added = false;
    for (u2 i = 0; i < count; ++i) {
        if (end - p < 6) return false;
        const u2 attr = rd16(p);
        const u4 size = rd32(p);
        if (size > size_t(end - p)) return false;
        const u1* attrEnd = p + size;
        wr16(result, attr);
        if (cpUtf8Value(attr) != "Code") {
            wr32(result, size); result.insert(result.end(), p, attrEnd); p = attrEnd; continue;
        }
        if (size < 12) return false;
        const u1* q = p + 4;
        const u4 codeLength = rd32(q);
        if (codeLength > size_t(attrEnd - q) || at >= codeLength) return false;
        q += codeLength;
        if (attrEnd - q < 2) return false;
        const u2 exceptions = rd16(q);
        if (size_t(attrEnd - q) < size_t(exceptions) * 8 + 2) return false;
        q += size_t(exceptions) * 8;
        const u1* countAt = q;
        const u2 nestedCount = rd16(q);
        std::vector<u1> nested;
        bool found = false;
        for (u2 j = 0; j < nestedCount; ++j) {
            if (attrEnd - q < 6) return false;
            const u2 id = rd16(q);
            const u4 length = rd32(q);
            if (length > size_t(attrEnd - q)) return false;
            const u1* nestedEnd = q + length;
            wr16(nested, id);
            if (cpUtf8Value(id) != "StackMapTable") {
                wr32(nested, length); nested.insert(nested.end(), q, nestedEnd); q = nestedEnd; continue;
            }
            if (found || length < 2) return false;
            found = true;
            const u2 frames = rd16(q);
            if (frames == 65535) return false;
            std::vector<u1> table;
            wr16(table, frames + 1);
            table.push_back(251); wr16(table, at); // same locals as method entry
            if (frames) {
                if (q == nestedEnd) return false;
                const u1 tag = *q++;
                u2 first;
                if (tag <= 63) first = tag;
                else if (tag <= 127) first = tag - 64;
                else {
                    if (tag < 247 || nestedEnd - q < 2) return false;
                    first = rd16(q);
                }
                if (first <= at) return false;
                const u2 delta = first - at - 1;
                if (tag <= 63) { table.push_back(251); wr16(table, delta); }
                else if (tag <= 127) { table.push_back(247); wr16(table, delta); }
                else { table.push_back(tag); wr16(table, delta); }
                table.insert(table.end(), q, nestedEnd);
            } else if (q != nestedEnd) return false;
            wr32(nested, static_cast<u4>(table.size()));
            nested.insert(nested.end(), table.begin(), table.end());
            q = nestedEnd;
        }
        if (q != attrEnd || (!found && nestedCount == 65535)) return false;
        if (!found) {
            wr16(nested, addUtf8("StackMapTable")); wr32(nested, 5);
            wr16(nested, 1); nested.push_back(251); wr16(nested, at);
        }
        std::vector<u1> body(p, countAt);
        wr16(body, nestedCount + (found ? 0 : 1));
        body.insert(body.end(), nested.begin(), nested.end());
        wr32(result, static_cast<u4>(body.size()));
        result.insert(result.end(), body.begin(), body.end());
        p = attrEnd; added = true;
    }
    if (!added || p != end) return false;
    method->attributes_raw_ = std::move(result);
    return true;
}

bool ClassFileEditor::insertCancelGuard(std::string_view name, std::string_view desc,
                                        std::string_view hookOwner,
                                        std::string_view hookName,
                                        std::string_view hookDesc,
                                        int argCount, bool entryReturn) {

    MethodInfo* method = findMethod(name, desc);
    if (!method) return false;
    if (entryReturn && (name == "<init>" || name == "<clinit>" || desc.empty() || desc.back() != 'V')) return false;

    const u1* code = nullptr;
    u4 codeLen = 0;
    std::vector<u4> offsets;
    if (!codeOf(*method, code, codeLen, offsets) || offsets.empty()) return false;

    const u4 last = offsets.back();
    if (!entryReturn && code[last] != 0xB1) return false;

    const bool needsReturnFrame = !branchesTo(code, codeLen, last);

    if (branchesTo(code, codeLen, 0)) return false;

    std::vector<char> types;
    std::vector<int> widths;
    if (!ParameterTypes(desc, types, widths)) return false;

    std::vector<char> localType{'L'};
    std::vector<int> localSlot{0};
    int next = 1;
    for (size_t k = 0; k < types.size(); ++k) {
        localType.push_back(types[k]);
        localSlot.push_back(next);
        next += widths[k];
    }
    if (argCount < 1 || argCount > int(localType.size())) return false;

    int hookArgs = 0, hookPushed = 0;
    if (!stackEffect(hookDesc, true, hookArgs, hookPushed)) return false;
    if (hookArgs != argCount || hookPushed != 1) return false;

    std::string_view returns;
    if (!ReturnDescriptor(hookDesc, returns) || returns != "Z") return false;

    if (!entryReturn && !needsReturnFrame && frameHasLocalFrom(*method, last, widths)) return false;

    std::vector<u1> insertion;
    for (int i = 0; i < argCount; ++i) {
        if (!LoadOpcode(localType[size_t(i)], localSlot[size_t(i)], insertion))
            return false;
    }

    const u2 hookRef = addMethodRef(hookOwner, hookName, hookDesc);
    insertion.push_back(0xB8);
    insertion.push_back(u1(hookRef >> 8));
    insertion.push_back(u1(hookRef & 0xFF));

    const size_t branchAt = insertion.size();
    insertion.push_back(entryReturn ? 0x99 : 0x9A);
    insertion.push_back(0x00);
    insertion.push_back(0x00);
    while (((insertion.size() + (entryReturn ? 1 : 0)) & 3u) != 0) insertion.push_back(0x00);
    if (entryReturn) insertion.push_back(0xB1);

    const u4 delta = static_cast<u4>(insertion.size());
    const i8 offset = i8((entryReturn ? 0 : last) + delta) - i8(branchAt);
    if (offset < -32768 || offset > 32767) return false;
    insertion[branchAt + 1] = u1((u2(offset) >> 8) & 0xFF);
    insertion[branchAt + 2] = u1(u2(offset) & 0xFF);

    const auto original = method->attributes_raw_;
    // Straight-line Runnables have no existing return frame. Add it only
    // after validating the guard, and roll back on a failed transformation.
    if (!entryReturn && needsReturnFrame && !addStraightLineReturnFrame(name, desc)) return false;
    if (!insertAt(name, desc, 0, insertion, u2(argCount + 1))) {
        method->attributes_raw_ = original;
        return false;
    }
    if (entryReturn && !addEntryFrame(name, desc, static_cast<u2>(delta))) {
        method->attributes_raw_ = original;
        return false;
    }
    return true;
}

bool ClassFileEditor::appendBeforeValueReturns(std::string_view name,
                                              std::string_view desc,
                                              const std::vector<u1>& insertion,
                                              u2 extraStack) {
    MethodInfo* method = findMethod(name, desc);
    const auto resultStart=desc.find(')');
    if (!method || resultStart==std::string_view::npos || resultStart+1>=desc.size()) return false;
    const char result=desc[resultStart+1];
    if (result!='F' && result!='D' && result!='L' && result!='[') return false;
    const u1 opcode = result=='F' ? 0xAE : result=='D' ? 0xAF : 0xB0;
    const u1* code = nullptr;
    u4 codeLen = 0;
    std::vector<u4> offsets, returns;
    if (!codeOf(*method, code, codeLen, offsets)) return false;
    for (const u4 at : offsets) if (code[at] == opcode) returns.push_back(at);
    if (returns.empty()) return false;
    const auto original = method->attributes_raw_;
    for (auto it = returns.rbegin(); it != returns.rend(); ++it) {
        if (!insertAt(name, desc, *it, insertion, extraStack)) {
            method->attributes_raw_ = original;
            return false;
        }
    }
    return true;
}

bool ClassFileEditor::appendBeforeVoidReturns(std::string_view name,
                                             std::string_view desc,
                                             const std::vector<u1>& insertion,
                                             u2 extraStack) {
    MethodInfo* method = findMethod(name, desc);
    if (!method || desc.empty() || desc.back() != 'V') return false;
    const u1* code = nullptr;
    u4 codeLen = 0;
    std::vector<u4> offsets, returns;
    if (!codeOf(*method, code, codeLen, offsets)) return false;
    for (const u4 at : offsets) if (code[at] == 0xB1) returns.push_back(at);
    if (returns.empty()) return false;
    const auto original = method->attributes_raw_;
    for (auto it = returns.rbegin(); it != returns.rend(); ++it) {
        if (!insertAt(name, desc, *it, insertion, extraStack)) {
            method->attributes_raw_ = original;
            return false;
        }
    }
    return true;
}

bool ClassFileEditor::appendBeforeLastReturn(std::string_view name,
                                             std::string_view desc,
                                             const std::vector<u1>& insertion,
                                             u2 extraStack) {
    MethodInfo* method = findMethod(name, desc);
    if (!method) return false;

    const u1* code = nullptr;
    u4 codeLen = 0;
    std::vector<u4> offsets;
    if (!codeOf(*method, code, codeLen, offsets) || offsets.empty()) return false;

    const u4 last = offsets.back();
    if (!(code[last] >= 0xAC && code[last] <= 0xB1)) return false;

    if (desc.empty()) return false;
    u1 want = 0;
    switch (desc.back()) {
        case 'V': want = 0xB1; break;
        case 'J': want = 0xAD; break;
        case 'F': want = 0xAE; break;
        case 'D': want = 0xAF; break;
        case ';': case '[': want = 0xB0; break;
        default:  want = 0xAC; break;
    }
    if (code[last] != want) return false;

    for (u4 at : offsets) {
        if ((code[at] >= 0xAC && code[at] <= 0xB1) && at != last) return false;
    }

    return insertAt(name, desc, last, insertion, extraStack);
}

std::vector<u1> ClassFileEditor::rebuildCodeAttribute(
        const std::vector<u1>& originalAttrs,
        u2 codeAttrNameIdx,
        u4 at,
        const std::vector<u1>& insertion,
        u2 extraStack) const {

    const u1* p   = originalAttrs.data();
    const u1* end = p + originalAttrs.size();
    if (end - p < 2) return {};
    u2 attrCount = rd16(p);

    std::vector<u1> out;
    wr16(out, attrCount);

    for (u2 i = 0; i < attrCount; ++i) {
        if (end - p < 6) return {};
        u2 nameIdx = rd16(p);
        u4 attrLen = rd32(p);
        const u1* attrEnd = p + attrLen;
        if (attrEnd > end) return {};

        if (nameIdx != codeAttrNameIdx) {
            wr16(out, nameIdx);
            wr32(out, attrLen);
            out.insert(out.end(), p, attrEnd);
            p = attrEnd;
            continue;
        }

        if (attrEnd - p < 12) return {};
        u2 maxStack  = rd16(p);
        u2 maxLocals = rd16(p);
        u4 codeLen   = rd32(p);
        const u1* codeBytes = p;
        if (u4(attrEnd - codeBytes) < codeLen) return {};
        p += codeLen;
        if (attrEnd - p < 2) return {};
        u2 excCount = rd16(p);
        const u1* excStart = p;
        if (u4(attrEnd - p) < size_t(excCount) * 8 + 2) return {};
        p += size_t(excCount) * 8;
        u2 innerAttrCount = rd16(p);
        const u1* innerAttrsStart = p;

        p = attrEnd;

        if (at > codeLen) return {};

        const u4 delta = static_cast<u4>(insertion.size());
        const u4 newCodeLen = codeLen + delta;

        if (newCodeLen >= 65536) return {};

        const u2 newMaxStack = (extraStack > maxStack) ? extraStack : maxStack;

        std::vector<u1> newCode(codeBytes, codeBytes + codeLen);
        if (!shiftBranches(newCode, at, delta)) return {};
        newCode.insert(newCode.begin() + at, insertion.begin(), insertion.end());

        std::vector<u1> innerAttrsOut;
        wr16(innerAttrsOut, innerAttrCount);
        const u1* ip = innerAttrsStart;
        for (u2 j = 0; j < innerAttrCount; ++j) {
            if (attrEnd - ip < 6) return {};
            u2 innerNameIdx = rd16(ip);
            u4 innerLen     = rd32(ip);
            const u1* innerEnd = ip + innerLen;
            if (innerEnd > attrEnd) return {};
            std::vector<u1> innerBody(ip, innerEnd);
            ip = innerEnd;

            std::string innerName = cpUtf8Value(innerNameIdx);
            std::vector<u1> patched;
            if (innerName == "StackMapTable") {
                patched = shiftStackMap(innerBody, delta, at);
            } else if (innerName == "LineNumberTable") {
                patched = shiftLineNumberTable(innerBody, delta, at);
            } else if (innerName == "LocalVariableTable" ||
                       innerName == "LocalVariableTypeTable") {
                patched = shiftLocalVarTable(innerBody, delta, at);
            } else {
                patched = std::move(innerBody);
            }
            wr16(innerAttrsOut, innerNameIdx);
            wr32(innerAttrsOut, static_cast<u4>(patched.size()));
            innerAttrsOut.insert(innerAttrsOut.end(), patched.begin(), patched.end());
        }

        std::vector<u1> excOut;
        wr16(excOut, excCount);
        const u1* ep = excStart;
        for (u2 j = 0; j < excCount; ++j) {
            u2 startPc  = rd16(ep);
            u2 endPc    = rd16(ep);
            u2 handler  = rd16(ep);
            u2 catchTyp = rd16(ep);
            if (startPc > at) startPc = static_cast<u2>(startPc + delta);
            if (endPc > at)   endPc   = static_cast<u2>(endPc + delta);
            if (handler >= at) handler = static_cast<u2>(handler + delta);
            wr16(excOut, startPc);
            wr16(excOut, endPc);
            wr16(excOut, handler);
            wr16(excOut, catchTyp);
        }

        std::vector<u1> codeBody;
        wr16(codeBody, newMaxStack);
        wr16(codeBody, maxLocals);
        wr32(codeBody, newCodeLen);
        codeBody.insert(codeBody.end(), newCode.begin(), newCode.end());
        codeBody.insert(codeBody.end(), excOut.begin(), excOut.end());
        codeBody.insert(codeBody.end(), innerAttrsOut.begin(), innerAttrsOut.end());

        wr16(out, nameIdx);
        wr32(out, static_cast<u4>(codeBody.size()));
        out.insert(out.end(), codeBody.begin(), codeBody.end());
    }

    return out;
}

std::vector<u1> ClassFileEditor::serialize() const {
    std::vector<u1> out;
    out.reserve(4096);

    wr32(out, magic_);
    wr16(out, minor_version_);
    wr16(out, major_version_);

    u2 count = static_cast<u2>(cp_.size());
    wr16(out, count);
    for (u2 i = 1; i < cp_.size(); ++i) {
        const CpEntry& e = cp_[i];
        if (e.tag == 0) continue;
        out.push_back(e.tag);
        out.insert(out.end(), e.body.begin(), e.body.end());
    }

    wr16(out, access_flags_);
    wr16(out, this_class_);
    wr16(out, super_class_);
    wr16(out, static_cast<u2>(interfaces_.size()));
    for (u2 x : interfaces_) wr16(out, x);

    out.insert(out.end(), fields_raw_.begin(), fields_raw_.end());

    wr16(out, static_cast<u2>(methods_.size()));
    for (const auto& m : methods_) {
        wr16(out, m.access_flags);
        wr16(out, m.name_index);
        wr16(out, m.descriptor_index);
        out.insert(out.end(), m.attributes_raw_.begin(), m.attributes_raw_.end());
    }

    out.insert(out.end(), class_attrs_raw_.begin(), class_attrs_raw_.end());
    return out;
}
