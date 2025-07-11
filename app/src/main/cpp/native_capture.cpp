/*
 * native_capture.cpp - Native崩溃捕获和分析工具
 * 
 * 本文件使用Google Breakpad库来捕获和处理native崩溃
 * 当应用程序发生原生代码崩溃时，会生成minidump文件用于后续分析
 * 主要用于稳定性测试和崩溃分析
 */

#include <jni.h>                                                            // JNI接口头文件
#include <android/log.h>                                                    // Android日志系统
#include <dlfcn.h>                                                          // 动态链接库操作
#include "breakpad/src/client/linux/handler/exception_handler.h"           // Breakpad异常处理器
#include "breakpad/src/client/linux/handler/minidump_descriptor.h"         // Breakpad minidump描述符

/*
 * Breakpad崩溃转储回调函数
 * 当发生崩溃时，Breakpad会调用此函数来处理minidump文件
 * 
 * @param descriptor: minidump文件描述符，包含文件路径等信息
 * @param context: 用户定义的上下文数据（本例中为NULL）
 * @param succeeded: 指示minidump是否成功生成
 * @return: 返回false表示让系统继续处理崩溃（通常会终止进程）
 *         返回true表示崩溃已被处理，程序可以继续运行（不推荐）
 */
bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                  void *context,
                  bool succeeded) {

    // 记录minidump文件生成结果到Android日志
    __android_log_print(ANDROID_LOG_DEBUG, "breakpad",
                        "Wrote breakpad minidump at %s succeeded=%d\n", 
                        descriptor.path(),    // minidump文件路径
                        succeeded);           // 是否成功生成

    // 返回false让系统继续处理崩溃
    // 这通常会导致进程终止，这是期望的行为
    return false;
}

/*
 * JNI接口：捕获Native崩溃
 * 从Java层调用，用于演示native崩溃捕获功能
 * 
 * 工作流程：
 * 1. 创建minidump描述符，指定crash dump文件的存储路径
 * 2. 创建异常处理器，注册崩溃回调函数
 * 3. 故意触发一个空指针解引用崩溃
 * 4. Breakpad捕获崩溃并生成minidump文件
 * 5. 调用回调函数记录处理结果
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_stability_StabilityExampleActivity_captureNativeCrash(JNIEnv *env,
                                                                               jobject thiz) {

    // 创建minidump描述符
    // 指定崩溃转储文件的存储目录为应用的私有数据目录
    google_breakpad::MinidumpDescriptor descriptor("/data/data/com.example.performance_optimize/");
    
    // 创建异常处理器
    // 参数说明：
    // - descriptor: minidump文件描述符
    // - NULL: 过滤器回调函数（不使用）
    // - dumpCallback: 崩溃转储完成后的回调函数
    // - NULL: 回调函数的上下文数据
    // - true: 安装信号处理器
    // - -1: 服务器文件描述符（不使用崩溃报告服务器）
    google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback,
                                         NULL, true, -1);
    
    // 故意触发空指针解引用崩溃
    // 这是一个演示用的崩溃，在实际应用中应该避免这种情况
    int* ptr = nullptr;   // 创建空指针
    *ptr = 10;           // 解引用空指针，触发SIGSEGV信号
    
    // 注意：这行代码永远不会被执行，因为上面的操作会导致程序崩溃
    // Breakpad会捕获这个崩溃，生成minidump文件，然后调用dumpCallback函数
}

