#include "encoderTest.h"
#include "flacEncoder.h"
#include "vorbisEncoder.h"
#include "mp3Encoder.h"
#include <iostream>
#include <iomanip>
#include <sys/resource.h>
#include <chrono>

EncoderTest::EncoderTest(std::string testAudioFile_) : testAudioFile(testAudioFile_) {
    
}

EncoderTest::EncoderTest(EncoderType selectedEncoder, std::string testAudioFile_) : testAudioFile(testAudioFile_) {
    encoder = encoderCreate(selectedEncoder);
}

EncoderTest::~EncoderTest() {
    // 智能指针自动管理内存
}

void EncoderTest::testEncoder() {
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
    
    // 记录资源使用情况
    struct rusage usage_start, usage_end;
    getrusage(RUSAGE_SELF, &usage_start);
    
    // 执行编码
    EncodeResult result = encoder->encode(testAudioFile, outputFile);
    
    getrusage(RUSAGE_SELF, &usage_end);
    
    // 计算CPU和内存使用情况
    TestResult testResult;
    testResult.encoderName = encoder->getEncoderName();
    testResult.result = result;
    
    // 计算CPU使用时间 (用户时间 + 系统时间)
    double cpu_time = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) + 
                     (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
                     (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1000000.0 +
                     (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1000000.0;
    
    testResult.cpuUsage = cpu_time;
    testResult.memoryUsage = usage_end.ru_maxrss; // 最大内存使用量(KB)
    
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
                  << cpu_time << " seconds" << std::endl;
        std::cout << "最大内存使用: " << (testResult.memoryUsage / 1024) << " MB" << std::endl;
    } else {
        std::cout << "编码失败: " << result.errorMessage << std::endl;
    }
    std::cout << std::string(50, '-') << std::endl;
}

void EncoderTest::testAllEncoders() {
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

void EncoderTest::testSingleEncoder(EncoderType type) {
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
    
    // 记录资源使用情况
    struct rusage usage_start, usage_end;
    getrusage(RUSAGE_SELF, &usage_start);
    
    // 执行编码
    EncodeResult result = testEncoder->encode(testAudioFile, outputFile);
    
    getrusage(RUSAGE_SELF, &usage_end);
    
    // 计算资源使用情况
    TestResult testResult;
    testResult.encoderName = encoderName;
    testResult.result = result;
    
    double cpu_time = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) + 
                     (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
                     (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1000000.0 +
                     (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1000000.0;
    
    testResult.cpuUsage = cpu_time;
    testResult.memoryUsage = usage_end.ru_maxrss;
    
    results.push_back(testResult);
    
    if (result.success) {
        std::cout << "✓ 编码成功" << std::endl;
    } else {
        std::cout << "✗ 编码失败: " << result.errorMessage << std::endl;
    }
    std::cout << std::endl;
}

void EncoderTest::compareEncoders() {
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
                      << std::setw(12) << (result.memoryUsage / 1024) << std::endl;
        }
    }
    
    std::cout << std::endl;
    printResults();
}

void EncoderTest::printResults() {
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

std::unique_ptr<Encoder> EncoderTest::encoderCreate(EncoderType selectedEncoder) {
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