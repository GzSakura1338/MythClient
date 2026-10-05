#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_network.h"
#include "third_party/fatalwidgets/imgui/imgui.h"
#include "ui/overlay/OverlaySpin.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/hud/world/ProjectedBox.h"

#include <windows.h>

// ChestESP -- a port of Naven's module of the same name, drawn with ImGui.
//
// Every loaded chest gets a translucent box around it, green while it is shut and
// red once the server has said somebody opened it. The opened flag comes from
// ClientboundBlockEventPacket, the same packet the original listens for.
//
// The original draws its boxes with the game's own buffer builder, inside the
// world render pass, where the camera matrices are already on the stack. The
// overlay runs in the swap hook using a value-only copy of that frame's world
// camera and matrices. Hand/GUI transforms cannot overwrite this snapshot.
// ImGui fills the same six faces after projecting their world-space corners.
//
// One consequence of projecting by hand: a face that crosses the near plane cannot
// be projected at all, so a box with any corner that close to the camera is
// dropped for the frame rather than drawn wrongly. Standing inside a chest makes
// it disappear; standing next to one does not.

namespace nova::module {
namespace {

namespace event = nova::event;

using minecraft::projection::CameraBasis;
using minecraft::projection::MakeBasis;
using minecraft::projection::Project;
using ui::ScopedSpin;
using minecraft::projection::ScreenPoint;
using ui::SpinLock;
using rotation::Vec3;

// The original's two colours, as RGBA bytes at its 0.25 alpha.
constexpr ImU32 kChestColor = IM_COL32(0, 255, 0, 64);
constexpr ImU32 kOpenedChestColor = IM_COL32(255, 0, 0, 64);

class ChestEspModule final : public Module {
public:
    ChestEspModule() : Module("ChestESP", Category::Render, "General", "Overlay") {
        enableOverlay();

        motion_ = event::EventBus::Get().Subscribe<event::MotionEvent>(
            [this](event::MotionEvent& e) { OnMotion(e); });
        packet_ = event::EventBus::Get().Subscribe<event::PacketEvent>(
            [this](event::PacketEvent& e) { OnPacket(e); });
        respawn_ = event::EventBus::Get().Subscribe<event::RespawnEvent>(
            [this](event::RespawnEvent& e) { (void)e; ClearOpened(); });
    }

private:
    void onEnable() override { java::SetIncomingPacketsWanted(true); }

    void onDisable() override {
        java::SetIncomingPacketsWanted(false);
        ClearOpened();
        ScopedSpin lock(boxesMutex_);
        boxes_.clear();
    }

    void ClearOpened() {
        ScopedSpin lock(openedMutex_);
        opened_.clear();
    }

    // Runs on the game thread, once a tick, and rebuilds the whole list -- the
    // original does the same rather than tracking additions and removals.
    void OnMotion(event::MotionEvent& e) {
        if (!isEnabled() || e.type != event::EventType::Pre)
            return;

        std::vector<java::ChestBox> fresh;
        if (!java::LoadedChestBoxes(fresh))
            fresh.clear();

        ScopedSpin lock(boxesMutex_);
        boxes_ = std::move(fresh);
    }

    // Runs on the netty I/O thread, which is why the list is behind a mutex and
    // holds plain coordinates rather than a Java object.
    void OnPacket(event::PacketEvent& e) {
        if (!isEnabled() || e.type != event::EventType::Incoming || e.packet == nullptr)
            return;

        int x = 0, y = 0, z = 0;
        if (!java::ReadChestOpenEvent(e.packet, x, y, z))
            return;

        ScopedSpin lock(openedMutex_);
        for (const java::ChestBox& known : opened_) {
            if (known.x == x && known.y == y && known.z == z)
                return;
        }
        java::ChestBox entry;
        entry.x = x;
        entry.y = y;
        entry.z = z;
        opened_.push_back(entry);
    }

    void onRender() override {
        if (ImGui::GetCurrentContext() == nullptr)
            return;

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        if (display.x <= 1.0f || display.y <= 1.0f)
            return;

        java::CameraView view;
        const float partial = java::FrameTime();
        if (!java::ReadCameraView(partial, view))
            return;

        std::vector<java::ChestBox> boxes;
        {
            ScopedSpin lock(boxesMutex_);
            boxes = boxes_;
        }
        if (boxes.empty())
            return;

        std::vector<java::ChestBox> opened;
        {
            ScopedSpin lock(openedMutex_);
            opened = opened_;
        }

        const CameraBasis basis = MakeBasis(view, partial);
        ImDrawList* draws = ImGui::GetBackgroundDrawList();

        // One line, on the first frame that has something to draw, recording every
        // input the projection depends on. If the boxes still sit wrong, this says
        // which of them the frame disagreed with.
        static bool reported = false;
        if (!reported) {
            reported = true;
            LogTo("Base: chest view eye=(%.3f,%.3f,%.3f) yaw=%.3f pitch=%.3f fov=%.2f "
                  "bob=%d walk=%.4f bobf=%.4f tx=%.4f ty=%.4f window=%dx%d display=(%.0f,%.0f) proj=[%.4f %.4f %.4f %.4f]",
                  view.x, view.y, view.z, static_cast<double>(view.yaw),
                  static_cast<double>(view.pitch), view.fov, view.bobView ? 1 : 0,
                  view.walkDist, view.bob, basis.bob.tx, basis.bob.ty,
                  view.windowWidth, view.windowHeight, static_cast<double>(display.x),
                  static_cast<double>(display.y), static_cast<double>(view.projection[0]),
                  static_cast<double>(view.projection[5]),
                  static_cast<double>(view.projection[10]),
                  static_cast<double>(view.projection[11]));
        }

        for (const java::ChestBox& box : boxes) {
            // The original keys the opened test on the corner the box starts at.
            const int keyX = static_cast<int>(std::floor(box.minX));
            const int keyY = static_cast<int>(std::floor(box.minY));
            const int keyZ = static_cast<int>(std::floor(box.minZ));

            bool isOpened = false;
            for (const java::ChestBox& seen : opened) {
                if (seen.x == keyX && seen.y == keyY && seen.z == keyZ) {
                    isOpened = true;
                    break;
                }
            }

            DrawBox(draws, basis, display, box, isOpened ? kOpenedChestColor : kChestColor);
        }
    }

    // The six faces the original submits, in its order, each as a filled quad. A
    // box touching the near plane is skipped whole rather than clipped.
    static void DrawBox(ImDrawList* draws, const CameraBasis& basis, const ImVec2& display,
                        const java::ChestBox& box, ImU32 color) {
        const rotation::Vec3 corners[8] = {
            {box.minX, box.minY, box.minZ}, {box.maxX, box.minY, box.minZ},
            {box.minX, box.maxY, box.minZ}, {box.maxX, box.maxY, box.minZ},
            {box.minX, box.minY, box.maxZ}, {box.maxX, box.minY, box.maxZ},
            {box.minX, box.maxY, box.maxZ}, {box.maxX, box.maxY, box.maxZ},
        };

        ui::ProjectedBox projected;
        for (int i = 0; i < 8; ++i) {
            ScreenPoint point;
            if (!Project(basis, corners[i], display, point))
                return;
            projected[i] = {point.x, point.y};
        }
        ui::DrawFilledBox(draws, projected, color);
    }

    event::Subscription motion_;
    event::Subscription packet_;
    event::Subscription respawn_;

    SpinLock boxesMutex_;
    std::vector<java::ChestBox> boxes_;

    SpinLock openedMutex_;
    std::vector<java::ChestBox> opened_;
};

}

std::unique_ptr<Module> MakeChestEspModule() {
    return std::make_unique<ChestEspModule>();
}

}
