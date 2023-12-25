#include <jni.h>
#include <malloc.h>
#include <elf.h>
#include <android/log.h>
#include <jni.h>

extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_stability_StabilityExampleActivity_mockCrash(JNIEnv *env,
                                                                                    jclass clazz) {
    int* ptr = nullptr;
    *ptr = 10;
}