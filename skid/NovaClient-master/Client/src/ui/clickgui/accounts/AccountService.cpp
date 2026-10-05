#include "contracts/AccountService.h"
#include "core/Instance.h"
#include <mutex>
#include <algorithm>
#include <optional>

namespace nova::accounts {
namespace {
struct Mailbox { std::mutex mutex; Snapshot view; std::optional<Request> request; bool stopped=false,open=false; };
Mailbox& State() { return Instance<Mailbox>(); }
}
Snapshot Read() { auto& s=State(); std::lock_guard lock(s.mutex); return s.view; }
bool Submit(Action action, std::string value) {
    auto& s=State(); std::lock_guard lock(s.mutex);
    if(s.stopped || !s.view.supported || (s.view.busy&&action!=Action::Cancel) || s.request) return false;
    s.request=Request{action,std::move(value)}; return true;
}
bool SubmitNative(Action action, std::string value) {
    auto& s=State(); std::lock_guard lock(s.mutex);
    if(s.stopped || s.request || (action!=Action::Microsoft&&action!=Action::Select))return false;
    s.request=Request{action,std::move(value)};return true;
}
bool Take(Request& request) {
    auto& s=State(); std::lock_guard lock(s.mutex);
    if(s.stopped || !s.request) return false;
    request=std::move(*s.request); s.request.reset(); return true;
}
void Publish(Snapshot view) { auto& s=State(); std::lock_guard lock(s.mutex); if(!s.stopped)s.view=std::move(view); }
void RequestOpen(){auto& s=State();std::lock_guard lock(s.mutex);if(!s.stopped)s.open=true;}
bool ConsumeOpen(){auto& s=State();std::lock_guard lock(s.mutex);const bool open=s.open;s.open=false;return open;}
void Stop() {
    auto& s=State(); std::lock_guard lock(s.mutex); s.stopped=true;
    if(s.request) { std::fill(s.request->value.begin(),s.request->value.end(),'\0'); s.request.reset(); }
    s.view={};s.open=false;
}
}
