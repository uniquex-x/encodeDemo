#include "flacEncoder.h"
#include <iostream>
#include <cstring>
#include <chrono>

FlacEncoder::FlacEncoder() : quality_(5), encoder_(nullptr), fin_(nullptr) {
    encoder_ = FLAC__stream_encoder_new();
}

FlacEncoder::~FlacEncoder() {
    if (encoder_) {
        FLAC__stream_encoder_delete(encoder_);
    }
    if (fin_) {
        fclose(fin_);
    }
}

void FlacEncoder::setQuality(int quality) {
    quality_ = std::max(0, std::min(8, quality)); // FLAC compression level 0-8
}

bool FlacEncoder::readWaveHeader(FILE* file, unsigned& channels, unsigned& sample_rate, 
                                unsigned& bits_per_sample, unsigned& total_samples) {
    char header[44];
    if (fread(header, 1, 44, file) != 44) {
        return false;
    }
    
    // 检查WAVE文件头
    if (strncmp(header, "RIFF", 4) != 0 || strncmp(header + 8, "WAVE", 4) != 0) {
        return false;
    }
    
    // 读取音频参数
    channels = *(unsigned short*)(header + 22);
    sample_rate = *(unsigned int*)(header + 24);
    bits_per_sample = *(unsigned short*)(header + 34);
    
    // 计算总样本数
    unsigned int data_size = *(unsigned int*)(header + 40);
    total_samples = data_size / (channels * bits_per_sample / 8);
    
    return true;
}

EncodeResult FlacEncoder::encode(const std::string& inputFile, const std::string& outputFile) {
    EncodeResult result;
    result.success = false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 打开输入文件
    fin_ = fopen(inputFile.c_str(), "rb");
    if (!fin_) {
        result.errorMessage = "无法打开输入文件: " + inputFile;
        return result;
    }
    
    // 读取WAV文件头
    unsigned channels, sample_rate, bits_per_sample, total_samples;
    if (!readWaveHeader(fin_, channels, sample_rate, bits_per_sample, total_samples)) {
        result.errorMessage = "无效的WAV文件格式";
        fclose(fin_);
        fin_ = nullptr;
        return result;
    }
    
    // 设置编码器参数
    FLAC__stream_encoder_set_channels(encoder_, channels);
    FLAC__stream_encoder_set_bits_per_sample(encoder_, bits_per_sample);
    FLAC__stream_encoder_set_sample_rate(encoder_, sample_rate);
    FLAC__stream_encoder_set_compression_level(encoder_, quality_);
    FLAC__stream_encoder_set_total_samples_estimate(encoder_, total_samples);
    
    // 初始化编码器
    if (FLAC__stream_encoder_init_file(encoder_, outputFile.c_str(), nullptr, nullptr) 
        != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
        result.errorMessage = "FLAC编码器初始化失败";
        fclose(fin_);
        fin_ = nullptr;
        return result;
    }
    
    // 编码数据
    const size_t buffer_size = 1024;
    FLAC__int32 buffer[buffer_size * channels];
    
    size_t bytes_per_sample = bits_per_sample / 8;
    size_t frame_size = channels * bytes_per_sample;
    
    while (!feof(fin_)) {
        size_t samples_read = 0;
        
        for (size_t i = 0; i < buffer_size && !feof(fin_); i++) {
            for (unsigned ch = 0; ch < channels; ch++) {
                if (bits_per_sample == 16) {
                    short sample;
                    if (fread(&sample, sizeof(short), 1, fin_) == 1) {
                        buffer[i * channels + ch] = sample;
                    } else {
                        break;
                    }
                }
            }
            if (!feof(fin_)) samples_read++;
        }
        
        if (samples_read > 0) {
            if (!FLAC__stream_encoder_process_interleaved(encoder_, buffer, samples_read)) {
                result.errorMessage = "编码过程中发生错误";
                break;
            }
        }
    }
    
    // 完成编码
    FLAC__stream_encoder_finish(encoder_);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // 计算文件大小
    fclose(fin_);
    fin_ = nullptr;
    
    // 获取文件大小
    FILE* input_file = fopen(inputFile.c_str(), "rb");
    if (input_file) {
        fseek(input_file, 0, SEEK_END);
        result.inputSize = ftell(input_file);
        fclose(input_file);
    }
    
    FILE* output_file = fopen(outputFile.c_str(), "rb");
    if (output_file) {
        fseek(output_file, 0, SEEK_END);
        result.outputSize = ftell(output_file);
        fclose(output_file);
        
        result.compressionRatio = (double)result.inputSize / result.outputSize;
        result.success = true;
        result.outputFile = outputFile;
    }
    
    return result;
}

FLAC__StreamEncoderWriteStatus FlacEncoder::write_callback(
    const FLAC__StreamEncoder *encoder,
    const FLAC__byte buffer[],
    size_t bytes,
    unsigned samples,
    unsigned current_frame,
    void *client_data) {
    // 这个回调函数在使用init_file时不需要实现
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}