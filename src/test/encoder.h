#ifndef ENCODER_H
#define ENCODER_H

#include <string>
#include <chrono>
#include "EncoderInfo.h"

struct EncodeResult {
    bool success;
    std::string outputFile;
    size_t inputSize;
    size_t outputSize;
    double compressionRatio;
    std::chrono::milliseconds encodeTime;
    std::string errorMessage;
    
    // 新增：音质分析结果
    AudioQualityResults qualityResults;
};

class Encoder {
public:
    virtual ~Encoder() = default;
    
    // 原有接口保持兼容性
    virtual EncodeResult encode(const std::string& inputFile, const std::string& outputFile, void* userData = nullptr) = 0;
    
    // 新增支持编码上下文的接口
    virtual EncodeResult encode(const std::string& inputFile, const std::string& outputFile, 
                               const EncoderParamContext& context) = 0;
    
    virtual std::string getEncoderName() const = 0;
    virtual void setQuality(int quality) = 0;  // 0-10, 10为最高质量
    
    // 获取编码器支持的参数范围信息
    virtual std::string getQualityDescription() const = 0;
    virtual int getMinQuality() const = 0;
    virtual int getMaxQuality() const = 0;
};

#endif