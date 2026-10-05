#pragma once

#include <cstddef>

namespace nova::module::aimassist {

// Candidates arrive in priority order; current is the index of the previous
// entity in the freshly filtered list, or count if it is no longer eligible.
class TargetSelection {
public:
    void Reset() { selectedAt_ = -1; switching_ = false; }

    std::size_t Choose(std::size_t count, std::size_t current, bool switching,
                       long long now, long long delay) {
        if (count == 0) { Reset(); return 0; }
        if (selectedAt_ < 0 || now < selectedAt_ || switching != switching_ || current >= count) {
            selectedAt_ = now;
            switching_ = switching;
            return current < count ? current : 0;
        }
        if (switching && count > 1 && now - selectedAt_ >= (delay > 0 ? delay : 0)) {
            selectedAt_ = now;
            return (current + 1) % count;
        }
        return current;
    }

private:
    long long selectedAt_ = -1;
    bool switching_ = false;
};

}
