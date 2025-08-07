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
            char id[4];
            uint32_t size;
        } chunk_header;

        if (fread(&chunk_header, sizeof(ChunkHeader), 1, file) != 1) {
            break;
        }

        if (strncmp(chunk_header.id, "fmt ", 4) == 0) {
            // Format chunk structure
            struct FormatChunk {
                uint16_t audio_format;      // 1 = PCM
                uint16_t num_channels;      // Number of channels
                uint32_t sample_rate;       // Sample rate
                uint32_t byte_rate;         // Bytes per second
                uint16_t block_align;       // Bytes per sample frame
                uint16_t bits_per_sample;   // Bits per sample
            } format_chunk;

            if (chunk_header.size < sizeof(FormatChunk) ||
                fread(&format_chunk, sizeof(FormatChunk), 1, file) != 1) {
                break;
            }

            // Validate format
            if (format_chunk.audio_format != 1) { // Only support PCM
                break;
            }

            channels = format_chunk.num_channels;
            sample_rate = format_chunk.sample_rate;
            bits_per_sample = format_chunk.bits_per_sample;

            // Validate parameters
            if (channels <= 0 || channels > 8 ||
                sample_rate <= 0 || sample_rate > 192000 ||
                (bits_per_sample != 8 && bits_per_sample != 16 && bits_per_sample != 24 && bits_per_sample != 32)) {
                break;
            }

            found_fmt = true;

            // Skip remaining bytes in format chunk
            if (chunk_header.size > sizeof(FormatChunk)) {
                fseek(file, chunk_header.size - sizeof(FormatChunk), SEEK_CUR);
            }
        }
        else if (strncmp(chunk_header.id, "data", 4) == 0) {
            if (!found_fmt) {
                break; // Format chunk must come first
            }

            uint32_t bytes_per_sample = (bits_per_sample / 8) * channels;
            if (bytes_per_sample == 0) {
                break;
            }

            total_samples = chunk_header.size / bytes_per_sample;
            found_data = true;
            
            // Position is now at start of audio data
        }
        else {
            // Skip unknown chunk
            // Ensure even alignment (WAV standard)
            uint32_t skip_size = (chunk_header.size % 2 == 0) ? 
                                chunk_header.size : chunk_header.size + 1;
            
            if (fseek(file, skip_size, SEEK_CUR) != 0) {
                break;
            }
        }
    }

    if (!found_fmt || !found_data) {
        fseek(file, original_pos, SEEK_SET);
        return false;
    }

    return true;
}

// 原有接口实现，保持兼容性
EncodeResult FlacEncoder::encode(const std::string& inputFile, const std::string& outputFile, void* userData) {
    EncoderParamContext context;
    context.encoderType = EncoderType::FLAC;
    context.quality = quality_;
    return encode(inputFile, outputFile, context);
}

// 新的带上下文参数的编码接口
EncodeResult FlacEncoder::encode(const std::string& inputFile, const std::string& outputFile, 
                                const EncoderParamContext& context) {
    EncodeResult result;
    result.success = false;
    
    // 根据上下文设置质量参数
    int use_quality = context.quality;
    if (context.encoderType == EncoderType::FLAC) {
        setQuality(use_quality);
    }
    
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
                } else if (bits_per_sample == 24) {
                    // 24位样本处理
                    unsigned char sample_bytes[3];
                    if (fread(sample_bytes, 1, 3, fin_) == 3) {
                        int32_t sample = (sample_bytes[0]) | 
                                        (sample_bytes[1] << 8) | 
                                        (sample_bytes[2] << 16);
                        // 符号扩展
                        if (sample & 0x800000) sample |= 0xFF000000;
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