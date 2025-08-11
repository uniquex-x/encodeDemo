#ifndef RESOURCE_MONITOR_H
#define RESOURCE_MONITOR_H

#include <chrono>

/**
 * 跨平台资源监控类
 * 负责监控CPU和内存使用情况
 * 支持macOS、Linux和Windows平台
 */
class ResourceMonitor {
public:
    /**
     * 资源使用情况结构体
     */
    struct ResourceUsage {
        double cpuTime = 0.0;      // CPU使用时间(秒)
        long memoryUsage = 0;      // 内存使用量(字节)
        double cpuPercent = 0.0;   // CPU使用率(%)
    };

    ResourceMonitor();
    ~ResourceMonitor();

    // 开始监控
    void startMonitoring();
    
    // 停止监控并计算使用情况
    ResourceUsage stopMonitoring();
    
    // 获取当前资源使用情况(静态方法)
    static ResourceUsage getCurrentUsage();

private:
    bool monitoring;
    std::chrono::high_resolution_clock::time_point startTime;
    
#ifdef __APPLE__
    // macOS特定的数据结构
    struct rusage startUsage;
#elif defined(_WIN32)
    // Windows特定的数据结构
    void* processHandle;
    unsigned long long startKernelTime;
    unsigned long long startUserTime;
#else
    // Linux特定的数据结构
    struct rusage startUsage;
#endif

    // 平台特定的私有方法
    void getSystemUsage(ResourceUsage& usage) const;
    double calculateCpuPercent(double cpuTime, double wallTime) const;
};

#endif // RESOURCE_MONITOR_H 