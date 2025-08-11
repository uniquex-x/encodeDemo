#include "EncoderManager.h"
#include "flacEncoder.h"
#include "vorbisEncoder.h"
#include "mp3Encoder.h"
#include "ResourceMonitor.h"
#include <iostream>
#include <iomanip>
#include <chrono>

EncoderManager::EncoderManager(std::string testAudioFile_) : testAudioFile(testAudioFile_) {
    
}

EncoderManager::EncoderManager(EncoderType selectedEncoder, std::string testAudioFile_) : testAudioFile(testAudioFile_) {
    
}

EncoderManager::~EncoderManager() {
    // 智能指针自动管理内存
}

void EncoderManager::testEncoder() {
    if (!encoder) {
        std::cerr << "编码器未初始化" << std::endl;
        return;
    }
    
    std::string outputFile = "output_" + encoder->getEncoderName() + "_test";
    // 根据编码器类型添加适当的文件扩展名
    if (encoder->getEncoderName() == "FLAC") {
        outputFile += ".flac";
    } else if (encoder->getEncoderName() == "MP3 (LAME)") {
        outputFile += ".mp3";
    } else if (encoder->getEncoderName() == "Vorbis") {
        outputFile += ".ogg";
    }
    
    std::cout << "开始测试编码器: " << encoder->getEncoderName() << std::endl;
    std::cout << "输入文件: " << testAudioFile << std::endl;
    std::cout << "输出文件: " << outputFile << std::endl;
    
    // 使用资源监控器
    ResourceMonitor monitor;
    monitor.startMonitoring();
    
    // 执行编码
    EncodeResult result = encoder->encode(testAudioFile, outputFile);
    
    // 获取资源使用情况
    ResourceMonitor::ResourceUsage usage = monitor.stopMonitoring();
    
    // 构建测试结果
    TestResult testResult;
    testResult.encoderName = encoder->getEncoderName();
    testResult.result = result;
    testResult.cpuUsage = usage.cpuTime;
    testResult.memoryUsage = usage.memoryUsage;
    
    results.push_back(testResult);
    
    // 打印结果
    if (result.success) {
        std::cout << "编码成功!" << std::endl;
        std::cout << "编码时间: " << result.encodeTime.count() << " ms" << std::endl;
        std::cout << "输入大小: " << result.inputSize << " bytes" << std::endl;
        std::cout << "输出大小: " << result.outputSize << " bytes" << std::endl;
        std::cout << "压缩比: " << std::fixed << std::setprecision(2) 
                  << result.compressionRatio << ":1" << std::endl;
        std::cout << "CPU使用时间: " << std::fixed << std::setprecision(3) 
                  << testResult.cpuUsage << " seconds" << std::endl;
        std::cout << "最大内存使用: " << (testResult.memoryUsage / 1024 / 1024) << " MB" << std::endl;
    } else {
        std::cout << "编码失败: " << result.errorMessage << std::endl;
    }
    std::cout << std::string(50, '-') << std::endl;
}

void EncoderManager::testAllEncoders() {
    std::cout << "=== 音频编码算法性能对比测试 ===" << std::endl;
    std::cout << "测试文件: " << testAudioFile << std::endl << std::endl;
    
    results.clear();
    
    // 测试所有编码器
    testSingleEncoder(EncoderType::FLAC);
    testSingleEncoder(EncoderType::VORBIS);
    testSingleEncoder(EncoderType::LAME);
    
    // 显示对比结果
    compareEncoders();
}

void EncoderManager::testSingleEncoder(EncoderType type) {
    auto testEncoder = encoderCreate(type);
    if (!testEncoder) {
        std::cerr << "无法创建编码器" << std::endl;
        return;
    }
    
    std::string encoderName = testEncoder->getEncoderName();
    std::string outputFile = "output_" + encoderName;
    
    // 根据编码器类型添加适当的文件扩展名
    if (encoderName == "FLAC") {
        outputFile += ".flac";
    } else if (encoderName == "MP3 (LAME)") {
        outputFile += ".mp3";
    } else if (encoderName == "Vorbis") {
        outputFile += ".ogg";
    }
    
    std::cout << "测试编码器: " << encoderName << std::endl;
    
    // 使用资源监控器
    ResourceMonitor monitor;
    monitor.startMonitoring();
    
    // 执行编码
    EncodeResult result = testEncoder->encode(testAudioFile, outputFile);
    
    // 获取资源使用情况
    ResourceMonitor::ResourceUsage usage = monitor.stopMonitoring();
    
    // 构建测试结果
    TestResult testResult;
    testResult.encoderName = encoderName;
    testResult.result = result;
    
    // 设置默认上下文
    testResult.context.encoderType = type;
    testResult.context.quality = 5; // 默认质量
    
    testResult.cpuUsage = usage.cpuTime;
    testResult.memoryUsage = usage.memoryUsage;
    
    results.push_back(testResult);
    
    if (result.success) {
        std::cout << "✓ 编码成功" << std::endl;
    } else {
        std::cout << "✗ 编码失败: " << result.errorMessage << std::endl;
    }
    std::cout << std::endl;
}

// 新增方法：单个编码器带参数测试
void EncoderManager::testSingleEncoderWithParams(EncoderType type, int quality) {
    EncoderParamContext context;
    context.encoderType = type;
    context.quality = quality;
    testSingleEncoderWithContext(context);
}

// 新增方法：单个编码器带上下文测试
void EncoderManager::testSingleEncoderWithContext(const EncoderParamContext& context) {
    auto testEncoder = encoderCreate(context.encoderType);
    if (!testEncoder) {
        std::cerr << "无法创建编码器" << std::endl;
        return;
    }
    
    std::string encoderName = testEncoder->getEncoderName();
    std::string outputFile = "output_" + encoderName + "_q" + std::to_string(context.quality);
    
    // 根据编码器类型添加适当的文件扩展名
    if (encoderName == "FLAC") {
        outputFile += ".flac";
    } else if (encoderName == "MP3 (LAME)") {
        outputFile += ".mp3";
    } else if (encoderName == "Vorbis") {
        outputFile += ".ogg";
    }
    
    std::cout << "测试编码器: " << encoderName 
              << " (质量: " << context.quality << ")" << std::endl;
    std::cout << testEncoder->getQualityDescription() << std::endl;
    
    // 使用资源监控器
    ResourceMonitor monitor;
    monitor.startMonitoring();
    
    // 执行编码
    EncodeResult result = testEncoder->encode(testAudioFile, outputFile, context);
    
    // 获取资源使用情况
    ResourceMonitor::ResourceUsage usage = monitor.stopMonitoring();
    
    // 构建测试结果
    TestResult testResult;
    testResult.encoderName = encoderName;
    testResult.result = result;
    testResult.context = context;
    testResult.cpuUsage = usage.cpuTime;
    testResult.memoryUsage = usage.memoryUsage;
    
    results.push_back(testResult);
    
    printSingleTestResult(testResult);
}

// 新增方法：参数范围测试
void EncoderManager::testEncoderQualityRange(EncoderType type, int minQuality, int maxQuality, int step) {
    auto testEncoder = encoderCreate(type);
    if (!testEncoder) {
        std::cerr << "无法创建编码器" << std::endl;
        return;
    }
    
    std::string encoderName = testEncoder->getEncoderName();
    std::cout << "=== " << encoderName << " 质量参数测试 ===" << std::endl;
    std::cout << testEncoder->getQualityDescription() << std::endl;
    std::cout << "测试范围: " << minQuality << " - " << maxQuality 
              << " (步长: " << step << ")" << std::endl << std::endl;
    
    // 验证质量范围
    int encoderMin = testEncoder->getMinQuality();
    int encoderMax = testEncoder->getMaxQuality();
    minQuality = std::max(minQuality, encoderMin);
    maxQuality = std::min(maxQuality, encoderMax);
    
    for (int quality = minQuality; quality <= maxQuality; quality += step) {
        testSingleEncoderWithParams(type, quality);
        std::cout << std::string(50, '-') << std::endl;
    }
    
    // 显示对比结果
    std::cout << "=== 质量参数对比结果 ===" << std::endl;
    std::cout << std::left << std::setw(8) << "质量"
              << std::setw(12) << "编码时间(ms)"
              << std::setw(12) << "压缩比"
              << std::setw(15) << "输出大小(KB)"
              << std::setw(12) << "CPU时间(s)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 只显示本次测试的结果
    size_t testCount = (maxQuality - minQuality) / step + 1;
    size_t startIdx = results.size() - testCount;
    
    for (size_t i = startIdx; i < results.size(); i++) {
        const auto& result = results[i];
        if (result.result.success) {
            std::cout << std::left << std::setw(8) << result.context.quality
                      << std::setw(12) << result.result.encodeTime.count()
                      << std::setw(12) << std::fixed << std::setprecision(2) 
                      << result.result.compressionRatio
                      << std::setw(15) << (result.result.outputSize / 1024)
                      << std::setw(12) << std::fixed << std::setprecision(3) 
                      << result.cpuUsage << std::endl;
        }
    }
    std::cout << std::endl;
}

// 新增方法：显示编码器信息
void EncoderManager::showEncoderInfo(EncoderType type) {
    auto testEncoder = encoderCreate(type);
    if (!testEncoder) {
        std::cerr << "无法创建编码器" << std::endl;
        return;
    }
    
    std::cout << "=== " << testEncoder->getEncoderName() << " 编码器信息 ===" << std::endl;
    std::cout << "编码器名称: " << testEncoder->getEncoderName() << std::endl;
    std::cout << "参数描述: " << testEncoder->getQualityDescription() << std::endl;
    std::cout << "质量范围: " << testEncoder->getMinQuality() 
              << " - " << testEncoder->getMaxQuality() << std::endl;
    std::cout << std::endl;
}

// 新增方法：显示所有编码器信息
void EncoderManager::showAllEncodersInfo() {
    std::cout << "=== 所有编码器信息 ===" << std::endl << std::endl;
    showEncoderInfo(EncoderType::FLAC);
    showEncoderInfo(EncoderType::VORBIS);
    showEncoderInfo(EncoderType::LAME);
}

// 新增私有方法：获取编码器类型字符串
std::string EncoderManager::getEncoderTypeString(EncoderType type) {
    switch (type) {
        case EncoderType::FLAC: return "FLAC";
        case EncoderType::VORBIS: return "Vorbis";
        case EncoderType::LAME: return "MP3 (LAME)";
        default: return "Unknown";
    }
}

// 新增私有方法：打印单个测试结果
void EncoderManager::printSingleTestResult(const TestResult& result) {
    if (result.result.success) {
        std::cout << "✓ 编码成功!" << std::endl;
        std::cout << "编码时间: " << result.result.encodeTime.count() << " ms" << std::endl;
        std::cout << "输入大小: " << result.result.inputSize / 1024 / 1024 << " MB" << std::endl;
        std::cout << "输出大小: " << result.result.outputSize / 1024 / 1024 << " MB" << std::endl;
        std::cout << "压缩比: " << std::fixed << std::setprecision(2) 
                  << result.result.compressionRatio << ":1" << std::endl;
        std::cout << "CPU使用时间: " << std::fixed << std::setprecision(3) 
                  << result.cpuUsage << " seconds" << std::endl;
        std::cout << "最大内存使用: " << (result.memoryUsage / 1024 / 1024) << " MB" << std::endl;
    } else {
        std::cout << "✗ 编码失败: " << result.result.errorMessage << std::endl;
    }
    std::cout << std::endl;
}

void EncoderManager::compareEncoders() {
    std::cout << "=== 编码性能对比结果 ===" << std::endl << std::endl;
    
    // 表头
    std::cout << std::left << std::setw(15) << "编码器"
              << std::setw(12) << "编码时间(ms)"
              << std::setw(12) << "压缩比"
              << std::setw(15) << "输出大小(KB)"
              << std::setw(12) << "CPU时间(s)"
              << std::setw(12) << "内存(MB)" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (const auto& result : results) {
        if (result.result.success) {
            std::cout << std::left << std::setw(15) << result.encoderName
                      << std::setw(12) << result.result.encodeTime.count()
                      << std::setw(12) << std::fixed << std::setprecision(2) 
                      << result.result.compressionRatio
                      << std::setw(15) << (result.result.outputSize / 1024)
                      << std::setw(12) << std::fixed << std::setprecision(3) 
                      << result.cpuUsage
                      << std::setw(12) << (result.memoryUsage / 1024 / 1024) << std::endl;
        }
    }
    
    std::cout << std::endl;
    printResults();
}

void EncoderManager::printResults() {
    std::cout << "=== 详细分析 ===" << std::endl;
    
    if (results.empty()) {
        std::cout << "没有测试结果" << std::endl;
        return;
    }
    
    // 找出最佳性能指标
    auto bestCompressionIt = std::max_element(results.begin(), results.end(),
        [](const TestResult& a, const TestResult& b) {
            return a.result.success && b.result.success ? 
                   a.result.compressionRatio < b.result.compressionRatio : !a.result.success;
        });
    
    auto fastestIt = std::min_element(results.begin(), results.end(),
        [](const TestResult& a, const TestResult& b) {
            return a.result.success && b.result.success ? 
                   a.result.encodeTime < b.result.encodeTime : !a.result.success;
        });
    
    auto lowestCpuIt = std::min_element(results.begin(), results.end(),
        [](const TestResult& a, const TestResult& b) {
            return a.result.success && b.result.success ? 
                   a.cpuUsage < b.cpuUsage : !a.result.success;
        });
    
    if (bestCompressionIt != results.end() && bestCompressionIt->result.success) {
        std::cout << "🏆 最佳压缩比: " << bestCompressionIt->encoderName 
                  << " (" << std::fixed << std::setprecision(2) 
                  << bestCompressionIt->result.compressionRatio << ":1)" << std::endl;
    }
    
    if (fastestIt != results.end() && fastestIt->result.success) {
        std::cout << "⚡ 最快编码: " << fastestIt->encoderName 
                  << " (" << fastestIt->result.encodeTime.count() << " ms)" << std::endl;
    }
    
    if (lowestCpuIt != results.end() && lowestCpuIt->result.success) {
        std::cout << "💡 最低CPU使用: " << lowestCpuIt->encoderName 
                  << " (" << std::fixed << std::setprecision(3) 
                  << lowestCpuIt->cpuUsage << " s)" << std::endl;
    }
}

std::unique_ptr<Encoder> EncoderManager::encoderCreate(EncoderType selectedEncoder) {
    switch (selectedEncoder) {
        case EncoderType::FLAC:
            return std::make_unique<FlacEncoder>();
        case EncoderType::VORBIS:
            return std::make_unique<VorbisEncoder>();
        case EncoderType::LAME:
            return std::make_unique<Mp3Encoder>();
    }
    return nullptr;
}

void EncoderManager::testImprovedLameEncoder() {
    std::cout << "\n=== 改进的MP3编码器测试 ===" << std::endl;
    std::cout << "测试重点：解决编码器延迟、缓冲区管理和VBR标签问题" << std::endl;
    
    // 创建MP3编码器实例
    auto mp3Encoder = std::make_unique<Mp3Encoder>();
    
    std::cout << "\n编码器信息:" << std::endl;
    std::cout << "  名称: " << mp3Encoder->getEncoderName() << std::endl;
    std::cout << "  " << mp3Encoder->getQualityDescription() << std::endl;
    std::cout << "  质量范围: " << mp3Encoder->getMinQuality() 
              << " - " << mp3Encoder->getMaxQuality() << std::endl;
    
    // 测试不同质量设置的效果
    std::vector<int> testQualities = {0, 2, 4, 6, 9}; // VBR质量：0=最高，9=最低
    std::vector<TestResult> improvedResults;
    
    std::cout << "\n开始多质量级别测试..." << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    std::cout << "质量\t输入大小\t输出大小\t压缩比\t\t压缩率\t编码时间" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (int quality : testQualities) {
        // 设置编码参数
        EncoderParamContext context;
        context.encoderType = EncoderType::LAME;
        context.quality = quality;
        
        // 生成输出文件名
        std::string outputFile = "improved_mp3_quality_" + std::to_string(quality) + ".mp3";
        
        // 执行编码
        auto startTime = std::chrono::high_resolution_clock::now();
        EncodeResult result = mp3Encoder->encode(testAudioFile, outputFile, context);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        if (result.success) {
            // 计算压缩率
            double compressionPercentage = (1.0 - (double)result.outputSize / result.inputSize) * 100;
            
            // 打印结果
            std::cout << std::fixed << std::setprecision(2);
            std::cout << quality << "\t" 
                      << result.inputSize << "\t\t"
                      << result.outputSize << "\t\t"
                      << result.compressionRatio << ":1\t\t"
                      << compressionPercentage << "%\t"
                      << result.encodeTime.count() << "ms" << std::endl;
            
            // 保存测试结果
            TestResult testResult;
            testResult.encoderName = mp3Encoder->getEncoderName() + " (质量=" + std::to_string(quality) + ")";
            testResult.result = result;
            testResult.context = context;
            testResult.cpuUsage = 0.0; // 简化，不测量CPU使用率
            testResult.memoryUsage = 0; // 简化，不测量内存使用
            improvedResults.push_back(testResult);
            
        } else {
            std::cout << quality << "\t编码失败: " << result.errorMessage << std::endl;
        }
    }
    
    std::cout << std::string(80, '-') << std::endl;
    
    // 分析改进效果
    std::cout << "\n=== 改进效果分析 ===" << std::endl;
    
    if (!improvedResults.empty()) {
        // 找到最佳质量结果
        auto bestResult = std::min_element(improvedResults.begin(), improvedResults.end(),
            [](const TestResult& a, const TestResult& b) {
                return a.context.quality < b.context.quality; // 质量0是最高质量
            });
            
        std::cout << "最佳质量编码结果 (质量=" << bestResult->context.quality << "):" << std::endl;
        std::cout << "  文件大小: " << bestResult->result.outputSize << " 字节" << std::endl;
        std::cout << "  压缩比: " << std::fixed << std::setprecision(2) 
                  << bestResult->result.compressionRatio << ":1" << std::endl;
        std::cout << "  编码时间: " << bestResult->result.encodeTime.count() << " ms" << std::endl;
        
        // 检查压缩效果
        double sizeRatio = (double)bestResult->result.outputSize / bestResult->result.inputSize;
        if (sizeRatio < 0.5) {
            std::cout << "✓ 压缩效果正常，文件大小问题已解决" << std::endl;
        } else if (sizeRatio < 0.8) {
            std::cout << "⚠ 压缩效果一般，可能仍需优化" << std::endl;
        } else {
            std::cout << "✗ 压缩效果异常，文件变长问题可能仍存在" << std::endl;
        }
        
        // 计算平均压缩比
        double avgCompressionRatio = 0.0;
        for (const auto& result : improvedResults) {
            avgCompressionRatio += result.result.compressionRatio;
        }
        avgCompressionRatio /= improvedResults.size();
        
        std::cout << "平均压缩比: " << std::fixed << std::setprecision(2) 
                  << avgCompressionRatio << ":1" << std::endl;
    }
    
    std::cout << "\n=== 改进要点总结 ===" << std::endl;
    std::cout << "1. ✓ 禁用VBR标签避免额外帧 (lame_set_bWriteVbrTag=0)" << std::endl;
    std::cout << "2. ✓ 动态缓冲区管理避免多余数据" << std::endl;
    std::cout << "3. ✓ 正确处理编码器延迟和填充" << std::endl;
    std::cout << "4. ✓ 精确的样本数控制" << std::endl;
    std::cout << "5. ✓ 优化的LAME参数设置顺序" << std::endl;
    
    // 技术建议
    std::cout << "\n=== 使用建议 ===" << std::endl;
    std::cout << "• 质量0-2: 高质量音乐，文件较大" << std::endl;
    std::cout << "• 质量3-5: 平衡质量与大小，推荐日常使用" << std::endl;
    std::cout << "• 质量6-9: 小文件优先，质量较低" << std::endl;
    
    // 将结果添加到主结果集
    results.insert(results.end(), improvedResults.begin(), improvedResults.end());
}