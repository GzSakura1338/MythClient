add_executable(NovaUiPreview
    Preview.cpp
    BlurRegression.cpp
    ${PROJECT_SOURCE_DIR}/src/config/Config.cpp
    ${PROJECT_SOURCE_DIR}/src/config/PropertyCodec.cpp
    ${PROJECT_SOURCE_DIR}/src/config/ConfigManager.cpp
    ${PROJECT_SOURCE_DIR}/src/bind/BindCapture.cpp
    ${PROJECT_SOURCE_DIR}/src/bind/BindProperty.cpp
    ${PROJECT_SOURCE_DIR}/src/bind/BindableManager.cpp
    ${PROJECT_SOURCE_DIR}/src/bind/InputManager.cpp
    ${PROJECT_SOURCE_DIR}/src/bind/KeyName.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/Color.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/Json.cpp)
target_compile_options(NovaUiPreview PRIVATE /utf-8)
target_link_libraries(NovaUiPreview PRIVATE NovaUI NovaMedia user32 gdi32 opengl32 shell32 imm32 dwmapi)
set_target_properties(NovaUiPreview PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/preview")

add_executable(NovaCSWinPreview CsWinPreview.cpp)
target_compile_options(NovaCSWinPreview PRIVATE /utf-8)
target_link_libraries(NovaCSWinPreview PRIVATE NovaUI user32 gdi32 opengl32 imm32 dwmapi)
set_target_properties(NovaCSWinPreview PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/preview")
