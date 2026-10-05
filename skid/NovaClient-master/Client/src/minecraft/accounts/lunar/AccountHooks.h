#pragma once
#include "minecraft/java/api/base.h"
namespace nova::minecraft::accounts {
void JNICALL BrowserAddAccount(JNIEnv*,jclass,jobject callback);
jobject JNICALL BrowserSelectAccount(JNIEnv*,jclass,jstring uuid);
jboolean JNICALL ScreenRequiresLogin(JNIEnv*,jclass,jobject screen);
jboolean JNICALL CanOpenMultiplayer(JNIEnv*,jclass,jobject manager);
void JNICALL BrowserRefreshAccount(JNIEnv*,jclass,jobject account,jobject callback);
}
