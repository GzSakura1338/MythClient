#pragma once

namespace nova::module::healthbypass {
// TargetHUD retains its existing option/config, using the same score service.
void SetHudEnabled(bool enabled);
void Apply();
}
