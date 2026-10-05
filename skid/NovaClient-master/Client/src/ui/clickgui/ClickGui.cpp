#include "ui/clickgui/ClickGui.h"
#include "ui/clickgui/proxy/ProxyPage.h"
#include "ui/clickgui/browser/BrowserPage.h"
#include "ui/clickgui/accounts/AccountPage.h"
#include "contracts/AccountService.h"
#include "chrome/BrowserService.h"
#include "bind/BrowserCapture.h"
#include "ui/workspace/ForegroundWorkspace.h"

#include <atomic>

#include "core/Instance.h"
#include "bind/BindCapture.h"
#include "bind/BindProperty.h"
#include "third_party/fatalwidgets/imgui/imgui.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/Menu.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/PageLayout.h"

namespace nova::clickgui {
namespace {

std::atomic<bool> open{false};
std::atomic<int> mode{static_cast<int>(Mode::Panel)};
std::atomic<void (*)()> closeHandler{nullptr};
std::atomic<void (*)()> openHandler{nullptr};
nova::menu::panel::PanelClickGui& Panel() {
    return Instance<nova::menu::panel::PanelClickGui>([] { return new nova::menu::panel::PanelClickGui(); });
}

}

bool IsOpen() { return open.load(std::memory_order_acquire); }
bool CapturesInput(){return IsOpen()||ui::workspace::CapturesInput();}
void SetOpen(bool value) { open.store(value, std::memory_order_release); if(!value&&!ui::workspace::Detached()){bind::browserCapture.store(false);chrome::Visible(false);} }
void Toggle() { SetOpen(!IsOpen()); }

Mode GetMode() { return static_cast<Mode>(mode.load(std::memory_order_acquire)); }
void SetMode(Mode value) { mode.store(static_cast<int>(value), std::memory_order_release); }

void Close() {
    SetOpen(false);
    if (const auto handler = closeHandler.load(std::memory_order_acquire)) handler();
}

void SetCloseHandler(void (*handler)()) { closeHandler.store(handler, std::memory_order_release); }
void SetOpenHandler(void (*handler)()) { openHandler.store(handler, std::memory_order_release); }
void OpenPanel(){if(const auto handler=openHandler.load())handler();SetMode(Mode::Panel);SetOpen(true);}
void OpenBrowser(){if(ui::workspace::Detached()){ui::workspace::Restore();return;}OpenPanel();Panel().layout().changePage(menu::panel::PageLayout::BrowserPage);}

void Render() {
    if(!IsOpen())ui::proxy::Hide();
    if(ImGui::GetCurrentContext()&&nova::accounts::ConsumeOpen()) {
        OpenPanel();ui::workspace::RestorePanel();Panel().layout().changePage(menu::panel::PageLayout::AccountsPage);
    }
    if(ImGui::GetCurrentContext())ui::workspace::PrepareFrame();
    if (!IsOpen()) {
        if(ImGui::GetCurrentContext())ui::accounts::Hide();
        if(ImGui::GetCurrentContext())ui::browser::Hide();
        auto& capture = bind::BindCapture::Get();
        if (capture.target() && capture.target()->keyOnly()) capture.Cancel();
        if(ImGui::GetCurrentContext())ui::workspace::Draw(Panel());
        return;
    }
    if (ImGui::GetCurrentContext() == nullptr) return;

    if (GetMode() == Mode::Classic) {
        ui::accounts::Hide();
        ui::browser::Hide();
        ImGui::PushFont(Medium12);
        Fatal::Menu::draw();
        ImGui::PopFont();
        ui::workspace::Draw(Panel());
        return;
    }

    if(!ui::workspace::PanelHidden()){Panel().render();ui::workspace::CapturePanel();}
    ui::workspace::Draw(Panel());
}

}
