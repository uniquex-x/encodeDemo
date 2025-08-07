#ifndef FLAC_ENCODER_H
#define FLAC_ENCODER_H

#include "encoder.h"
#include <FLAC/stream_encoder.h>
#include <cstdio>

class FlacEncoder : public Encoder {
public:
    FlacEncoder();
    ~FlacEncoder() override;

    // 实现基类接口
    EncodeResult encode(const std::string& inputFile, const std::string& outputFile, void* userData = nullptr) override;
    EncodeResult encode(const std::string& inputFile, const std::string& outputFile, 
                       const EncoderParamContext& context) override;
    
    std::string getEncoderName() const override { return "FLAC"; }
    void setQuality(int quality) override;
    
    // 新增接口
    std::string getQualityDescription() const override { return "压缩级别 (0-8, 8为最高压缩)"; }
    int getMinQuality() const override { return 0; }
    int getMaxQuality() const override { return 8; }

private:
    bool readWaveHeader(FILE* file, unsigned& channels, unsigned& sample_rate, 
                       unsigned& bits_per_sample, unsigned& total_samples);
    
    static FLAC__StreamEncoderWriteStatus write_callback(
        const FLAC__StreamEncoder *encoder,
        const FLAC__byte buffer[],
        size_t bytes,
        unsigned samples,
        unsigned current_frame,
        void *client_data);

    int quality_;
    FLAC__StreamEncoder* encoder_;
    FILE* fin_;
};

#endif