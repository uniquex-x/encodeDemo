#include "mp3Encoder.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>

Mp3Encoder::Mp3Encoder() 
    : lame_(nullptr), quality_(2), buffer_index_(0), 
      encoder_delay_(0), encoder_padding_(0), use_vbr_tag_(false) {
    lame_ = lame_init();
    if (lame_) {
        // 初始化缓冲区
        mp3_buffer_.resize(BUFFER_SIZE);
        
        // 设置默认参数 - 参考FFmpeg实现
        lame_set_VBR(lame_, vbr_default);
        lame_set_VBR_quality(lame_, quality_);
        
        // 关键：禁用VBR标签以避免额外的帧
        lame_set_bWriteVbrTag(lame_, 0);
        use_vbr_tag_ = false;
        
        // 设置比特率储备池（参考FFmpeg默认启用）
        lame_set_disable_reservoir(lame_, 0);
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

int Mp3Encoder::reallocBuffer() {
    if (mp3_buffer_.size() - buffer_index_ < BUFFER_SIZE) {
        int new_size = buffer_index_ + 2 * BUFFER_SIZE;
        mp3_buffer_.resize(new_size);
    }
    return 0;
}

void Mp3Encoder::flushMp3Buffer(FILE* fout, std::vector<unsigned char>& output_data) {
    if (buffer_index_ > 0) {
        // 将缓冲区数据添加到输出
        output_data.insert(output_data.end(), 
                          mp3_buffer_.begin(), 
                          mp3_buffer_.begin() + buffer_index_);
        buffer_index_ = 0;
    }
}

void Mp3Encoder::handleEncoderDelay(int sample_rate, int channels) {
    if (lame_) {
        // 获取编码器延迟信息
        encoder_delay_ = lame_get_encoder_delay(lame_);
        encoder_padding_ = lame_get_encoder_padding(lame_);
        
        // 根据FFmpeg的实现，添加额外的延迟补偿
        // FFmpeg adds: encoder_delay + 528 + 1
        encoder_delay_ += 528 + 1;
        
        std::cout << "编码器延迟: " << encoder_delay_ << " 样本" << std::endl;
        std::cout << "编码器填充: " << encoder_padding_ << " 样本" << std::endl;
    }
}

bool Mp3Encoder::readWaveHeader(FILE* file, int& channels, int& sample_rate, 
                        int& bits_per_sample, long& total_samples) {
    if (!file) {
        return false;
    }

    // Save current file position
    long original_pos = ftell(file);
    
    // Read and validate RIFF header
    struct RiffHeader {
        char riff_id[4];        // "RIFF"
        uint32_t file_size;     // File size minus 8 bytes
        char wave_id[4];        // "WAVE"
    } riff_header;

    if (fread(&riff_header, sizeof(RiffHeader), 1, file) != 1) {
        fseek(file, original_pos, SEEK_SET);
        return false;
    }

    if (strncmp(riff_header.riff_id, "RIFF", 4) != 0 || 
        strncmp(riff_header.wave_id, "WAVE", 4) != 0) {
        fseek(file, original_pos, SEEK_SET);
        return false;
    }

    bool found_fmt = false;
    bool found_data = false;
    
    while (!found_data && !feof(file)) {
        struct ChunkHeader {
            char chunk_id[4];
            uint32_t chunk_size;
        } chunk_header;
        
        if (fread(&chunk_header, sizeof(ChunkHeader), 1, file) != 1) {
            break;
        }
        
        if (strncmp(chunk_header.chunk_id, "fmt ", 4) == 0) {
            struct FmtChunk {
                uint16_t audio_format;      // 1 for PCM
                uint16_t num_channels;      // Number of channels
                uint32_t sample_rate;       // Sample rate
                uint32_t byte_rate;         // Byte rate
                uint16_t block_align;       // Block align
                uint16_t bits_per_sample;   // Bits per sample
            } fmt_chunk;
            
            if (fread(&fmt_chunk, sizeof(FmtChunk), 1, file) != 1) {
                break;
            }
            
            if (fmt_chunk.audio_format != 1) { // Only support PCM
                break;
            }
            
            channels = fmt_chunk.num_channels;
            sample_rate = fmt_chunk.sample_rate;
            bits_per_sample = fmt_chunk.bits_per_sample;
            found_fmt = true;
            
            // Skip any extra format bytes
            if (chunk_header.chunk_size > sizeof(FmtChunk)) {
                fseek(file, chunk_header.chunk_size - sizeof(FmtChunk), SEEK_CUR);
            }
        } else if (strncmp(chunk_header.chunk_id, "data", 4) == 0) {
            if (found_fmt) {
                // Calculate total samples
                total_samples = chunk_header.chunk_size / (channels * (bits_per_sample / 8));
                found_data = true;
                // Don't seek past the data chunk header - we want to read the data
            } else {
                // Skip data chunk if fmt not found yet
                fseek(file, chunk_header.chunk_size, SEEK_CUR);
            }
        } else {
            // Skip unknown chunks
            fseek(file, chunk_header.chunk_size, SEEK_CUR);
        }
    }
    
    if (!found_fmt || !found_data) {
        fseek(file, original_pos, SEEK_SET);
        return false;
    }
    
    return true;
}

// 新的带上下文参数的编码接口
EncodeResult Mp3Encoder::encode(const std::string& inputFile, const std::string& outputFile, 
                               const EncoderParamContext& context) {
    EncodeResult result;
    result.success = false;
    
    // 根据上下文设置质量参数
    int use_quality = context.quality;
    if (context.encoderType == EncoderType::LAME) {
        setQuality(use_quality);
    }
    
    if (!lame_) {
        result.errorMessage = "LAME初始化失败";
        return result;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 重置缓冲区
    buffer_index_ = 0;
    std::vector<unsigned char> output_data;
    output_data.reserve(1024 * 1024); // 预分配1MB
    
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
    
    std::cout << "输入音频信息:" << std::endl;
    std::cout << "  声道数: " << channels << std::endl;
    std::cout << "  采样率: " << sample_rate << " Hz" << std::endl;
    std::cout << "  位深度: " << bits_per_sample << " bits" << std::endl;
    std::cout << "  总样本数: " << total_samples << std::endl;
    
    // 设置LAME参数 - 参考FFmpeg的设置顺序
    lame_set_num_channels(lame_, channels);
    lame_set_mode(lame_, channels > 1 ? JOINT_STEREO : MONO);
    lame_set_in_samplerate(lame_, sample_rate);
    lame_set_out_samplerate(lame_, sample_rate);
    
    // 设置质量和VBR参数
    lame_set_VBR(lame_, vbr_default);
    lame_set_VBR_quality(lame_, quality_);
    
    // 关键设置：禁用VBR标签和比特储备池配置
    lame_set_bWriteVbrTag(lame_, 0);
    lame_set_disable_reservoir(lame_, 0); // 启用比特储备池以提高质量
    
    // 设置样本总数以获得更准确的编码
    lame_set_num_samples(lame_, total_samples);
    
    if (lame_init_params(lame_) < 0) {
        result.errorMessage = "LAME参数初始化失败";
        fclose(fin);
        return result;
    }
    
    // 获取编码器延迟信息
    handleEncoderDelay(sample_rate, channels);
    
    // 获取帧大小
    int frame_size = lame_get_framesize(lame_);
    std::cout << "LAME帧大小: " << frame_size << " 样本" << std::endl;
    
    // 编码数据 - 使用更精确的缓冲区管理
    const int samples_per_frame = frame_size;
    std::vector<short> pcm_buffer(samples_per_frame * channels);
    std::vector<short> left_channel(samples_per_frame);
    std::vector<short> right_channel(samples_per_frame);
    
    long samples_processed = 0;
    int bytes_encoded;
    
    while (samples_processed < total_samples && !feof(fin)) {
        // 计算本次读取的样本数
        int samples_to_read = std::min((long)samples_per_frame, 
                                     total_samples - samples_processed);
        
        int samples_read = fread(pcm_buffer.data(), 
                               sizeof(short) * channels, 
                               samples_to_read, fin);
        
        if (samples_read <= 0) break;
        
        // 确保缓冲区足够
        if (reallocBuffer() < 0) {
            result.errorMessage = "缓冲区重分配失败";
            fclose(fin);
            return result;
        }
        
        if (channels == 1) {
            bytes_encoded = lame_encode_buffer(lame_, 
                                             pcm_buffer.data(), nullptr, 
                                             samples_read,
                                             mp3_buffer_.data() + buffer_index_, 
                                             mp3_buffer_.size() - buffer_index_);
        } else {
            // 分离左右声道 - 更高效的实现
            for (int i = 0; i < samples_read; i++) {
                left_channel[i] = pcm_buffer[i * 2];
                right_channel[i] = pcm_buffer[i * 2 + 1];
            }
            
            bytes_encoded = lame_encode_buffer(lame_, 
                                             left_channel.data(), 
                                             right_channel.data(), 
                                             samples_read,
                                             mp3_buffer_.data() + buffer_index_, 
                                             mp3_buffer_.size() - buffer_index_);
        }
        
        if (bytes_encoded < 0) {
            result.errorMessage = "编码过程中发生错误，错误代码: " + std::to_string(bytes_encoded);
            fclose(fin);
            return result;
        }
        
        buffer_index_ += bytes_encoded;
        samples_processed += samples_read;
        
        // 定期刷新缓冲区以避免内存过度使用
        if (buffer_index_ > BUFFER_SIZE) {
            flushMp3Buffer(nullptr, output_data);
        }
    }
    
    // 刷新编码器缓冲区 - 关键步骤
    bytes_encoded = lame_encode_flush(lame_, 
                                    mp3_buffer_.data() + buffer_index_, 
                                    mp3_buffer_.size() - buffer_index_);
    if (bytes_encoded > 0) {
        buffer_index_ += bytes_encoded;
    }
    
    // 最终刷新所有缓冲区数据
    flushMp3Buffer(nullptr, output_data);
    
    fclose(fin);
    
    // 写入输出文件
    FILE* fout = fopen(outputFile.c_str(), "wb");
    if (!fout) {
        result.errorMessage = "无法创建输出文件: " + outputFile;
        return result;
    }
    
    if (!output_data.empty()) {
        size_t written = fwrite(output_data.data(), 1, output_data.size(), fout);
        if (written != output_data.size()) {
            result.errorMessage = "写入输出文件失败";
            fclose(fout);
            return result;
        }
    }
    
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
        
        std::cout << "编码完成:" << std::endl;
        std::cout << "  输入大小: " << result.inputSize << " 字节" << std::endl;
        std::cout << "  输出大小: " << result.outputSize << " 字节" << std::endl;
        std::cout << "  压缩比: " << result.compressionRatio << ":1" << std::endl;
        std::cout << "  编码时间: " << result.encodeTime.count() << " ms" << std::endl;
    }
    
    return result;
}

// 保持向后兼容的简单接口
EncodeResult Mp3Encoder::encode(const std::string& inputFile, const std::string& outputFile, void* userData) {
    EncoderParamContext context;
    context.quality = quality_;
    context.encoderType = EncoderType::LAME;
    return encode(inputFile, outputFile, context);
}