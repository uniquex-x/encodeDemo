#include "AudioQualityAnalyzer.h"
#include <iostream>
#include <iomanip>

void printQualityReport(const AudioQualityAnalyzer::QualityAnalysisResult& result) {
    std::cout << "\n=== 音质分析报告 ===" << std::endl;
    std::cout << "分析时间: " << result.analysisTimestamp << std::endl;
    std::cout << "分析状态: " << (result.analysisSuccess ? "成功" : "失败") << std::endl;
    
    if (!result.analysisSuccess) {
        std::cout << "错误信息: " << result.errorMessage << std::endl;
        return;
    }
    
    std::cout << std::fixed << std::setprecision(2);
    
    // 客观音质指标
    std::cout << "\n--- 客观音质指标 ---" << std::endl;
    std::cout << "信噪比 (SNR): " << result.objective.snr << " dB" << std::endl;
    std::cout << "总谐波失真 (THD): " << result.objective.thd << " %" << std::endl;
    std::cout << "动态范围: " << result.objective.dynamicRange << " dB" << std::endl;
    std::cout << "声道分离度: " << result.objective.channelSeparation << " dB" << std::endl;
    std::cout << "立体声成像: " << result.objective.stereoImaging << std::endl;
    std::cout << "峰值/RMS比值: " << result.objective.peakToRmsRatio << " dB" << std::endl;
    
    // 感知音质指标
    std::cout << "\n--- 感知音质指标 ---" << std::endl;
    std::cout << "PESQ评分: " << result.perceptual.pesq_score << " (1.0-4.5)" << std::endl;
    std::cout << "STOI评分: " << result.perceptual.stoi_score << " (0-1)" << std::endl;
    std::cout << "MOS评分: " << result.perceptual.mos_score << " (1-5)" << std::endl;
    std::cout << "LUFS响度: " << result.perceptual.loudness_lufs << " LUFS" << std::endl;
    std::cout << "响度范围: " << result.perceptual.loudness_range_lu << " LU" << std::endl;
    std::cout << "综合音质评分: " << result.perceptual.audio_quality_score << " (0-100)" << std::endl;
    std::cout << "感知熵: " << result.perceptual.perceptual_entropy << std::endl;
    std::cout << "Bark频谱失真: " << result.perceptual.bark_spectral_distortion << " (0-10)" << std::endl;
    std::cout << "Mel倒谱失真: " << result.perceptual.mel_cepstral_distortion << " (0-5)" << std::endl;
    
    // 频域分析
    std::cout << "\n--- 频域分析 ---" << std::endl;
    std::cout << "频谱质心: " << result.frequency.spectral_centroid << " Hz" << std::endl;
    std::cout << "频谱滚降点: " << result.frequency.spectral_rolloff << " Hz" << std::endl;
    std::cout << "高频能量比例: " << result.frequency.high_frequency_energy << " %" << std::endl;
    std::cout << "中频能量比例: " << result.frequency.mid_frequency_energy << " %" << std::endl;
    std::cout << "低频能量比例: " << result.frequency.low_frequency_energy << " %" << std::endl;
    std::cout << "过零率: " << result.frequency.zero_crossing_rate << std::endl;
    
    std::cout << "\n===================" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "音质分析器测试程序" << std::endl;
    if (argc < 2) {
        std::cout << "请提供测试音频文件路径。" << std::endl;
        return 1;

    }

    std::string testFile = argv[1];

    AudioQualityAnalyzer analyzer;
    
    // 设置分析参数
    analyzer.setAnalysisParameters(4096, 0.5, true);
    
    // 测试单文件分析
    std::cout << "\n测试1: 单文件音质分析" << std::endl;
    auto result1 = analyzer.analyzeAudioFile(testFile);
    printQualityReport(result1);
    
    // 测试文件比较
    if (argc > 2) {
        std::cout << "\n测试2: 文件比较分析" << std::endl;
        std::cout << "与原始音频比较" << std::endl;
        std::string originalFile = argv[2];
        std::string encodedFile = testFile;
        auto result2 = analyzer.compareAudioFiles(originalFile, encodedFile);
        printQualityReport(result2);
    }
    
    // 测试音频文件信息
    std::cout << "\n测试4: 音频文件信息" << std::endl;
    auto fileInfo = analyzer.getAudioFileInfo(testFile);
    std::cout << "采样率: " << fileInfo.sampleRate << " Hz" << std::endl;
    std::cout << "声道数: " << fileInfo.channels << std::endl;
    std::cout << "位深度: " << fileInfo.bitDepth << " bit" << std::endl;
    std::cout << "时长: " << fileInfo.duration << " 秒" << std::endl;
    std::cout << "格式: " << fileInfo.format << std::endl;
    
    return 0;
}
