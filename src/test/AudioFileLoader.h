#ifndef AUDIO_FILE_LOADER_H
#define AUDIO_FILE_LOADER_H

#include <string>
#include <vector>

/**
 * 音频文件信息结构体
 */
struct AudioFileInfo {
    int sampleRate = 0;
    int channels = 0;
    int bitDepth = 0;
    double duration = 0.0;              // 秒
    size_t totalSamples = 0;
    std::string format;
};

/**
 * 音频文件加载器
 * 支持多种格式：WAV, OGG/Vorbis, MP3等
 */
class AudioFileLoader {
public:
    AudioFileLoader();
    ~AudioFileLoader();

    /**
     * 加载音频文件
     * @param filename 音频文件路径
     * @param info 返回的文件信息
     * @return 音频数据，每个内层vector代表一个声道
     */
    std::vector<std::vector<double>> loadAudioFile(const std::string& filename, AudioFileInfo& info);

    /**
     * 检查文件格式是否支持
     */
    bool isFormatSupported(const std::string& filename);

    /**
     * 获取支持的格式列表
     */
    std::vector<std::string> getSupportedFormats();

private:
    // 格式特定的加载函数
    std::vector<std::vector<double>> loadWAV(const std::string& filename, AudioFileInfo& info);
    std::vector<std::vector<double>> loadOGG(const std::string& filename, AudioFileInfo& info);
    std::vector<std::vector<double>> loadMP3(const std::string& filename, AudioFileInfo& info);

    // 工具函数
    std::string getFileExtension(const std::string& filename);
    std::string toLowercase(const std::string& str);
    
    // 错误处理
    std::string lastError;
};

#endif // AUDIO_FILE_LOADER_H
