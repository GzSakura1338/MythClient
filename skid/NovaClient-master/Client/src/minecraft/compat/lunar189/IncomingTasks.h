#pragma once

#include <mutex>
#include <deque>
#include <utility>

namespace nova::compat::lunar {

// Tags vanilla Runnable instances, not packets: untagged vanilla/replay tasks
// must keep their original run() path and must not emit another PacketEvent.
template<class Object>
class IncomingTasks {
public:
    template<class Env>
    bool Add(Env* env, Object task) {
        Object kept = env->NewGlobalRef(task);
        if (!kept) return false;
        try {
            std::lock_guard lock(mutex_);
            tasks_.push_back(kept);
        } catch (...) {
            env->DeleteGlobalRef(kept);
            return false;
        }
        return true;
    }

    template<class Env>
    bool Take(Env* env, Object task) {
        Object kept = nullptr;
        {
            std::lock_guard lock(mutex_);
            for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
                // IsSameObject only compares JNI identities; never call Java,
                // schedule work or invoke a module while holding this mutex.
                if (!env->IsSameObject(*it, task)) continue;
                kept = *it;
                tasks_.erase(it);
                break;
            }
        }
        if (kept) env->DeleteGlobalRef(kept);
        return kept != nullptr;
    }

    template<class Env>
    void Clear(Env* env) {
        std::deque<Object> pending;
        {
            std::lock_guard lock(mutex_);
            pending.swap(tasks_);
        }
        for (auto task : pending) env->DeleteGlobalRef(task);
    }

private:
    std::mutex mutex_;
    std::deque<Object> tasks_;
};

}
