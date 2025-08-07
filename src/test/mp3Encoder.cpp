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
EncodeResult Mp3Encoder::encode(const std::string& inputFile, const std::string& outputFile, void* userData) {
    EncoderParamContext context;
    context.encoderType = EncoderType::LAME;
    context.quality = quality_;
    return encode(inputFile, outputFile, context);
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