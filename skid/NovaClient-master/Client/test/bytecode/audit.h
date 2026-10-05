// A second opinion on a rewritten class file.
//
// ClassFileEditor decides for itself which instruction is where; if that
// decision were wrong the resulting class would still come out of the machine
// looking plausible. So the finished bytes get taken apart again here, by a
// parser that shares no code with the rewriter, and every offset that has to
// name an instruction boundary is checked against the boundaries this parser
// found. Branch targets, switch targets, exception ranges and stack map frames
// are the four things the JVM's verifier will reject a class for.
//
// It is not a substitute for loading the class -- that still happens, for the
// one class the test can execute. It is what makes the check possible at all
// for the Minecraft classes, which cannot be loaded outside the game.

#pragma once

#include "minecraft/java/api/classfile.h"

#include <algorithm>
#include <string>
#include <vector>

namespace audit {

struct Pool {
    std::vector<u1> tag;
    std::vector<u2> a, b;
    std::vector<std::vector<u1>> bytes;
};

inline u4 Be32(const u1* p) {
    return (u4(p[0]) << 24) | (u4(p[1]) << 16) | (u4(p[2]) << 8) | u4(p[3]);
}
inline u2 Be16(const u1* p) { return u2((u2(p[0]) << 8) | p[1]); }

// `p` is expected at the constant_pool_count field: the magic and the two
// version numbers have already been read by the caller.
inline bool ParsePool(const u1*& p, const u1* end, Pool& cp) {
    if (end - p < 2) return false;
    const u2 count = Be16(p);
    p += 2;
    cp.tag.assign(count, 0);
    cp.a.assign(count, 0);
    cp.b.assign(count, 0);
    cp.bytes.resize(count);

    for (u2 i = 1; i < count; ++i) {
        if (p >= end) return false;
        const u1 tag = *p++;
        cp.tag[i] = tag;
        switch (tag) {
            case 1: {
                if (end - p < 2) return false;
                const u2 len = Be16(p);
                p += 2;
                if (u4(end - p) < len) return false;
                cp.bytes[i].assign(p, p + len);
                p += len;
                break;
            }
            case 3: case 4:
                if (end - p < 4) return false;
                p += 4;
                break;
            case 5: case 6:
                if (end - p < 8) return false;
                p += 8;
                ++i;                      // long and double take two slots
                break;
            case 7: case 8: case 16:
                if (end - p < 2) return false;
                cp.a[i] = Be16(p);
                p += 2;
                break;
            case 9: case 10: case 11: case 12: case 17: case 18:
                if (end - p < 4) return false;
                cp.a[i] = Be16(p);
                cp.b[i] = Be16(p + 2);
                p += 4;
                break;
            case 15:
                if (end - p < 3) return false;
                p += 3;
                break;
            case 19: case 20:
                if (end - p < 2) return false;
                p += 2;
                break;
            default:
                return false;
        }
    }
    return true;
}

// JVMS table 7.2, written out rather than derived, so that a mistake in the
// rewriter's walker cannot hide behind the same mistake here.
inline u4 FixedLength(u1 op) {
    if (op <= 0x0F) return 1;
    if (op == 0x10 || op == 0x12) return 2;
    if (op == 0x11 || op == 0x13 || op == 0x14) return 3;
    if (op >= 0x15 && op <= 0x19) return 2;
    if (op >= 0x1A && op <= 0x35) return 1;
    if (op >= 0x36 && op <= 0x3A) return 2;
    if (op >= 0x3B && op <= 0x83) return 1;
    if (op == 0x84) return 3;
    if (op >= 0x85 && op <= 0x98) return 1;
    if (op >= 0x99 && op <= 0xA8) return 3;
    if (op == 0xA9) return 2;
    if (op >= 0xAC && op <= 0xB1) return 1;
    if (op >= 0xB2 && op <= 0xB8) return 3;
    if (op == 0xB9 || op == 0xBA) return 5;
    if (op == 0xBB || op == 0xBD) return 3;
    if (op == 0xBC) return 2;
    if (op == 0xBE || op == 0xBF) return 1;          // arraylength, athrow
    if (op == 0xC0 || op == 0xC1) return 3;          // checkcast, instanceof
    if (op == 0xC2 || op == 0xC3) return 1;          // monitorenter, monitorexit
    if (op == 0xC5) return 4;
    if (op == 0xC6 || op == 0xC7) return 3;
    if (op == 0xC8 || op == 0xC9) return 5;
    return 1;                             // 0xCA, 0xFE, 0xFF and the unused tail
}

inline bool Next(const u1* c, u4 len, u4 pc, u4& next) {
    const u1 op = c[pc];
    if (op == 0xAA) {
        const u4 q = (pc + 4) & ~3u;
        if (q + 12 > len) return false;
        const i8 lo = static_cast<i4>(Be32(c + q + 4)), hi = static_cast<i4>(Be32(c + q + 8));
        if (hi < lo) return false;
        if (hi - lo > (len - q - 12) / 4) return false;
        next = q + 12 + 4 * (hi - lo + 1);
        return next <= len;
    }
    if (op == 0xAB) {
        const u4 q = (pc + 4) & ~3u;
        if (q + 8 > len) return false;
        const u4 n = Be32(c + q + 4);
        if (n > (len - q - 8) / 8) return false;
        next = q + 8 + 8 * n;
        return next <= len;
    }
    if (op == 0xC4) {                     // wide
        if (pc + 2 > len) return false;
        const u4 size = (c[pc + 1] == 0x84) ? 6u : 4u;
        next = pc + size;
        return next <= len;
    }
    next = pc + FixedLength(op);
    return next <= len;
}

inline bool Walk(const u1* code, u4 len, std::vector<u1>& starts) {
    starts.assign(len, 0);
    u4 pc = 0;
    while (pc < len) {
        starts[pc] = 1;
        u4 next = 0;
        if (!Next(code, len, pc, next) || next <= pc) return false;
        pc = next;
    }
    return pc == len;
}

// Every absolute offset a StackMapTable names, in order.
inline bool StackMapOffsets(const u1* p, const u1* end, std::vector<u4>& out) {
    if (end - p < 2) return false;
    const u2 n = Be16(p);
    p += 2;
    u4 abs = 0;
    for (u2 i = 0; i < n; ++i) {
        if (p >= end) return false;
        const u1 tag = *p++;
        u4 delta = 0;
        bool narrow = false;
        if (tag <= 63) { delta = tag; narrow = true; }
        else if (tag <= 127) { delta = tag - 64; narrow = true; }
        else {
            if (end - p < 2) return false;
            delta = Be16(p);
            p += 2;
        }
        abs = (i == 0) ? delta : abs + delta + 1;
        out.push_back(abs);

        auto vti = [&]() -> bool {
            if (p >= end) return false;
            switch (*p) {
                case 0: case 1: case 2: case 3: case 4: case 5: case 6:
                    p += 1; return true;
                case 7: case 8:
                    if (end - p < 3) return false;
                    p += 3; return true;
                default:
                    return false;
            }
        };
        auto n_ = [&](u2 k) { for (u2 j = 0; j < k; ++j) if (!vti()) return false; return true; };

        if (narrow) { if (tag >= 64 && !vti()) return false; continue; }
        // 247 is the extended spelling of the same one-stack-item shape the
        // 64..127 range covers, and it carries the same type info. Missing it
        // does not fail loudly: the next frame is read from a byte that is
        // still frame data, and what comes out is a plausible-looking offset.
        if (tag == 247) { if (!vti()) return false; }
        else if (tag >= 252 && tag <= 254) { if (!n_(u2(tag - 251))) return false; }
        else if (tag == 255) {
            if (end - p < 2) return false;
            const u2 nl = Be16(p); p += 2;
            if (!n_(nl)) return false;
            if (end - p < 2) return false;
            const u2 ns = Be16(p); p += 2;
            if (!n_(ns)) return false;
        }
    }
    return true;
}

struct Method {
    std::string name, desc;
    const u1* code = nullptr;
    u4 codeLen = 0;
    const u1* excStart = nullptr;
    u2 excCount = 0;
    const u1* innerStart = nullptr;
    const u1* innerLimit = nullptr;      // end of the Code attribute
    u2 innerCount = 0;
};

inline bool CheckCode(const Pool& cp, const Method& m, std::string& err) {
    std::vector<u1> starts;
    if (!Walk(m.code, m.codeLen, starts)) {
        err = m.name + m.desc + ": code walk desynchronised";
        return false;
    }
    const u1* c = m.code;
    const u4 len = m.codeLen;

    auto boundary = [&](u4 t, const char* what, u4 at) {
        if (t > len || (t < len && !starts[t])) {
            err = m.name + m.desc + ": " + what + " at " + std::to_string(at) +
                  " names " + std::to_string(t) + ", not an instruction boundary";
            return false;
        }
        return true;
    };

    u4 pc = 0;
    while (pc < len) {
        const u1 op = c[pc];
        if ((op >= 0x99 && op <= 0xA8) || op == 0xC6 || op == 0xC7) {
            const i2 off = i2((u2(c[pc + 1]) << 8) | c[pc + 2]);
            if (!boundary(u4(i8(pc) + off), "branch", pc)) return false;
        } else if (op == 0xC8 || op == 0xC9) {
            if (!boundary(u4(i8(pc) + i4(Be32(c + pc + 1))), "goto_w", pc)) return false;
        } else if (op == 0xAA || op == 0xAB) {
            const u4 q = (pc + 4) & ~3u;
            if (!boundary(u4(i8(pc) + i4(Be32(c + q))), "switch default", pc)) return false;
            if (op == 0xAA) {
                const i8 lo = static_cast<i4>(Be32(c + q + 4)), hi = static_cast<i4>(Be32(c + q + 8));
                for (u4 k = 0; k <= hi - lo; ++k) {
                    const u4 at = q + 12 + 4 * k;
                    if (!boundary(u4(i8(pc) + i4(Be32(c + at))), "switch case", pc)) return false;
                }
            } else {
                const u4 n = Be32(c + q + 4);
                for (u4 k = 0; k < n; ++k) {
                    if (!boundary(u4(i8(pc) + i4(Be32(c + q + 8 + 8 * k + 4))),
                                  "switch case", pc)) return false;
                }
            }
        }
        u4 next = 0;
        if (!Next(c, len, pc, next)) return false;
        pc = next;
    }

    for (u2 i = 0; i < m.excCount; ++i) {
        const u1* e = m.excStart + size_t(i) * 8;
        const u4 s = Be16(e), en = Be16(e + 2), h = Be16(e + 4);
        if (s >= en || en > len) {
            err = m.name + m.desc + ": exception range " + std::to_string(s) + ".." +
                  std::to_string(en) + " is not inside the code";
            return false;
        }
        if (!boundary(s, "exception start", i) ||
            !boundary(en, "exception end", i) ||
            !boundary(h, "exception handler", i)) {
            return false;
        }
    }

    // The StackMapTable is the part of the rewrite with the most ways to be
    // subtly wrong, so every absolute offset it names is resolved here and
    // checked against the boundaries the walker found. A frame that lands
    // inside an instruction is a VerifyError at load.
    const u1* ip = m.innerStart;             // walking, not indexing: the
    for (u2 i = 0; i < m.innerCount; ++i) {  // records are variable length
        if (ip > m.innerLimit || m.innerLimit - ip < 6) {
            err = m.name + m.desc + ": inner attribute " + std::to_string(i) +
                  " runs past the end of the Code attribute";
            return false;
        }
        const u2 nameIdx = Be16(ip);
        const u4 innerLen = Be32(ip + 2);
        const u1* innerBody = ip + 6;
        if (u4(m.innerLimit - innerBody) < innerLen) {
            err = m.name + m.desc + ": inner attribute " + std::to_string(i) +
                  " declares more bytes than the Code attribute holds";
            return false;
        }
        ip = innerBody + innerLen;
        if (nameIdx >= cp.tag.size() || cp.tag[nameIdx] != 1) {
            err = m.name + m.desc + ": inner attribute " + std::to_string(i) +
                  " has a name that is not Utf8";
            return false;
        }
        const std::vector<u1>& nm = cp.bytes[nameIdx];
        const bool isStackMap = (nm.size() == 13) &&
            std::equal(nm.begin(), nm.end(), "StackMapTable");
        if (!isStackMap) continue;

        std::vector<u4> frames;
        if (!StackMapOffsets(innerBody, innerBody + innerLen, frames)) {
            err = m.name + m.desc + ": StackMapTable did not parse";
            return false;
        }
        for (u4 off : frames) {
            if (!boundary(off, "stack map frame", off)) return false;
        }
    }
    return true;
}

inline bool AuditClass(const std::vector<u1>& bytes, std::string& err) {
    const u1* p = bytes.data();
    const u1* end = p + bytes.size();
    if (end - p < 10 || Be32(p) != 0xCAFEBABE) { err = "not a class file"; return false; }
    p += 8;                                  // magic, minor, major

    Pool cp;
    if (!ParsePool(p, end, cp)) { err = "constant pool did not parse"; return false; }
    if (end - p < 8) { err = "truncated after the pool"; return false; }
    p += 6;                                  // access, this, super
    const u2 nInterfaces = Be16(p);
    p += 2 + size_t(nInterfaces) * 2;
    if (p > end) { err = "truncated interfaces"; return false; }

    auto skipMembers = [&](bool wantCode) {
        if (end - p < 2) return false;
        const u2 n = Be16(p);
        p += 2;
        for (u2 i = 0; i < n; ++i) {
            if (end - p < 8) return false;
            const u2 nameIdx = Be16(p + 2);
            const u2 descIdx = Be16(p + 4);
            const u2 attrCount = Be16(p + 6);
            p += 8;
            for (u2 j = 0; j < attrCount; ++j) {
                if (end - p < 6) return false;
                const u2 an = Be16(p);
                const u4 al = Be32(p + 2);
                const u1* body = p + 6;
                if (u4(end - body) < al) return false;
                if (wantCode && an < cp.tag.size() && cp.tag[an] == 1 &&
                    cp.bytes[an] == std::vector<u1>{'C', 'o', 'd', 'e'}) {
                    if (al < 12) return false;
                    Method m;
                    if (nameIdx < cp.bytes.size()) m.name.assign(cp.bytes[nameIdx].begin(), cp.bytes[nameIdx].end());
                    if (descIdx < cp.bytes.size()) m.desc.assign(cp.bytes[descIdx].begin(), cp.bytes[descIdx].end());
                    const u4 codeLen = Be32(body + 4);
                    if (u4(al - 8) < codeLen) return false;
                    m.code = body + 8;
                    m.codeLen = codeLen;
                    const u1* q = m.code + codeLen;
                    if (u4(body + al - q) < 2) return false;
                    m.excCount = Be16(q); q += 2;
                    if (u4(body + al - q) < size_t(m.excCount) * 8 + 2) return false;
                    m.excStart = q; q += size_t(m.excCount) * 8;
                    m.innerCount = Be16(q); q += 2;
                    m.innerStart = q;
                    m.innerLimit = body + al;
                    if (!CheckCode(cp, m, err)) return false;
                }
                p = body + al;
            }
        }
        return true;
    };

    if (!skipMembers(false)) { err = "fields did not parse"; return false; }
    if (!skipMembers(true)) { if (err.empty()) err = "methods did not parse"; return false; }
    return true;
}

// The length of one method's code array, or false when the class does not
// declare it. Used to hold a redirect to its contract: three bytes substituted
// in place, so the array cannot have changed length.
// The bytes of one method's StackMapTable body, or false when the class does
// not have one for that method.
//
// A table can come back the same length and still parse while having had its
// frames re-spelled, and the one thing that sees that is the bytes themselves.
// Written for the walker test, where re-spelling is the whole failure mode.
inline bool StackMapBody(const std::vector<u1>& bytes, std::string_view wantName,
                         std::string_view wantDesc, std::vector<u1>& out) {
    const u1* p = bytes.data();
    const u1* end = p + bytes.size();
    if (end - p < 10 || Be32(p) != 0xCAFEBABE) return false;
    p += 8;

    Pool cp;
    if (!ParsePool(p, end, cp)) return false;
    if (end - p < 8) return false;
    p += 6;
    const u2 nInterfaces = Be16(p);
    p += 2 + size_t(nInterfaces) * 2;
    if (p > end) return false;

    for (int section = 0; section < 2; ++section) {
        if (end - p < 2) return false;
        const u2 n = Be16(p);
        p += 2;
        for (u2 i = 0; i < n; ++i) {
            if (end - p < 8) return false;
            const u2 nameIdx = Be16(p + 2);
            const u2 descIdx = Be16(p + 4);
            const u2 attrCount = Be16(p + 6);
            p += 8;
            for (u2 j = 0; j < attrCount; ++j) {
                if (end - p < 6) return false;
                const u2 an = Be16(p);
                const u4 al = Be32(p + 2);
                const u1* body = p + 6;
                if (u4(end - body) < al) return false;
                p = body + al;
                if (section != 1 || al < 12) continue;
                if (an >= cp.tag.size() || cp.tag[an] != 1) continue;
                if (nameIdx >= cp.bytes.size() || descIdx >= cp.bytes.size()) continue;
                if (cp.bytes[an] != std::vector<u1>{'C', 'o', 'd', 'e'}) continue;
                if (std::string_view(reinterpret_cast<const char*>(cp.bytes[nameIdx].data()),
                                     cp.bytes[nameIdx].size()) != wantName) continue;
                if (std::string_view(reinterpret_cast<const char*>(cp.bytes[descIdx].data()),
                                     cp.bytes[descIdx].size()) != wantDesc) continue;

                const u4 codeLen = Be32(body + 4);
                if (u4(al - 8) < codeLen) return false;
                const u1* q = body + 8 + codeLen;
                if (u4(body + al - q) < 2) return false;
                const u2 excCount = Be16(q); q += 2;
                if (u4(body + al - q) < size_t(excCount) * 8 + 2) return false;
                q += size_t(excCount) * 8;
                const u2 innerCount = Be16(q); q += 2;
                for (u2 k = 0; k < innerCount; ++k) {
                    if (u4(body + al - q) < 6) return false;
                    const u2 innerName = Be16(q);
                    const u4 innerLen = Be32(q + 2);
                    const u1* innerBody = q + 6;
                    if (u4(body + al - innerBody) < innerLen) return false;
                    q = innerBody + innerLen;
                    if (innerName < cp.tag.size() && cp.tag[innerName] == 1 &&
                        cp.bytes[innerName] == std::vector<u1>{'S','t','a','c','k','M','a','p',
                                                               'T','a','b','l','e'}) {
                        out.assign(innerBody, innerBody + innerLen);
                        return true;
                    }
                }
                return false;
            }
        }
    }
    return false;
}

inline bool CodeLength(const std::vector<u1>& bytes, std::string_view wantName,
                       std::string_view wantDesc, u4& out) {
    const u1* p = bytes.data();
    const u1* end = p + bytes.size();
    if (end - p < 10 || Be32(p) != 0xCAFEBABE) return false;
    p += 8;

    Pool cp;
    if (!ParsePool(p, end, cp)) return false;
    if (end - p < 8) return false;
    p += 6;
    const u2 nInterfaces = Be16(p);
    p += 2 + size_t(nInterfaces) * 2;
    if (p > end) return false;

    for (int section = 0; section < 2; ++section) {
        if (end - p < 2) return false;
        const u2 n = Be16(p);
        p += 2;
        for (u2 i = 0; i < n; ++i) {
            if (end - p < 8) return false;
            const u2 nameIdx = Be16(p + 2);
            const u2 descIdx = Be16(p + 4);
            const u2 attrCount = Be16(p + 6);
            p += 8;
            for (u2 j = 0; j < attrCount; ++j) {
                if (end - p < 6) return false;
                const u2 an = Be16(p);
                const u4 al = Be32(p + 2);
                const u1* body = p + 6;
                if (u4(end - body) < al) return false;
                if (section == 1 && al >= 8 && an < cp.tag.size() && cp.tag[an] == 1 &&
                    cp.bytes[an] == std::vector<u1>{'C', 'o', 'd', 'e'} &&
                    nameIdx < cp.bytes.size() && descIdx < cp.bytes.size() &&
                    std::string_view(reinterpret_cast<const char*>(cp.bytes[nameIdx].data()),
                                     cp.bytes[nameIdx].size()) == wantName &&
                    std::string_view(reinterpret_cast<const char*>(cp.bytes[descIdx].data()),
                                     cp.bytes[descIdx].size()) == wantDesc) {
                    out = Be32(body + 4);
                    return true;
                }
                p = body + al;
            }
        }
    }
    return false;
}

} // namespace audit
