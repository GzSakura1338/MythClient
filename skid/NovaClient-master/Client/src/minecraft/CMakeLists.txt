target_sources(NovaClientBase PRIVATE
    proxy/ProxyHooks.cpp
    proxy/ProxyEdits.cpp
    accounts/AccountManager.cpp
    accounts/AccountPortraits.cpp
    accounts/TokenAuth.cpp
    accounts/MicrosoftAuth.cpp
    accounts/CredentialStore.cpp
    accounts/lunar/AccountHooks.cpp
    accounts/lunar/AccountCompatibility.cpp
    accounts/lunar/LunarAccountAdapter.cpp
    host/MinecraftHost.cpp
    host/input/ImeInput.cpp
    host/presentation/UiCatalog.cpp
    host/presentation/Config.cpp
    host/render/WorldProjection.cpp
    host/render/OverlayHost.cpp
    events/EventBus.cpp
    hooks/Hooks.cpp
    hooks/Edits.cpp
    hooks/Transformer.cpp
    utils/inventory/InventoryUtils.cpp
    utils/rotation/AimUtils.cpp
    utils/rotation/FallingPlayer.cpp
    utils/rotation/MovementFix.cpp
    utils/rotation/Random.cpp
    utils/rotation/RayCastUtil.cpp
    utils/rotation/RotationBridge.cpp
    utils/rotation/RotationManager.cpp
    utils/rotation/RotationUtils.cpp)

add_subdirectory(compat)
add_subdirectory(java)
add_subdirectory(modules)
target_link_libraries(NovaClientBase PRIVATE winhttp bcrypt crypt32 ws2_32 shell32)
