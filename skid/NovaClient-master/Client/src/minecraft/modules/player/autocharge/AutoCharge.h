#pragma once
namespace nova::module::autocharge {
bool IsWorking();
// Game thread only; AutoMLG calls this after finding a real rescue placement.
void YieldForRescue();
}
