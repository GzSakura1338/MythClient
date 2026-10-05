#pragma once

namespace nova::module::crystal {
// True while an executable crystal action owns this tick's rotation.
bool IsWorking();
}
