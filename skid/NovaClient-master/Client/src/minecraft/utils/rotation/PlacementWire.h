#pragma once
#include <cmath>
#include <mutex>

namespace nova::rotation {
// Mirrors the outbound order consumed by Grim's handleFlying:
// queued places use the PREVIOUS RotationUpdate, then the new look is published.
// This is independent of modules and of client MotionEvent (which can emit no
// look packet, or can describe a packet that another module cancels).
struct PlacementWireHistory {
    bool hasLook=false,deltaKnown=false,rotated=false,pendingPlace=false,hasPlaced=false;
    float yaw=0,pitch=0,delta=0,placedDelta=0;
    bool Ready()const {
        if(!hasLook||!deltaKnown||pendingPlace)return false;
        if(!rotated)return true;
        // On injection/respawn the server's last placed delta is unknown.
        // Establish it with a small/zero turn before allowing a large one.
        return delta<=2.f||(hasPlaced&&std::fabs(delta-placedDelta)>=.0001f);
    }
    void Place(){pendingPlace=true;}
    // USE_ITEM may place a bucket/lilypad, but may also just eat/use a rocket.
    // Without reproducing server inventory/world checks, forget the old placed
    // delta rather than treating every item use as a confirmed block placement.
    void UnknownUse(){hasPlaced=false;pendingPlace=false;}
    void Move(bool look,float nextYaw=0,float nextPitch=0) {
        if(pendingPlace) {
            if(rotated) {placedDelta=delta;hasPlaced=deltaKnown;rotated=false;}
            pendingPlace=false;
        }
        if(!look)return; // position/status packets do not create a rotation update
        if(!std::isfinite(nextYaw)||!std::isfinite(nextPitch)){*this={};return;}
        deltaKnown=hasLook;
        if(hasLook)delta=std::fabs(nextYaw-yaw); // Java float subtraction, no wrap
        yaw=nextYaw;pitch=nextPitch;hasLook=true;rotated=true;
    }
};
inline std::mutex placementWireMutex;
inline PlacementWireHistory placementWire;
inline PlacementWireHistory ReadPlacementWire(){std::lock_guard lock(placementWireMutex);return placementWire;}
inline void ResetPlacementWire(){std::lock_guard lock(placementWireMutex);placementWire={};}
inline void ObservePlacementPacket(){std::lock_guard lock(placementWireMutex);placementWire.Place();}
inline void ObserveUnclassifiedItemUse(){std::lock_guard lock(placementWireMutex);placementWire.UnknownUse();}
inline void ObservePlacementMovement(bool look,float yaw,float pitch){std::lock_guard lock(placementWireMutex);placementWire.Move(look,yaw,pitch);}
}
