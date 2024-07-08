#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <vector>
#include "bytehook.h"
#include <android/log.h>
#include <unistd.h>
#include <unwind.h>
#include <inttypes.h>
#include <linux/elf.h>
#include <sys/mman.h>
#include <iostream>
#include <fstream>
#include <dlfcn.h>

bool thread_hooked = false;


namespace {
    struct BacktraceState {
        void **current;
        void **end;
    };

    static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context *context, void *arg) {
        BacktraceState *state = static_cast<BacktraceState *>(arg);
        uintptr_t pc = _Unwind_GetIP(context);
        if (pc) {
            if (state->current == state->end) {
                return _URC_END_OF_STACK;
            } else {
                *state->current++ = (void *)pc;
            }
        }
        return _URC_NO_REASON;
    }
}

static void saveStackBySp(void *secret) {
    // 获取上下文信息
    ucontext_t *uc = (ucontext_t *)secret;
    int i = 0;
    Dl_info  dl_info;
    const void **frame_pointer = (const void **)uc->uc_mcontext.arm_fp;
    const void *return_address = (const void *)uc->uc_mcontext.arm_pc;
    printf("\nStack trace:");
    while (return_address) {
        memset(&dl_info, 0, sizeof(Dl_info));
        if (!dladdr((void *)return_address, &dl_info))        break;
        const char *sname = dl_info.dli_sname;
        //打印函数调用的信息，包括计数器、返回地址、函数名、偏移量和文件名
        printf("%02d: %p <%s + %u> (%s)", ++i, return_address, sname,
               ((uintptr_t)return_address - (uintptr_t)dl_info.dli_saddr),
               dl_info.dli_fname);
        //如果帧指针为空指针，表示已经到达调用链的终点，因此跳出循环
        if (!frame_pointer)        break;
        //获取上一个函数的LR的值，即返回地址
        return_address = frame_pointer[-1];
        //获取上一个函数的FP的值
        frame_pointer = (const void **)frame_pointer[-2];

    }
    printf("Stack trace end.");
}


void dumpBacktrace(void **buffer, size_t depth) {
    for (size_t idx = 0; idx < depth; ++idx) {
        void *addr = buffer[idx];
        Dl_info info;
        if (dladdr(addr, &info)) {
            const uintptr_t addr_relative =
                    ((uintptr_t) addr - (uintptr_t) info.dli_fbase);
            __android_log_print(ANDROID_LOG_DEBUG, "MallocHook",
                                "# %d : %p : %s(%p)(%s)(%p)",
                                idx,
                                addr, info.dli_fname, addr_relative, info.dli_sname,
                                info.dli_saddr);
        }
    }
}

void printNativeStack() {
    const size_t max = 30; // 调用的层数
    void *buffer[max];
    BacktraceState state = {buffer, buffer + max};
    //捕获堆栈
    _Unwind_Backtrace(unwindCallback, &state);
    //返回堆栈的深度
    size_t depth =  state.current - buffer;
    //打印堆栈
    dumpBacktrace(buffer, depth);
}

void *malloc_hook(size_t len) {
    BYTEHOOK_STACK_SCOPE();
    if (len > 1024 * 1024) {
        __android_log_print(ANDROID_LOG_DEBUG, "MallocHook", "size:%d", len);
        //打印native堆栈
        printNativeStack();
    }

    return BYTEHOOK_CALL_PREV(malloc_hook, len);
}

ssize_t my_write(int fd, const void* const buf, size_t count) {
    BYTEHOOK_STACK_SCOPE();
    if (buf != nullptr) {
        char *content = (char *) buf;
        std::ofstream file("/data/data/com.example.performance_optimize/example_anr.txt", std::ios::app);
        if (file.is_open()) {
            file << content;
            file.close();
        }
    }
    return BYTEHOOK_CALL_PREV(my_write,fd, buf,count);
}


typedef struct {
    uint64_t flags;
    void *reserved_addr;
    size_t reserved_size;
    int relro_fd;
    int library_fd;
    off64_t library_fd_offset;
    struct android_namespace_t *library_namespace;
} android_dlextinfo;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_memory_NativeLeakActivity_hookMallocByBHook(
        JNIEnv *env,
        jobject thiz) {

    bytehook_stub_t stub = bytehook_hook_single(
            "libexample.so",
            nullptr,
            "malloc",
            (void *)malloc_hook,
            nullptr,
            nullptr);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_stability_StabilityExampleActivity_hookAnrByBHook(
        JNIEnv *env,
        jobject thiz) {

    bytehook_hook_all(
            "libc.so",
            "write",
            (void *)my_write,
            nullptr,
            nullptr);
}




uintptr_t originFunc;

void *malloc_hook_by_plt(size_t len) {
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "origin malloc size:%d", len);
    if(len > 20*1024*1024){
        __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "do somethings");
        printNativeStack();
    }
    // 调用原函数
    return reinterpret_cast<void *(*)(size_t)>((void*)originFunc)(len);
}

// 根据st_value获取符号名称
std::string getSymbolNameByValue(uintptr_t base_addr , Elf32_Sym *sym) {
    Elf32_Ehdr *header = (Elf32_Ehdr *) (base_addr);
    // 获取段头部表的地址
    __android_log_print(ANDROID_LOG_DEBUG, "getSymbolNameByValue", "base_addr %p", base_addr);
    Elf32_Shdr *seg_table = (Elf32_Shdr *) (base_addr + header->e_shoff);
    // 段的数量
    size_t seg_count = header->e_shnum;
    __android_log_print(ANDROID_LOG_DEBUG, "getSymbolNameByValue", "seg_table2 %p", seg_table);
    Elf32_Shdr* stringTableHeader = nullptr;
    for (int i = 0; i < seg_count ; i++) {
        if (seg_table[i].sh_type == SHT_STRTAB) {
            __android_log_print(ANDROID_LOG_DEBUG, "getSymbolNameByValue", "sh_type %d ,%p", seg_table[i].sh_type,seg_table[i].sh_addr);
            stringTableHeader = &seg_table[i];
            break;
        }
    }
    char* stringTable = (char*)(base_addr + stringTableHeader->sh_offset);

    std::string symbolName = std::string(stringTable + sym->st_name);
    return symbolName;
}

extern "C"
[[noreturn]] JNIEXPORT void JNICALL
Java_com_example_performance_1optimize_memory_NativeLeakActivity_hookMallocByPLTHook(JNIEnv *env, jobject thiz) {
    FILE *fp = fopen("/proc/self/maps", "r");
    char line[1024];
    uintptr_t base_addr = 0;
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "1");
    while (fgets(line, sizeof(line), fp)) {
        __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "line:%s", line);
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

    //将 base_addr 强制转换成Elf_Ehdr格式
    Elf32_Ehdr *header = (Elf32_Ehdr *) (base_addr);


    size_t phr_count = header->e_phnum;  // 程序头表项个数
    Elf32_Phdr *phdr_table = (Elf32_Phdr *) (base_addr + header->e_phoff);  // 程序头部表的地址
    unsigned long dynamicAddr = 0;
    unsigned int dynamicSize = 0;
    for (int i = 0; i < phr_count; i++) {
        if (phdr_table[i].p_type == PT_DYNAMIC) {
            //so基地址加dynamic段的偏移地址，就是dynamic段的实际地址
            dynamicAddr = phdr_table[i].p_vaddr + base_addr;
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "dynamicBashAddr:%p",
                                (void *) phdr_table[i].p_vaddr);
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "dynamicAddr:%p",
                                dynamicAddr);
            dynamicSize = phdr_table[i].p_memsz;
            break;
        }
    }

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

    //读写权限改为可写
    mprotect((void *) (symbolTableAddr & ~(PAGE_SIZE - 1)), PAGE_SIZE,PROT_READ | PROT_WRITE);
    //目标函数偏移地址
    originFunc = 0x52474 + base_addr;
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "originFunc:%p",originFunc);
    //替换的hook函数的偏移地址
    uintptr_t newFunc = (uintptr_t) &malloc_hook_by_plt;
    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "newFunc:%p", newFunc);

    int *symbolTable = (int *) symbolTableAddr;
    for (int i = 0;; i++) {
        if ((uintptr_t) &symbolTable[i] == originFunc) {
            originFunc = symbolTable[i];
            symbolTable[i] = newFunc;
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "match origin:%p",&symbolTable[i]);
            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "match new:%p",symbolTable[i]);
            break;
        }
    }

//
//    Dl_info info;
//    if (dladdr((void *)originFunc, &info)) {
//        __android_log_print(ANDROID_LOG_DEBUG, "originFunc symbol",
//                            "%p : %s(%s)(%p)",
//                            originFunc, info.dli_fname, info.dli_sname,
//                            info.dli_saddr);
//    }
//
//    Elf32_Rel *rela;
//    Elf32_Sym *sym;
//    size_t pltrel_size = 0;
//    // 遍历动态信息表，查找DT_JMPREL和DT_PLTRELSZ标签
//    for (int i = 0; i < dynamicSize; i++) {
//        if (dynamic_table[i].d_tag == DT_JMPREL) {
//            rela = (Elf32_Rel *)(dynamic_table[i].d_un.d_ptr + base_addr);
//            __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "DT_PLTRELSZ2 size:%d", dynamic_table[i].d_un.d_val);
//        } else if(dynamic_table[i].d_tag == DT_PLTRELSZ){
//            pltrel_size = dynamic_table[i].d_un.d_val;
//        } else if(dynamic_table[i].d_tag == DT_SYMTAB){
//            sym = (Elf32_Sym *)(dynamic_table[i].d_un.d_val + base_addr);
//        }
//    }
//    __android_log_print(ANDROID_LOG_DEBUG, "hookMallocByPLTHook", "rela:%p,size:%d,sym:%p", rela,pltrel_size,sym);
//    // 遍历重定位表，找到函数的重定位项
//    size_t entries = pltrel_size / sizeof(Elf32_Rel);
//    for (size_t i = 0; i < entries; ++i) {
//        Elf32_Rel *reloc = &rela[i];
//        size_t symbol_index = ELF32_R_SYM(reloc->r_info);
//        // 根据symbol_index获取符号表中的符号信息
//        std::string name = getSymbolNameByValue(base_addr,&sym[symbol_index]);
//        __android_log_print(ANDROID_LOG_DEBUG, "get symbol",
//                            "%d,name:(%p)(%s)(%p)",
//                            symbol_index,&sym[symbol_index].st_value,name.c_str(),reloc->r_offset);
//        if(name.find("malloc")!= std::string::npos){
//            originFunc = reloc->r_offset + base_addr;
//            break;
//        }
//    }

}






