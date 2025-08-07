#include "mp3Encoder.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <vector>

Mp3Encoder::Mp3Encoder() : lame_(nullptr), quality_(2) {
    lame_ = lame_init();
    if (lame_) {
        lame_set_VBR(lame_, vbr_default);
        lame_set_VBR_quality(lame_, quality_);
    }
}

Mp3Encoder::~Mp3Encoder() {
    if (lame_) {
        lame_close(lame_);
    }
}

void Mp3Encoder::setQuality(int quality) {
    quality_ = std::max(0, std::min(9, quality)); // LAME VBR quality 0-9
    if (lame_) {
        lame_set_VBR_quality(lame_, quality_);
    }
}

bool Mp3Encoder::readWaveHeader(FILE* file, int& channels, int& sample_rate, 
                               int& bits_per_sample, long& total_samples) {
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

EncodeResult Mp3Encoder::encode(const std::string& inputFile, const std::string& outputFile) {
    EncodeResult result;
    result.success = false;
    
    if (!lame_) {
        result.errorMessage = "LAME初始化失败";
        return result;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 打开输入文件
    FILE* fin = fopen(inputFile.c_str(), "rb");
    if (!fin) {
        result.errorMessage = "无法打开输入文件: " + inputFile;
        return result;
    }
    
    // 读取WAV文件头
    int channels, sample_rate, bits_per_sample;
    long total_samples;
    if (!readWaveHeader(fin, channels, sample_rate, bits_per_sample, total_samples)) {
        result.errorMessage = "无效的WAV文件格式";
        fclose(fin);
        return result;
    }
    
    // 设置LAME参数
    lame_set_in_samplerate(lame_, sample_rate);
    lame_set_num_channels(lame_, channels);
    lame_set_VBR_quality(lame_, quality_);
    
    if (lame_init_params(lame_) < 0) {
        result.errorMessage = "LAME参数初始化失败";
        fclose(fin);
        return result;
    }
    
    // 打开输出文件
    FILE* fout = fopen(outputFile.c_str(), "wb");
    if (!fout) {
        result.errorMessage = "无法创建输出文件: " + outputFile;
        fclose(fin);
        return result;
    }
    
    // 编码数据
    const int buffer_size = 1024;
    short int pcm_buffer[buffer_size * 2]; // 立体声
    unsigned char mp3_buffer[buffer_size * 5]; // MP3输出缓冲区
    
    int bytes_encoded;
    while (!feof(fin)) {
        int samples_read = fread(pcm_buffer, sizeof(short int) * channels, buffer_size, fin);
        
        if (samples_read == 0) break;
        
        if (channels == 1) {
            bytes_encoded = lame_encode_buffer(lame_, pcm_buffer, nullptr, samples_read, 
                                             mp3_buffer, sizeof(mp3_buffer));
        } else {
            // 分离左右声道
            short int left[buffer_size], right[buffer_size];
            for (int i = 0; i < samples_read; i++) {
                left[i] = pcm_buffer[i * 2];
                right[i] = pcm_buffer[i * 2 + 1];
            }
            bytes_encoded = lame_encode_buffer(lame_, left, right, samples_read, 
                                             mp3_buffer, sizeof(mp3_buffer));
        }
        
        if (bytes_encoded < 0) {
            result.errorMessage = "编码过程中发生错误";
            fclose(fin);
            fclose(fout);
            return result;
        }
        
        if (bytes_encoded > 0) {
            fwrite(mp3_buffer, 1, bytes_encoded, fout);
        }
    }
    
    // 刷新编码器缓冲区
    bytes_encoded = lame_encode_flush(lame_, mp3_buffer, sizeof(mp3_buffer));
    if (bytes_encoded > 0) {
        fwrite(mp3_buffer, 1, bytes_encoded, fout);
    }
    
    fclose(fin);
    fclose(fout);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
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