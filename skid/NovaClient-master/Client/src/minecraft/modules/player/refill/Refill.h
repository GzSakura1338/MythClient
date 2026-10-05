#pragma once

namespace nova::module::refill {
// InvManager yields to the one-shot refill until its inventory session ends.
bool IsWorking();
}
