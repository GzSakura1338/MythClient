// Native loader fixture, not a JVM or Java helper. Reject host startup promptly.
#include <windows.h>
__declspec(dllexport) int WINAPI JNI_GetCreatedJavaVMs(void** machines, int capacity, int* count) {
    (void)machines; (void)capacity;
    *count = 0;
    return 0;
}
