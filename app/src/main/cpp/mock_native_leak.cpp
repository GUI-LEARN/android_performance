#include <jni.h>
#include <malloc.h>
#include <elf.h>
#include <android/log.h>

extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_memory_NativeLeakActivity_mallocLeak(JNIEnv *env,
                                                                               jobject thiz) {
    void * result = malloc(100*1000*1000);
}