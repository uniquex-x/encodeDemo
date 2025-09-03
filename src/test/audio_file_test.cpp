#include "AudioFileLoader.h"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "用法: " << argv[0] << " <音频文件路径>" << std::endl;
        std::cout << "支持的格式: .ogg, .wav" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::cout << "测试音频文件加载器" << std::endl;
    std::cout << "文件: " << filename << std::endl;

    AudioFileLoader loader;
    
    // 检查格式支持
    if (!loader.isFormatSupported(filename)) {
        std::cout << "错误: 不支持的文件格式" << std::endl;
        std::cout << "支持的格式: ";
        for (const auto& format : loader.getSupportedFormats()) {
            std::cout << format << " ";
        }
        std::cout << std::endl;
        return 1;
    }

    // 加载音频文件
    AudioFileInfo info;
    auto audioData = loader.loadAudioFile(filename, info);

    if (audioData.empty()) {
        std::cout << "错误: 无法加载音频文件" << std::endl;
        return 1;
    }

    // 显示文件信息
    std::cout << "\n=== 音频文件信息 ===" << std::endl;
    std::cout << "格式: " << info.format << std::endl;
    std::cout << "采样率: " << info.sampleRate << " Hz" << std::endl;
    std::cout << "声道数: " << info.channels << std::endl;
    std::cout << "位深度: " << info.bitDepth << " bits" << std::endl;
    std::cout << "时长: " << std::fixed << std::setprecision(2) << info.duration << " 秒" << std::endl;
    std::cout << "总采样数: " << info.totalSamples << std::endl;

    // 显示音频数据统计
    std::cout << "\n=== 音频数据统计 ===" << std::endl;
    for (int ch = 0; ch < info.channels; ++ch) {
        if (ch < static_cast<int>(audioData.size()) && !audioData[ch].empty()) {
            // 计算RMS和峰值
            double sum = 0.0, peak = 0.0;
            for (double sample : audioData[ch]) {
                sum += sample * sample;
                peak = std::max(peak, std::abs(sample));
            }
            double rms = std::sqrt(sum / audioData[ch].size());
            
            std::cout << "声道 " << (ch + 1) << ":" << std::endl;
            std::cout << "  RMS: " << std::fixed << std::setprecision(4) << rms << std::endl;
            std::cout << "  峰值: " << std::fixed << std::setprecision(4) << peak << std::endl;
            std::cout << "  动态范围: " << std::fixed << std::setprecision(2) 
                      << (rms > 1e-10 ? 20.0 * std::log10(peak / rms) : 100.0) << " dB" << std::endl;
        }
    }

    // 显示前几个和后几个采样值
    std::cout << "\n=== 采样数据预览 ===" << std::endl;
    const int previewSamples = 10;
    
    for (int ch = 0; ch < std::min(2, info.channels); ++ch) {
        if (ch < static_cast<int>(audioData.size()) && !audioData[ch].empty()) {
            std::cout << "声道 " << (ch + 1) << " 前" << previewSamples << "个采样:" << std::endl;
            for (int i = 0; i < std::min(previewSamples, static_cast<int>(audioData[ch].size())); ++i) {
                std::cout << "  [" << i << "]: " << std::fixed << std::setprecision(6) 
                          << audioData[ch][i] << std::endl;
            }
            
            if (audioData[ch].size() > previewSamples * 2) {
                std::cout << "  ..." << std::endl;
                std::cout << "声道 " << (ch + 1) << " 后" << previewSamples << "个采样:" << std::endl;
                size_t start = audioData[ch].size() - previewSamples;
                for (int i = 0; i < previewSamples; ++i) {
                    std::cout << "  [" << (start + i) << "]: " << std::fixed << std::setprecision(6) 
                              << audioData[ch][start + i] << std::endl;
                }
            }
        }
    }

    std::cout << "\n文件加载成功！" << std::endl;
    return 0;
}
