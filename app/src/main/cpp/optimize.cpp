/*
 * optimize.cpp - CPU性能优化工具
 * 
 * 本文件包含Android设备CPU性能检测和优化相关的工具函数
 * 主要用于获取CPU核心数量和最大频率信息，为性能优化提供基础数据
 */

#include <jni.h>          // JNI接口头文件，用于与Java层交互
#include <android/log.h>  // Android日志系统头文件
#include <dirent.h>       // 目录操作头文件，用于读取文件系统目录
#include <string>         // C++字符串操作头文件

/*
 * 获取CPU核心数量
 * 
 * 通过读取 /sys/devices/system/cpu/ 目录下的文件来统计CPU核心数量
 * 该目录下每个cpu[数字]格式的文件夹都代表一个CPU核心
 * 
 * 返回值：
 *   - 成功：返回CPU核心数量
 *   - 失败：返回0
 */
int getNumberOfCPUCores() {
    int cores = 0;                  // 初始化核心数量计数器
    DIR *dir;                       // 目录指针
    struct dirent *ent;             // 目录项结构体指针
    
    // 打开系统CPU信息目录
    if ((dir = opendir("/sys/devices/system/cpu/")) != NULL) {
        // 遍历目录下的所有文件和子目录
        while ((ent = readdir(dir)) != NULL) {
            std::string path = ent->d_name;  // 获取文件名
            
            // 检查文件名是否以"cpu"开头
            if (path.find("cpu") == 0) {
                bool isCore = true;
                
                // 检查"cpu"后面是否都是数字（cpu0, cpu1, cpu2...）
                // 从第4个字符开始检查（跳过"cpu"三个字符）
                for (int i = 3; i < path.length(); i++) {
                    if (path[i] < '0' || path[i] > '9') {
                        isCore = false;  // 如果包含非数字字符，则不是CPU核心
                        break;
                    }
                }
                
                // 如果是有效的CPU核心，增加计数
                if (isCore) {
                    cores++;
                }
            }
        }
        closedir(dir);  // 关闭目录
    }
    return cores;  // 返回检测到的CPU核心数量
}

/*
 * 获取CPU最大频率
 * 
 * 遍历所有CPU核心，读取每个核心的最大频率信息
 * 返回所有核心中的最大频率值
 * 
 * 返回值：
 *   - 成功：返回CPU最大频率（单位：KHz）
 *   - 失败：返回-1
 */
int getMaxFreqCPU() {
    int maxFreq = -1;  // 初始化最大频率为-1（表示未找到）
    
    // 遍历所有CPU核心
    for (int i = 0; i < getNumberOfCPUCores(); i++) {
        // 构建CPU频率信息文件路径
        // 格式：/sys/devices/system/cpu/cpu[编号]/cpufreq/cpuinfo_max_freq
        std::string filename = "/sys/devices/system/cpu/cpu" +
                               std::to_string(i) + "/cpufreq/cpuinfo_max_freq";
        
        // 打开频率信息文件
        FILE* cpuInfoMaxFreqFile = fopen(filename.c_str(), "r");
        if (cpuInfoMaxFreqFile != nullptr) {
            char line[256];  // 缓冲区用于读取文件内容
            
            // 读取文件的第一行（频率值）
            if (fgets(line, sizeof(line), cpuInfoMaxFreqFile) != nullptr) {
                try {
                    // 将字符串转换为整数（频率值）
                    int freqBound = std::stoi(line);
                    
                    // 更新最大频率
                    if (freqBound > maxFreq) {
                        maxFreq = freqBound;
                    }
                } catch (const std::exception& e) {
                    // 如果字符串转换失败，忽略此核心的频率信息
                    // 这里故意留空，继续处理下一个核心
                }
            }
            fclose(cpuInfoMaxFreqFile);  // 关闭文件
        }
    }
    return maxFreq;  // 返回检测到的最大频率
}
