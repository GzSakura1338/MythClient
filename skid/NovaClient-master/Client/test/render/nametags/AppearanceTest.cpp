#include <cassert>
#include <cstdio>
#include "minecraft/host/presentation/NameTagsConfig.h"

using nova::config::LoadRows;
using nova::config::SaveRows;
using nova::minecraft::MigrateNameTagsSettings;
using nova::ui::nametags::Appearance;

int main() {
    Appearance edited;
    auto legacy=nova::json::Value::Object();
    legacy["Mode"]="Classic";
    legacy["Scale"]=0.9;
    legacy["Range"]=48;
    legacy["Type"]="Static";
    legacy["Range#2"]=17;
    legacy["Range#3"]=29;
    legacy["Static Color"]=nova::json::Value::Object();
    legacy["Static Color"]["h"]=0;
    legacy["Static Color"]["s"]=0;
    legacy["Static Color"]["b"]=0;
    legacy["Static Color"]["a"]=0.227027;
    legacy["Text Shadow"]=true;

    const auto migrated=MigrateNameTagsSettings(legacy);
    Appearance original,classic;
    LoadRows(*migrated.Find("Default Appearance"),original.ConfigRows());
    LoadRows(*migrated.Find("Classic Appearance"),classic.ConfigRows());
    assert(original.background.getColorAbgr()==0xCC331914u);
    assert(original.text.getColorAbgr()==0xFFFFEAE5u && !original.shadow.get());
    assert((classic.background.getColorAbgr()&0xffffffu)==0);
    assert(classic.shadow.get());
    assert(classic.background.rangeProp().get()==17 && classic.text.rangeProp().get()==29);
    const auto alpha=classic.background.getColorAbgr()>>24;
    assert(alpha>=57 && alpha<=58);
    assert(migrated.Find("Scale")->asNumber(0)==0.9 && migrated.Find("Range")->asNumber(0)==48);

    legacy["Mode"]=0;
    auto oldDefault=MigrateNameTagsSettings(legacy);
    LoadRows(*oldDefault.Find("Default Appearance"),original.ConfigRows());
    LoadRows(*oldDefault.Find("Classic Appearance"),classic.ConfigRows());
    assert((original.background.getColorAbgr()&0xffffffu)==0 && original.shadow.get());
    assert(classic.background.getColorAbgr()==0xCC331914u && !classic.shadow.get());

    original.background.rangeProp().set(33);
    auto saved=nova::json::Value::Object();
    saved["Default Appearance"]=SaveRows(original.ConfigRows());
    saved["Classic Appearance"]=SaveRows(classic.ConfigRows());
    saved["Mode"]="Classic";
    const auto stable=MigrateNameTagsSettings(saved);
    assert(stable.Dump()==saved.Dump());
    Appearance restored;
    LoadRows(*stable.Find("Default Appearance"),restored.ConfigRows());
    assert(restored.background.rangeProp().get()==33 && restored.shadow.get());
    assert(classic.background.rangeProp().get()==25);

    bool classicMode=false;
    auto defaultRows=original.Rows([&] {return !classicMode;});
    auto classicRows=classic.Rows([&] {return classicMode;});
    assert(defaultRows.front().isVisible() && !classicRows.front().isVisible());
    assert(!defaultRows[1].isVisible());
    classicMode=true;
    assert(!defaultRows.front().isVisible() && classicRows.front().isVisible());
    assert(!defaultRows.back().isVisible() && classicRows.back().isVisible());

    const auto empty=MigrateNameTagsSettings(nova::json::Value::Object());
    LoadRows(*empty.Find("Default Appearance"),original.ConfigRows());
    assert(original.background.getColorAbgr()==0xCC331914u && !original.shadow.get());
    legacy["Mode"]=1;
    assert(MigrateNameTagsSettings(legacy).Find("Classic Appearance")->Dump()==migrated.Find("Classic Appearance")->Dump());
    std::puts("PASS: NameTags mode appearance isolation, legacy migration, range keys, UI visibility and config round trip");
}
