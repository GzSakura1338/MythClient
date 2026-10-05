
#pragma once

#include "classfile.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class ClassFileEditor {
public:

    explicit ClassFileEditor(const u1* bytes, size_t length);

    u2 addUtf8(std::string_view s);
    u2 addClass(std::string_view internalName);
    u2 addNameAndType(std::string_view name, std::string_view desc);
    u2 addMethodRef(std::string_view klass,
                    std::string_view name,
                    std::string_view desc);

    struct CallSite {
        u4 offset = 0;
        u1 opcode = 0;

        [[nodiscard]] u4 end() const { return offset + (opcode == 0xB9 ? 5 : 3); }
    };

    std::vector<CallSite> findCalls(std::string_view name,
                                    std::string_view desc,
                                    std::string_view calleeOwner,
                                    std::string_view calleeName,
                                    std::string_view calleeDesc,
                                    u1 opcodeMask = 0) const;

    // name="*", desc="" searches all methods by the exact callee signature.
    // This form requires unlimited/unordinal matching and commits atomically.
    bool redirectCalls(std::string_view name,
                       std::string_view desc,
                       std::string_view calleeOwner,
                       std::string_view calleeName,
                       std::string_view calleeDesc,
                       std::string_view newOwner,
                       std::string_view newName,
                       std::string_view newDesc,
                       u1 opcodeMask = 0,
                       int limit = -1,
                       int ordinal = -1);

    bool insertBeforeCall(std::string_view name,
                          std::string_view desc,
                          std::string_view calleeOwner,
                          std::string_view calleeName,
                          std::string_view calleeDesc,
                          int ordinal,
                          const std::vector<u1>& insertion,
                          u2 extraStack,
                          u1 opcodeMask = 0,
                          bool after = false);

    bool insertHookCall(std::string_view name,
                        std::string_view desc,
                        std::string_view calleeOwner,
                        std::string_view calleeName,
                        std::string_view calleeDesc,
                        bool calleeIsStatic,
                        int ordinal,
                        std::string_view hookOwner,
                        std::string_view hookName,
                        std::string_view hookDesc,
                        int argCount,
                        int storeSlot,
                        u1 opcodeMask = 0);

    bool insertCancelGuard(std::string_view name,
                           std::string_view desc,
                           std::string_view hookOwner,
                           std::string_view hookName,
                           std::string_view hookDesc,
                           int argCount, bool entryReturn = false);

    bool prependToMethodCode(std::string_view name,
                             std::string_view desc,
                             const std::vector<u1>& insertion,
                             u2 extraStack);

    bool appendBeforeLastReturn(std::string_view name,
                                std::string_view desc,
                                const std::vector<u1>& insertion,
                                u2 extraStack);

    // Void RETURN hooks, including early returns introduced by host mixins.
    bool appendBeforeValueReturns(std::string_view name, std::string_view desc,
                                  const std::vector<u1>& insertion, u2 extraStack = 0);

    bool appendBeforeVoidReturns(std::string_view name, std::string_view desc,
                                 const std::vector<u1>& insertion, u2 extraStack);

    bool hasMethod(std::string_view name, std::string_view desc) const;

    std::vector<u1> serialize() const;

    const std::string& thisClassInternalName() const { return this_name_; }

    static bool stackEffect(std::string_view descriptor, bool isStatic,
                            int& popped, int& pushed);

private:
    bool addEntryFrame(std::string_view name, std::string_view desc, u2 at);
    bool addStraightLineReturnFrame(std::string_view name, std::string_view desc);

    struct CpEntry {
        u1              tag = 0;
        std::vector<u1> body;
        bool            wide = false;
    };

    std::vector<CpEntry> cp_;

    u2 addRawEntry(u1 tag, std::vector<u1> body, bool wide = false);
    u2 findUtf8(std::string_view s) const;
    std::string cpUtf8Value(u2 idx) const;

    bool callTarget(const u1* code, u4 codeLen, const CallSite& site,
                    std::string& owner, std::string& calleeName,
                    std::string& calleeDesc, bool& isStatic) const;

    static bool nextPc(const u1* code, u4 codeLen, u4 pc, u4& next);

    bool instructionOffsets(const u1* code, u4 codeLen,
                            std::vector<u4>& out) const;

    static bool branchesTo(const u1* code, u4 codeLen, u4 target);

    u4 magic_ = 0;
    u2 minor_version_ = 0;
    u2 major_version_ = 0;
    u2 access_flags_ = 0;
    u2 this_class_ = 0;
    u2 super_class_ = 0;
    std::string this_name_;
    std::vector<u2> interfaces_;

    std::vector<u1> fields_raw_;
    std::vector<u1> class_attrs_raw_;

    struct MethodInfo {
        u2 access_flags = 0;
        u2 name_index = 0;
        u2 descriptor_index = 0;

        std::vector<u1> attributes_raw_;
    };
    std::vector<MethodInfo> methods_;

    void parse(const u1* p, const u1* end);
    static u2 rd16(const u1*& p);
    static u4 rd32(const u1*& p);
    static void wr16(std::vector<u1>& o, u2 v);
    static void wr32(std::vector<u1>& o, u4 v);

    static void copyN(std::vector<u1>& out, const u1*& p, size_t n);

    static size_t cpBodySize(u1 tag, const u1* p);

    bool frameHasLocalFrom(const MethodInfo& method, u4 at,
                           const std::vector<int>& paramWidths) const;

    MethodInfo* findMethod(std::string_view name, std::string_view desc);
    const MethodInfo* findMethod(std::string_view name, std::string_view desc) const;

    bool insertAt(std::string_view name,
                  std::string_view desc,
                  u4 at,
                  const std::vector<u1>& insertion,
                  u2 extraStack);

    std::vector<u1> rebuildCodeAttribute(
        const std::vector<u1>& originalAttrs,
        u2 codeAttrNameIdx,
        u4 insertAt,
        const std::vector<u1>& insertion,
        u2 extraStack) const;

    bool codeOf(const MethodInfo& method, const u1*& code, u4& codeLen,
                std::vector<u4>& offsets) const;

    static bool shiftBranches(std::vector<u1>& code, u4 insertAt, u4 delta);

    static std::vector<u1> shiftStackMap(const std::vector<u1>& originalBody,
                                         u4 delta,
                                         u4 from);

    static std::vector<u1> shiftLineNumberTable(const std::vector<u1>& originalBody,
                                                u4 delta,
                                                u4 from);

    static std::vector<u1> shiftLocalVarTable(const std::vector<u1>& originalBody,
                                              u4 delta,
                                              u4 from);
};
