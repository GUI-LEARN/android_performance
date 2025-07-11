/*
 * mock_native_crash.cpp - 模拟原生崩溃工具
 * 
 * 本文件包含用于演示和测试崩溃处理功能的代码
 * 故意创建崩溃场景，用于验证崩溃捕获和处理工具的有效性
 * 主要用于测试Breakpad等崩溃捕获工具和应用稳定性分析
 */

#include <jni.h>            // JNI接口头文件
#include <malloc.h>         // 内存分配函数
#include <elf.h>            // ELF文件格式定义
#include <android/log.h>    // Android日志系统

/*
 * JNI接口：模拟Native崩溃
 * 从Java层调用，用于演示native崩溃场景
 * 
 * 功能说明：
 * - 故意触发空指针解引用崩溃
 * - 这会导致SIGSEGV信号，可以被崩溃捕获工具检测到
 * - 用于测试Breakpad等崩溃处理工具的有效性
 * - 验证崩溃报告生成和处理流程
 * 
 * 崩溃类型：SIGSEGV (Segmentation Fault)
 * 触发原因：空指针解引用
 * 
 * 注意：这是一个演示用的崩溃代码，在实际应用中应该避免这种情况
 * 调用此函数会导致应用崩溃，应该在测试环境中使用
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_stability_StabilityExampleActivity_mockCrash(JNIEnv *env,
                                                                                    jclass clazz) {
    
    // 记录即将发生的崩溃
    __android_log_print(ANDROID_LOG_DEBUG, "MockCrash", 
                        "About to trigger a mock native crash (null pointer dereference)");
    
    // 创建空指针
    int* ptr = nullptr;
    
    // 故意解引用空指针，触发SIGSEGV信号
    // 这会导致程序崩溃，产生段错误(Segmentation Fault)
    *ptr = 10;
    
    // 注意：这行代码永远不会被执行，因为上面的操作会导致程序崩溃
    // 如果安装了崩溃捕获工具（如Breakpad），它会：
    // 1. 捕获SIGSEGV信号
    // 2. 生成崩溃报告文件（如minidump）
    // 3. 调用相应的崩溃处理回调函数
    // 4. 记录崩溃信息用于后续分析
    
    __android_log_print(ANDROID_LOG_DEBUG, "MockCrash", 
                        "This line will never be reached due to the crash above");
}