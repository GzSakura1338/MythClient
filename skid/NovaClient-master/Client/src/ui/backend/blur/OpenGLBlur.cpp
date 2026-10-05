#include "ui/backend/blur/Blur.h"
#include "ui/backend/blur/Gaussian.h"
#include "ui/backend/GlExt.h"
#include <array>

namespace nova::ui::blur {
namespace {
using Gen = void(APIENTRY*)(GLsizei, GLuint*);
using Del = void(APIENTRY*)(GLsizei, const GLuint*);
using Attach = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using Status = GLenum(APIENTRY*)(GLenum);
using Blit = void(APIENTRY*)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
Gen gen{}; Del del{}; Attach attach{}; Status status{}; Blit blit{};
constexpr GLenum colorAttachment = 0x8CE0, complete = 0x8CD5, rgb8 = 0x8051;
constexpr GLenum unpackBuffer = 0x88EC, unpackBinding = 0x88EF, framebufferSrgb = 0x8DB9;
struct Level { GLuint texture = 0, fbo = 0; int w = 0, h = 0; };
struct Cache {
    std::array<Level, 4> levels;
    Refresh refresh;
    int width = 0, height = 0;
    bool failed = false;
};
Cache world, layer;

#include "ui/backend/blur/OpenGLGaussian.h"

bool Load() {
    gen = reinterpret_cast<Gen>(gl::resolve("glGenFramebuffers"));
    del = reinterpret_cast<Del>(gl::resolve("glDeleteFramebuffers"));
    attach = reinterpret_cast<Attach>(gl::resolve("glFramebufferTexture2D"));
    status = reinterpret_cast<Status>(gl::resolve("glCheckFramebufferStatus"));
    blit = reinterpret_cast<Blit>(gl::resolve("glBlitFramebuffer"));
    return gen && del && attach && status && blit && gl::bindFramebuffer() && gl::bindBuffer() && LoadGaussian();
}
void Copy(const Level& from, const Level& to, GLenum filter) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, from.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to.fbo);
    blit(0, 0, from.w, from.h, 0, 0, to.w, to.h, GL_COLOR_BUFFER_BIT, filter);
}
void Release(Cache& cache) {
    for (auto& level : cache.levels) {
        if (level.fbo && del) del(1, &level.fbo);
        if (level.texture) glDeleteTextures(1, &level.texture);
        level = {};
    }
    cache = {};
}
void RenderCache(Cache& cache, Options options, int w, int h, bool capture, bool force = false) {
    if (w <= 0 || h <= 0 || cache.failed) return;
    if (!blit && !Load()) { cache.failed = true; return; }
    auto& levels = cache.levels;
    GaussianState restore;
    GLint read = 0, draw = 0, texture = 0, unpack = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(unpackBinding, &unpack);
    const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    const GLboolean srgb = glIsEnabled(framebufferSrgb);
    glDisable(GL_SCISSOR_TEST);
    // Preserve the scene's stored colors; the cache is RGB8, not sRGB.
    glDisable(framebufferSrgb);
    // nullptr must mean no initial pixels, not offset zero in a game's PBO.
    glBindBuffer(unpackBuffer, 0);
    if (cache.width != w || cache.height != h) {
        Release(cache); cache.width = w; cache.height = h;
        int lw = w, lh = h;
        for (auto& level : levels) {
            level.w = lw; level.h = lh;
            glGenTextures(1, &level.texture);
            glBindTexture(GL_TEXTURE_2D, level.texture);
            // Presented game color can have zero alpha (e.g. a cleared target
            // blitted with alpha writes disabled). It is still an opaque scene.
            // RGB storage samples alpha=1, leaving coverage to BlurDraw instead
            // of accidentally making the entire backdrop texture invisible.
            glTexImage2D(GL_TEXTURE_2D, 0, rgb8, lw, lh, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
            gen(1, &level.fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, level.fbo);
            attach(GL_DRAW_FRAMEBUFFER, colorAttachment, GL_TEXTURE_2D, level.texture, 0);
            if (status(GL_DRAW_FRAMEBUFFER) != complete) { cache.failed = true; break; }
            if(&level==&levels[0]){lw = std::max(1, lw / 2); lh = std::max(1, lh / 2);}
        }
    }
    if (!cache.failed && !CreateGaussian())cache.failed=true;
    if (!cache.failed && capture) {
        if (force || cache.refresh.Due(options)) {
            // First resolve at full size (also valid for multisampled game targets).
            Copy({0, static_cast<GLuint>(draw), w, h}, levels[0], GL_NEAREST);
            Copy(levels[0],levels[1],GL_LINEAR);
            GaussianFilter(levels[1],levels[2],GaussianPass(options.strength,1.f/levels[1].w,0));
            GaussianFilter(levels[2],levels[3],GaussianPass(options.strength,0,1.f/levels[1].h));
            cache.refresh.Updated(options);
        }
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindBuffer(unpackBuffer, unpack);
    if (scissor) glEnable(GL_SCISSOR_TEST);
    if (srgb) glEnable(framebufferSrgb);
}
void CaptureLayer(const ImDrawList*, const ImDrawCmd*) {
    RenderCache(layer, GetOptions(), layer.width, layer.height, true, true);
}
}

void ReleaseOpenGL() {
    Publish(0); ClearLayerBackdrop();
    Release(world); Release(layer); DeleteGaussian();
}
void ResetOpenGLContext(bool contextAvailable) {
    if (contextAvailable) ReleaseOpenGL();
    else {
        Publish(0); ClearLayerBackdrop(); world = {}; layer = {};
        gaussianProgram=gaussianVao=0;
    }
    gen = nullptr; del = nullptr; attach = nullptr; status = nullptr; blit = nullptr;
    gaussianApi={};
}
void RenderOpenGL(Options options, int w, int h) {
    Publish(0); ClearLayerBackdrop();
    if (!options.enabled) { world.refresh.valid = false; return; }
    RenderCache(world, options, w, h, true);
    if (!world.failed && world.refresh.valid) Publish(static_cast<ImTextureID>(world.levels[3].texture), true);
    RenderCache(layer, options, w, h, false);
    if (!layer.failed) SetLayerBackdrop(static_cast<ImTextureID>(layer.levels[3].texture), true, CaptureLayer);
}
}
