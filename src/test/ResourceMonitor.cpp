#include "ResourceMonitor.h"

#ifdef __APPLE__
    #include <sys/resource.h>
    #include <mach/mach.h>
    #include <mach/task.h>
#elif defined(_WIN32)
    #include <windows.h>
    #include <psapi.h>
#else
    #include <sys/resource.h>
    #include <unistd.h>
    #include <fstream>
    #include <sstream>
#endif

ResourceMonitor::ResourceMonitor() : monitoring(false) {
#ifdef _WIN32
    processHandle = GetCurrentProcess();
#endif
}

ResourceMonitor::~ResourceMonitor() {
    // 析构函数无需特殊处理
}

void ResourceMonitor::startMonitoring() {
    monitoring = true;
    startTime = std::chrono::high_resolution_clock::now();
    
#ifdef __APPLE__
    getrusage(RUSAGE_SELF, &startUsage);
#elif defined(_WIN32)
    FILETIME creationTime, exitTime, kernelTime, userTime;
    GetProcessTimes((HANDLE)processHandle, &creationTime, &exitTime, &kernelTime, &userTime);
    
    startKernelTime = ((unsigned long long)kernelTime.dwHighDateTime << 32) + kernelTime.dwLowDateTime;
    startUserTime = ((unsigned long long)userTime.dwHighDateTime << 32) + userTime.dwLowDateTime;
#else
    getrusage(RUSAGE_SELF, &startUsage);
#endif
}

ResourceMonitor::ResourceUsage ResourceMonitor::stopMonitoring() {
    if (!monitoring) {
        return ResourceUsage();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto wallTime = std::chrono::duration<double>(endTime - startTime).count();
    
    ResourceUsage usage;
    getSystemUsage(usage);
    
    // 计算相对于开始时间的增量
#ifdef __APPLE__
    struct rusage endUsage;
    getrusage(RUSAGE_SELF, &endUsage);
    
    double userTime = (endUsage.ru_utime.tv_sec - startUsage.ru_utime.tv_sec) + 
                      (endUsage.ru_utime.tv_usec - startUsage.ru_utime.tv_usec) / 1000000.0;
    double sysTime = (endUsage.ru_stime.tv_sec - startUsage.ru_stime.tv_sec) + 
                     (endUsage.ru_stime.tv_usec - startUsage.ru_stime.tv_usec) / 1000000.0;
    
    usage.cpuTime = userTime + sysTime;
    usage.memoryUsage = endUsage.ru_maxrss; // macOS返回字节数
    
#elif defined(_WIN32)
    FILETIME creationTime, exitTime, kernelTime, userTime;
    GetProcessTimes((HANDLE)processHandle, &creationTime, &exitTime, &kernelTime, &userTime);
    
    unsigned long long endKernelTime = ((unsigned long long)kernelTime.dwHighDateTime << 32) + kernelTime.dwLowDateTime;
    unsigned long long endUserTime = ((unsigned long long)userTime.dwHighDateTime << 32) + userTime.dwLowDateTime;
    
    // 转换为秒 (FILETIME单位是100纳秒)
    usage.cpuTime = ((endKernelTime - startKernelTime) + (endUserTime - startUserTime)) / 10000000.0;
    
    // 获取内存使用量
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo((HANDLE)processHandle, &pmc, sizeof(pmc))) {
        usage.memoryUsage = pmc.WorkingSetSize;
    }
    
#else
    struct rusage endUsage;
    getrusage(RUSAGE_SELF, &endUsage);
    
    double userTime = (endUsage.ru_utime.tv_sec - startUsage.ru_utime.tv_sec) + 
                      (endUsage.ru_utime.tv_usec - startUsage.ru_utime.tv_usec) / 1000000.0;
    double sysTime = (endUsage.ru_stime.tv_sec - startUsage.ru_stime.tv_sec) + 
                     (endUsage.ru_stime.tv_usec - startUsage.ru_stime.tv_usec) / 1000000.0;
    
    usage.cpuTime = userTime + sysTime;
    usage.memoryUsage = endUsage.ru_maxrss * 1024; // Linux返回KB，转换为字节
#endif

    usage.cpuPercent = calculateCpuPercent(usage.cpuTime, wallTime);
    monitoring = false;
    
    return usage;
}

ResourceMonitor::ResourceUsage ResourceMonitor::getCurrentUsage() {
    ResourceUsage usage;
    
#ifdef __APPLE__
    struct rusage currentUsage;
    getrusage(RUSAGE_SELF, &currentUsage);
    
    usage.cpuTime = currentUsage.ru_utime.tv_sec + currentUsage.ru_utime.tv_usec / 1000000.0 +
                    currentUsage.ru_stime.tv_sec + currentUsage.ru_stime.tv_usec / 1000000.0;
    usage.memoryUsage = currentUsage.ru_maxrss;
    
    // 获取更精确的内存使用量 (通过mach API)
    task_basic_info_data_t taskInfo;
    mach_msg_type_number_t infoCount = TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO, 
                  (task_info_t)&taskInfo, &infoCount) == KERN_SUCCESS) {
        usage.memoryUsage = taskInfo.resident_size;
    }
    
#elif defined(_WIN32)
    HANDLE process = GetCurrentProcess();
    FILETIME creationTime, exitTime, kernelTime, userTime;
    
    if (GetProcessTimes(process, &creationTime, &exitTime, &kernelTime, &userTime)) {
        unsigned long long kernelTimeMs = ((unsigned long long)kernelTime.dwHighDateTime << 32) + kernelTime.dwLowDateTime;
        unsigned long long userTimeMs = ((unsigned long long)userTime.dwHighDateTime << 32) + userTime.dwLowDateTime;
        
        usage.cpuTime = (kernelTimeMs + userTimeMs) / 10000000.0;
    }
    
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
        usage.memoryUsage = pmc.WorkingSetSize;
    }
    
#else
    struct rusage currentUsage;
    getrusage(RUSAGE_SELF, &currentUsage);
    
    usage.cpuTime = currentUsage.ru_utime.tv_sec + currentUsage.ru_utime.tv_usec / 1000000.0 +
                    currentUsage.ru_stime.tv_sec + currentUsage.ru_stime.tv_usec / 1000000.0;
    usage.memoryUsage = currentUsage.ru_maxrss * 1024;
    
    // 尝试从/proc/self/status获取更准确的内存使用量
    std::ifstream statusFile("/proc/self/status");
    std::string line;
    while (std::getline(statusFile, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string dummy;
            int memKB;
            iss >> dummy >> memKB;
            usage.memoryUsage = memKB * 1024;
            break;
        }
    }
#endif
    
    return usage;
}

void ResourceMonitor::getSystemUsage(ResourceUsage& usage) const {
    usage = getCurrentUsage();
}

double ResourceMonitor::calculateCpuPercent(double cpuTime, double wallTime) const {
    if (wallTime <= 0) return 0.0;
    
    // CPU使用率 = (CPU时间 / 实际经过时间) * 100%
    double percent = (cpuTime / wallTime) * 100.0;
    
    // 限制在合理范围内 (考虑多核情况，可能超过100%)
    return std::min(percent, 800.0); // 最多8核满载
} 