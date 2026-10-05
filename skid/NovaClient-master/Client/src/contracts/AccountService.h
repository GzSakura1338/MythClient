#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace nova::accounts {
enum class Source { Lunar, Offline, Token, Current, Microsoft };
struct Account {
    std::string id, name, uuid;
    Source source = Source::Current;
    bool current = false, invalid = false;
    std::uint64_t portrait = 0;
};
struct Snapshot {
    std::vector<Account> rows;
    std::string currentName, status = "Account management is available in Minecraft.";
    bool supported = false, inWorld = false, busy = false, hasHidden = false;
    std::string clientId;
    bool authorizing = false;
};
enum class Action { Microsoft, Offline, Token, Select, Remove, RestoreHidden, SetClientId, Cancel };
struct Request { Action action; std::string value; };
// The UI exchanges values only. Credentials never appear in a snapshot/config.
Snapshot Read();
bool Submit(Action action, std::string value = {});
// Native Lunar callbacks have just cleared Lunar's busy flags; the published
// snapshot may lag behind. The game-thread manager still checks live state.
bool SubmitNative(Action action, std::string value = {});
bool Take(Request& request);
void Publish(Snapshot snapshot);
void Stop();
void RequestOpen();
bool ConsumeOpen();
}
