#ifndef FLAC_ENCODER_H
#define FLAC_ENCODER_H

#include "encoder.h"
#include "FLAC/stream_encoder.h"

class FlacEncoder : public Encoder {
public:
    FlacEncoder();
    ~FlacEncoder();

    EncodeResult encode(const std::string& inputFile, const std::string& outputFile) override;
    std::string getEncoderName() const override { return "FLAC"; }
    void setQuality(int quality) override;

private:
    bool readWaveHeader(FILE* file, unsigned& channels, unsigned& sample_rate, 
                       unsigned& bits_per_sample, unsigned& total_samples);
    
    int quality_;
    FLAC__StreamEncoder* encoder_;
    FILE* fin_;
    
    static FLAC__StreamEncoderWriteStatus write_callback(
        const FLAC__StreamEncoder *encoder,
        const FLAC__byte buffer[],
        size_t bytes,
        unsigned samples,
        unsigned current_frame,
        void *client_data);
};

#endif