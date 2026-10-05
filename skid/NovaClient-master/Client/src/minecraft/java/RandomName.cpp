#include "minecraft/java/api/random_name.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <iterator>
#include <mutex>
#include <random>
#include <string>

namespace {

constexpr const char* kPackageWords[] = {
    "adapter",     "allocation", "annotation", "archive",    "bootstrap",
    "buffer",      "builder",    "cache",      "channel",    "codec",
    "collection",  "compiler",   "component",  "config",     "connection",
    "container",   "context",    "converter",  "coordinator","decoder",
    "delegate",    "descriptor", "dispatcher", "encoder",    "endpoint",
    "executor",    "extension",  "factory",    "filter",     "framework",
    "gateway",     "handler",    "helper",     "injector",   "inspector",
    "internal",    "iterator",   "kernel",     "lifecycle",  "listener",
    "loader",      "logging",    "manager",    "mapper",     "matcher",
    "metrics",     "model",      "monitor",    "normalizer", "observer",
    "parser",      "pipeline",   "platform",   "pool",       "processor",
    "profiler",    "protocol",   "provider",   "publisher",  "reader",
    "registry",    "repository", "resolver",   "runtime",    "scheduler",
    "scanner",     "schema",     "serializer", "service",    "session",
    "settings",    "source",     "storage",    "stream",     "subscriber",
    "support",     "task",       "tokenizer",  "transport",  "validator",
    "visitor",     "wrapper",    "writer",
};

constexpr const char* kClassWords[] = {
    "Adapter",    "Allocator",   "Assembler",  "Bootstrap",  "Buffer",
    "Builder",    "Cache",       "Channel",    "Codec",      "Collector",
    "Compiler",   "Composer",    "Connection", "Container",  "Context",
    "Controller", "Converter",   "Coordinator","Decoder",    "Delegate",
    "Descriptor", "Dispatcher",  "Encoder",    "Endpoint",   "Executor",
    "Extractor",  "Factory",     "Filter",     "Gateway",    "Handler",
    "Helper",     "Injector",    "Inspector",  "Iterator",   "Kernel",
    "Lifecycle",  "Listener",    "Loader",     "Manager",    "Mapper",
    "Matcher",    "Metrics",     "Monitor",    "Normalizer", "Observer",
    "Parser",     "Pipeline",    "Pool",       "Processor",  "Profiler",
    "Protocol",   "Provider",    "Publisher",  "Reader",     "Registry",
    "Repository", "Resolver",    "Runtime",    "Scheduler",  "Scanner",
    "Serializer", "Service",     "Session",    "Settings",   "Source",
    "Storage",    "Stream",      "Subscriber", "Suppressor", "Tokenizer",
    "Transport",  "Validator",   "Visitor",    "Wrapper",    "Writer",
};

std::mt19937_64 make_rng() {
    LARGE_INTEGER pc{};
    QueryPerformanceCounter(&pc);
    FILETIME ft{};
    GetSystemTimeAsFileTime(&ft);
    uint64_t seed =
        (uint64_t)pc.QuadPart ^
        ((uint64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime) ^
        ((uint64_t)GetCurrentProcessId() << 16) ^
        (uint64_t)GetCurrentThreadId();
    return std::mt19937_64(seed);
}

}

std::string GenerateRandomClassName(int minWords, int maxWords) {
    if (minWords < 1) minWords = 1;
    if (maxWords < minWords) maxWords = minWords;

    std::mt19937_64 rng = make_rng();

    std::uniform_int_distribution<int> nDist(minWords, maxWords);
    std::uniform_int_distribution<size_t> wDist(0, std::size(kClassWords) - 1);

    int n = nDist(rng);
    std::string out;
    out.reserve(48);
    for (int i = 0; i < n; ++i) {
        out += kClassWords[wDist(rng)];
    }
    return out;
}

std::string MakeInternalName(const std::string& internalPackage,
                             const std::string& simpleName) {
    if (internalPackage.empty()) return simpleName;
    std::string out = internalPackage;
    if (out.back() != '/') out.push_back('/');
    out += simpleName;
    return out;
}

namespace {

std::string GenerateRandomPackageName() {
    std::mt19937_64 rng = make_rng();
    std::uniform_int_distribution<int> depthDist(1, 3);
    std::uniform_int_distribution<size_t> wDist(0, std::size(kPackageWords) - 1);

    int depth = depthDist(rng);
    std::string out;
    for (int i = 0; i < depth; ++i) {
        if (i > 0) {
            out += "/";
        }
        out += kPackageWords[wDist(rng)];
    }
    return out;
}

std::string g_trampolinePackage;
std::once_flag g_packageInitFlag;

}

const std::string& GetTrampolinePackage() {
    std::call_once(g_packageInitFlag, []() {
        g_trampolinePackage = GenerateRandomPackageName();
    });
    return g_trampolinePackage;
}
