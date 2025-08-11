#ifndef MP3_ENCODER_H
#define MP3_ENCODER_H

#include "encoder.h"
#include "lame.h"
#include <cstdio>
#include <vector>

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
    
    // 新增：精确的缓冲区管理
    int reallocBuffer();
    void flushMp3Buffer(FILE* fout, std::vector<unsigned char>& output_data);
    
    // 新增：处理编码器延迟和填充
    void handleEncoderDelay(int sample_rate, int channels);

    lame_global_flags* lame_;
    int quality_;
    
    // 新增：缓冲区管理
    std::vector<unsigned char> mp3_buffer_;
    int buffer_index_;
    static const int BUFFER_SIZE = 7200 + 2 * 1152 + 1152/4; // 基于FFmpeg的计算
    
    // 新增：编码器延迟处理
    int encoder_delay_;
    int encoder_padding_;
    bool use_vbr_tag_;
};

#endif