#ifndef VORBIS_ENCODER_H
#define VORBIS_ENCODER_H

#include "encoder.h"
#include "vorbis/vorbisenc.h"

class VorbisEncoder : public Encoder {
public:
    VorbisEncoder();
    ~VorbisEncoder();

    EncodeResult encode(const std::string& inputFile, const std::string& outputFile) override;
    std::string getEncoderName() const override { return "Vorbis"; }
    void setQuality(int quality) override;

private:
    bool readWaveHeader(FILE* file, int& channels, long& sample_rate, 
                       int& bits_per_sample, long& total_samples);
    
    float quality_; // -1.0 to 1.0
};

#endif