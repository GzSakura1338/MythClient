#pragma once
#include <string>
#include "ui/overlay/Portrait.h"

namespace nova::ui::cswin {
inline constexpr double Duration = 12.016;
enum class Style { CT, T };
struct Snapshot { std::string name="Player"; Portrait portrait; Style style=Style::T; };
// Elapsed time is supplied by the host; this view owns no game state.
void Draw(double elapsed, const Snapshot& player, double duration=Duration);
}
