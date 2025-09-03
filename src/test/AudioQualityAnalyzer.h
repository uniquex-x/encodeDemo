#ifndef AUDIO_QUALITY_ANALYZER_H
#define AUDIO_QUALITY_ANALYZER_H

#include <string>
#include <vector>
#include <complex>
#include "AudioFileLoader.h"

/**
 * 音频质量分析器
 * 提供客观音质指标和感知音质指标的计算
 */
class AudioQualityAnalyzer {
public:
    /**
     * 客观音质指标结构体
     */
    struct ObjectiveQualityMetrics {
        double snr = 0.0;                    // 信噪比 (dB)
        double thd = 0.0;                    // 总谐波失真 (%)
        double thd_n = 0.0;                  // 总谐波失真+噪声 (%)
        double dynamicRange = 0.0;           // 动态范围 (dB)
        double frequencyResponseFlatness = 0.0; // 频率响应平坦度 (±dB)
        double channelSeparation = 0.0;      // 声道分离度 (dB)
        double stereoImaging = 0.0;          // 立体声成像质量 (0-1)
        double intermodulationDistortion = 0.0; // 互调失真 (%)
        double peakToRmsRatio = 0.0;         // 峰值/RMS比值 (dB)
        double crestFactor = 0.0;            // 波峰因数 (dB)
    };

    /**
     * 感知音质指标结构体
     */
    struct PerceptualQualityMetrics {
        double pesq_score = 0.0;             // PESQ评分 (1.0-4.5)
        double stoi_score = 0.0;             // 短时客观可懂度指数 (0-1)
        double mos_score = 0.0;              // 平均意见分数 (1-5)
        double loudness_lufs = 0.0;          // 响度LUFS值
        double loudness_range_lu = 0.0;      // 响度范围LU值
        double perceptual_entropy = 0.0;     // 感知熵
        double bark_spectral_distortion = 0.0; // Bark频谱失真
        double mel_cepstral_distortion = 0.0;   // Mel倒谱失真
        double audio_quality_score = 0.0;    // 综合音质评分 (0-100)
    };

    /**
     * 频域分析结果结构体
     */
    struct FrequencyAnalysis {
        std::vector<double> magnitude_spectrum;  // 幅度谱
        std::vector<double> phase_spectrum;      // 相位谱
        std::vector<double> power_spectrum;      // 功率谱
        double spectral_centroid = 0.0;         // 频谱质心 (Hz)
        double spectral_rolloff = 0.0;          // 频谱滚降点 (Hz)
        double spectral_flatness = 0.0;         // 频谱平坦度
        double spectral_flux = 0.0;             // 频谱流量
        double high_frequency_energy = 0.0;     // 高频能量比例 (%)
        double low_frequency_energy = 0.0;      // 低频能量比例 (%)
        double mid_frequency_energy = 0.0;      // 中频能量比例 (%)
        double zero_crossing_rate = 0.0;        // 过零率
    };

    /**
     * 完整的音质分析结果
     */
    struct QualityAnalysisResult {
        ObjectiveQualityMetrics objective;
        PerceptualQualityMetrics perceptual;
        FrequencyAnalysis frequency;
        std::string analysisTimestamp;
        bool analysisSuccess = false;
        std::string errorMessage;
    };

    /**
     * 音频文件信息结构体
     */
    using AudioFileInfo = ::AudioFileInfo;

public:
    AudioQualityAnalyzer();
    ~AudioQualityAnalyzer();

    /**
     * 分析单个音频文件的质量
     * @param audioFile 音频文件路径
     * @return 音质分析结果
     */
    QualityAnalysisResult analyzeAudioFile(const std::string& audioFile);

    /**
     * 比较两个音频文件的质量差异（原始 vs 编码后）
     * @param originalFile 原始音频文件
     * @param encodedFile 编码后的音频文件
     * @return 音质分析结果（包含差异信息）
     */
    QualityAnalysisResult compareAudioFiles(const std::string& originalFile, 
                                           const std::string& encodedFile);

    /**
     * 获取音频文件基本信息
     */
    AudioFileInfo getAudioFileInfo(const std::string& audioFile);

    /**
     * 设置分析参数
     */
    void setAnalysisParameters(double analysisWindowSize = 4096, 
                              double overlapRatio = 0.5,
                              bool enableDetailedAnalysis = true);

    /**
     * 静态方法：计算两个信号的SNR
     */
    static double calculateSNR(const std::vector<double>& signal, 
                              const std::vector<double>& noise);

    /**
     * 静态方法：计算信号的THD
     */
    static double calculateTHD(const std::vector<double>& signal, 
                              int sampleRate, 
                              double fundamentalFreq);

    /**
     * 静态方法：计算LUFS响度
     */
    static double calculateLUFS(const std::vector<std::vector<double>>& audioChannels, 
                               int sampleRate);

private:
    // 私有成员变量
    double m_analysisWindowSize;
    double m_overlapRatio;
    bool m_enableDetailedAnalysis;
    AudioFileLoader m_fileLoader;

    // 私有方法
    std::vector<std::vector<double>> loadAudioData(const std::string& audioFile, AudioFileInfo& info);
    
    ObjectiveQualityMetrics calculateObjectiveMetrics(const std::vector<std::vector<double>>& audioData, 
                                                     const AudioFileInfo& info);
    
    PerceptualQualityMetrics calculatePerceptualMetrics(const std::vector<std::vector<double>>& audioData, 
                                                       const AudioFileInfo& info);
    
    FrequencyAnalysis calculateFrequencyAnalysis(const std::vector<std::vector<double>>& audioData, 
                                                const AudioFileInfo& info);

    // FFT相关方法
    std::vector<std::complex<double>> performFFT(const std::vector<double>& signal);
    std::vector<double> calculateMagnitudeSpectrum(const std::vector<std::complex<double>>& fft_result);
    std::vector<double> calculatePhaseSpectrum(const std::vector<std::complex<double>>& fft_result);

    // 感知模型相关方法
    std::vector<double> applyBarkScale(const std::vector<double>& spectrum, int sampleRate);
    std::vector<double> applyMelScale(const std::vector<double>& spectrum, int sampleRate);
    double calculatePerceptualEntropy(const std::vector<double>& spectrum);
    
    // 新增的感知音质计算方法
    double calculateBarkSpectralDistortion(const std::vector<double>& barkSpectrum);
    double calculateMelCepstralDistortion(const std::vector<double>& melSpectrum);
    double calculatePESQScore(const std::vector<std::vector<double>>& audioData, const AudioFileInfo& info);
    double calculateSTOIScore(const std::vector<std::vector<double>>& audioData, const AudioFileInfo& info);
    double calculateComprehensiveMOSScore(const PerceptualQualityMetrics& metrics, const AudioFileInfo& info);
    double calculateComprehensiveAudioQualityScore(const PerceptualQualityMetrics& metrics);

    // 音质评估算法
    double calculateMOSScore(const ObjectiveQualityMetrics& objective);
    double calculateAudioQualityScore(const ObjectiveQualityMetrics& objective, 
                                     const PerceptualQualityMetrics& perceptual);

    // 工具方法
    double rmsValue(const std::vector<double>& signal);
    double peakValue(const std::vector<double>& signal);
    double calculateDynamicRange(const std::vector<double>& signal);
    std::vector<double> applyWindow(const std::vector<double>& signal, const std::string& windowType = "hann");
};

#endif // AUDIO_QUALITY_ANALYZER_H
