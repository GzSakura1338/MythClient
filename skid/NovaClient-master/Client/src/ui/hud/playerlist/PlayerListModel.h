#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <vector>
#include "ui/overlay/Portrait.h"
#include "ui/text/RollingText.h"

namespace nova::ui::playerlist {
struct Snapshot {
    int id = 0;
    std::string name;
    float distance = 0, bearing = 0;
    bool teammate = false;
    Portrait portrait;
    float health = -1; // Negative means the game did not provide a valid value.
};

inline bool InRange(double distance, bool limited, double range) {
    return std::isfinite(distance) && distance >= 0 && (!limited || distance <= range);
}

// Keep occupied rows in place. A full list admits a significantly closer player
// only after a sustained lead, so crowds cannot churn the last visible slot.
class Selection {
public:
    void Reset() { order_.clear(); pending_ = false; }
    void Update(std::vector<Snapshot>& entries, std::size_t maximum, double now) {
        std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b) {
            return a.distance != b.distance ? a.distance < b.distance : a.id < b.id;
        });
        auto find = [&](int id) { return std::find_if(entries.begin(),entries.end(),[&](const auto& e){return e.id==id;}); };
        std::erase_if(order_,[&](int id){return find(id)==entries.end();});
        if (order_.size()>maximum) order_.resize(maximum);
        auto selected = [&](int id){return std::find(order_.begin(),order_.end(),id)!=order_.end();};
        for (const auto& e:entries) {
            if (order_.size()>=maximum) break;
            if (!selected(e.id)) order_.push_back(e.id);
        }
        const auto newcomer = std::find_if(entries.begin(),entries.end(),[&](const auto& e){return !selected(e.id);});
        auto farthest = std::max_element(order_.begin(),order_.end(),[&](int a,int b){return find(a)->distance<find(b)->distance;});
        if (newcomer!=entries.end() && farthest!=order_.end() &&
            newcomer->distance+std::max(3.f,find(*farthest)->distance*.2f)<find(*farthest)->distance) {
            if (!pending_ || challenger_!=newcomer->id || victim_!=*farthest) {
                pending_=true; challenger_=newcomer->id; victim_=*farthest; since_=now;
            } else if (now-since_>=.8) { *farthest=newcomer->id; pending_=false; }
        } else pending_=false;
        std::vector<Snapshot> visible;
        visible.reserve(order_.size());
        for (int id:order_) visible.push_back(*find(id));
        entries=std::move(visible);
    }
private:
    std::vector<int> order_;
    bool pending_ = false;
    int challenger_ = 0, victim_ = 0;
    double since_ = 0;
};

inline float Ease(float t) { return 1.f - (1.f-t)*(1.f-t)*(1.f-t); }
inline float FollowAngle(float from, float target, float amount) {
    return from + std::remainder(target - from, 360.f) * amount;
}

struct Card {
    Snapshot snapshot;
    double lastSeen = 0;
    float progress = 0, angle = 0, y = 0;
    int order = 0;
    bool placed = false, live = false;
    text::RollingText healthText, distanceText;
    void UpdateNumbers(float delta, bool rolling) {
        const bool valid = std::isfinite(snapshot.health) && snapshot.health>=0;
        const auto health = valid ? std::format("{:.1f}",snapshot.health) : "--";
        healthText.Update(health,delta,!rolling || !valid || healthText.current=="--");
        distanceText.Update(std::format("{:.0f}",snapshot.distance),delta,!rolling);
    }
};

class Model {
public:
    std::vector<Card> cards;
    void Update(std::span<const Snapshot> entries, bool enabled, std::uint64_t generation,
                double now, float delta, float cameraYaw) {
        if (generation != generation_) { cards.clear(); generation_ = generation; }
        for (auto& card : cards) card.live = false;
        if (enabled) for (std::size_t i=0; i<entries.size(); ++i) {
            const auto& entry = entries[i];
            if (!InRange(entry.distance, false, 0) || !std::isfinite(entry.bearing)) continue;
            auto it = std::find_if(cards.begin(), cards.end(), [&](const auto& c) { return c.snapshot.id == entry.id; });
            if (it == cards.end()) {
                Card card;
                card.snapshot = entry;
                card.angle = entry.bearing - cameraYaw;
                cards.push_back(std::move(card)); it = cards.end()-1;
            }
            it->snapshot = entry; it->lastSeen = now;
            it->order = static_cast<int>(i); it->live = true;
        }
        const float dt = std::clamp(delta, 0.f, .1f);
        const float follow = 1.f - std::exp(-12.f * dt);
        for (auto& card : cards) {
            const bool show = enabled && (card.live || now-card.lastSeen < .2);
            card.progress = std::clamp(card.progress + dt/(show ? .18f : .3f)*(show ? 1.f : -1.f), 0.f, 1.f);
            card.angle = FollowAngle(card.angle, card.snapshot.bearing-cameraYaw, follow);
        }
        std::erase_if(cards, [](const auto& c) { return !c.live && c.progress <= 0; });
        std::stable_sort(cards.begin(), cards.end(), [](const auto& a, const auto& b) { return a.order < b.order; });
    }
private:
    std::uint64_t generation_ = 0;
};
}
