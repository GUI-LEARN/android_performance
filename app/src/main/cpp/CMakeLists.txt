# CMakeLists.txt for Android Performance Optimization Project
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
# Version 3.10.2 is the minimum required for Android NDK integration
cmake_minimum_required(VERSION 3.10.2)

# 添加编译选项：保留函数调用栈帧指针，用于调试和性能分析
# This option preserves frame pointers for better debugging and profiling
add_compile_options(-fno-omit-frame-pointer)

# 创建第一个共享库：optimize
# This library contains CPU performance optimization utilities and function hooking
add_library(
        optimize                    # 库名称 (Library name)
        SHARED                      # 共享库类型 (Shared library type)
        optimize.cpp               # CPU性能优化工具 (CPU performance optimization utilities)
        hook_functions.cpp)        # 函数钩子实现 (Function hooking implementations)

# 创建第二个共享库：example
# This library contains example implementations for testing crashes and memory leaks
add_library(
        example                     # 库名称 (Library name)
        SHARED                      # 共享库类型 (Shared library type)
        mock_native_leak.cpp       # 模拟原生内存泄漏 (Mock native memory leak examples)
        mock_native_crash.cpp      # 模拟原生崩溃 (Mock native crash examples)
        native_capture.cpp)        # 原生崩溃捕获 (Native crash capture using Breakpad)

# 查找Android系统日志库
# Find the Android system logging library
find_library(
        log-lib                     # 变量名称 (Variable name to store the library path)
        log)                        # 系统库名称 (System library name)

# 查找ByteHook库包
# Find the ByteHook library package for function hooking
find_package(bytehook REQUIRED CONFIG)

# 构建Google Breakpad静态库
# Build Google Breakpad static library for native crash reporting
add_library(breakpad STATIC
        # 崩溃生成客户端 (Crash generation client)
        breakpad/src/client/linux/crash_generation/crash_generation_client.cc
        # 转储写入器通用组件 (Common dump writer components)
        breakpad/src/client/linux/dump_writer_common/thread_info.cc
        breakpad/src/client/linux/dump_writer_common/ucontext_reader.cc
        # 异常处理器 (Exception handler - main crash handling component)
        breakpad/src/client/linux/handler/exception_handler.cc
        breakpad/src/client/linux/handler/minidump_descriptor.cc
        # 日志组件 (Logging component)
        breakpad/src/client/linux/log/log.cc
        # 微转储写入器 (Microdump writer for minimal crash dumps)
        breakpad/src/client/linux/microdump_writer/microdump_writer.cc
        # 小转储写入器 (Minidump writer components)
        breakpad/src/client/linux/minidump_writer/linux_dumper.cc
        breakpad/src/client/linux/minidump_writer/linux_ptrace_dumper.cc
        breakpad/src/client/linux/minidump_writer/minidump_writer.cc
        breakpad/src/client/linux/minidump_writer/pe_file.cc
        # 小转储文件写入器 (Minidump file writer)
        breakpad/src/client/minidump_file_writer.cc
        # 通用工具 (Common utilities)
        breakpad/src/common/convert_UTF.cc       # UTF字符串转换 (UTF string conversion)
        breakpad/src/common/md5.cc              # MD5哈希计算 (MD5 hash calculation)
        breakpad/src/common/string_conversion.cc # 字符串转换工具 (String conversion utilities)
        # Linux特定组件 (Linux-specific components)
        breakpad/src/common/linux/breakpad_getcontext.S    # 汇编代码：获取上下文 (Assembly code for context capture)
        breakpad/src/common/linux/elfutils.cc              # ELF文件处理工具 (ELF file utilities)
        breakpad/src/common/linux/file_id.cc               # 文件ID生成 (File ID generation)
        breakpad/src/common/linux/guid_creator.cc          # GUID创建器 (GUID creator)
        breakpad/src/common/linux/linux_libc_support.cc    # Linux libc支持 (Linux libc support)
        breakpad/src/common/linux/memory_mapped_file.cc    # 内存映射文件 (Memory mapped file support)
        breakpad/src/common/linux/safe_readlink.cc)        # 安全读取符号链接 (Safe readlink implementation)

# 设置breakpad库使用ARM模式编译（而不是Thumb模式）
# Set breakpad library to compile in ARM mode (not Thumb mode) for better performance
set_target_properties(breakpad PROPERTIES ANDROID_ARM_MODE arm)

# 设置汇编文件的语言类型为C
# Set the assembly file language type to C for proper compilation
set_property(SOURCE breakpad/src/common/linux/breakpad_getcontext.S PROPERTY LANGUAGE C)

# 设置breakpad库的头文件包含目录
# Set include directories for breakpad library
target_include_directories(breakpad PUBLIC
        breakpad/src/common/android/include    # Android特定头文件 (Android-specific headers)
        breakpad/src)                          # Breakpad源码根目录 (Breakpad source root directory)

# 链接example库的依赖项
# Link dependencies for the example library
target_link_libraries(
        example                     # 目标库名称 (Target library name)
        ${log-lib}                 # Android日志库 (Android logging library)
        breakpad)                  # Breakpad崩溃报告库 (Breakpad crash reporting library)

# 链接optimize库的依赖项
# Link dependencies for the optimize library
target_link_libraries(
        optimize                    # 目标库名称 (Target library name)
        ${log-lib}                 # Android日志库 (Android logging library)
        bytehook::bytehook)        # ByteHook函数钩子库 (ByteHook function hooking library)
