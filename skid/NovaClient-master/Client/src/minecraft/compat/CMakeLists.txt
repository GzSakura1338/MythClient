# Explicit source ownership; add a new implementation in its feature group.
target_sources(NovaClientBase PRIVATE
    # Infrastructure
    Profile.cpp
    easecation/EaseCation.cpp
    neoforge12110/NeoForge12110.cpp
    lunar12111/Mapping.cpp
    lunar12111/LunarEdits.cpp
    lunar12111/Lunar12111.cpp

    # lunar189
    lunar189/LunarRuntime.cpp

    # minecraft263
    minecraft263/Minecraft263.cpp
    minecraft263/Minecraft263Access.cpp
    minecraft263/Minecraft263Actions.cpp
    minecraft263/Minecraft263Entity.cpp
    minecraft263/Minecraft263Hooks.cpp
    minecraft263/Minecraft263Inventory.cpp
    minecraft263/Minecraft263Skin.cpp
)
