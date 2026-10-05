#pragma once

namespace nova::compat::lunar12111 {
struct Member { const char* owner; const char* name; const char* descriptor; };
// Pure name translation, independent of the selected profile. Returned strings
// remain stable for the lifetime of the DLL, including event-rule registration.
const char* ClassName(const char* name);
const char* Descriptor(const char* descriptor);
Member Translate(Member member);
}
