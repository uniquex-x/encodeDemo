#ifndef MP3_ENCODER_H
#define MP3_ENCODER_H

#include "encoder.h"
#include "lame.h"
#include <cstdio>

class Mp3Encoder : public Encoder {
public:
    Mp3Encoder();
    ~Mp3Encoder() override;

    // 实现基类接口
    EncodeResult encode(const std::string& inputFile, const std::string& outputFile, void* userData = nullptr) override;
    EncodeResult encode(const std::string& inputFile, const std::string& outputFile, 
                       const EncoderParamContext& context) override;
    
    std::string getEncoderName() const override { return "MP3 (LAME)"; }
    void setQuality(int quality) override;
    
    // 新增接口
    std::string getQualityDescription() const override { return "VBR质量 (0-9, 0为最高质量)"; }
    int getMinQuality() const override { return 0; }
    int getMaxQuality() const override { return 9; }

private:
    bool readWaveHeader(FILE* file, int& channels, int& sample_rate, 
                       int& bits_per_sample, long& total_samples);

    lame_global_flags* lame_;
    int quality_;
};

#endif