#include "vorbisEncoder.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <cstdlib>

VorbisEncoder::VorbisEncoder() : quality_(0.4f) {
    // 默认质量设置为0.4 (中等质量)
}

VorbisEncoder::~VorbisEncoder() {
    // Vorbis结构体会在encode方法中局部管理
}

void VorbisEncoder::setQuality(int quality) {
    // 将0-10的质量等级映射到-1.0到1.0的范围
    quality_ = (quality - 5) * 0.2f; // 5对应0.0, 0对应-1.0, 10对应1.0
    quality_ = std::max(-1.0f, std::min(1.0f, quality_));
}

bool VorbisEncoder::readWaveHeader(FILE* file, int& channels, long& sample_rate, 
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
EncodeResult VorbisEncoder::encode(const std::string& inputFile, const std::string& outputFile, void* userData) {
    EncoderParamContext context;
    context.encoderType = EncoderType::VORBIS;
    context.quality = static_cast<int>((quality_ + 1.0f) * 5.0f); // 将-1.0到1.0映射回0-10
    return encode(inputFile, outputFile, context);
}

// 新的带上下文参数的编码接口
EncodeResult VorbisEncoder::encode(const std::string& inputFile, const std::string& outputFile, 
                                  const EncoderParamContext& context) {
    EncodeResult result;
    result.success = false;
    
    // 根据上下文设置质量参数
    int use_quality = context.quality;
    if (context.encoderType == EncoderType::VORBIS) {
        setQuality(use_quality);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 打开输入文件
    FILE* fin = fopen(inputFile.c_str(), "rb");
    if (!fin) {
        result.errorMessage = "无法打开输入文件: " + inputFile;
        return result;
    }
    
    // 读取WAV文件头
    int channels, bits_per_sample;
    long sample_rate, total_samples;
    if (!readWaveHeader(fin, channels, sample_rate, bits_per_sample, total_samples)) {
        result.errorMessage = "无效的WAV文件格式";
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
    
    // 初始化Vorbis结构体
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    
    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;
    
    vorbis_info_init(&vi);
    
    // 设置编码参数 (VBR质量模式)
    if (vorbis_encode_init_vbr(&vi, channels, sample_rate, quality_) != 0) {
        result.errorMessage = "Vorbis编码器初始化失败";
        vorbis_info_clear(&vi);
        fclose(fin);
        fclose(fout);
        return result;
    }
    
    vorbis_comment_init(&vc);
    vorbis_comment_add_tag(&vc, "ENCODER", "Custom Audio Encoder Test");
    
    vorbis_analysis_init(&vd, &vi);
    vorbis_block_init(&vd, &vb);
    
    // 生成随机序列号
    srand(time(NULL));
    ogg_stream_init(&os, rand());
    
    // 创建并写入头信息
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;
    
    vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
    ogg_stream_packetin(&os, &header);
    ogg_stream_packetin(&os, &header_comm);
    ogg_stream_packetin(&os, &header_code);
    
    // 写入头页面
    while (ogg_stream_flush(&os, &og) != 0) {
        fwrite(og.header, 1, og.header_len, fout);
        fwrite(og.body, 1, og.body_len, fout);
    }
    
    // 编码音频数据
    const int buffer_size = 1024;
    short pcm_buffer[buffer_size * channels];
    
    bool eos = false;
    while (!eos) {
        long samples_read = fread(pcm_buffer, sizeof(short) * channels, buffer_size, fin);
        
        if (samples_read == 0) {
            vorbis_analysis_wrote(&vd, 0);
        } else {
            float **buffer = vorbis_analysis_buffer(&vd, samples_read);
            
            // 转换PCM数据到float格式
            for (int i = 0; i < samples_read; i++) {
                for (int ch = 0; ch < channels; ch++) {
                    buffer[ch][i] = pcm_buffer[i * channels + ch] / 32768.0f;
                }
            }
            
            vorbis_analysis_wrote(&vd, samples_read);
        }
        
        while (vorbis_analysis_blockout(&vd, &vb) == 1) {
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);
            
            while (vorbis_bitrate_flushpacket(&vd, &op)) {
                ogg_stream_packetin(&os, &op);
                
                while (!eos) {
                    int result_code = ogg_stream_pageout(&os, &og);
                    if (result_code == 0) break;
                    
                    fwrite(og.header, 1, og.header_len, fout);
                    fwrite(og.body, 1, og.body_len, fout);
                    
                    if (ogg_page_eos(&og)) eos = true;
                }
            }
        }
    }
    
    // 清理资源
    ogg_stream_clear(&os);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    
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