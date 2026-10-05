#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>

#define GL_ACTIVE_TEXTURE           0x84E0
#define GL_ARRAY_BUFFER             0x8892
#define GL_ARRAY_BUFFER_BINDING     0x8894
#define GL_VERTEX_ARRAY_BINDING     0x85B5
#define GL_CURRENT_PROGRAM          0x8B8D
#define GL_READ_FRAMEBUFFER         0x8CA8
#define GL_DRAW_FRAMEBUFFER         0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
// Windows' GL/gl.h stops at GL 1.1, so the strings the context reports its own
// version through have to be named here too.
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C

namespace nova::gl {

inline void* resolve(const char* name) {
    void* fn = reinterpret_cast<void*>(wglGetProcAddress(name));
    if (fn == nullptr || fn == reinterpret_cast<void*>(1) ||
        fn == reinterpret_cast<void*>(2) || fn == reinterpret_cast<void*>(3) ||
        fn == reinterpret_cast<void*>(-1)) {
        fn = reinterpret_cast<void*>(
            GetProcAddress(GetModuleHandleA("opengl32.dll"), name));
    }
    return fn;
}

namespace detail {

template <typename Fn>
Fn Lazy(Fn& slot, const char* name) {
    if (slot == nullptr) slot = reinterpret_cast<Fn>(resolve(name));
    return slot;
}

}

inline void(APIENTRY* g_useProgram)(GLuint) = nullptr;
inline void(APIENTRY* g_activeTexture)(GLenum) = nullptr;
inline void(APIENTRY* g_bindBuffer)(GLenum, GLuint) = nullptr;
inline void(APIENTRY* g_bindVertexArray)(GLuint) = nullptr;
inline void(APIENTRY* g_bindFramebuffer)(GLenum, GLuint) = nullptr;

inline void ResetPointers() {
    g_useProgram = nullptr;
    g_activeTexture = nullptr;
    g_bindBuffer = nullptr;
    g_bindVertexArray = nullptr;
    g_bindFramebuffer = nullptr;
}

inline void(APIENTRY* useProgram())(GLuint) {
    return detail::Lazy(g_useProgram, "glUseProgram");
}

inline void(APIENTRY* activeTexture())(GLenum) {
    return detail::Lazy(g_activeTexture, "glActiveTexture");
}

inline void(APIENTRY* bindBuffer())(GLenum, GLuint) {
    return detail::Lazy(g_bindBuffer, "glBindBuffer");
}

inline void(APIENTRY* bindVertexArray())(GLuint) {
    return detail::Lazy(g_bindVertexArray, "glBindVertexArray");
}

inline void(APIENTRY* bindFramebuffer())(GLenum, GLuint) {
    return detail::Lazy(g_bindFramebuffer, "glBindFramebuffer");
}

}

#define glUseProgram      (::nova::gl::useProgram())
#define glActiveTexture   (::nova::gl::activeTexture())
#define glBindBuffer      (::nova::gl::bindBuffer())
#define glBindVertexArray (::nova::gl::bindVertexArray())
#define glBindFramebuffer (::nova::gl::bindFramebuffer())
