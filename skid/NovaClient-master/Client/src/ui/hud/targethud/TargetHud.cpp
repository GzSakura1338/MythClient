#include "ui/overlay/HudBackground.h"
#include "ui/hud/targethud/TargetHud.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "third_party/fatalwidgets/imgui/imgui.h"
#include "ui/clickgui/Fonts.h"
#include "ui/overlay/ClientOverlay.h"
#include "ui/text/ImTextRenderer.h"
#include "utils/Color.h"

// TargetHUD -- who you are fighting, and how they are doing.
//
// The panel is a card: the target's face, their name, their health as a number and
// as a bar. It takes Naven's and Starain's shape -- both draw the same 125x40 card
// with the head on the left and the bar under the name -- with the box measured
// from the text rather than fixed, so a long name widens the card instead of
// running off it. The originals compute a bar width and then let a wide name push
// the card past it; here the bar and the text simply share one content width.
//
// The host supplies target identities and generic portrait layers. Target
// selection, world lifetime and avatar UV rules are outside this renderer.
//
// --- the stack -----------------------------------------------------------------
//
// A switching aura holds several targets at once, and with Multi Target on each one
// gets a card of its own, stacked top to bottom under the anchor. Cards are keyed by
// entity id rather than by position, because the aura re-sorts its list every tick
// and a card tied to a position would swap its contents out from under the player
// whenever two targets traded places. A card that leaves the list finishes leaving
// where it was, and the ones below it slide up into the gap.
//
// With one target there is only ever one card, and a change of target there is not
// a card arriving beside the old one: the card on screen leaves and the next one
// takes its place, which is what the pending snapshot is for.
//
// Draw borrows snapshots synchronously on the render thread. Animated cards own
// their last snapshot, so removing an entry cannot invalidate an exit animation.

namespace nova::ui::targethud {
namespace {
// The card, as fractions of the overlay's own text size. Text is the unit rather
// than pixels because the overlay scales with the interface, and a card that did
// not would drift out of proportion with the array list beside it.
constexpr float kPadRatio = 0.26f;
constexpr float kHeadRatio = 1.58f;
constexpr float kBarHeightRatio = 0.32f;
constexpr float kRowGapRatio = 0.21f;

// The card's corners, as a fraction of the text size rather than of the padding.
// Taking them off the padding is what made them invisible: the padding is five
// pixels, so a fraction of it is a fraction of five, and the card came out square.
constexpr float kCardRoundingRatio = 0.42f;

// The head is square and clipped to a rounded square a little tighter than the
// card's own corners, the way both originals round theirs.
constexpr float kHeadRounding = 0.18f;

// The gap between the name and anything drawn next to it on the same line.
constexpr float kInlineGap = 6.0f;

// The gap between one card in the stack and the next.
constexpr float kStackGapRatio = 0.32f;

constexpr ImU32 kBarTrough = IM_COL32(0, 0, 0, 110);
constexpr ImU32 kMissingHead = IM_COL32(0, 0, 0, 90);
constexpr ImU32 kHurt = IM_COL32(255, 80, 80, 255);

// How hard the hurt flash lands. The originals draw theirs at 120 of 255 over the
// head; the same weight here.
constexpr float kHurtStrength = 120.0f / 255.0f;

// Appearing is quick and leaving is the Fade setting, so the card is not something
// the player waits for when a fight starts.
constexpr double kEnterSeconds = 0.18;
constexpr double kMaxFrameSeconds = 0.1;

// A target that blinks out for one tick -- a dropped raycast, a rotation still
// travelling -- must not start the leave. Three ticks of nothing is what counts as
// gone, which is under the shortest fade and invisible next to it.
constexpr long long kLostGraceMillis = 200;

// Long enough without a card on screen and the health number is taken as it is
// rather than rolled to -- there is nothing for it to have rolled from.
constexpr long long kRollFreshMillis = 300;

// The transition, on top of the fade: at the start of the leave the card sits this
// far back towards the corner it is anchored to, and travels to nothing by the time
// it is fully opaque. Position only, deliberately -- see the note on UpdateCards.
constexpr float kSlide = 8.0f;

// How fast a card catches up to a new measurement or a new place in the stack. Every
// health change moves the width of the number on the card's right, and the card is
// anchored on the edge that moves, so without this the whole thing twitches on every
// hit.
constexpr float kResizeRate = 10.0f;

// The health number rolls rather than swapping: a character that changes slides out
// and its replacement slides in behind it. This is Starain's rate, 4.5 a second, so
// one character takes a little over a fifth of a second to settle.
constexpr float kRollRate = 4.5f;

// The bar chases the health it is drawing instead of snapping to it, one more thing
// that would otherwise step on every hit.
constexpr float kBarRate = 7.0f;

[[nodiscard]] long long NowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Fast at the start and settled at the end, which is what arriving should look
// like. Both directions use it, and that is the point: it has zero velocity at the
// fully-shown end, so arriving settles into that end and leaving departs from it --
// neither one begins with a jerk.
[[nodiscard]] float EaseOut(float t) {
    const float inverse = 1.0f - t;
    return 1.0f - inverse * inverse * inverse;
}

// An ImGui colour with its own alpha scaled rather than replaced: the background
// is configured with one, and the fade has to multiply into it, not overwrite it.
[[nodiscard]] ImU32 WithAlpha(ImU32 color, float alpha) {
    const float base = static_cast<float>(nova::color::Alpha(color));
    return nova::color::ReAlpha(
        color, static_cast<int>(std::lround(base * std::clamp(alpha, 0.0f, 1.0f))));
}

// The horizontal furniture the card spends outside its content: the padding on the
// left, the head, the gap after it, and the padding on the right. Taken from the
// font rather than from a measurement so the overlay window can be sized without
// one -- see ContentWidth.
[[nodiscard]] float ContentInset(float fontSize) {
    const float pad = std::round(fontSize * kPadRatio);
    const float head = std::round(fontSize * kHeadRatio);
    return pad * 3.0f + head;
}

// Health, absorption included, with the base value held to the maximum: a server
// can report more health than the entity's own maximum, and a bar past its own end
// is worse than a bar that stops at it.
[[nodiscard]] float CurrentHealth(const Snapshot& snap) {
    const float maximum = snap.maxHealth > 0.0f ? snap.maxHealth : 20.0f;
    return std::max(0.0f, std::min(snap.health, maximum)) + std::max(0.0f, snap.absorption);
}

[[nodiscard]] float HealthRatio(const Snapshot& snap) {
    const float maximum =
        (snap.maxHealth > 0.0f ? snap.maxHealth : 20.0f) + std::max(0.0f, snap.absorption);
    if (maximum <= 0.0f)
        return 0.0f;
    return std::clamp(CurrentHealth(snap) / maximum, 0.0f, 1.0f);
}

// The health string split into one entry per character.
//
// Everything this is handed comes out of std::format and is ASCII, but a multi-byte
// sequence is kept whole rather than cut in half: the split feeds a per-character
// draw call, and half a sequence is not a character any font can render.
[[nodiscard]] std::vector<std::string> SplitGlyphs(const std::string& text) {
    std::vector<std::string> glyphs;
    std::size_t at = 0;
    while (at < text.size()) {
        const unsigned char lead = static_cast<unsigned char>(text[at]);
        std::size_t length = 1;
        if ((lead & 0xE0u) == 0xC0u)
            length = 2;
        else if ((lead & 0xF0u) == 0xE0u)
            length = 3;
        else if ((lead & 0xF8u) == 0xF0u)
            length = 4;
        if (at + length > text.size())
            length = 1;
        glyphs.push_back(text.substr(at, length));
        at += length;
    }
    return glyphs;
}

// One character of the health number, with the one it is replacing.
struct RollSlot {
    std::string shown;
    std::string incoming;
    float progress = 1.0f;

    // Which way the old one leaves: +1 when the number went up, -1 when it went
    // down, so the digit travels the way the value did.
    float direction = 1.0f;
};

// One card, and everything its own animation needs. A row of them is a row of
// these, with nothing shared between them.
struct Card {
    // On this frame's list. False means it is leaving.
    bool live = false;

    // Whether the drawn size and place have been taken from a measurement yet. They
    // are primed rather than chased the first time, because there is nothing to
    // travel from.
    bool placed = false;

    // The tick this card last had a target for it, which is what the lost grace is
    // measured from.
    long long lastSeen = 0;

    // Where it sits in the aura's list, so a re-sort moves the cards rather than
    // leaving them in the order they happened to be created in.
    int order = 0;

    Snapshot snapshot;

    // The card this one is leaving to make room for. Single target only: with a
    // stack there is another card beside it to take the slot, and with one card
    // there is not.
    Snapshot pending;

    float progress = 0.0f;
    float eased = 0.0f;

    float width = 0.0f;
    float height = 0.0f;
    float left = 0.0f;
    float top = 0.0f;

    float barFill = 0.0f;
    bool barPrimed = false;

    std::vector<RollSlot> roll;
    std::string rollText;
    float rollValue = 0.0f;
    long long rollSeen = 0;
    bool rollPrimed = false;
};

// The card's measurements for one snapshot, all derived from the pushed font.
struct Layout {
    float pad = 0.0f;
    float head = 0.0f;
    float gap = 0.0f;
    float barHeight = 0.0f;
    float lineHeight = 0.0f;
    float contentHeight = 0.0f;
    float contentWidth = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float nameWidth = 0.0f;
    float healthWidth = 0.0f;
    float distanceWidth = 0.0f;
    std::string health;
    std::string distance;
};

}

struct TargetHud::Impl final : overlay::ClientOverlay {
public:
    explicit Impl(Settings& s) {
        fade_=&s.fade;width_=&s.width;health_=&s.health;distance_=&s.distance;
        anchor_=&s.anchor;stack_=&s.stack;background_=&s.background;textColor_=&s.text;
        textShadow_=&s.shadow;barColor_=&s.bar;hurtFlash_=&s.hurtFlash;
    }

    // --- the draw, on the render thread --------------------------------------

    void Draw(bool editing,std::span<const Snapshot> snapshots) {
        if (ImGui::GetCurrentContext() == nullptr || ImGui::GetFont() == nullptr)
            return;
        snapshots_=snapshots;
        Render(editing);
        snapshots_={};
    }

    // Advances every card and takes whatever the game thread last left behind.
    //
    // A card keeps drawing its last live snapshot while it leaves, so a target that
    // walks out of sight slides away instead of vanishing. Leaving is timed from the
    // last tick that had something for that card rather than from the tick the list
    // went empty: the game thread clears a target on a single missed tick -- a
    // dropped raycast, a rotation still travelling -- and fading on that made the
    // card flicker every time the player's aim crossed a gap.
    //
    // Nothing here is a jump cut. A card that is no longer wanted leaves at the enter
    // speed, and the ones below it slide up into the gap rather than snapping into
    // it. The earlier single-card version reset the transition on the swapping frame,
    // which dropped the card from full size to nothing between two frames.
    //
    // The motion is position, size and opacity only, and that is deliberate. Scaling
    // it would mean pushing the overlay font at a new size every frame, and ImGui
    // rasterises glyphs per size per font -- a continuously varying size adds a set
    // of glyphs to the atlas on every frame of every transition and re-uploads the
    // atlas with them. It stutters, and the advance widths come out on whole pixels
    // so the text jitters as it grows. Moving costs nothing.
    void UpdateCards() {
        const long long now = NowMillis();
        frameDelta_ = lastFrame_ == 0
                          ? 0.0
                          : std::min(kMaxFrameSeconds,
                                     static_cast<double>(now - lastFrame_) / 1000.0);
        lastFrame_ = now;

        const auto fresh=snapshots_;

        if (fresh.size() > 1)
            GatherStack(now, fresh);
        else
            GatherSingle(now, fresh.empty() ? Snapshot{} : fresh.front());

        for (Card& card : cards_)
            AdvanceCard(card, now);

        // Anything that has finished leaving and is not on this frame's list goes.
        // The last card standing is kept: with one target the next one takes its
        // place rather than a new card being made for it.
        for (std::size_t i = cards_.size(); i-- > 0;) {
            if (cards_.size() <= 1)
                break;
            if (cards_[i].live || cards_[i].progress > 0.0f)
                continue;
            cards_.erase(cards_.begin() + static_cast<std::ptrdiff_t>(i));
        }
    }

    void AdvanceCard(Card& card, long long now) {
        const bool held =
            card.snapshot.valid && !card.pending.valid && now - card.lastSeen <= kLostGraceMillis;

        if (held) {
            card.progress = static_cast<float>(
                std::min(1.0, static_cast<double>(card.progress) + frameDelta_ / kEnterSeconds));
        } else {
            // Leaving, and leaving to make room for the next card, are the same
            // motion; only how long it takes differs. A swap is the quick one.
            const double fall =
                card.pending.valid ? kEnterSeconds : std::max(0.001, fade_->get() / 1000.0);
            card.progress = static_cast<float>(
                std::max(0.0, static_cast<double>(card.progress) - frameDelta_ / fall));
        }

        card.eased = EaseOut(card.progress);

        if (card.progress > 0.0f)
            return;

        card.eased = 0.0f;
        card.placed = false;

        if (!card.pending.valid)
            return;

        // The old card is gone; the new one takes its place and grows in from the
        // same corner. Started at this frame's worth of the rise rather than at
        // zero, so no frame lands in between with nothing on screen at all.
        card.snapshot = std::move(card.pending);
        card.pending = Snapshot{};
        card.progress = static_cast<float>(std::min(1.0, frameDelta_ / kEnterSeconds));
        card.eased = EaseOut(card.progress);
    }

    // One card, and a change of target is one card leaving and the next arriving in
    // the same place.
    void GatherSingle(long long now, Snapshot fresh) {
        if (cards_.empty())
            cards_.push_back(Card{});

        Card& card = cards_.front();
        card.live = true;
        card.order = 0;

        if (fresh.valid) {
            card.lastSeen = now;
            if (card.pending.valid) {
                // A swap is already under way; keep the newest version of the card
                // that is waiting to come in.
                card.pending = std::move(fresh);
            } else if (!card.snapshot.valid || fresh.id == card.snapshot.id) {
                card.snapshot = std::move(fresh);
            } else {
                card.pending = std::move(fresh);
            }
        } else if (card.pending.valid) {
            // Whatever the card was making room for is gone as well, so there is
            // nothing left to swap to.
            card.pending = Snapshot{};
        }

        // Leftovers from a stack that has just broken up: they finish leaving and go.
        for (std::size_t i = 1; i < cards_.size(); ++i)
            cards_[i].live = false;
    }

    // A card per target, keyed by entity id rather than by position in the list.
    void GatherStack(long long now, std::span<const Snapshot> fresh) {
        for (Card& card : cards_)
            card.live = false;

        for (std::size_t i = 0; i < fresh.size(); ++i) {
            const Snapshot& snap = fresh[i];

            Card* card = nullptr;
            for (Card& candidate : cards_) {
                if (!candidate.live && candidate.snapshot.id == snap.id) {
                    card = &candidate;
                    break;
                }
            }
            if (card == nullptr) {
                cards_.push_back(Card{});
                card = &cards_.back();
            }

            card->snapshot = snap;
            card->pending = Snapshot{};
            card->live = true;
            card->order = static_cast<int>(i);
            card->lastSeen = now;
        }
    }

    // Where the card grows from: +1 to the right, -1 to the left. Auto hands the
    // question back to ClientOverlay, which is what every other overlay does.
    [[nodiscard]] float AnchorX(const ImVec2& offset) const {
        switch (anchor_->get()) {
            case 0:
                return 1.0f;
            case 1:
                return -1.0f;
            default:
                return offset.x;
        }
    }

    // The same question vertically: +1 downwards, -1 upwards.
    [[nodiscard]] float AnchorY(const ImVec2& offset) const {
        switch (stack_->get()) {
            case 0:
                return 1.0f;
            case 1:
                return -1.0f;
            default:
                return offset.y;
        }
    }

    // --- the health number's roll --------------------------------------------

    void SnapRoll(Card& card, const std::string& text) {
        card.roll.clear();
        for (const std::string& glyph : SplitGlyphs(text)) {
            RollSlot slot;
            slot.shown = glyph;
            slot.incoming = glyph;
            slot.progress = 1.0f;
            card.roll.push_back(std::move(slot));
        }
    }

    // Only the characters that actually changed get a slot reset, so a number going
    // from 14.5 to 9.5 moves two digits and leaves the point where it is.
    void SyncRoll(Card& card, const std::string& text, float direction) {
        const std::vector<std::string> glyphs = SplitGlyphs(text);

        while (card.roll.size() > glyphs.size())
            card.roll.pop_back();

        while (card.roll.size() < glyphs.size()) {
            RollSlot slot;
            slot.shown = " ";
            slot.incoming = glyphs[card.roll.size()];
            slot.progress = 0.0f;
            slot.direction = direction;
            card.roll.push_back(std::move(slot));
        }

        for (std::size_t i = 0; i < glyphs.size(); ++i) {
            RollSlot& slot = card.roll[i];
            if (slot.incoming == glyphs[i])
                continue;
            slot.shown = slot.incoming;
            slot.incoming = glyphs[i];
            slot.progress = 0.0f;
            slot.direction = direction;
        }
    }

    void UpdateRoll(Card& card, const std::string& text, float value) {
        const long long now = NowMillis();
        const bool fresh = !card.rollPrimed || now - card.rollSeen > kRollFreshMillis;
        const float direction = value >= card.rollValue ? 1.0f : -1.0f;

        if (fresh) {
            SnapRoll(card, text);
        } else if (text != card.rollText) {
            // The digits only move when the number does, but the string can change
            // on its own -- switching the Health setting replaces all of it.
            SyncRoll(card, text, direction);
        }

        card.rollPrimed = true;
        card.rollSeen = now;
        card.rollText = text;
        card.rollValue = value;

        const float step = static_cast<float>(frameDelta_ * kRollRate);
        for (RollSlot& slot : card.roll)
            slot.progress = std::min(1.0f, slot.progress + step);
    }

    // Drawn a character at a time so a changing one can leave while its replacement
    // arrives. Clipped to the number's own box for the length of the roll, because
    // the two travelling characters are drawn a whole line outside it and would
    // otherwise cross the name beside them and the bar below.
    void DrawHealthText(ImDrawList* draws, const Card& card, const Layout& layout, float x,
                        float y, ImU32 color, bool shadow) const {
        bool rolling = false;
        for (const RollSlot& slot : card.roll) {
            if (slot.progress < 1.0f) {
                rolling = true;
                break;
            }
        }

        if (rolling) {
            draws->PushClipRect(ImVec2(x - 1.0f, y - 1.0f),
                                ImVec2(x + layout.healthWidth + 3.0f,
                                       y + layout.lineHeight + 3.0f),
                                true);
        }

        const float travel = layout.lineHeight + 2.0f;
        float penX = x;
        for (const RollSlot& slot : card.roll) {
            if (slot.progress >= 1.0f) {
                nova::text::DrawText(draws, slot.incoming.c_str(), penX, y, color, shadow);
            } else {
                const float eased = EaseOut(slot.progress);
                nova::text::DrawText(draws, slot.shown.c_str(), penX,
                                     y - slot.direction * travel * eased,
                                     WithAlpha(color, 1.0f - eased), shadow);
                nova::text::DrawText(draws, slot.incoming.c_str(), penX,
                                     y + slot.direction * travel * (1.0f - eased),
                                     WithAlpha(color, eased), shadow);
            }
            // The incoming character's width, which is what the settled number will
            // be laid out with -- taking the outgoing one would make the row shuffle
            // sideways for the length of the roll.
            penX += nova::text::TextWidth(slot.incoming.c_str());
        }

        if (rolling)
            draws->PopClipRect();
    }

    // --- drawing one card -----------------------------------------------------

    void DrawCard(ImDrawList* draws, Card& card, const Layout& layout) {
        const Snapshot& snap = card.snapshot;
        if (!snap.valid)
            return;

        const float alpha = card.eased;
        const float inset = ContentInset(ImGui::GetFontSize());
        const float contentX = card.left + layout.pad + layout.head + layout.pad;
        const float contentWidth = std::max(0.0f, card.width - inset);

        const ImU32 background = background_->getColorAbgr();
        const ImU32 text = WithAlpha(textColor_->getColorAbgr(), alpha);
        const ImU32 bar = WithAlpha(barColor_->getColorAbgr(), alpha);
        const bool shadow = textShadow_->get();

        const float rounding = std::round(ImGui::GetFontSize() * kCardRoundingRatio);
        ui::hud::Background(draws, ImVec2(card.left, card.top),
                             ImVec2(card.left + card.width, card.top + card.height), background,
                             rounding, 0, alpha);

        const float headX = card.left + layout.pad;
        const float headY = card.top + (card.height - layout.head) * 0.5f;
        DrawHead(draws, snap, headX, headY, layout.head, alpha);

        const float textY = card.top + (card.height - layout.contentHeight) * 0.5f;

        nova::text::DrawText(draws, snap.name.c_str(), contentX, textY, text, shadow);

        if (layout.distanceWidth > 0.0f) {
            nova::text::DrawText(draws, layout.distance.c_str(),
                                 contentX + layout.nameWidth + kInlineGap, textY,
                                 nova::text::Dimmed(text), shadow);
        }

        if (layout.healthWidth > 0.0f) {
            UpdateRoll(card, layout.health, CurrentHealth(snap));
            // Right-aligned on the width the settled number will have, not on the
            // one being rolled through, so the text does not drift while it moves.
            DrawHealthText(draws, card, layout, contentX + contentWidth - layout.healthWidth,
                           textY, text, shadow);
        }

        const float barY = textY + layout.lineHeight + layout.gap;
        const float barRounding = layout.barHeight * 0.5f;
        draws->AddRectFilled(ImVec2(contentX, barY),
                             ImVec2(contentX + contentWidth, barY + layout.barHeight),
                             WithAlpha(kBarTrough, alpha), barRounding);

        // The bar chases the health rather than taking it, the same way the card
        // chases its measurement. Primed rather than eased on a card that has just
        // appeared: there is no previous value to travel from, and at that point the
        // card is at nothing anyway.
        const float target = HealthRatio(snap);
        if (card.barPrimed) {
            card.barFill += (target - card.barFill) *
                            std::min(1.0f, static_cast<float>(frameDelta_ * kBarRate));
        } else {
            card.barFill = target;
            card.barPrimed = true;
        }

        const float filled = contentWidth * card.barFill;
        if (filled > 0.5f) {
            draws->AddRectFilled(ImVec2(contentX, barY),
                                 ImVec2(contentX + filled, barY + layout.barHeight), bar,
                                 barRounding);
        }
    }

    void DrawHead(ImDrawList* draws, const Snapshot& snap, float x, float y, float size,
                  float alpha) const {
        const ImVec2 min(x, y);
        const ImVec2 max(x + size, y + size);
        const float rounding = size * kHeadRounding;
        const ImU32 tint =
            IM_COL32(255, 255, 255, static_cast<int>(std::lround(alpha * 255.0f)));

        if (snap.portrait.texture != 0) {
            ui::DrawPortrait(draws, snap.portrait, min, max, tint, rounding);
        } else {
            // Nothing to draw: a mob, or a player whose sheet has not been stitched
            // yet. The plate keeps the card's shape so the name still has a column.
            draws->AddRectFilled(min, max, WithAlpha(kMissingHead, alpha), rounding);
        }

        if (hurtFlash_->get() && snap.hurtTime > 0) {
            const float intensity =
                std::clamp(static_cast<float>(snap.hurtTime) / 10.0f, 0.0f, 1.0f);
            draws->AddRectFilled(min, max, WithAlpha(kHurt, alpha * intensity * kHurtStrength),
                                 rounding);
        }
    }

    // The card is measured from the text it has to hold. The width setting is the
    // floor rather than the answer, so a long name widens the card instead of
    // running through the health number.
    [[nodiscard]] Layout Measure(const Snapshot& snap) const {
        Layout layout;
        const float font = ImGui::GetFontSize();
        layout.pad = std::round(font * kPadRatio);
        layout.head = std::round(font * kHeadRatio);
        layout.gap = std::round(font * kRowGapRatio);
        layout.barHeight = std::max(3.0f, std::round(font * kBarHeightRatio));
        layout.lineHeight = nova::text::TextHeight();
        layout.contentHeight = layout.lineHeight + layout.gap + layout.barHeight;

        layout.health = HealthText(snap);
        layout.distance = std::format("{:.1f}m", static_cast<double>(snap.distance));

        layout.nameWidth = nova::text::TextWidth(snap.name.c_str());
        layout.healthWidth =
            layout.health.empty() ? 0.0f : nova::text::TextWidth(layout.health.c_str());
        layout.distanceWidth =
            distance_->get() ? nova::text::TextWidth(layout.distance.c_str()) : 0.0f;

        const float trailing = layout.healthWidth > 0.0f ? kInlineGap + layout.healthWidth : 0.0f;
        const float middle = layout.distanceWidth > 0.0f ? kInlineGap + layout.distanceWidth : 0.0f;
        layout.contentWidth = std::max(static_cast<float>(width_->get()),
                                       layout.nameWidth + middle + trailing);
        layout.width = ContentInset(font) + layout.contentWidth;
        layout.height = 2.0f * layout.pad + std::max(layout.head, layout.contentHeight);
        return layout;
    }

    [[nodiscard]] std::string HealthText(const Snapshot& snap) const {
        const float ratio = HealthRatio(snap);
        const float current = CurrentHealth(snap);
        switch (health_->get()) {
            case 1:
                return std::format("{:.0f}%", ratio * 100.0f);
            case 2:
                return std::format("{:.1f} ({:.0f}%)", current, ratio * 100.0f);
            case 3:
                return {};
            default:
                return std::format("{:.1f}", current);
        }
    }

    void OnDraw(const Placement& placement, ImDrawList* draws) override {
        UpdateCards();
        if (cards_.empty())
            return;

        ImGui::PushFont(Medium12, kOverlayTextSize);

        // The stack's order: the cards still on the list, in the order the aura gave
        // them, then whatever is leaving. A card that is leaving therefore never sits
        // where a card arriving wants to be, and a re-sort moves cards rather than
        // swapping their contents.
        std::vector<Card*> ordered;
        ordered.reserve(cards_.size());
        for (Card& card : cards_)
            ordered.push_back(&card);
        std::ranges::stable_sort(ordered, [](const Card* a, const Card* b) {
            if (a->live != b->live)
                return a->live;
            return a->order < b->order;
        });

        // Measure each card, then take the drawn size towards it. The box catches up
        // to a new measurement instead of snapping to it: every health change moves
        // the width of the number on the card's right, and the card is anchored on
        // the edge that moves, so a box that took the new width at once twitched on
        // every hit.
        //
        // It is safe to draw at an intermediate size because the eased width only
        // ever sits between the old measurement and the new one, and the new one is
        // wide enough for everything drawn in it -- on the way down the box is larger
        // than it needs to be, never smaller.
        const float catchUp = std::min(1.0f, static_cast<float>(frameDelta_ * kResizeRate));
        std::vector<Layout> layouts(ordered.size());
        std::vector<bool> primed(ordered.size(), false);
        for (std::size_t i = 0; i < ordered.size(); ++i) {
            Card& card = *ordered[i];
            layouts[i] = Measure(card.snapshot);

            primed[i] = !card.placed;
            if (primed[i]) {
                card.width = layouts[i].width;
                card.height = layouts[i].height;
            } else {
                card.width += (layouts[i].width - card.width) * catchUp;
                card.height += (layouts[i].height - card.height) * catchUp;
            }
            card.placed = true;
        }

        // The stack hangs off the anchor: downwards when the overlay sits in the top
        // half of the screen, upwards when it does not, which is the direction the
        // array list grows in too.
        const ImVec2 pos = placement.pos;
        const ImVec2 offset = placement.offset;
        const float anchorX = AnchorX(offset);
        const float anchorY = AnchorY(offset);
        const float gap = std::round(ImGui::GetFontSize() * kStackGapRatio);

        float cursor = anchorY > 0.0f ? placement.downY : placement.upY;
        for (std::size_t i = 0; i < ordered.size(); ++i) {
            Card& card = *ordered[i];
            // Each card carries its own slide, because each one arrives and leaves on
            // its own clock -- the stack is not a single thing that moves together.
            const float slide = (1.0f - card.eased) * kSlide;

            const float slotTop =
                (anchorY > 0.0f ? cursor : cursor - card.height) - anchorY * slide;
            if (primed[i])
                card.top = slotTop;
            else
                card.top += (slotTop - card.top) * catchUp;
            cursor += (card.height + gap) * anchorY;

            const float slotLeft =
                (anchorX > 0.0f ? placement.min.x : placement.max.x - card.width) -
                anchorX * slide;
            if (primed[i])
                card.left = slotLeft;
            else
                card.left += (slotLeft - card.left) * catchUp;
        }

        for (std::size_t i = 0; i < ordered.size(); ++i) {
            Card& card = *ordered[i];
            if (card.eased <= 0.005f || !card.snapshot.valid)
                continue;
            DrawCard(draws, card, layouts[i]);
        }

        ImGui::PopFont();
    }

    // --- the overlay window --------------------------------------------------

    [[nodiscard]] const char* OverlayId() const override { return "overlays.targethud"; }
    [[nodiscard]] const char* OverlayName() const override { return "TargetHUD"; }
    [[nodiscard]] bool IsVisible() const override { return true; }

    [[nodiscard]] float ContentWidth() const override {
        // Sized from the Width setting alone, with nothing in it, and that is the
        // whole point of this override.
        //
        // ClientOverlay does not hang the card off the window's top-left: it hands
        // back a pos that IS the window's right edge whenever the overlay sits on
        // the right of the screen, and anchors the card to the screen half the
        // window's centre falls in. So the card's position is a function of the
        // window's width -- any change to one drags the other with it.
        //
        // Reading the live snapshot here made the window follow the card, and the
        // card goes empty on the tick a target is lost while it is still fully
        // opaque behind the lost grace. The window snapped back to its default width
        // on that frame and took the card sideways with it, before the leave had
        // moved anything. A width that only changes when the setting does cannot do
        // that, and it stays put when the stack grows or shrinks for the same reason.
        ImGui::PushFont(Medium12, kOverlayTextSize);
        const float width = ContentInset(ImGui::GetFontSize()) + static_cast<float>(width_->get());
        ImGui::PopFont();
        return width;
    }

    Properties::NumberProp* fade_ = nullptr;
    Properties::NumberProp* width_ = nullptr;
    Properties::ModeProperty* health_ = nullptr;
    Properties::BoolProperty* distance_ = nullptr;
    Properties::ModeProperty* anchor_ = nullptr;
    Properties::ModeProperty* stack_ = nullptr;
    Properties::ColorProp* background_ = nullptr;
    Properties::ColorProp* textColor_ = nullptr;
    Properties::BoolProperty* textShadow_ = nullptr;
    Properties::ColorProp* barColor_ = nullptr;
    Properties::BoolProperty* hurtFlash_ = nullptr;

    std::span<const Snapshot> snapshots_;

    // Render thread only.
    std::vector<Card> cards_;
    long long lastFrame_ = 0;
    double frameDelta_ = 0.0;
};

TargetHud::TargetHud():impl_(std::make_unique<Impl>(settings_)) {}
TargetHud::~TargetHud()=default;
void TargetHud::RestartTiming() {impl_->lastFrame_=0;}
void TargetHud::Draw(bool editing,std::span<const Snapshot> snapshots) {impl_->Draw(editing,snapshots);}

}
