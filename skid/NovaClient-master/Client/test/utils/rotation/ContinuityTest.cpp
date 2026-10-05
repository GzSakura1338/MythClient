#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RotationUtils.h"

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "Rotation test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)

namespace mock {
float yaw = 0, pitch = 0, previousYaw = 0, previousPitch = 0;
float armYaw = 0, previousArmYaw = 0;
bool freecam = false;
bool sensitivityAvailable = true;
double sensitivity = .5;
int rebases = 0;
}
namespace nova::java {
bool PlayerRotation(float& yaw, float& pitch) { yaw = mock::yaw; pitch = mock::pitch; return true; }
bool PlayerPreviousRotation(float& yaw, float& pitch) { yaw = mock::previousYaw; pitch = mock::previousPitch; return true; }
bool PlayerSensitivity(double& value) { value = mock::sensitivity; return mock::sensitivityAvailable; }
bool FreeCamActive() { return mock::freecam; }
int ClientFps() { return 120; }
int PlayerTickCount() { return 100; }
void PickCursor(float) {}
bool SetPlayerRotation(float yaw, float pitch) { mock::yaw = yaw; mock::pitch = pitch; return true; }
bool SetPlayerCameraRotation(float yaw, float pitch, double amount) {
    const float rebase = rotation::CameraYawRebase({mock::yaw,mock::pitch},{yaw,pitch});
    mock::armYaw += rebase;mock::previousArmYaw += rebase;
    const auto before = rotation::CameraPreviousAfterTurn({mock::previousYaw, mock::previousPitch},
        {mock::yaw, mock::pitch}, {yaw, pitch}, amount);
    mock::previousYaw = before.yaw; mock::previousPitch = before.pitch;
    ++mock::rebases;
    return SetPlayerRotation(yaw, pitch);
}
EntityHandle::~EntityHandle() = default;
bool EntityHandle::IsSame(const EntityHandle& other) const { return raw() == other.raw(); }
EntityHandle PlayerEntity() { return {}; }
}
namespace nova::rotation { int randomCalls=0; double Random() { ++randomCalls;return .5; } }

namespace {
using namespace nova::rotation;
void Near(float a, float b, float tolerance = .001f) { CHECK(std::abs(a - b) < tolerance); }
void Reset(float yaw = 0, float pitch = 0) {
    OnRespawn(); State() = {};
    mock::yaw = mock::previousYaw = yaw;
    mock::pitch = mock::previousPitch = pitch;
    mock::armYaw = yaw - 4;mock::previousArmYaw = yaw - 7;
    mock::freecam = false; mock::rebases = 0;
    mock::sensitivity = .5; mock::sensitivityAvailable = true;
    UpdateGlobalYaw();
}
struct Wire {
    float yaw, lastDelta = 0;
    explicit Wire(float start) : yaw(start) {}
    void Push(float next) {
        const float delta = next - yaw;
        // Exact non-exempt AimModulo360 predicate from the local Grim source.
        CHECK(!(next < 360 && next > -360 && std::abs(delta) > 320 && std::abs(lastDelta) < 30));
        CHECK(std::isfinite(next));
        CHECK(std::abs(delta) <= 180.01f);
        lastDelta = delta; yaw = next;
    }
    void Tick() { float next = mock::yaw, pitch = mock::pitch; OnMotionPre(next, pitch); Push(next); }
};
}

int main() {
    using namespace nova::rotation;
    // Caller noise must be the only random source throughout the real manager.
    Reset(179);
    State().offset={12,8};State().randomAngle=45;
    randomCalls=0;
    SetRotations({-150,15},12,[](const Rotation&){return false;},RotationPriority::Medium,false);
    CHECK(randomCalls==0&&!State().randomize&&State().moveInputFix);
    Near(State().offset.yaw,0);Near(State().offset.pitch,0);
    const auto expected=Smooth({179,0},{-150,15},12,{179,0},.5,120,true,false);
    Near(State().rotations.yaw,expected.yaw);Near(State().rotations.pitch,expected.pitch);
    for(int fps:{20,60,144,500}) {
        const auto value=Smooth({179,0},{-150,15},12,{179,0},.5,fps,true,false);
        Near(value.yaw,expected.yaw);Near(value.pitch,expected.pitch);
    }
    CHECK(randomCalls==0);
    State().smoothed=false;
    SetRotations({-150,15},12);
    CHECK(State().randomize&&randomCalls>0); // Next normal owner keeps legacy behavior.
    std::puts("PASS: caller noise disables all shared randomness, clears offsets, retains sensitivity and restores normal request policy");
    for (float shift : {-720.f,-360.f,360.f,720.f}) {
        const float current=179, previous=177, arm=175, oldArm=172;
        Near(CameraYawRebase({current,10},{current+shift,10}),shift);
        const auto prior=CameraPreviousAfterTurn({previous,10},{current,10},{current+shift,10},100);
        for (float partial : {0.f,.25f,.5f,.75f,1.f}) {
            const float before=(previous+(current-previous)*partial)-(oldArm+(arm-oldArm)*partial);
            const float after=(prior.yaw+(current+shift-prior.yaw)*partial)-
                (oldArm+shift+(arm-oldArm)*partial);
            Near(after,before);
        }
    }
    Near(CameraYawRebase({10,20},{30,20}),0);
    Near(CameraYawRebase({10,20},{370,30}),0);
    Near(CameraYawRebase({10,20},{371,20}),0);
    std::puts("PASS: whole-turn camera rebase preserves hand sway at every interpolation point; normal turns retain vanilla lag");
    // Run the actual manager's release/rebase, checking both the wire predicate
    // and the hand endpoints. Fixing hand sway must not restore normalized yaw.
    for(float sign:{-1.f,1.f}) {
        Reset(sign*179);
        Wire wire(mock::yaw);
        CHECK(SetDirectRotations({sign*-179,30}));wire.Tick();
        // The live camera has a different whole-turn representation from the
        // silent 181/-181 angle. This is the release path that needs rebasing.
        mock::yaw=sign*-179;mock::previousYaw=sign*-181;
        mock::armYaw=mock::yaw-4;mock::previousArmYaw=mock::previousYaw-7;
        const float armDelta=mock::yaw-mock::armYaw;
        const float oldArmDelta=mock::previousYaw-mock::previousArmYaw;
        ReleaseDirectRotations();
        Near(mock::yaw-mock::armYaw,armDelta);
        Near(mock::previousYaw-mock::previousArmYaw,oldArmDelta);
        wire.Tick();
        CHECK(mock::rebases==1);
    }
    std::puts("PASS: shared release keeps AimModulo360 continuity and both hand endpoints together");
    for (float sign : {-1.f, 1.f}) {
        Reset(sign * 179);
        Wire wire(mock::yaw);
        CHECK(SetDirectRotations({sign * -179, 30}));
        Near(State().rotations.yaw, ApplySensitivityPatch({sign*181,30},{sign*179,0},.5,false).yaw);
        wire.Tick(); wire.Tick();
        ReleaseDirectRotations();
        wire.Tick();
        Near(std::remainder(mock::yaw - sign * 179, 360.f), 0);
        Near(mock::yaw - mock::previousYaw, 0);
    }
    std::puts("PASS: direct requests cross +/-180 without a full-turn snap");

    for (int mode = 0; mode < 3; ++mode) {
        for (float camera : {-719.f, -359.f, 1.f, 361.f, 1081.f}) {
            Reset(camera); Wire wire(camera);
            // Build a continuous complete revolution while the real camera stays still.
            for (int step = 1; step <= 36; ++step) {
                const Rotation target{camera + step * 10, 20};
                if (mode == 0) CHECK(SetLinearRotations(target, 400, 400));
                if (mode == 1) CHECK(SetDirectRotations(target));
                if (mode == 2) SetRotations(target, 30);
                wire.Tick();
            }
            for (int tick = 0; tick < 2; ++tick) wire.Tick();
            if (mode == 0) ReturnLinearRotations(720);
            if (mode == 1) ReleaseDirectRotations();
            for (int tick = 0; tick < 100 && State().active; ++tick) {
                UpdateGlobalYaw(); wire.Tick();
            }
            CHECK(!State().active);
            wire.Tick(); wire.Tick();
            Near(std::remainder(mock::yaw - camera, 360.f), 0);
            Near(mock::pitch, 0);
            Near(mock::yaw - mock::previousYaw, 0);
            // Reacquiring immediately must use the same continuous angle branch.
            CHECK(SetLinearRotations({camera + 15, 0}, 60, 60)); wire.Tick();
        }
    }
    std::puts("PASS: full-circle linear/ordinary/direct handoff, signed multi-turn camera and reacquire");

    for (float sign : {-1.f, 1.f}) {
        Reset(sign * 1);
        Wire wire(mock::yaw); int owner = 0;
        for (int step = 1; step <= 36; ++step) {
            CHECK(SetExclusiveRotations(&owner, {sign * (1 + step * 10), 20})); wire.Tick();
        }
        ReleaseExclusiveRotations(&owner);
        for (int tick = 0; tick < 20 && State().active; ++tick) { UpdateGlobalYaw(); wire.Tick(); }
        CHECK(!State().active && !OwnsExclusiveRotations(&owner)); wire.Tick();
    }
    std::puts("PASS: exclusive AutoMLG-style rotation and linear return preserve yaw continuity");

    Reset(0);int chargeOwner=0,otherOwner=0;
    CHECK(!SetExclusiveMovementYaw(&chargeOwner,0));
    CHECK(SetExclusiveRotations(&chargeOwner,{90,-85}));
    CHECK(SetExclusiveMovementYaw(&chargeOwner,0));
    CHECK(!SetExclusiveMovementYaw(&otherOwner,40));
    CHECK(!SetExclusiveMovementYaw(&chargeOwner,NAN));
    mock::yaw=90;MoveInput input{1,0};OnMoveInput(input);
    Near(std::remainder(Direction(input.forward,input.strafe,90),360.f),0);
    ReleaseExclusiveRotations(&otherOwner);CHECK(OwnsExclusiveRotations(&chargeOwner));
    ReleaseExclusiveRotations(&chargeOwner);
    CHECK(!SetExclusiveMovementYaw(&chargeOwner,0));
    CHECK(SetExclusiveRotations(&otherOwner,{90,-85}));
    input={1,0};OnMoveInput(input);Near(input.forward,1);Near(input.strafe,0);
    CHECK(SetExclusiveMovementYaw(&otherOwner,0));OnRespawn();
    CHECK(SetExclusiveRotations(&chargeOwner,{90,-85}));
    input={1,0};OnMoveInput(input);Near(input.forward,1);Near(input.strafe,0);
    std::puts("PASS: visible AutoCharge camera preserves movement heading; leases reject foreign writers and reset on release/respawn");

    Reset(5); CHECK(SetLinearRotations({95, 0}, 180, 180));
    State().lastRotations = {365, 0};
    mock::previousYaw = 4.5f;
    ReleaseLinearRotations();
    Near(mock::yaw, 365); Near(mock::previousYaw, 364.5f);
    Near(mock::pitch, 0); CHECK(mock::rebases == 1);
    // The correction must preserve the whole interpolation segment, not turn the camera.
    for (float partial : {0.f, .25f, .5f, 1.f})
        Near(std::remainder(mock::previousYaw + (mock::yaw - mock::previousYaw) * partial - (4.5f + .5f * partial), 360.f), 0);

    Reset(179); const auto instant = Smooth({179, 0}, {-179, 0}, 0, {179, 0}, .5, 120, true);
    Near(instant.yaw, 181);
    SetRotations({NAN, 0}, 20); CHECK(!State().active);
    SetRotations({0, INFINITY}, 20); CHECK(!State().active);
    SetRotations({0, 0}, -1); CHECK(!State().active);
    SetRotations({0, 0}, INFINITY); CHECK(!State().active);
    CHECK(SetDirectRotations({0, 0}));
    State().rotations.yaw = INFINITY;
    float yaw = mock::yaw, pitch = mock::pitch; OnMotionPre(yaw, pitch);
    CHECK(std::isfinite(yaw) && std::isfinite(pitch));

    Reset(5); CHECK(SetDirectRotations({95, 20}));
    State().lastRotations = {365, 20}; mock::yaw = mock::previousYaw = -40;
    OnRespawn(); CHECK(!State().active && !State().hasRotations);
    Near(mock::yaw, -40); CHECK(mock::rebases == 0);
    mock::freecam = true;
    CHECK(!SetDirectRotations({45, 20}));
    CHECK(!SetLinearRotations({45, 20}, 60, 60));
    SetRotations({45, 20}, 60); CHECK(!State().active);
    std::puts("PASS: interpolation, immediate release, instant turn, invalid input, respawn and FreeCam isolation");

    // Reproduce MLG use -> release -> another module -> movement, including
    // multiple full yaw turns. Compare exact floats as BadPacketsJ does.
    for(float base : {-1080.f,-360.f,0.f,360.f,1080.f}) {
        Reset(base); BeginInteractionTick(); int owner=0;
        CHECK(SetExclusiveRotations(&owner,{base+90,80}));
        CHECK(SubmitItemUse()); const auto use=CommitInteractionRotation();
        ReleaseExclusiveRotations(&owner);
        SetDirectRotations({base-50,-30}); UpdateGlobalYaw();
        CHECK(!SubmitItemUse()); // no second item/prediction this tick
        float y=0,p=0; OnItemRayTrace(y,p); CHECK(y==use.yaw && p==use.pitch);
        OnMotionPre(y,p); PublishInteractionMotion(y,p);
        CHECK(y==use.yaw && p==use.pitch);
        CHECK(AutomatedItemUseSubmitted());
        BeginInteractionTick(); CHECK(CanSubmitItemUse());
        OnMotionPre(y,p); PublishInteractionMotion(y,p);
        CHECK(!SubmitItemUse()); // defer late automated use to next tick
        mock::yaw+=15; const auto manual=CommitInteractionRotation();
        CHECK(manual.yaw==y && manual.pitch==p); // native use after movement
        BeginInteractionTick(); const auto first=CommitInteractionRotation();
        mock::yaw+=45; CHECK(CommitInteractionRotation().yaw==first.yaw);
        CHECK(!SubmitItemUse()); // vanilla use also reserves the angle
        OnMotionPre(y,p); PublishInteractionMotion(y,p);
        CHECK(y==first.yaw && p==first.pitch);
        OnRespawn(); CHECK(CanSubmitItemUse());
    }
    std::puts("PASS: exact modern item/motion rotation, competing uses, return, manual use and tick boundaries");
    Reset(0);BeginInteractionTick();int mace=0,other=0;
    CHECK(!BeginAttackSubmission(&mace));
    CHECK(AcquireAttack(&mace));CHECK(!AcquireAttack(&other));
    CHECK(!CanSubmitItemUse());CHECK(!CanCallNativeAttack());
    CHECK(SetExclusiveRotations(&mace,{120,85}));
    const auto maceAngle=GetRotation();
    CHECK(BeginAttackSubmission(&mace));CHECK(EnterNativeAttack());
    CHECK(!EnterNativeAttack());CHECK(EndAttackSubmission());
    CHECK(!BeginAttackSubmission(&mace));
    ReleaseAttack(&mace);ReleaseExclusiveRotations(&mace);
    CHECK(!CanSubmitItemUse());CHECK(!CanCallNativeAttack());
    float attackYaw=0,attackPitch=0;OnMotionPre(attackYaw,attackPitch);
    PublishInteractionMotion(attackYaw,attackPitch);CHECK(attackYaw==maceAngle.yaw&&attackPitch==maceAngle.pitch);
    BeginInteractionTick();CHECK(AcquireAttack(&mace));
    PublishInteractionMotion(attackYaw,attackPitch);CHECK(!BeginAttackSubmission(&mace));
    ReleaseAttack(&mace);BeginInteractionTick();CHECK(CanSubmitItemUse());
    CHECK(SubmitItemUse());CHECK(!AcquireAttack(&mace));OnRespawn();
    std::puts("PASS: attack ownership, native single entry, item conflict, locked rotation and no post-motion attack");
    Reset(0);BeginInteractionTick();int crystal=0;
    CHECK(EnterNativeAttack()); // A manual/other module attack already happened.
    CHECK(!SubmitItemUse(&crystal));CHECK(!AttackReserved());
    BeginInteractionTick();CHECK(SetExclusiveRotations(&crystal,{60,40}));
    const auto crystalAngle=GetRotation();
    CHECK(SubmitItemUse(&crystal));CHECK(AutomatedItemUseSubmitted());
    CHECK(!EnterNativeAttack());CHECK(!AcquireAttack(&mace));CHECK(!SubmitItemUse(&crystal));
    OnMotionPre(attackYaw,attackPitch);PublishInteractionMotion(attackYaw,attackPitch);
    CHECK(attackYaw==crystalAngle.yaw&&attackPitch==crystalAngle.pitch);
    BeginInteractionTick();ReleaseAttack(&crystal);ReleaseExclusiveRotations(&crystal);
    CHECK(EnterNativeAttack());OnRespawn();
    std::puts("PASS: exclusive crystal use refuses prior attacks, blocks later attacks and releases next tick");

    // Validate outgoing increments, not just the requested target or helper.
    for(double sensitivity:{0.,.17,.5,1.}) {
        const float factor=static_cast<float>(sensitivity*.6f+.2f);
        const double unit=factor*factor*factor*8.f*.15;
        for(int mode=0;mode<4;++mode) {
            Reset(179.03f,7.07f);mock::sensitivity=sensitivity;int owner=0;
            Rotation previous{mock::yaw,mock::pitch};
            for(int tick=0;tick<40;++tick) {
                BeginInteractionTick();
                const Rotation target{179.03f+tick*7.137f,7.07f+std::sin(tick*.3f)*12.f};
                if(mode==0)SetRotations(target,13.33,nullptr,RotationPriority::Medium,false);
                if(mode==1)CHECK(SetLinearRotations(target,125,67));
                if(mode==2)CHECK(SetDirectRotations(target));
                if(mode==3)CHECK(SetExclusiveRotations(&owner,target));
                float yaw=mock::yaw,pitch=mock::pitch;OnMotionPre(yaw,pitch);PublishInteractionMotion(yaw,pitch);
                for(double delta:{double(yaw-previous.yaw),double(pitch-previous.pitch)}) {
                    CHECK(std::fabs(delta-std::round(delta/unit)*unit)<.0001);
                }
                CHECK(std::fabs(yaw-previous.yaw)<180.1f);
                previous={yaw,pitch};
                // The real camera has a different grid origin from silent output.
                mock::previousYaw=mock::yaw;mock::yaw+=.031f;
                mock::previousPitch=mock::pitch;mock::pitch+=.017f;
            }
        }
    }
    std::puts("PASS: ordinary, linear, direct and exclusive outgoing angles share one sensitivity grid across camera motion and +/-180");

    Reset();BeginInteractionTick();
    for(int tick=0;tick<100;++tick) {
        CHECK(SetLinearRotations({10.02f,10},1,0));
        float yaw=0,pitch=0;OnMotionPre(yaw,pitch);Near(pitch,0);
    }
    Near(State().rotations.yaw,5,.08f);
    for(int tick=0;tick<150;++tick) {
        CHECK(SetLinearRotations({10.02f,10},1,0));
        float yaw=0,pitch=0;OnMotionPre(yaw,pitch);Near(pitch,0);
    }
    const auto settled=State().rotations;
    for(int tick=0;tick<20;++tick) {
        CHECK(SetLinearRotations({10.02f,10},1,0));
        CHECK(State().rotations.yaw==settled.yaw);
        float yaw=0,pitch=0;OnMotionPre(yaw,pitch);
    }
    ReturnLinearRotations(125);
    for(int tick=0;tick<30&&State().active;++tick) {
        UpdateGlobalYaw();float yaw=0,pitch=0;OnMotionPre(yaw,pitch);
    }
    CHECK(!State().active);Near(mock::yaw,0);Near(mock::pitch,0);
    std::puts("PASS: sub-mouse-step speeds accumulate, disabled axes hold, aligned targets do not oscillate and quantized returns terminate");

    Reset();BeginInteractionTick();int unavailableOwner=0;
    mock::sensitivityAvailable=false;
    CHECK(!SetDirectRotations({17.13f,5}));
    CHECK(!SetExclusiveRotations(&unavailableOwner,{17.13f,5}));
    SetRotations({17.13f,5},10,nullptr,RotationPriority::Medium,false);
    Near(State().rotations.yaw,0);Near(State().rotations.pitch,0);
    mock::sensitivityAvailable=true;State().smoothed=false;
    SetRotations({17.13f,5},10,nullptr,RotationPriority::Medium,false);
    CHECK(State().rotations.yaw>0);
    std::puts("PASS: missing sensitivity never emits raw automated angles and resumes after JNI recovery");
}
