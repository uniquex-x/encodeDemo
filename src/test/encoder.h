#ifndef ENCODER_H
#define ENCODER_H

#include <string>
#include <chrono>

struct EncodeResult {
    bool success;
    std::string outputFile;
    size_t inputSize;
    size_t outputSize;
    double compressionRatio;
    std::chrono::milliseconds encodeTime;
    std::string errorMessage;
};

class Encoder {
public:
    virtual ~Encoder() = default;
    virtual EncodeResult encode(const std::string& inputFile, const std::string& outputFile) = 0;
    virtual std::string getEncoderName() const = 0;
    virtual void setQuality(int quality) = 0;  // 0-10, 10为最高质量
};

#endif