#pragma once
namespace nova::module::automlg {
// True only during a rescue/recovery, not merely when the module is enabled.
bool IsWorking();
// Game-thread handoff. A bucket use already submitted this tick is never undone.
bool YieldForMace();
}
