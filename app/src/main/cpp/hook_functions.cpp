/*
 * hook_functions.cpp - 函数钩子和内存监控工具
 * 
 * 本文件包含使用ByteHook库和PLT Hook技术实现的函数钩子功能
 * 主要用于监控内存分配、堆栈跟踪和系统调用拦截
 * 支持malloc函数钩子和write函数钩子，用于内存泄漏检测和ANR分析
 */

#include <jni.h>              // JNI接口头文件
#include <string>             // C++字符串操作
#include <dlfcn.h>            // 动态链接库操作
#include <vector>             // STL向量容器
#include "bytehook.h"         // ByteHook库头文件
#include <android/log.h>      // Android日志系统
#include <unistd.h>           // Unix标准系统调用
#include <unwind.h>           // 堆栈回溯功能
#include <inttypes.h>         // 整数类型格式化
#include <linux/elf.h>        // ELF文件格式定义
#include <sys/mman.h>         // 内存映射操作
#include <iostream>           // 输入输出流
#include <fstream>            // 文件流操作
#include <dlfcn.h>            // 动态链接库操作（重复包含）

// 全局变量：标记线程是否已被钩子
bool thread_hooked = false;

// 匿名命名空间：内部使用的数据结构和函数
namespace {
    /*
     * 堆栈回溯状态结构体
     * 用于存储堆栈回溯过程中的当前位置和结束位置
     */
    struct BacktraceState {
        void **current;    // 当前堆栈帧指针
        void **end;        // 堆栈帧结束指针
    };

    /*
     * 堆栈回溯回调函数
     * 被_Unwind_Backtrace调用，用于收集每一帧的程序计数器(PC)值
     * 
     * @param context: 展开上下文信息
     * @param arg: 用户传递的参数（BacktraceState结构体）
     * @return: 展开操作的返回码
     */
    static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context *context, void *arg) {
        BacktraceState *state = static_cast<BacktraceState *>(arg);
        uintptr_t pc = _Unwind_GetIP(context);  // 获取当前帧的程序计数器
        
        if (pc) {
            // 检查是否还有空间存储更多帧
            if (state->current == state->end) {
                return _URC_END_OF_STACK;  // 栈空间已满
            } else {
                *state->current++ = (void *)pc;  // 存储当前帧的PC值
            }
        }
        return _URC_NO_REASON;  // 继续展开
    }
}

/*
 * 通过栈指针保存堆栈信息
 * 使用ARM架构的寄存器信息来遍历调用栈
 * 
 * @param secret: 指向ucontext_t结构的指针，包含CPU上下文信息
 */
static void saveStackBySp(void *secret) {
    // 获取上下文信息
    ucontext_t *uc = (ucontext_t *)secret;
    int i = 0;
    Dl_info dl_info;  // 动态链接信息结构体
    
    // 从ARM寄存器中获取帧指针和程序计数器
    const void **frame_pointer = (const void **)uc->uc_mcontext.arm_fp;  // 帧指针
    const void *return_address = (const void *)uc->uc_mcontext.arm_pc;   // 程序计数器
    
    printf("\nStack trace:");
    
    // 遍历调用栈
    while (return_address) {
        memset(&dl_info, 0, sizeof(Dl_info));
        
        // 获取返回地址对应的符号信息
        if (!dladdr((void *)return_address, &dl_info))
            break;
        
        const char *sname = dl_info.dli_sname;  // 符号名称
        
        // 打印函数调用的信息，包括计数器、返回地址、函数名、偏移量和文件名
        printf("%02d: %p <%s + %u> (%s)", ++i, return_address, sname,
               ((uintptr_t)return_address - (uintptr_t)dl_info.dli_saddr),
               dl_info.dli_fname);
        
        // 如果帧指针为空指针，表示已经到达调用链的终点，因此跳出循环
        if (!frame_pointer)
            break;
        
        // 获取上一个函数的LR的值，即返回地址
        return_address = frame_pointer[-1];
        // 获取上一个函数的FP的值
        frame_pointer = (const void **)frame_pointer[-2];
    }
    printf("Stack trace end.");
}

/*
 * 打印堆栈回溯信息
 * 将堆栈帧缓冲区中的地址转换为可读的符号信息
 * 
 * @param buffer: 存储堆栈帧地址的缓冲区
 * @param depth: 堆栈深度（帧数量）
 */
void dumpBacktrace(void **buffer, size_t depth) {
    for (size_t idx = 0; idx < depth; ++idx) {
        void *addr = buffer[idx];
        Dl_info info;
        
        // 获取地址对应的符号信息
        if (dladdr(addr, &info)) {
            // 计算相对地址偏移
            const uintptr_t addr_relative =
                    ((uintptr_t) addr - (uintptr_t) info.dli_fbase);
            
            // 输出调试日志
            __android_log_print(ANDROID_LOG_DEBUG, "MallocHook",
                                "# %d : %p : %s(%p)(%s)(%p)",
                                idx,
                                addr, info.dli_fname, addr_relative, info.dli_sname,
                                info.dli_saddr);
        }
    }
}

/*
 * 打印native堆栈信息
 * 使用libunwind库进行堆栈回溯，获取调用链信息
 */
void printNativeStack() {
    const size_t max = 30;  // 调用的层数限制
    void *buffer[max];      // 存储堆栈帧地址的缓冲区
    
    // 初始化回溯状态
    BacktraceState state = {buffer, buffer + max};
    
    // 捕获堆栈：使用libunwind进行堆栈回溯
    _Unwind_Backtrace(unwindCallback, &state);
    
    // 返回堆栈的深度
    size_t depth = state.current - buffer;
    
    // 打印堆栈信息
    dumpBacktrace(buffer, depth);
}

/*
 * malloc函数钩子
 * 拦截malloc调用，监控大内存分配并打印堆栈信息
 * 
 * @param len: 要分配的内存大小
 * @return: 分配的内存指针
 */
void *malloc_hook(size_t len) {
    BYTEHOOK_STACK_SCOPE();  // ByteHook栈作用域宏
    
    // 监控大于1MB的内存分配
    if (len > 1024 * 1024) {
        __android_log_print(ANDROID_LOG_DEBUG, "MallocHook", "size:%d", len);
        // 打印native堆栈，帮助定位大内存分配的来源
        printNativeStack();
    }

    // 调用原始malloc函数
    return BYTEHOOK_CALL_PREV(malloc_hook, len);
}

/*
 * write函数钩子
 * 拦截write系统调用，将写入内容保存到文件中用于ANR分析
 * 
 * @param fd: 文件描述符
 * @param buf: 写入数据的缓冲区
 * @param count: 写入数据的字节数
 * @return: 实际写入的字节数
 */
ssize_t my_write(int fd, const void* const buf, size_t count) {
    BYTEHOOK_STACK_SCOPE();  // ByteHook栈作用域宏
    
    if (buf != nullptr) {
        char *content = (char *) buf;
        // 将写入内容追加到ANR分析文件中
        std::ofstream file("/data/data/com.example.performance_optimize/example_anr.txt", std::ios::app);
        if (file.is_open()) {
            file << content;
            file.close();
        }
    }
    
    // 调用原始write函数
    return BYTEHOOK_CALL_PREV(my_write, fd, buf, count);
}

/*
 * Android动态链接扩展信息结构体
 * 用于dlopen_ext函数的参数传递
 */
typedef struct {
    uint64_t flags;                              // 标志位
    void *reserved_addr;                         // 保留地址
    size_t reserved_size;                        // 保留大小
    int relro_fd;                                // RELRO文件描述符
    int library_fd;                              // 库文件描述符
    off64_t library_fd_offset;                   // 库文件偏移量
    struct android_namespace_t *library_namespace; // 库命名空间
} android_dlextinfo;

/*
 * JNI接口：使用ByteHook钩子malloc函数
 * 从Java层调用，启动对libexample.so中malloc函数的钩子
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_memory_NativeLeakActivity_hookMallocByBHook(
        JNIEnv *env,
        jobject thiz) {

    // 创建ByteHook存根，钩子libexample.so中的malloc函数
    bytehook_stub_t stub = bytehook_hook_single(
            "libexample.so",     // 目标库名称
            nullptr,             // 调用者过滤器（不使用）
            "malloc",            // 目标函数名称
            (void *)malloc_hook, // 钩子函数指针
            nullptr,             // 钩子安装回调
            nullptr);            // 钩子移除回调
}

/*
 * JNI接口：使用ByteHook钩子write函数
 * 从Java层调用，启动对libc.so中write函数的钩子，用于ANR分析
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_stability_StabilityExampleActivity_hookAnrByBHook(
        JNIEnv *env,
        jobject thiz) {

    // 钩子所有库中的write函数
    bytehook_hook_all(
            "libc.so",           // 目标库名称
            "write",             // 目标函数名称
            (void *)my_write,    // 钩子函数指针
            nullptr,             // 钩子安装回调
            nullptr);            // 钩子移除回调
}

// 全局变量：保存原始函数地址
uintptr_t originFunc;

/*
 * 通过PLT Hook实现的malloc钩子函数
 * 手动实现的PLT Hook技术，用于拦截malloc调用
 * 
 * @param len: 要分配的内存大小
 * @return: 分配的内存指针
 */
void *malloc_hook_by_plt(size_t len) {
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "origin malloc size:%d", len);
    
    // 监控大于20MB的内存分配
    if(len > 20*1024*1024){
        __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "do somethings");
        printNativeStack();  // 打印堆栈信息
    }
    
    // 调用原函数
    return reinterpret_cast<void *(*)(size_t)>((void*)originFunc)(len);
}

/*
 * 根据符号的st_value获取符号名称
 * 通过解析ELF文件的字符串表来获取符号的名称
 * 
 * @param base_addr: 库的基地址
 * @param sym: 符号表项指针
 * @return: 符号名称字符串
 */
std::string getSymbolNameByValue(uintptr_t base_addr, Elf32_Sym *sym) {
    Elf32_Ehdr *header = (Elf32_Ehdr *) (base_addr);
    
    // 获取段头部表的地址
    __android_log_print(ANDROID_LOG_DEBUG, "getSymbolNameByValue", "base_addr %p", base_addr);
    Elf32_Shdr *seg_table = (Elf32_Shdr *) (base_addr + header->e_shoff);
    
    // 段的数量
    size_t seg_count = header->e_shnum;
    __android_log_print(ANDROID_LOG_DEBUG, "getSymbolNameByValue", "seg_table2 %p", seg_table);
    
    // 查找字符串表段
    Elf32_Shdr* stringTableHeader = nullptr;
    for (int i = 0; i < seg_count; i++) {
        if (seg_table[i].sh_type == SHT_STRTAB) {
            __android_log_print(ANDROID_LOG_DEBUG, "getSymbolNameByValue", "sh_type %d ,%p", 
                                seg_table[i].sh_type, seg_table[i].sh_addr);
            stringTableHeader = &seg_table[i];
            break;
        }
    }
    
    // 获取字符串表的实际地址
    char* stringTable = (char*)(base_addr + stringTableHeader->sh_offset);
    
    // 返回符号名称
    std::string symbolName = std::string(stringTable + sym->st_name);
    return symbolName;
}

/*
 * JNI接口：使用PLT Hook技术钩子malloc函数
 * 手动实现PLT Hook，通过修改GOT表来拦截malloc调用
 * 这是一个高级钩子技术，需要深入了解ELF文件格式和动态链接原理
 */
extern "C"
[[noreturn]] JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_memory_NativeLeakActivity_hookMallocByPLTHook(JNIEnv *env, jobject thiz) {
    
    // 第一步：解析/proc/self/maps文件，找到目标库的基地址
    FILE *fp = fopen("/proc/self/maps", "r");
    char line[1024];
    uintptr_t base_addr = 0;
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "1");
    
    while (fgets(line, sizeof(line), fp)) {
        __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "line:%s", line);
        
        // 查找libexample.so的内存映射
        if (NULL != strstr(line, "libexample.so")) {
            std::string targetLine = line;
            std::size_t pos = targetLine.find('-');
            if (pos != std::string::npos) {
                std::string addressStr = targetLine.substr(0, pos);
                // stoull函数用于将字符串转换为无符号长长整型
                base_addr = std::stoull(addressStr, nullptr, 16);
                break;
            }
        }
    }
    fclose(fp);
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "base_addr:%p", base_addr);

    // 第二步：解析ELF文件头
    // 将base_addr强制转换成Elf_Ehdr格式
    Elf32_Ehdr *header = (Elf32_Ehdr *) (base_addr);

    // 第三步：查找DYNAMIC段
    size_t phr_count = header->e_phnum;  // 程序头表项个数
    Elf32_Phdr *phdr_table = (Elf32_Phdr *) (base_addr + header->e_phoff);  // 程序头部表的地址
    unsigned long dynamicAddr = 0;
    unsigned int dynamicSize = 0;
    
    for (int i = 0; i < phr_count; i++) {
        if (phdr_table[i].p_type == PT_DYNAMIC) {
            // so基地址加dynamic段的偏移地址，就是dynamic段的实际地址
            dynamicAddr = phdr_table[i].p_vaddr + base_addr;
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "dynamicBashAddr:%p",
                                (void *) phdr_table[i].p_vaddr);
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "dynamicAddr:%p",
                                dynamicAddr);
            dynamicSize = phdr_table[i].p_memsz;
            break;
        }
    }

    // 第四步：解析动态信息表，查找GOT表地址
    uintptr_t symbolTableAddr;
    Elf32_Dyn *dynamic_table = (Elf32_Dyn *) dynamicAddr;
    
    for (int i = 0; i < dynamicSize; i++) {
        if (dynamic_table[i].d_tag == DT_PLTGOT) {
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "symbolTableBaseAddr:%p",
                                (void *) dynamic_table[i].d_un.d_ptr);
            symbolTableAddr = dynamic_table[i].d_un.d_ptr + base_addr;
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "symbolTableAddr:%p",
                                symbolTableAddr);
            break;
        }
    }

    // 第五步：修改内存保护属性，允许写入
    mprotect((void *) (symbolTableAddr & ~(PAGE_SIZE - 1)), PAGE_SIZE, PROT_READ | PROT_WRITE);
    
    // 第六步：准备Hook
    // 目标函数偏移地址（这里是硬编码的偏移地址）
    originFunc = 0x52474 + base_addr;
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "originFunc:%p", originFunc);
    
    // 替换的hook函数的偏移地址
    uintptr_t newFunc = (uintptr_t) &malloc_hook_by_plt;
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "newFunc:%p", newFunc);

    // 第七步：执行Hook操作
    int *symbolTable = (int *) symbolTableAddr;
    for (int i = 0;; i++) {
        if ((uintptr_t) &symbolTable[i] == originFunc) {
            originFunc = symbolTable[i];    // 保存原函数地址
            symbolTable[i] = newFunc;       // 替换为新函数地址
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "match origin:%p", &symbolTable[i]);
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "match new:%p", symbolTable[i]);
            break;
        }
    }

    /*
     * 以下是注释掉的代码，展示了另一种PLT Hook的实现方式
     * 通过解析重定位表来查找目标函数的GOT表项
     * 这种方式更加通用，但实现复杂度更高
     */
    
    // 注释掉的代码：通过符号表和重定位表进行Hook
    // 这部分代码展示了如何通过解析ELF文件的重定位表来找到函数的GOT表项
    // 然后修改GOT表项来实现函数钩子
    
    /*
    // 获取符号信息
    Dl_info info;
    if (dladdr((void *)originFunc, &info)) {
        __android_log_print(ANDROID_LOG_DEBUG, "originFunc symbol",
                            "%p : %s(%s)(%p)",
                            originFunc, info.dli_fname, info.dli_sname,
                            info.dli_saddr);
    }

    // 解析重定位表
    Elf32_Rel *rela;
    Elf32_Sym *sym;
    size_t pltrel_size = 0;
    
    // 遍历动态信息表，查找DT_JMPREL和DT_PLTRELSZ标签
    for (int i = 0; i < dynamicSize; i++) {
        if (dynamic_table[i].d_tag == DT_JMPREL) {
            rela = (Elf32_Rel *)(dynamic_table[i].d_un.d_ptr + base_addr);
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "DT_PLTRELSZ2 size:%d", dynamic_table[i].d_un.d_val);
        } else if(dynamic_table[i].d_tag == DT_PLTRELSZ){
            pltrel_size = dynamic_table[i].d_un.d_val;
        } else if(dynamic_table[i].d_tag == DT_SYMTAB){
            sym = (Elf32_Sym *)(dynamic_table[i].d_un.d_val + base_addr);
        }
    }
    
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "rela:%p,size:%d,sym:%p", rela, pltrel_size, sym);
    
    // 遍历重定位表，找到函数的重定位项
    size_t entries = pltrel_size / sizeof(Elf32_Rel);
    for (size_t i = 0; i < entries; ++i) {
        Elf32_Rel *reloc = &rela[i];
        size_t symbol_index = ELF32_R_SYM(reloc->r_info);
        
        // 根据symbol_index获取符号表中的符号信息
        std::string name = getSymbolNameByValue(base_addr, &sym[symbol_index]);
        __android_log_print(ANDROID_LOG_DEBUG, "get symbol",
                            "%d,name:(%p)(%s)(%p)",
                            symbol_index, &sym[symbol_index].st_value, name.c_str(), reloc->r_offset);
        
        // 如果找到malloc符号，设置Hook
        if(name.find("malloc") != std::string::npos){
            originFunc = reloc->r_offset + base_addr;
            break;
        }
    }
    */
}






