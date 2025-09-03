#ifndef ENCODER_INFO_H
#define ENCODER_INFO_H

#include <string>

// 前向声明
struct ObjectiveQualityMetrics;
struct PerceptualQualityMetrics;
struct FrequencyAnalysis;

enum class EncoderType{
    FLAC,
    VORBIS,
    LAME
};

// mp3特定参数上下文
struct Mp3ParamContext{
    int vbr_quality;
    int vbr_quality_min;
    int vbr_quality_max;
};

// vorbis特定参数上下文
struct VorbisParamContext{
    int vbr_quality;
    
    int maxBitrate; //允许的最大比特率 
    int averageBitrate;  //流的期望平均比特率
    int minBitrate;  //最小比特率
    float reservoirSize; //储备池子大小
    float reservoirBias;  //储备池子偏差
    float averageTrackDamping;  //平均轨道阻尼

    //编码模型调整参数
    int managementMode; //配置是否使用比特率管理
    int Coupling; //立体声编码中是否启用声道耦合
    float ImpulseCodingAggressiveness; //脉冲编码激进程度
    
};

// flac特定参数上下文
struct FlacParamContext{
    int compression_level;      //压缩级别
    int compression_level_min;
    int compression_level_max;
};

//编码器参数上下文
struct EncoderParamContext{
    // 通用的编码参数
    EncoderType encoderType;
    int quality;
    
    Mp3ParamContext* mp3Ctx;    //mp3参数上下文
    VorbisParamContext* vorbisCtx;  //vorbis参数上下文
    FlacParamContext* FlacCtx;  //flac参数上下文
};

// 音质评估结果结构体
struct AudioQualityResults {
    // 客观音质指标
    struct {
        double snr = 0.0;                    // 信噪比 (dB)
        double thd = 0.0;                    // 总谐波失真 (%)
        double dynamicRange = 0.0;           // 动态范围 (dB)
        double channelSeparation = 0.0;      // 声道分离度 (dB)
        double stereoImaging = 0.0;          // 立体声成像质量 (0-1)
    } objective;
    
    // 感知音质指标
    struct {
        double mos_score = 0.0;              // 平均意见分数 (1-5)
        double loudness_lufs = 0.0;          // 响度LUFS值
        double audio_quality_score = 0.0;    // 综合音质评分 (0-100)
    } perceptual;
    
    // 频域分析
    struct {
        double spectral_centroid = 0.0;      // 频谱质心 (Hz)
        double high_frequency_energy = 0.0;  // 高频能量比例 (%)
        double spectral_flatness = 0.0;      // 频谱平坦度
    } frequency;
    
    bool analysisPerformed = false;
    std::string analysisError;
};

#endif