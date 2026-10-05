#pragma once
#include "ui/backend/GlExt.h"

namespace nova::glhook {
// A host renderer may leave sub-image offsets or a PBO bound. ImGui uploads
// CPU pixels, so those settings must not leak into its dynamic font atlas.
class PixelUnpackGuard {
    static constexpr GLenum bufferTarget = 0x88EC;
    static constexpr GLenum bufferBinding = 0x88EF;
    GLint buffer_{}, alignment_{}, rowLength_{}, skipRows_{}, skipPixels_{};
public:
    PixelUnpackGuard() {
        glGetIntegerv(bufferBinding, &buffer_);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment_);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength_);
        glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows_);
        glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels_);
        glBindBuffer(bufferTarget, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    }
    ~PixelUnpackGuard() {
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment_);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength_);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows_);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels_);
        glBindBuffer(bufferTarget, buffer_);
    }
    PixelUnpackGuard(const PixelUnpackGuard&) = delete;
    PixelUnpackGuard& operator=(const PixelUnpackGuard&) = delete;
};
}
