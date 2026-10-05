#pragma once

#include "base.h"

#include <string>

namespace nova::bridge {

struct HookDef {
    const char* name;
    const char* descriptor;
    void*       impl;
};

bool Install(JNIEnv* env, jobject mcLoader);
bool Release(JNIEnv* env);

bool IsInstalled();

const std::string& InternalName();

const std::string& DotName();

const HookDef* Hooks();
int HookCount();

}
