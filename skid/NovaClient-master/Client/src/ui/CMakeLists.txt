# Shared targets deliberately have no src/minecraft/java/api (JNI) root. Minecraft's
# window/frame entry points belong to minecraft/host/render/OverlayHost.cpp.
add_library(NovaRender STATIC
    backend/VulkanOverlay.cpp
    backend/Dx11Overlay.cpp
    backend/blur/Dx11Blur.cpp
)
target_include_directories(NovaRender PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_include_directories(NovaRender PRIVATE ${PROJECT_SOURCE_DIR}/src/third_party/vulkan/include)
target_compile_options(NovaRender PRIVATE /utf-8)
target_link_libraries(NovaRender PUBLIC NovaFatalWidgets d3d11 dxgi d3dcompiler)
target_link_libraries(NovaClientBase PRIVATE NovaRender)

add_library(NovaUI STATIC)
target_include_directories(NovaUI PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_options(NovaUI PRIVATE /utf-8)
target_link_libraries(NovaUI PUBLIC NovaFatalWidgets)
target_link_libraries(NovaUI PUBLIC NovaChromeClient)
target_link_libraries(NovaUI PUBLIC NovaProxy)
target_sources(NovaUI PRIVATE clickgui/browser/BrowserPage.cpp clickgui/browser/BrowserBackdrop.cpp ${PROJECT_SOURCE_DIR}/src/config/BrowserSettings.cpp)
target_link_libraries(NovaRender PUBLIC NovaChromeClient)
target_link_libraries(NovaClientBase PRIVATE NovaUI)

# Explicit source ownership; add a new implementation in its feature group.
target_sources(NovaUI PRIVATE
    # render services (moved to NovaRender in the renderer migration stage)
    backend/blur/Blur.cpp
    backend/blur/BlurDraw.cpp
    backend/blur/OpenGLBlur.cpp

    # clickgui
    workspace/ForegroundWorkspace.cpp
    clickgui/ClickGui.cpp
    clickgui/FeatureTree.cpp
    clickgui/Fonts.cpp
    clickgui/Menu.cpp
    clickgui/accounts/AccountService.cpp
    clickgui/accounts/AccountPage.cpp
    clickgui/proxy/ProxyPage.cpp

    # clickgui/panel
    clickgui/panel/ClientSettings.cpp
    clickgui/panel/Elements.cpp
    clickgui/panel/PopupAnimation.cpp
    clickgui/panel/ReferenceStyle.cpp
    clickgui/panel/PageLayout.cpp
    clickgui/panel/PanelClickGui.cpp
    clickgui/panel/PropertyComponent.cpp
    clickgui/panel/Theme.cpp
    clickgui/panel/TopBar.cpp
    clickgui/panel/Widgets.cpp

    # Shared media HUD controls
    hud/musichud/SourcePicker.cpp
    hud/musichud/MusicHud.cpp
    hud/musichud/CoverTexture.cpp
    hud/lyrics/LyricsOverlay.cpp
    hud/arraylist/ArrayListHud.cpp
    hud/autoblockhud/AutoBlockHud.cpp
    hud/compass/CompassHud.cpp
    hud/bedplates/BedPlateHud.cpp
    hud/itemesp/ItemLabel.cpp
    hud/esp2d/Esp2DView.cpp
    hud/watermark/WatermarkHud.cpp
    hud/targethud/TargetHud.cpp
    hud/targetesp/TargetRing.cpp
    hud/playerlist/PlayerListHud.cpp
    hud/nametags/NameTagView.cpp
    hud/breaker/BreakerHud.cpp
    hud/cswin/CsWin.cpp
    hud/cswin/CsWinAudio.cpp

    # notifications
    notifications/Notifications.cpp
    notifications/NotificationHud.cpp

    # overlay
    overlay/ClientOverlay.cpp
    overlay/HudBackground.cpp
    overlay/WindowPlacement.cpp

    # skin
    skin/SkinTextures.cpp

    # text
    text/ImTextRenderer.cpp
)

foreach(NOVA_CSWIN_TRACK IN ITEMS Inhuman EZ4ENCE)
set(NOVA_CSWIN_MUSIC "${CMAKE_CURRENT_BINARY_DIR}/CsWinMusic${NOVA_CSWIN_TRACK}.cpp")
add_custom_command(OUTPUT "${NOVA_CSWIN_MUSIC}"
    COMMAND ${CMAKE_COMMAND}
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/hud/cswin/assets/${NOVA_CSWIN_TRACK}.wav"
        "-DTRACK=${NOVA_CSWIN_TRACK}"
        "-DOUTPUT=${NOVA_CSWIN_MUSIC}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/hud/cswin/EmbedMusic.cmake"
    DEPENDS "hud/cswin/assets/${NOVA_CSWIN_TRACK}.wav" hud/cswin/EmbedMusic.cmake
    VERBATIM)
target_sources(NovaUI PRIVATE "${NOVA_CSWIN_MUSIC}")
endforeach()
target_link_libraries(NovaUI PUBLIC winmm)
