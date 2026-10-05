#include "minecraft/host/render/WorldProjection.h"

#include <cmath>

namespace nova::minecraft::projection {
namespace {

constexpr double kPi = 3.14159265358979323846;

// Minecraft's near plane, which is where a projected point stops being meaningful.
constexpr double kNearPlane = 0.05;

}

double Dot(const rotation::Vec3& a, const rotation::Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

rotation::Vec3 Cross(const rotation::Vec3& a, const rotation::Vec3& b) {
    return rotation::Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

BobTransform MakeBob(const java::CameraView& view, float partialTick) {
    BobTransform bob;
    if (!view.bobView)
        return bob;

    // Straight out of GameRenderer.bobView, same lerp and same constants. The 0.2
    // sits inside the cosine, subtracted from the angle rather than from the walk
    // distance.
    const double delta = static_cast<double>(partialTick);
    const double walked =
        static_cast<double>(view.walkDist) - static_cast<double>(view.walkDistO);
    const double f1 = -(static_cast<double>(view.walkDist) + walked * delta);
    const double f2 = static_cast<double>(view.oBob) +
                      delta * (static_cast<double>(view.bob) - static_cast<double>(view.oBob));

    const double angle = f1 * kPi;
    const double sinA = std::sin(angle);
    const double cosA = std::cos(angle);

    bob.tx = sinA * f2 * 0.5;
    bob.ty = -std::fabs(cosA * f2);

    const double roll = sinA * f2 * 3.0 * (kPi / 180.0);
    const double nudge = std::fabs(std::cos(angle - 0.2) * f2) * 5.0 * (kPi / 180.0);

    bob.cosZ = std::cos(roll);
    bob.sinZ = std::sin(roll);
    bob.cosX = std::cos(nudge);
    bob.sinX = std::sin(nudge);
    bob.active = bob.tx != 0.0 || bob.ty != 0.0 || bob.sinZ != 0.0 || bob.sinX != 0.0;
    return bob;
}

CameraBasis MakeBasis(const java::CameraView& view, float partialTick) {
    CameraBasis basis;
    basis.eye = rotation::Vec3{view.x, view.y, view.z};

    const double yaw = view.yaw * (kPi / 180.0);
    const double pitch = view.pitch * (kPi / 180.0);
    const double cosPitch = std::cos(pitch);
    basis.look = rotation::Vec3{-std::sin(yaw) * cosPitch, -std::sin(pitch), std::cos(yaw) * cosPitch};
    basis.right = rotation::Vec3{-std::cos(yaw), 0.0, -std::sin(yaw)};
    basis.up = Cross(basis.right, basis.look);

    for (int i = 0; i < 16; ++i)
        basis.projection[i] = view.projection[i];
    for (int i = 0; i < 16; ++i)
        basis.modelView[i] = view.modelView[i];
    basis.hasModelView = view.hasModelView;
    basis.windowWidth = view.windowWidth;
    basis.windowHeight = view.windowHeight;
    basis.bob = MakeBob(view, partialTick);
    return basis;
}

bool Project(const CameraBasis& basis, const rotation::Vec3& world, const ImVec2& display,
             ScreenPoint& out) {
    const rotation::Vec3 rel{world.x - basis.eye.x, world.y - basis.eye.y, world.z - basis.eye.z};
    double viewX = Dot(basis.right, rel);
    double viewY = Dot(basis.up, rel);
    double viewZ = -Dot(basis.look, rel);
    if (basis.hasModelView) {
        const float* m = basis.modelView;
        viewX = m[0] * rel.x + m[4] * rel.y + m[8] * rel.z + m[12];
        viewY = m[1] * rel.x + m[5] * rel.y + m[9] * rel.z + m[13];
        viewZ = m[2] * rel.x + m[6] * rel.y + m[10] * rel.z + m[14];
    }

    // The bob is a camera transform, so it lands on the view-space point rather
    // than the world one: rotate about X, then about Z, then translate -- the order
    // the original's one translate and two mulPose calls compose in.
    if (!basis.hasModelView && basis.bob.active) {
        const BobTransform& bob = basis.bob;
        const double y1 = viewY * bob.cosX - viewZ * bob.sinX;
        const double z1 = viewY * bob.sinX + viewZ * bob.cosX;
        const double x2 = viewX * bob.cosZ - y1 * bob.sinZ;
        const double y2 = viewX * bob.sinZ + y1 * bob.cosZ;
        viewX = x2 + bob.tx;
        viewY = y2 + bob.ty;
        viewZ = z1;
    }

    // JOML stores the matrix column-major, so m<col><row> sits at [col * 4 + row].
    const float* m = basis.projection;
    const double clipX = static_cast<double>(m[0]) * viewX + static_cast<double>(m[4]) * viewY +
                         static_cast<double>(m[8]) * viewZ + static_cast<double>(m[12]);
    const double clipY = static_cast<double>(m[1]) * viewX + static_cast<double>(m[5]) * viewY +
                         static_cast<double>(m[9]) * viewZ + static_cast<double>(m[13]);
    const double clipW = static_cast<double>(m[3]) * viewX + static_cast<double>(m[7]) * viewY +
                         static_cast<double>(m[11]) * viewZ + static_cast<double>(m[15]);

    if (clipW < kNearPlane)
        return false;

    const double ndcX = clipX / clipW;
    const double ndcY = clipY / clipW;

    out.x = static_cast<float>((ndcX * 0.5 + 0.5) * static_cast<double>(display.x));
    out.y = static_cast<float>((1.0 - (ndcY * 0.5 + 0.5)) * static_cast<double>(display.y));
    return true;
}

}
