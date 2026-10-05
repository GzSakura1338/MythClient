#pragma once

namespace nova::module::autorod {
// True during an accepted hold, item use, or the delayed slot restoration.
// Simply enabling AutoRod does not reserve the hotbar.
bool OwnsItemUse();
}
