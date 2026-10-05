#pragma once
#include "contracts/AccountService.h"

namespace nova::minecraft::accounts::portraits {
// Called only from the account tick under its lifecycle lock. Texture publication
// stays here; the HTTP worker owns CPU pixels and never touches the renderer.
void Update(nova::accounts::Snapshot&);
void Stop();
bool WaitStopped(unsigned milliseconds);
}
