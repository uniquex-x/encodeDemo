#ifndef VORBIS_ENCODER_H
#define VORBIS_ENCODER_H

#include "encoder.h"
#include <vorbis/vorbisenc.h>
#include <ogg/ogg.h>
#include <cstdio>

class VorbisEncoder : public Encoder {
public:
    VorbisEncoder();
    ~VorbisEncoder() override;

    // 实现基类接口
    EncodeResult encode(const std::string& inputFile, const std::string& outputFile, void* userData = nullptr) override;
    EncodeResult encode(const std::string& inputFile, const std::string& outputFile, 
                       const EncoderParamContext& context) override;
    
    std::string getEncoderName() const override { return "Vorbis"; }
    void setQuality(int quality) override;
    
    // 新增接口
    std::string getQualityDescription() const override { return "质量参数 (0-10, 10为最高质量)"; }
    int getMinQuality() const override { return 0; }
    int getMaxQuality() const override { return 10; }

private:
    bool readWaveHeader(FILE* file, int& channels, long& sample_rate, 
                       int& bits_per_sample, long& total_samples);

    float quality_;
};

#endif