# Build-only binary embedding keeps the reflective DLL independent of resource
# loaders, external asset paths and the reference project's directory.
file(READ "${INPUT}" MUSIC_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," MUSIC_BYTES "${MUSIC_HEX}")
file(WRITE "${OUTPUT}" "#include <cstddef>\nnamespace nova::ui::cswin {\nextern const unsigned char Music${TRACK}[]={${MUSIC_BYTES}};\nextern const std::size_t Music${TRACK}Size=sizeof(Music${TRACK});\n}\n")
