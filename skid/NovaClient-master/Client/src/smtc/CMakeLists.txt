add_library(NovaMedia STATIC
    SmtcService.cpp
    Lyrics.cpp
    backend/WindowsSession.cpp
    backend/NetEaseTimeline.cpp
    backend/NetEaseLyrics.cpp
    backend/ArtworkDecoder.cpp
)
target_include_directories(NovaMedia PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_options(NovaMedia PRIVATE /utf-8)
target_link_libraries(NovaMedia PUBLIC runtimeobject windowscodecs shcore ole32 winhttp)
target_link_libraries(NovaClientBase PRIVATE NovaMedia)
