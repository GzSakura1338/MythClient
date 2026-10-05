#include "minecraft/java/api/base.h"
#include "minecraft/events/EventBus.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

namespace nova::compat { jobject ClassLoader(JNIEnv*) { return nullptr; } }
namespace {
void Check(bool ok, const char* why) {
    if (!ok) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
std::atomic<int> jvmtiRequests{}, attaches{}, detaches{}, disposals{};
std::atomic<bool> failDisposal{false};
JNIEnv_ fakeJni{};
jvmtiEnv fakeJvmti{};
jint JNICALL GetEnv(JavaVM*, void** out, jint version) {
    if (version == JVMTI_VERSION_1_2) {
        ++jvmtiRequests; *out = &fakeJvmti; return JNI_OK;
    }
    *out = nullptr; return JNI_EDETACHED;
}
jint JNICALL Attach(JavaVM*, void** out, void*) { ++attaches; *out = &fakeJni; return JNI_OK; }
jint JNICALL Detach(JavaVM*) { ++detaches; return JNI_OK; }
jvmtiError JNICALL Dispose(jvmtiEnv*) {
    ++disposals; return failDisposal ? JVMTI_ERROR_INTERNAL : JVMTI_ERROR_NONE;
}
struct Ordered : nova::event::CancellableEvent { std::vector<int> order; };
struct Reentrant : nova::event::Event { int depth{}; };
struct Failure : nova::event::Event {};
struct Concurrent : nova::event::Event {};
struct Unregistered : nova::event::Event {};
struct Retired : nova::event::Event {};
}

int main() {
    static_assert(!std::is_copy_constructible_v<JniAttach>);
    JNIInvokeInterface_ vmTable{};
    vmTable.GetEnv = GetEnv; vmTable.AttachCurrentThreadAsDaemon = Attach;
    vmTable.DetachCurrentThread = Detach;
    JavaVM_ vm{&vmTable};
    jvmtiInterface_1_ tiTable{}; tiTable.DisposeEnvironment = Dispose;
    fakeJvmti.functions = &tiTable;
    g_vm = &vm; g_jvmti = &fakeJvmti;
    std::vector<std::thread> lookups;
    for (int t = 0; t < 4; ++t) lookups.emplace_back([] {
        JniAttach attach;
        Check(attach.env == &fakeJni, "attach owns this thread's environment");
        for (int i = 0; i < 10000; ++i)
            Check(CurrentJvmti() == &fakeJvmti, "JVMTI environment identity");
    });
    for (auto& thread : lookups) thread.join();
    Check(jvmtiRequests == 0, "lookup must not allocate new JVMTI environments");
    Check(attaches == 4 && detaches == 4, "one detach per owned attachment");
    failDisposal = true;
    Check(!DisposeJvmti() && CurrentJvmti() == &fakeJvmti, "failed disposal retains environment");
    failDisposal = false;
    Check(DisposeJvmti() && !CurrentJvmti(), "successful disposal clears environment");
    Check(DisposeJvmti() && disposals == 2, "no double disposal");
    g_vm = nullptr;

    auto& bus = nova::event::EventBus::Get();
    auto late = bus.Subscribe<Ordered>([](auto& e) { e.order.push_back(3); });
    auto first = bus.Subscribe<Ordered>([](auto& e) { e.order.push_back(1); e.Cancel(); }, 0);
    auto second = bus.Subscribe<Ordered>([&](auto& e) { e.order.push_back(2); late.Reset(); }, 5);
    bus.Seal();
    Ordered ordered;
    Check(bus.Post(ordered) && ordered.order == std::vector<int>({1, 2}), "priority, cancellation and unsubscribe during dispatch");

    int addedCalls = 0;
    nova::event::Subscription added;
    auto reentrant = bus.Subscribe<Reentrant>([&](auto& e) {
        if (e.depth) return;
        added = bus.Subscribe<Reentrant>([&](auto&) { ++addedCalls; });
        Reentrant nested; nested.depth = 1; (void)bus.Post(nested);
    });
    Reentrant outer; (void)bus.Post(outer);
    Check(addedCalls == 1, "nested dispatch sees new listener; outer snapshot remains stable");

    auto throwing = bus.Subscribe<Failure>([](auto&) { throw std::runtime_error("fixture"); });
    Failure failure;
    try { (void)bus.Post(failure); Check(false, "exception expected"); } catch (const std::runtime_error&) {}
    throwing.Reset();
    Check(!bus.HasListeners<Failure>() && !bus.Post(failure), "exception leaves no dispatch state or held lock");

    int retiredCalls = 0;
    auto child = std::make_shared<nova::event::Subscription>(
        bus.Subscribe<Retired>([&](auto&) { ++retiredCalls; }));
    auto parent = bus.Subscribe<Retired>([child](auto&) {});
    child.reset(); parent.Reset();
    auto replacement = bus.Subscribe<Retired>([](auto&) {});
    Retired retired; (void)bus.Post(retired);
    Check(retiredCalls == 0, "retired handler may destroy a captured subscription without deadlock");

    std::atomic<int> calls{0};
    auto stable = bus.Subscribe<Concurrent>([&](auto&) { ++calls; });
    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    for (int t = 0; t < 4; ++t) workers.emplace_back([&] {
        while (!start.load()) std::this_thread::yield();
        for (int i = 0; i < 10000; ++i) {
            Concurrent e; (void)bus.Post(e);
            Unregistered unknown; (void)bus.Post(unknown);
        }
    });
    workers.emplace_back([&] {
        while (!start.load()) std::this_thread::yield();
        for (int i = 0; i < 4000; ++i) {
            auto temporary = bus.Subscribe<Concurrent>([](auto&) {});
            temporary.Reset();
        }
    });
    start = true;
    for (auto& worker : workers) worker.join();
    Check(calls == 40000, "concurrent publication preserves every stable listener call");
    std::cout << "PASS: JVMTI ownership, JNI attachment, event ordering/reentry/exceptions and concurrent publication\n";
}
