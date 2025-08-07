#ifndef MP3_ENCODER_H
#define MP3_ENCODER_H

#include "encoder.h"
#include "lame.h"

class Mp3Encoder : public Encoder {
public:
    Mp3Encoder();
    ~Mp3Encoder();

    EncodeResult encode(const std::string& inputFile, const std::string& outputFile) override;
    std::string getEncoderName() const override { return "MP3 (LAME)"; }
    void setQuality(int quality) override;

private:
    bool readWaveHeader(FILE* file, int& channels, int& sample_rate, 
                       int& bits_per_sample, long& total_samples);
    
    lame_global_flags* lame_;
    int quality_; // 0-9, 0为最高质量
};

#endif