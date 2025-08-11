#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <memory>
#include <vector>
#include <string>
#include "EncoderInfo.h"
#include "encoder.h"

struct TestResult {
    std::string encoderName;
    EncodeResult result;
    double cpuUsage;
    size_t memoryUsage;
    EncoderParamContext context;  // 添加编码上下文信息
};

class EncoderManager {
public:
    EncoderManager(std::string testAudioFile_);
    EncoderManager(EncoderType selectedEncoder, std::string testAudioFile_);
    ~EncoderManager();

    // 原有方法
    void testEncoder();
    void testAllEncoders();
    void compareEncoders();
    std::vector<TestResult> getResults() const { return results; }
    
    // 新增方法：单个编码器测试
    void testSingleEncoderWithParams(EncoderType type, int quality);
    void testSingleEncoderWithContext(const EncoderParamContext& context);
    
    // 新增方法：参数范围测试
    void testEncoderQualityRange(EncoderType type, int minQuality, int maxQuality, int step = 1);
    
    // 新增方法：显示编码器信息
    void showEncoderInfo(EncoderType type);
    void showAllEncodersInfo();
    
    // 新增方法：测试改进的MP3编码器
    void testImprovedLameEncoder();
    
    // 工厂方法，创建编码器实例
    std::unique_ptr<Encoder> encoderCreate(EncoderType selectedEncoder);

private:
    void testSingleEncoder(EncoderType type);
    void printResults();
    
    // 新增私有方法
    std::string getEncoderTypeString(EncoderType type);
    void printSingleTestResult(const TestResult& result);

private:
    std::unique_ptr<Encoder> encoder;
    std::vector<TestResult> results;
    std::string testAudioFile;
};

#endif