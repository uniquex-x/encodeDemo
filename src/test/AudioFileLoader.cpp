#include "AudioFileLoader.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

// Vorbis库头文件
extern "C" {
#include "vorbis/vorbisfile.h"
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AudioFileLoader::AudioFileLoader() {
}

AudioFileLoader::~AudioFileLoader() {
}

std::vector<std::vector<double>> 
AudioFileLoader::loadAudioFile(const std::string& filename, AudioFileInfo& info) {
    if (!isFormatSupported(filename)) {
        lastError = "Unsupported file format: " + getFileExtension(filename);
        return {};
    }

    std::string ext = toLowercase(getFileExtension(filename));
    
    if (ext == "ogg") {
        return loadOGG(filename, info);
    } else if (ext == "wav") {
        return loadWAV(filename, info);
    } else if (ext == "mp3") {
        return loadMP3(filename, info);
    }
    
    lastError = "No loader implemented for format: " + ext;
    return {};
}

bool AudioFileLoader::isFormatSupported(const std::string& filename) {
    std::string ext = toLowercase(getFileExtension(filename));
    return ext == "ogg" || ext == "wav" || ext == "mp3";
}

std::vector<std::string> AudioFileLoader::getSupportedFormats() {
    return {"ogg", "wav", "mp3"};
}

std::vector<std::vector<double>> 
AudioFileLoader::loadOGG(const std::string& filename, AudioFileInfo& info) {
    std::vector<std::vector<double>> audioData;
    
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        lastError = "Cannot open file: " + filename;
        return audioData;
    }

    OggVorbis_File vf;
    int result = ov_open_callbacks(file, &vf, nullptr, 0, OV_CALLBACKS_DEFAULT);
    
    if (result < 0) {
        fclose(file);
        lastError = "Invalid OGG/Vorbis file: " + filename;
        return audioData;
    }

    // 获取文件信息
    vorbis_info* vi = ov_info(&vf, -1);
    if (!vi) {
        ov_clear(&vf);
        lastError = "Cannot get Vorbis info from file: " + filename;
        return audioData;
    }

    info.sampleRate = vi->rate;
    info.channels = vi->channels;
    info.bitDepth = 16; // Vorbis is always decoded to 16-bit equivalent
    info.format = "OGG/Vorbis";
    
    // 获取总采样数
    ogg_int64_t totalSamples = ov_pcm_total(&vf, -1);
    if (totalSamples < 0) {
        totalSamples = 0;
    }
    
    info.totalSamples = static_cast<size_t>(totalSamples);
    info.duration = static_cast<double>(totalSamples) / info.sampleRate;

    // 初始化音频数据容器
    audioData.resize(info.channels);
    for (int ch = 0; ch < info.channels; ++ch) {
        audioData[ch].reserve(info.totalSamples);
    }

    // 读取音频数据
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    int current_section = 0;
    
    while (true) {
        long bytesRead = ov_read(&vf, buffer, BUFFER_SIZE, 0, 2, 1, &current_section);
        
        if (bytesRead == 0) {
            break; // EOF
        } else if (bytesRead < 0) {
            // 错误处理
            if (bytesRead == OV_HOLE) {
                continue; // 数据中断，继续读取
            } else {
                lastError = "Error reading OGG data: " + std::to_string(bytesRead);
                break;
            }
        }
        
        // 转换PCM数据
        int samplesRead = bytesRead / (2 * info.channels); // 16-bit stereo
        short* samples = reinterpret_cast<short*>(buffer);
        
        for (int i = 0; i < samplesRead; ++i) {
            for (int ch = 0; ch < info.channels; ++ch) {
                // 转换为-1.0到1.0的浮点数
                double sample = static_cast<double>(samples[i * info.channels + ch]) / 32768.0;
                audioData[ch].push_back(sample);
            }
        }
    }

    ov_clear(&vf);
    
    // 更新实际读取的采样数
    if (!audioData.empty()) {
        info.totalSamples = audioData[0].size();
        info.duration = static_cast<double>(info.totalSamples) / info.sampleRate;
    }

    std::cout << "Successfully loaded OGG file: " << filename << std::endl;
    std::cout << "  Sample Rate: " << info.sampleRate << " Hz" << std::endl;
    std::cout << "  Channels: " << info.channels << std::endl;
    std::cout << "  Duration: " << info.duration << " seconds" << std::endl;
    std::cout << "  Total Samples: " << info.totalSamples << std::endl;

    return audioData;
}

std::vector<std::vector<double>> 
AudioFileLoader::loadWAV(const std::string& filename, AudioFileInfo& info) {
    std::vector<std::vector<double>> audioData;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        lastError = "Cannot open WAV file: " + filename;
        return audioData;
    }

    // 读取WAV头部
    char chunkID[4];
    uint32_t chunkSize;
    char format[4];
    
    file.read(chunkID, 4);
    file.read(reinterpret_cast<char*>(&chunkSize), 4);
    file.read(format, 4);
    
    if (std::memcmp(chunkID, "RIFF", 4) != 0 || std::memcmp(format, "WAVE", 4) != 0) {
        lastError = "Invalid WAV file format: " + filename;
        return audioData;
    }

    // 查找fmt chunk
    char subchunk1ID[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    
    file.read(subchunk1ID, 4);
    file.read(reinterpret_cast<char*>(&subchunk1Size), 4);
    file.read(reinterpret_cast<char*>(&audioFormat), 2);
    file.read(reinterpret_cast<char*>(&numChannels), 2);
    file.read(reinterpret_cast<char*>(&sampleRate), 4);
    file.read(reinterpret_cast<char*>(&byteRate), 4);
    file.read(reinterpret_cast<char*>(&blockAlign), 2);
    file.read(reinterpret_cast<char*>(&bitsPerSample), 2);
    
    if (std::memcmp(subchunk1ID, "fmt ", 4) != 0) {
        lastError = "Invalid WAV fmt chunk: " + filename;
        return audioData;
    }
    
    if (audioFormat != 1) { // PCM
        lastError = "Unsupported WAV audio format (only PCM supported): " + filename;
        return audioData;
    }

    // 查找data chunk
    char subchunk2ID[4];
    uint32_t subchunk2Size;
    
    file.read(subchunk2ID, 4);
    file.read(reinterpret_cast<char*>(&subchunk2Size), 4);
    
    if (std::memcmp(subchunk2ID, "data", 4) != 0) {
        lastError = "Invalid WAV data chunk: " + filename;
        return audioData;
    }

    // 设置文件信息
    info.sampleRate = sampleRate;
    info.channels = numChannels;
    info.bitDepth = bitsPerSample;
    info.format = "WAV";
    info.totalSamples = subchunk2Size / (numChannels * bitsPerSample / 8);
    info.duration = static_cast<double>(info.totalSamples) / sampleRate;

    // 读取音频数据
    audioData.resize(numChannels);
    for (int ch = 0; ch < numChannels; ++ch) {
        audioData[ch].reserve(info.totalSamples);
    }

    if (bitsPerSample == 16) {
        std::vector<int16_t> buffer(numChannels);
        for (size_t i = 0; i < info.totalSamples; ++i) {
            file.read(reinterpret_cast<char*>(buffer.data()), numChannels * sizeof(int16_t));
            for (int ch = 0; ch < numChannels; ++ch) {
                double sample = static_cast<double>(buffer[ch]) / 32768.0;
                audioData[ch].push_back(sample);
            }
        }
    } else {
        lastError = "Unsupported bit depth: " + std::to_string(bitsPerSample);
        return {};
    }

    std::cout << "Successfully loaded WAV file: " << filename << std::endl;
    std::cout << "  Sample Rate: " << info.sampleRate << " Hz" << std::endl;
    std::cout << "  Channels: " << info.channels << std::endl;
    std::cout << "  Bit Depth: " << info.bitDepth << " bits" << std::endl;
    std::cout << "  Duration: " << info.duration << " seconds" << std::endl;

    return audioData;
}

std::vector<std::vector<double>> 
AudioFileLoader::loadMP3(const std::string& filename, AudioFileInfo& info) {
    std::vector<std::vector<double>> audioData;
    
    // MP3解码需要更复杂的实现，这里提供基本框架
    // 实际项目中建议使用专门的MP3解码库如mpg123
    
    lastError = "MP3 support not yet implemented. Please use OGG or WAV format.";
    return audioData;
}

// 工具函数实现
std::string AudioFileLoader::getFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    return filename.substr(dotPos + 1);
}

std::string AudioFileLoader::toLowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
