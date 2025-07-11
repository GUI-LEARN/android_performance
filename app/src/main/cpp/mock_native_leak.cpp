/*
 * mock_native_leak.cpp - 模拟原生内存泄漏工具
 * 
 * 本文件包含用于演示和测试内存泄漏检测功能的代码
 * 故意创建内存泄漏场景，用于验证内存监控和检测工具的有效性
 * 主要用于内存泄漏检测工具的测试和演示
 */

#include <jni.h>            // JNI接口头文件
#include <malloc.h>         // 内存分配函数
#include <elf.h>            // ELF文件格式定义
#include <android/log.h>    // Android日志系统

/*
 * JNI接口：模拟malloc内存泄漏
 * 从Java层调用，用于演示native内存泄漏场景
 * 
 * 功能说明：
 * - 分配大量内存（100MB）但故意不释放
 * - 这会导致内存泄漏，可以被内存监控工具检测到
 * - 用于测试ByteHook、LeakCanary等内存监控工具的有效性
 * 
 * 注意：这是一个演示用的内存泄漏代码，在实际应用中应该避免这种情况
 * 每次调用都会泄漏100MB内存，多次调用可能导致OOM
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_memory_NativeLeakActivity_mallocLeak(JNIEnv *env,
                                                                               jobject thiz) {
    
    // 分配100MB内存
    // 这是一个相当大的内存分配，足以被内存监控工具检测到
    void * result = malloc(100*1000*1000);  // 100MB = 100 * 1000 * 1000 bytes
    
    // 故意不调用free(result)来释放内存
    // 这会导致内存泄漏，分配的内存永远不会被释放
    // 在实际应用中，应该在适当的时候调用free(result)来释放内存
    
    // 可以添加日志来记录内存分配
    __android_log_print(ANDROID_LOG_DEBUG, "NativeLeak", 
                        "Allocated 100MB at address: %p (intentionally not freed)", result);
    
    // 注意：这个函数返回后，result变量会超出作用域
    // 但分配的内存仍然存在，且没有任何指针指向它
    // 这就形成了典型的内存泄漏场景
}