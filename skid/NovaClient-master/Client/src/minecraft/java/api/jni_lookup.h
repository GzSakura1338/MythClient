#pragma once

#include "base.h"

namespace nova_base {
jmethodID findMethodByDescriptor(jclass klass, const char* desc, bool wantStatic);
int findMethodsByDescriptor(jclass klass, const char* desc, bool wantStatic, jmethodID* out, int maxOut);
jmethodID findMethodByDescriptorExcept(jclass klass, const char* desc, bool wantStatic, const char* const* excl,
                                       int nExcl);
jfieldID findFieldByDescriptor(jclass klass, const char* desc, bool wantStatic);
jclass findLoadedBySig(JNIEnv* env, const char* sig);
jclass loadOrFind(JNIEnv* env, jobject mcLoader, const char* dot, const char* sig);
}
std::string JniClassName(JNIEnv* env, jobject object);
