#!/usr/bin/env python3
"""
音频质量评估工具测试脚本
生成测试音频文件并验证评估工具的功能
"""

import os
import sys
import numpy as np
import json
import subprocess
from datetime import datetime

def generate_test_wav(filename, duration=5, sample_rate=16000, frequency=1000):
    """
    生成测试用的WAV文件
    
    Args:
        filename: 输出文件名
        duration: 时长（秒）
        sample_rate: 采样率
        frequency: 信号频率
    """
    import wave
    import struct
    
    # 生成正弦波
    t = np.linspace(0, duration, int(sample_rate * duration))
    signal = np.sin(2 * np.pi * frequency * t)
    
    # 转换为16位整数
    signal_int = (signal * 32767).astype(np.int16)
    
    # 写入WAV文件
    with wave.open(filename, 'wb') as wav_file:
        wav_file.setnchannels(1)  # 单声道
        wav_file.setsampwidth(2)  # 16位
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(signal_int.tobytes())
    
    print(f"生成测试文件: {filename}")

def add_noise_to_signal(signal, snr_db):
    """
    向信号添加噪声
    
    Args:
        signal: 原始信号
        snr_db: 目标信噪比(dB)
    
    Returns:
        添加噪声后的信号
    """
    # 计算信号功率
    signal_power = np.mean(signal ** 2)
    
    # 计算噪声功率
    snr_linear = 10 ** (snr_db / 10)
    noise_power = signal_power / snr_linear
    
    # 生成噪声
    noise = np.random.normal(0, np.sqrt(noise_power), len(signal))
    
    return signal + noise

def generate_degraded_wav(original_file, degraded_file, degradation_type="noise", snr_db=20):
    """
    生成劣化版本的音频文件
    
    Args:
        original_file: 原始文件
        degraded_file: 劣化文件
        degradation_type: 劣化类型
        snr_db: 信噪比（如果是噪声劣化）
    """
    import wave
    import struct
    
    # 读取原始文件
    with wave.open(original_file, 'rb') as wav_file:
        frames = wav_file.getnframes()
        sample_rate = wav_file.getframerate()
        audio_data = wav_file.readframes(frames)
        
        # 转换为浮点数组
        signal = np.array(struct.unpack(f'<{frames}h', audio_data), dtype=np.float32) / 32768.0
    
    # 应用劣化
    if degradation_type == "noise":
        degraded_signal = add_noise_to_signal(signal, snr_db)
    elif degradation_type == "lowpass":
        # 简单的低通滤波（截断高频）
        fft_signal = np.fft.fft(signal)
        cutoff_freq = 4000  # 4kHz截止频率
        cutoff_bin = int(cutoff_freq * len(signal) / sample_rate)
        fft_signal[cutoff_bin:-cutoff_bin] = 0
        degraded_signal = np.real(np.fft.ifft(fft_signal))
    elif degradation_type == "compression":
        # 简单的动态范围压缩
        degraded_signal = np.sign(signal) * np.power(np.abs(signal), 0.7)
    else:
        degraded_signal = signal
    
    # 限制幅度范围
    degraded_signal = np.clip(degraded_signal, -1.0, 1.0)
    
    # 转换回16位整数
    degraded_int = (degraded_signal * 32767).astype(np.int16)
    
    # 写入WAV文件
    with wave.open(degraded_file, 'wb') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(degraded_int.tobytes())
    
    print(f"生成劣化文件: {degraded_file} (类型: {degradation_type})")

def convert_wav_to_ogg(wav_file, ogg_file, quality=3):
    """
    使用oggenc将WAV转换为OGG
    
    Args:
        wav_file: 输入WAV文件
        ogg_file: 输出OGG文件
        quality: Vorbis质量等级 (-1到10)
    """
    try:
        # 尝试使用oggenc
        cmd = ['oggenc', '-q', str(quality), '-o', ogg_file, wav_file]
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            print(f"转换为OGG: {ogg_file} (质量: {quality})")
            return True
        else:
            print(f"oggenc转换失败: {result.stderr}")
            return False
    except FileNotFoundError:
        try:
            # 尝试使用ffmpeg
            cmd = ['ffmpeg', '-i', wav_file, '-c:a', 'libvorbis', '-q:a', str(quality), '-y', ogg_file]
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                print(f"转换为OGG (ffmpeg): {ogg_file}")
                return True
            else:
                print(f"ffmpeg转换失败: {result.stderr}")
                return False
        except FileNotFoundError:
            print("错误: 未找到oggenc或ffmpeg，无法转换为OGG格式")
            return False

def test_evaluator(evaluator_script, ref_file, deg_file, output_file):
    """
    测试评估器
    
    Args:
        evaluator_script: 评估器脚本路径
        ref_file: 参考文件
        deg_file: 劣化文件
        output_file: 输出文件
    
    Returns:
        测试是否成功
    """
    try:
        cmd = ['python3', evaluator_script, ref_file, deg_file, '-o', output_file]
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            print(f"✓ {evaluator_script} 测试成功")
            return True
        else:
            print(f"✗ {evaluator_script} 测试失败:")
            print(result.stderr)
            return False
    except Exception as e:
        print(f"✗ {evaluator_script} 测试出错: {str(e)}")
        return False

def analyze_test_results(result_files):
    """分析测试结果"""
    print("\n" + "="*50)
    print("测试结果分析")
    print("="*50)
    
    for result_file in result_files:
        if os.path.exists(result_file):
            try:
                with open(result_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                
                print(f"\n{os.path.basename(result_file)}:")
                if 'overall_quality_score' in data:
                    print(f"  综合质量评分: {data['overall_quality_score']:.3f}")
                if 'snr_db' in data:
                    snr = data['snr_db']
                    if snr == float('inf'):
                        print(f"  信噪比: 无穷大 (完美)")
                    else:
                        print(f"  信噪比: {snr:.2f} dB")
                if 'correlation' in data:
                    print(f"  相关系数: {data['correlation']:.4f}")
                
            except Exception as e:
                print(f"✗ 无法解析 {result_file}: {str(e)}")
        else:
            print(f"✗ 结果文件不存在: {result_file}")

def main():
    """主测试函数"""
    print("音频质量评估工具测试")
    print("="*50)
    
    # 创建测试目录
    test_dir = "test_audio"
    os.makedirs(test_dir, exist_ok=True)
    
    # 生成测试文件
    print("\n1. 生成测试音频文件...")
    original_wav = os.path.join(test_dir, "original.wav")
    generate_test_wav(original_wav, duration=3, frequency=1000)
    
    # 生成不同类型的劣化文件
    noise_wav = os.path.join(test_dir, "noise_degraded.wav")
    lowpass_wav = os.path.join(test_dir, "lowpass_degraded.wav")
    
    generate_degraded_wav(original_wav, noise_wav, "noise", snr_db=15)
    generate_degraded_wav(original_wav, lowpass_wav, "lowpass")
    
    # 转换为OGG格式
    print("\n2. 转换为OGG格式...")
    original_ogg = os.path.join(test_dir, "original.ogg")
    noise_ogg = os.path.join(test_dir, "noise_degraded.ogg")
    
    convert_wav_to_ogg(original_wav, original_ogg, quality=6)
    convert_wav_to_ogg(noise_wav, noise_ogg, quality=3)
    
    # 测试评估器
    print("\n3. 测试评估器...")
    
    test_cases = [
        ("original.wav", "noise_degraded.wav", "应该显示中等质量"),
        ("original.wav", "lowpass_degraded.wav", "应该显示频率受限"),
        ("original.wav", "original.ogg", "应该显示高质量"),
        ("original.wav", "noise_degraded.ogg", "应该显示编码+噪声劣化")
    ]
    
    # 测试简化版评估器
    simple_evaluator = "simple_quality_evaluator.py"
    if os.path.exists(simple_evaluator):
        print(f"\n测试 {simple_evaluator}:")
        result_files = []
        
        for i, (ref, deg, description) in enumerate(test_cases):
            ref_path = os.path.join(test_dir, ref)
            deg_path = os.path.join(test_dir, deg)
            result_file = f"test_result_{i+1}.json"
            
            if os.path.exists(ref_path) and os.path.exists(deg_path):
                print(f"\n测试案例 {i+1}: {description}")
                print(f"  参考: {ref}")
                print(f"  测试: {deg}")
                
                if test_evaluator(simple_evaluator, ref_path, deg_path, result_file):
                    result_files.append(result_file)
        
        # 分析结果
        analyze_test_results(result_files)
    
    # 测试完整版评估器
    advanced_evaluator = "audio_quality_evaluator.py"
    if os.path.exists(advanced_evaluator):
        print(f"\n\n测试 {advanced_evaluator}:")
        
        # 检查依赖
        try:
            import librosa
            import soundfile
            print("✓ 依赖库已安装")
            
            # 运行一个简单测试
            ref_path = os.path.join(test_dir, "original.wav")
            deg_path = os.path.join(test_dir, "noise_degraded.wav")
            
            if os.path.exists(ref_path) and os.path.exists(deg_path):
                test_evaluator(advanced_evaluator, ref_path, deg_path, "advanced_test_result.json")
        
        except ImportError as e:
            print(f"✗ 依赖库缺失: {str(e)}")
            print("请运行 install_dependencies.sh 安装依赖")
    
    # 测试批量评估器
    batch_evaluator = "batch_quality_evaluator.py"
    if os.path.exists(batch_evaluator):
        print(f"\n\n测试 {batch_evaluator}:")
        
        # 创建批量测试目录
        ref_dir = os.path.join(test_dir, "references")
        deg_dir = os.path.join(test_dir, "degraded")
        os.makedirs(ref_dir, exist_ok=True)
        os.makedirs(deg_dir, exist_ok=True)
        
        # 复制文件到批量测试目录
        import shutil
        
        if os.path.exists(original_wav):
            shutil.copy(original_wav, os.path.join(ref_dir, "test1.wav"))
            shutil.copy(original_wav, os.path.join(ref_dir, "test2.wav"))
        
        if os.path.exists(noise_wav):
            shutil.copy(noise_wav, os.path.join(deg_dir, "test1.ogg"))
        if os.path.exists(lowpass_wav):
            shutil.copy(lowpass_wav, os.path.join(deg_dir, "test2.ogg"))
        
        # 运行批量测试
        try:
            cmd = ['python3', batch_evaluator, '-r', ref_dir, '-d', deg_dir, 
                  '--deg-pattern', '*.ogg', '-o', 'batch_test_results']
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                print("✓ 批量评估测试成功")
            else:
                print(f"✗ 批量评估测试失败: {result.stderr}")
        except Exception as e:
            print(f"✗ 批量评估测试出错: {str(e)}")
    
    print("\n" + "="*50)
    print("测试完成!")
    print(f"测试文件位于: {test_dir}/")
    print("如果看到错误，请检查依赖安装或参考README.md")
    print("="*50)

if __name__ == '__main__':
    main()
