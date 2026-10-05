#pragma once
#include <atomic>
namespace nova::module::explosive {
// Main-thread arbitration: anchor publishes before crystal preparation; anchor
// applies its rotation after crystal has compared both offers.
inline double anchorDamage=0;
inline bool anchorPending=false,crystalChosen=false;
inline std::atomic<bool> anchorWorking{false};
inline std::atomic<bool> feetWorking{false};
inline bool anchorConfirming=false,crystalConfirming=false;
inline std::atomic<bool> autoCrystalEnabled{false},autoCrystalWorking{false};
inline std::atomic<bool> anchorAuraEnabled{false};
inline bool autoCrystalConfirming=false;
inline void ResetAnchor(){anchorDamage=0;anchorPending=false;crystalChosen=false;anchorWorking=false;}
inline void OfferAnchor(double damage,bool pending){anchorDamage=damage;anchorPending=pending;}
inline bool ChooseCrystal(double damage,bool pending){
    if(feetWorking&&!pending){crystalChosen=false;return false;}
    crystalChosen=pending||(!anchorPending&&(anchorAuraEnabled?damage>anchorDamage:damage>=anchorDamage));
    return crystalChosen;
}
inline bool ChooseAnchor(){return anchorDamage>0&&!crystalChosen&&(!feetWorking||anchorPending);}
}
