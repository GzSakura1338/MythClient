#pragma once

namespace nova::module::cheststealer {

// InvManager stands down while the stealer has just acted, so that the two are
// not clicking the same container in the same tick. Starain asks the stealer's
// own timer; this mirrors it.
bool IsWorking();

}
