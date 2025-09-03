# 音质分析功能使用指南

## 概述

新增的音质分析功能为音频编码器测试程序提供了全面的音质评估能力，包括客观音质指标、感知音质指标和频域分析。

## 功能特性

### 1. 客观音质指标 (ObjectiveQualityMetrics)

- **信噪比 (SNR)**: 信号与噪声功率的比值，单位dB
- **总谐波失真 (THD)**: 谐波分量与基波的比值，百分比
- **总谐波失真+噪声 (THD+N)**: 包含噪声的总失真
- **动态范围**: 最大与最小信号的比值，单位dB
- **声道分离度**: 左右声道的隔离程度，单位dB
- **立体声成像**: 立体声成像质量，范围0-1
- **互调失真 (IMD)**: 不同频率间的相互干扰
- **峰值/RMS比值**: 峰值与有效值的比值，单位dB
- **波峰因数**: 信号的峰值特性

### 2. 感知音质指标 (PerceptualQualityMetrics)

- **PESQ评分**: 感知语音质量评估，范围1.0-4.5
- **STOI评分**: 短时客观可懂度指数，范围0-1
- **MOS评分**: 平均意见分数，范围1-5
- **LUFS响度**: ITU-R BS.1770标准响度
- **响度范围**: 响度变化范围，单位LU
- **感知熵**: 频谱的感知复杂度
- **Bark频谱失真**: 基于Bark标度的频谱失真
- **Mel倒谱失真**: 基于Mel标度的倒谱失真
- **综合音质评分**: 0-100的综合评分

### 3. 频域分析 (FrequencyAnalysis)

- **幅度谱**: 频率域的幅度分布
- **相位谱**: 频率域的相位分布
- **功率谱**: 频率域的功率分布
- **频谱质心**: 频谱的重心频率，单位Hz
- **频谱滚降点**: 85%能量对应的频率点
- **频谱平坦度**: 频谱的平坦程度
- **频谱流量**: 频谱随时间的变化率
- **频段能量分布**: 低频、中频、高频的能量比例
- **过零率**: 信号过零点的频率

## 使用方法

### 1. 基本用法

```cpp
#include "AudioQualityAnalyzer.h"

// 创建分析器实例
AudioQualityAnalyzer analyzer;

// 设置分析参数
analyzer.setAnalysisParameters(4096, 0.5, true);

// 分析单个音频文件
auto result = analyzer.analyzeAudioFile("audio.wav");

// 检查分析结果
if (result.analysisSuccess) {
    std::cout << "SNR: " << result.objective.snr << " dB" << std::endl;
    std::cout << "MOS: " << result.perceptual.mos_score << std::endl;
}
```

### 2. 文件比较分析

```cpp
// 比较原始文件和编码后文件
auto comparison = analyzer.compareAudioFiles("original.wav", "encoded.mp3");

if (comparison.analysisSuccess) {
    std::cout << "编码质量分析:" << std::endl;
    std::cout << "SNR: " << comparison.objective.snr << " dB" << std::endl;
    std::cout << "音质评分: " << comparison.perceptual.audio_quality_score << std::endl;
}
```

### 3. 集成到编码器测试

```cpp
// 在EncoderManager中启用音质分析
EncoderManager manager("test_audio.wav");
manager.enableQualityAnalysis(true);

// 测试编码器时自动进行音质分析
manager.testSingleEncoderWithParams(EncoderType::VORBIS, 6);
```

### 4. 静态方法使用

```cpp
// 计算SNR
std::vector<double> signal = {...};
std::vector<double> noise = {...};
double snr = AudioQualityAnalyzer::calculateSNR(signal, noise);

// 计算THD
double thd = AudioQualityAnalyzer::calculateTHD(signal, 44100, 1000.0);

// 计算LUFS
std::vector<std::vector<double>> channels = {...};
double lufs = AudioQualityAnalyzer::calculateLUFS(channels, 44100);
```

## 配置参数

### 分析窗口大小 (analysisWindowSize)
- 默认值: 4096
- 影响: 频域分析精度，值越大精度越高但速度越慢

### 重叠比例 (overlapRatio)
- 默认值: 0.5
- 范围: 0.0-1.0
- 影响: 时域分析的平滑程度

### 详细分析 (enableDetailedAnalysis)
- 默认值: true
- 影响: 是否执行完整的感知模型分析

## 输出示例

```
=== 音质分析报告 ===
分析时间: 2025-09-03 14:30:15
分析状态: 成功

--- 客观音质指标 ---
信噪比 (SNR): 45.23 dB
总谐波失真 (THD): 0.12 %
动态范围: 72.45 dB
声道分离度: 65.78 dB
立体声成像: 0.85
峰值/RMS比值: 18.32 dB

--- 感知音质指标 ---
MOS评分: 4.2 (1-5)
LUFS响度: -16.5 LUFS
综合音质评分: 87.3 (0-100)
感知熵: 6.42

--- 频域分析 ---
频谱质心: 2456.7 Hz
频谱滚降点: 8932.1 Hz
高频能量比例: 15.6 %
中频能量比例: 68.2 %
低频能量比例: 16.2 %
过零率: 0.0823
```

## 应用场景

### 1. 编码器性能评估
- 比较不同编码器的音质表现
- 评估编码参数对音质的影响
- 验证编码器改进的效果

### 2. 音频质量监控
- 实时监控音频处理质量
- 检测音频传输过程中的质量损失
- 音频设备质量评估

### 3. 研发与优化
- Vorbis声道耦合改进验证
- 新算法音质效果评估
- 参数调优指导

## 注意事项

1. **性能考虑**: 详细分析会消耗较多计算资源，建议在开发阶段使用
2. **文件格式**: 目前主要支持WAV格式，其他格式需要额外的解码支持
3. **精度限制**: 某些指标（如PESQ）为简化实现，实际应用中可能需要专业库
4. **参考标准**: 分析结果应结合具体应用场景和标准进行解释

## 扩展说明

### 后续改进方向
1. 集成专业音质评估库（如PESQ、STOI的官方实现）
2. 支持更多音频格式的直接分析
3. 添加实时音质监控功能
4. 增加可视化分析图表
5. 支持批量文件分析和统计报告

### 依赖库建议
- **FFTW**: 用于高性能FFT计算
- **libsndfile**: 用于多格式音频文件支持
- **PESQ库**: 用于标准PESQ评估
- **STOI库**: 用于标准STOI计算

通过这套音质分析系统，您可以全面评估Vorbis声道耦合改进算法的效果，量化分析音质提升程度。
