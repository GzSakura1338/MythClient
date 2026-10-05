cmake_minimum_required(VERSION 3.24)
project(NovaClientBase LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The reflective entry executes before imports/relocations and cannot call a
# security-cookie helper. RuntimeImports uses a bounded stack path buffer.
if(MSVC)
    set_source_files_properties(src/core/ReflectiveLoader.c PROPERTIES COMPILE_OPTIONS /GS-)
endif()

add_library(NovaFatalWidgets STATIC
    src/third_party/fatalwidgets/imgui/imgui.cpp
    src/third_party/fatalwidgets/imgui/imgui_draw.cpp
    src/third_party/fatalwidgets/imgui/imgui_tables.cpp
    src/third_party/fatalwidgets/imgui/imgui_widgets.cpp
    src/third_party/fatalwidgets/imgui/imgui_impl_win32.cpp
    src/third_party/fatalwidgets/imgui/imgui_impl_opengl3.cpp
    src/third_party/fatalwidgets/imgui/imgui_impl_dx11.cpp
    src/third_party/fatalwidgets/imgui/imgui_impl_vulkan.cpp
    src/third_party/fatalwidgets/src/FatalWidgets.cpp)
target_include_directories(NovaFatalWidgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/fatalwidgets/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/fatalwidgets/imgui)
target_include_directories(NovaFatalWidgets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/vulkan/include)
set_source_files_properties(src/third_party/fatalwidgets/imgui/imgui_impl_vulkan.cpp PROPERTIES
    COMPILE_DEFINITIONS IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

add_library(NovaClientBase SHARED
    src/core/ReflectiveLoader.c
    src/core/Loader.cpp
    src/core/RuntimeServices.cpp
    src/core/SelfUnload.cpp
    src/core/Hosts.cpp
    src/config/Config.cpp
    src/config/PropertyCodec.cpp
    src/config/ConfigManager.cpp
    src/bind/BindCapture.cpp
    src/bind/BindProperty.cpp
    src/bind/BindableManager.cpp
    src/bind/InputManager.cpp
    src/bind/KeyName.cpp
    src/utils/Color.cpp
    src/utils/Json.cpp
    src/utils/Keyboard.cpp)

# Subsystems own their source lists; module files are grouped by feature.
add_subdirectory(src/minecraft)
add_subdirectory(src/proxy)
add_subdirectory(src/cs2)
target_sources(NovaClientBase PRIVATE src/core/modules/Module.cpp)
add_subdirectory(src/chrome)
add_subdirectory(src/ui)
add_subdirectory(src/smtc)

option(NOVA_UI_PREVIEW "Build the native UI/HUD preview without Minecraft or JNI" ON)
if(NOVA_UI_PREVIEW)
    add_subdirectory(test/ui/preview)
endif()

option(NOVA_CS2_SMOKE "Build the standalone DX11/CS2 host integration test" ON)
if(NOVA_CS2_SMOKE)
    add_subdirectory(test/cs2)
endif()

# Headers appear alongside implementations in Visual Studio. Only headers
# are globbed; compilation units remain explicitly listed above/in each subsystem.
# Skip downloaded trees before traversing them. Filtering a recursive src/*
# glob afterward still walks the entire Chromium checkout on every build.
file(GLOB_RECURSE NOVA_CLIENT_HEADERS CONFIGURE_DEPENDS include/*.h)
file(GLOB NOVA_HEADER_ROOTS CONFIGURE_DEPENDS src/*)
foreach(NOVA_HEADER_ROOT IN LISTS NOVA_HEADER_ROOTS)
    if(NOVA_HEADER_ROOT MATCHES "/src/third_party$")
        continue()
    elseif(NOVA_HEADER_ROOT MATCHES "/src/chrome$")
        file(GLOB NOVA_CHROME_HEADER_ROOTS CONFIGURE_DEPENDS src/chrome/*)
        foreach(NOVA_CHROME_HEADER_ROOT IN LISTS NOVA_CHROME_HEADER_ROOTS)
            if(NOVA_CHROME_HEADER_ROOT MATCHES "/vendor$")
                continue()
            endif()
            if(IS_DIRECTORY "${NOVA_CHROME_HEADER_ROOT}")
                file(GLOB_RECURSE NOVA_OWNED_HEADERS CONFIGURE_DEPENDS "${NOVA_CHROME_HEADER_ROOT}/*.h" "${NOVA_CHROME_HEADER_ROOT}/*.inc")
                list(APPEND NOVA_CLIENT_HEADERS ${NOVA_OWNED_HEADERS})
            elseif(NOVA_CHROME_HEADER_ROOT MATCHES "\\.(h|inc)$")
                list(APPEND NOVA_CLIENT_HEADERS "${NOVA_CHROME_HEADER_ROOT}")
            endif()
        endforeach()
    elseif(IS_DIRECTORY "${NOVA_HEADER_ROOT}")
        file(GLOB_RECURSE NOVA_OWNED_HEADERS CONFIGURE_DEPENDS "${NOVA_HEADER_ROOT}/*.h" "${NOVA_HEADER_ROOT}/*.inc")
        list(APPEND NOVA_CLIENT_HEADERS ${NOVA_OWNED_HEADERS})
    elseif(NOVA_HEADER_ROOT MATCHES "\\.(h|inc)$")
        list(APPEND NOVA_CLIENT_HEADERS "${NOVA_HEADER_ROOT}")
    endif()
endforeach()
target_sources(NovaClientBase PRIVATE ${NOVA_CLIENT_HEADERS})
get_target_property(NOVA_CLIENT_SOURCES NovaClientBase SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${NOVA_CLIENT_SOURCES})

target_sources(NovaClientBase PRIVATE
    src/third_party/minhook/src/buffer.c src/third_party/minhook/src/hde/hde64.c
    src/third_party/minhook/src/hook.c src/third_party/minhook/src/trampoline.c)

# src/ for all game and shared APIs, include/ for reflective loader headers.
# Every quoted include is written from one of these three
# roots. Moving a file requires updating every reference to its rooted path.
target_include_directories(NovaClientBase PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/vulkan/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/minhook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/minhook/src)

target_compile_definitions(NovaClientBase PRIVATE WIN_X64 REFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR)
target_compile_definitions(NovaClientBase PRIVATE NOVA_PROXY_RUNTIME)

# The sources are UTF-8. Without this MSVC reads them in the system code page,
# which mangles any non-ASCII byte in a comment or a literal.
target_compile_options(NovaClientBase PRIVATE /utf-8)
target_link_libraries(NovaClientBase PRIVATE user32 comctl32 opengl32 shell32 imm32 NovaFatalWidgets)
set_target_properties(NovaClientBase PROPERTIES OUTPUT_NAME "NovaClientBase")
# Keep native address-to-function evidence beside every reflective build.
# Manually mapped images are absent from Windows minidump module lists.
target_link_options(NovaClientBase PRIVATE "/MAP:$<TARGET_FILE_DIR:NovaClientBase>/NovaClientBase.map")

# ReflectiveLoader does not register Windows static TLS. A thread_local object
# would otherwise overwrite a host module's TLS when used on an existing thread.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_custom_command(TARGET NovaClientBase POST_BUILD
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test/loader/verify_no_static_tls.py"
                "$<TARGET_FILE:NovaClientBase>"
        COMMENT "Checking reflective DLL static TLS compatibility"
        VERBATIM)
endif()
