#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include "breakpad/src/client/linux/handler/exception_handler.h"
#include "breakpad/src/client/linux/handler/minidump_descriptor.h"

bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                  void *context,
                  bool succeeded) {

    __android_log_print(ANDROID_LOG_DEBUG, "breakpad",
                        "Wrote breakpad minidump at %s succeeded=%d\n", descriptor.path(),
                        succeeded);
    return false;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_stability_StabilityExampleActivity_captureNativeCrash(JNIEnv *env,
                                                                               jobject thiz) {

    google_breakpad::MinidumpDescriptor descriptor("/data/data/com.example.performance_optimize/");
    google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback,
                                         NULL, true, -1);
    int* ptr = nullptr;
    *ptr = 10;
}

