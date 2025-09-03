# 音频质量评估工具

这个工具包提供了多种方式来评估Vorbis编码后的OGG文件与原始音频文件的质量差异，支持PESQ、POLQA等多种音频质量指标的计算。

## 工具概览

### 1. 完整版评估工具 (`audio_quality_evaluator.py`)
- 支持PESQ、POLQA、SNR、THD+N、频谱失真等多种指标
- 依赖外部库（librosa、soundfile、pesq等）
- 提供最全面的质量评估

### 2. 简化版评估工具 (`simple_quality_evaluator.py`)
- 基于基础Python库实现
- 支持SNR、相关系数、MSE、PESQ估算等基础指标
- 无复杂外部依赖，仅需ffmpeg处理OGG文件

### 3. 批量评估工具 (`batch_quality_evaluator.py`)
- 支持批量处理多个音频文件对
- 自动生成汇总报告和CSV文件
- 可配置文件匹配模式

## 安装依赖

### 方法1：使用安装脚本（推荐）
```bash
cd testQuality
chmod +x install_dependencies.sh
./install_dependencies.sh
```

### 方法2：手动安装
```bash
# 基础依赖（简化版工具必需）
brew install ffmpeg  # macOS上安装ffmpeg

# 完整版工具的额外依赖
pip3 install numpy scipy librosa soundfile pesq
```

## 使用方法

### 1. 单文件质量评估

#### 使用完整版工具
```bash
# 基本用法
python3 audio_quality_evaluator.py reference.wav encoded.ogg

# 指定采样率和输出文件
python3 audio_quality_evaluator.py -r reference.wav -d encoded.ogg -s 16000 -o results.json

# 只保存结果不显示
python3 audio_quality_evaluator.py reference.wav encoded.ogg -o results.json --no-display
```

#### 使用简化版工具
```bash
# 基本用法（推荐新手使用）
python3 simple_quality_evaluator.py reference.wav encoded.ogg

# 保存结果
python3 simple_quality_evaluator.py -r reference.wav -d encoded.ogg -o results.json
```

### 2. 批量质量评估

```bash
# 批量评估两个目录中的音频文件
python3 batch_quality_evaluator.py -r original_audio/ -d encoded_audio/ -o batch_results/

# 指定文件模式
python3 batch_quality_evaluator.py -r original/ -d encoded/ \
    --ref-pattern "*.wav" --deg-pattern "*.ogg" -o results/

# 单文件批量模式
python3 batch_quality_evaluator.py --single original.wav encoded.ogg -o results/
```

## 支持的音频格式

### 完整版工具
- WAV, OGG, MP3, FLAC, M4A等（通过librosa支持）

### 简化版工具
- WAV（原生支持）
- OGG（需要ffmpeg）

## 输出指标说明

### 基础指标
- **SNR (信噪比)**: 信号与噪声的功率比，单位dB，越高越好
- **相关系数**: 原始信号与编码信号的相似度，范围0-1，越高越好
- **MSE (均方误差)**: 信号差异的平方均值，越小越好
- **频谱失真**: 频域差异，单位dB，越小越好

### 感知质量指标
- **PESQ**: 感知语音质量评估，范围1.0-4.5，越高越好
- **POLQA**: 感知客观听觉质量分析，范围1.0-5.0，越高越好
- **综合质量评分**: 基于多个指标的综合评分，范围1.0-5.0

### 质量等级
- **优秀** (4.5-5.0): 几乎无法察觉的差异
- **良好** (4.0-4.5): 轻微差异，可接受的质量
- **一般** (3.0-4.0): 明显差异，但仍可使用
- **较差** (2.0-3.0): 显著劣化，质量有限
- **很差** (1.0-2.0): 严重劣化，不推荐使用

## 输出文件

### JSON结果文件
包含所有计算的指标和元数据：
```json
{
  "timestamp": "2025-09-03T10:30:00",
  "reference_file": "original.wav",
  "degraded_file": "encoded.ogg",
  "snr_db": 25.6,
  "correlation": 0.9845,
  "pesq_estimated": 3.2,
  "overall_quality_score": 4.1
}
```

### 批量评估输出
- `detailed_results.json`: 所有文件的详细评估结果
- `summary_report.json`: 汇总统计信息
- `results.csv`: CSV格式的结果表格

## 使用场景

### 1. 编码器参数优化
比较不同编码参数对音频质量的影响：
```bash
# 评估不同比特率的编码结果
python3 simple_quality_evaluator.py original.wav encoded_128k.ogg -o results_128k.json
python3 simple_quality_evaluator.py original.wav encoded_192k.ogg -o results_192k.json
python3 simple_quality_evaluator.py original.wav encoded_256k.ogg -o results_256k.json
```

### 2. 编码器性能测试
批量评估编码器在不同音频类型上的表现：
```bash
python3 batch_quality_evaluator.py -r test_audio/ -d encoded_audio/ -o performance_test/
```

### 3. 质量阈值验证
确定可接受的质量阈值：
```bash
# 生成质量报告
python3 batch_quality_evaluator.py -r references/ -d candidates/ -o quality_analysis/
```

## 注意事项

1. **采样率一致性**: 确保参考音频和待测音频有相同的采样率，工具会自动重采样但可能影响精度

2. **文件长度**: 工具会自动对齐音频长度，取较短的长度进行比较

3. **PESQ限制**: 真实的PESQ评估需要商业软件，此工具提供的是基于其他指标的估算值

4. **OGG文件处理**: 简化版工具需要ffmpeg来处理OGG文件

5. **内存使用**: 处理大文件时注意内存使用，建议将长音频切分为较短段落

## 故障排除

### 常见错误

1. **找不到ffmpeg**
   ```bash
   # macOS
   brew install ffmpeg
   
   # Ubuntu/Debian
   sudo apt-get install ffmpeg
   ```

2. **Python库缺失**
   ```bash
   pip3 install numpy scipy librosa soundfile
   ```

3. **文件格式不支持**
   - 简化版工具：转换为WAV格式
   - 完整版工具：检查librosa是否支持该格式

4. **采样率不匹配警告**
   - 这是正常的，工具会自动处理
   - 如需精确比较，请预先统一采样率

## 开发和扩展

### 添加新的质量指标
在评估器类中添加新的方法：
```python
def calculate_new_metric(self, ref_audio, deg_audio):
    # 实现新的指标计算
    return metric_value
```

### 支持新的音频格式
修改 `load_audio_file` 方法或添加格式转换逻辑。

### 自定义评分算法
修改 `calculate_quality_score` 方法中的权重和公式。

## 许可证

此工具包基于开源协议提供，可自由使用和修改。
