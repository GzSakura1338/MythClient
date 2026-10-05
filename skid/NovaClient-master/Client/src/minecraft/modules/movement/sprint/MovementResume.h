#pragma once

namespace nova::module::sprint {
enum class Screen { None, Container, Other };
struct MovementResume {
    bool pending = false;
    bool worldPending = false;
    void WorldChanged() { pending = false; worldPending = true; }
    template<class Restore>
    void Tick(bool allowed, Screen screen, Restore&& restore) {
        if (!allowed) { pending = worldPending = false; return; }
        // Joining may publish the new player before the loading screen closes.
        // Keep the one-shot recovery armed until gameplay actually resumes.
        if (screen == Screen::Other) { pending = false; return; }
        if (screen == Screen::Container) { pending = true; return; }
        if ((pending || worldPending) && restore()) pending = worldPending = false;
    }
};
}
