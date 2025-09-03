#!/usr/bin/env python3
"""
音频质量评估工具
支持PESQ、POLQA、SNR、THD+N等多种音频质量指标的计算
"""

import argparse
import os
import sys
import numpy as np
import librosa
import soundfile as sf
from scipy import signal
from scipy.stats import pearsonr
import json
from datetime import datetime
import warnings
warnings.filterwarnings('ignore')

class AudioQualityEvaluator:
    """音频质量评估器"""
    
    def __init__(self, sample_rate=16000):
        """
        初始化评估器
        
        Args:
            sample_rate: 采样率，PESQ和POLQA通常需要8kHz或16kHz
        """
        self.sample_rate = sample_rate
        self.results = {}
        
    def load_audio(self, file_path, target_sr=None):
        """
        加载音频文件
        
        Args:
            file_path: 音频文件路径
            target_sr: 目标采样率
            
        Returns:
            tuple: (音频数据, 采样率)
        """
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"音频文件不存在: {file_path}")
            
        try:
            # 使用librosa加载音频，自动转换为单声道
            audio, sr = librosa.load(file_path, sr=target_sr or self.sample_rate, mono=True)
            return audio, sr
        except Exception as e:
            raise ValueError(f"无法加载音频文件 {file_path}: {str(e)}")
    
    def align_audio_length(self, ref_audio, deg_audio):
        """
        对齐两个音频的长度
        
        Args:
            ref_audio: 参考音频
            deg_audio: 待测音频
            
        Returns:
            tuple: 对齐后的音频数据
        """
        min_len = min(len(ref_audio), len(deg_audio))
        return ref_audio[:min_len], deg_audio[:min_len]
    
    def calculate_snr(self, ref_audio, deg_audio):
        """
        计算信噪比 (Signal-to-Noise Ratio)
        
        Args:
            ref_audio: 参考音频
            deg_audio: 待测音频
            
        Returns:
            float: SNR值 (dB)
        """
        # 对齐音频长度
        ref_aligned, deg_aligned = self.align_audio_length(ref_audio, deg_audio)
        
        # 计算噪声（差值）
        noise = ref_aligned - deg_aligned
        
        # 计算信号功率和噪声功率
        signal_power = np.mean(ref_aligned ** 2)
        noise_power = np.mean(noise ** 2)
        
        # 避免除零
        if noise_power == 0:
            return float('inf')
        
        snr = 10 * np.log10(signal_power / noise_power)
        return snr
    
    def calculate_thd_n(self, audio, sr, fundamental_freq=1000):
        """
        计算总谐波失真加噪声 (THD+N)
        
        Args:
            audio: 音频数据
            sr: 采样率
            fundamental_freq: 基频
            
        Returns:
            float: THD+N值 (%)
        """
        # 计算FFT
        fft = np.fft.fft(audio)
        freqs = np.fft.fftfreq(len(audio), 1/sr)
        
        # 找到基频位置
        fundamental_idx = np.argmin(np.abs(freqs - fundamental_freq))
        
        # 计算基频幅值
        fundamental_amplitude = np.abs(fft[fundamental_idx])
        
        # 计算总功率
        total_power = np.sum(np.abs(fft) ** 2)
        
        # 计算基频功率
        fundamental_power = fundamental_amplitude ** 2
        
        # THD+N = (总功率 - 基频功率) / 基频功率
        if fundamental_power == 0:
            return float('inf')
        
        thd_n = np.sqrt((total_power - fundamental_power) / fundamental_power) * 100
        return thd_n
    
    def calculate_correlation(self, ref_audio, deg_audio):
        """
        计算音频相关性
        
        Args:
            ref_audio: 参考音频
            deg_audio: 待测音频
            
        Returns:
            float: 相关系数
        """
        ref_aligned, deg_aligned = self.align_audio_length(ref_audio, deg_audio)
        correlation, _ = pearsonr(ref_aligned, deg_aligned)
        return correlation
    
    def calculate_spectral_distortion(self, ref_audio, deg_audio, sr):
        """
        计算频谱失真
        
        Args:
            ref_audio: 参考音频
            deg_audio: 待测音频
            sr: 采样率
            
        Returns:
            float: 频谱失真 (dB)
        """
        # 对齐音频长度
        ref_aligned, deg_aligned = self.align_audio_length(ref_audio, deg_audio)
        
        # 计算STFT
        _, _, ref_stft = signal.stft(ref_aligned, fs=sr, nperseg=512)
        _, _, deg_stft = signal.stft(deg_aligned, fs=sr, nperseg=512)
        
        # 计算功率谱
        ref_power = np.abs(ref_stft) ** 2
        deg_power = np.abs(deg_stft) ** 2
        
        # 计算对数功率谱差值
        ref_log = 10 * np.log10(ref_power + 1e-10)
        deg_log = 10 * np.log10(deg_power + 1e-10)
        
        # 计算均方根差值
        spectral_dist = np.sqrt(np.mean((ref_log - deg_log) ** 2))
        return spectral_dist
    
    def calculate_pesq_estimation(self, ref_audio, deg_audio, sr):
        """
        PESQ估算（简化版本，因为真实PESQ需要专门的库）
        
        Args:
            ref_audio: 参考音频
            deg_audio: 待测音频
            sr: 采样率
            
        Returns:
            float: PESQ估算值
        """
        # 这是一个简化的PESQ估算，基于SNR和频谱失真
        snr = self.calculate_snr(ref_audio, deg_audio)
        spec_dist = self.calculate_spectral_distortion(ref_audio, deg_audio, sr)
        correlation = self.calculate_correlation(ref_audio, deg_audio)
        
        # 简化的PESQ映射公式（经验公式）
        if snr == float('inf'):
            pesq_est = 4.5
        else:
            pesq_est = 1.0 + 3.5 * (1 / (1 + np.exp(-(snr - 20) / 5))) * correlation * (1 / (1 + spec_dist / 10))
        
        return max(1.0, min(4.5, pesq_est))
    
    def try_pesq(self, ref_audio, deg_audio, sr):
        """
        尝试使用真实的PESQ库
        
        Args:
            ref_audio: 参考音频
            deg_audio: 待测音频
            sr: 采样率
            
        Returns:
            float or None: PESQ值或None
        """
        try:
            from pesq import pesq
            # PESQ要求采样率为8kHz或16kHz
            if sr not in [8000, 16000]:
                # 重采样到16kHz
                ref_resampled = librosa.resample(ref_audio, orig_sr=sr, target_sr=16000)
                deg_resampled = librosa.resample(deg_audio, orig_sr=sr, target_sr=16000)
                sr = 16000
            else:
                ref_resampled, deg_resampled = ref_audio, deg_audio
            
            # 对齐长度
            ref_aligned, deg_aligned = self.align_audio_length(ref_resampled, deg_resampled)
            
            # 计算PESQ
            pesq_score = pesq(sr, ref_aligned, deg_aligned, 'wb')  # wideband
            return pesq_score
        except ImportError:
            print("Warning: PESQ库未安装，使用估算值。可通过 'pip install pesq' 安装")
            return None
        except Exception as e:
            print(f"Warning: PESQ计算失败: {str(e)}")
            return None
    
    def try_polqa(self, ref_audio, deg_audio, sr):
        """
        尝试使用POLQA（通常需要商业许可）
        
        Args:
            ref_audio: 参考音频
            deg_audio: 待测音频
            sr: 采样率
            
        Returns:
            float or None: POLQA值或None
        """
        # POLQA通常需要商业许可，这里返回None
        print("Warning: POLQA需要商业许可，使用基于机器学习的质量估算")
        return None
    
    def calculate_ml_quality_score(self, ref_audio, deg_audio, sr):
        """
        基于机器学习的音频质量评分（简化版）
        
        Args:
            ref_audio: 参考音频
            deg_audio: 待测音频
            sr: 采样率
            
        Returns:
            float: 质量评分 (1-5)
        """
        # 提取多个特征
        snr = self.calculate_snr(ref_audio, deg_audio)
        correlation = self.calculate_correlation(ref_audio, deg_audio)
        spec_dist = self.calculate_spectral_distortion(ref_audio, deg_audio, sr)
        
        # 简化的机器学习模型（基于经验权重）
        if snr == float('inf'):
            snr_score = 5.0
        else:
            snr_score = max(1.0, min(5.0, 1 + 4 * (1 / (1 + np.exp(-(snr - 15) / 5)))))
        
        corr_score = max(1.0, correlation * 5.0)
        spec_score = max(1.0, 5.0 - spec_dist / 2.0)
        
        # 加权平均
        ml_score = 0.4 * snr_score + 0.3 * corr_score + 0.3 * spec_score
        return max(1.0, min(5.0, ml_score))
    
    def evaluate(self, reference_file, degraded_file):
        """
        执行完整的音频质量评估
        
        Args:
            reference_file: 参考音频文件路径
            degraded_file: 待测音频文件路径
            
        Returns:
            dict: 评估结果
        """
        print(f"正在评估音频质量...")
        print(f"参考文件: {reference_file}")
        print(f"测试文件: {degraded_file}")
        
        # 加载音频文件
        try:
            ref_audio, ref_sr = self.load_audio(reference_file)
            deg_audio, deg_sr = self.load_audio(degraded_file)
            
            # 确保采样率一致
            if ref_sr != deg_sr:
                print(f"采样率不一致，将测试文件从 {deg_sr}Hz 重采样到 {ref_sr}Hz")
                deg_audio = librosa.resample(deg_audio, orig_sr=deg_sr, target_sr=ref_sr)
                deg_sr = ref_sr
            
            sr = ref_sr
            
        except Exception as e:
            raise ValueError(f"音频加载失败: {str(e)}")
        
        results = {
            'timestamp': datetime.now().isoformat(),
            'reference_file': reference_file,
            'degraded_file': degraded_file,
            'sample_rate': sr,
            'reference_duration': len(ref_audio) / sr,
            'degraded_duration': len(deg_audio) / sr,
        }
        
        # 计算各种指标
        print("计算中...")
        
        # 基础指标
        results['snr_db'] = self.calculate_snr(ref_audio, deg_audio)
        results['correlation'] = self.calculate_correlation(ref_audio, deg_audio)
        results['spectral_distortion_db'] = self.calculate_spectral_distortion(ref_audio, deg_audio, sr)
        
        # THD+N（仅对单音信号有意义）
        try:
            results['thd_n_percent'] = self.calculate_thd_n(deg_audio, sr)
        except:
            results['thd_n_percent'] = None
        
        # PESQ
        pesq_real = self.try_pesq(ref_audio, deg_audio, sr)
        if pesq_real is not None:
            results['pesq'] = pesq_real
        else:
            results['pesq_estimated'] = self.calculate_pesq_estimation(ref_audio, deg_audio, sr)
        
        # POLQA
        polqa_score = self.try_polqa(ref_audio, deg_audio, sr)
        if polqa_score is not None:
            results['polqa'] = polqa_score
        else:
            results['polqa_estimated'] = self.calculate_ml_quality_score(ref_audio, deg_audio, sr)
        
        # 综合质量评分
        results['overall_quality_score'] = self.calculate_ml_quality_score(ref_audio, deg_audio, sr)
        
        self.results = results
        return results
    
    def print_results(self):
        """打印评估结果"""
        if not self.results:
            print("没有评估结果可显示")
            return
        
        print("\n" + "="*60)
        print("音频质量评估结果")
        print("="*60)
        
        print(f"评估时间: {self.results['timestamp']}")
        print(f"参考文件: {os.path.basename(self.results['reference_file'])}")
        print(f"测试文件: {os.path.basename(self.results['degraded_file'])}")
        print(f"采样率: {self.results['sample_rate']} Hz")
        print(f"参考音频时长: {self.results['reference_duration']:.2f} 秒")
        print(f"测试音频时长: {self.results['degraded_duration']:.2f} 秒")
        
        print("\n基础指标:")
        print(f"  信噪比 (SNR): {self.results['snr_db']:.2f} dB")
        print(f"  相关系数: {self.results['correlation']:.4f}")
        print(f"  频谱失真: {self.results['spectral_distortion_db']:.2f} dB")
        
        if self.results.get('thd_n_percent'):
            print(f"  THD+N: {self.results['thd_n_percent']:.2f}%")
        
        print("\n感知质量指标:")
        if 'pesq' in self.results:
            print(f"  PESQ: {self.results['pesq']:.3f} (1.0-4.5, 越高越好)")
        elif 'pesq_estimated' in self.results:
            print(f"  PESQ (估算): {self.results['pesq_estimated']:.3f} (1.0-4.5, 越高越好)")
        
        if 'polqa' in self.results:
            print(f"  POLQA: {self.results['polqa']:.3f} (1.0-5.0, 越高越好)")
        elif 'polqa_estimated' in self.results:
            print(f"  POLQA (估算): {self.results['polqa_estimated']:.3f} (1.0-5.0, 越高越好)")
        
        print(f"  综合质量评分: {self.results['overall_quality_score']:.3f} (1.0-5.0, 越高越好)")
        
        # 质量等级
        score = self.results['overall_quality_score']
        if score >= 4.5:
            quality_level = "优秀"
        elif score >= 4.0:
            quality_level = "良好"
        elif score >= 3.0:
            quality_level = "一般"
        elif score >= 2.0:
            quality_level = "较差"
        else:
            quality_level = "很差"
        
        print(f"  质量等级: {quality_level}")
        print("="*60)
    
    def save_results(self, output_file):
        """保存结果到JSON文件"""
        if not self.results:
            print("没有评估结果可保存")
            return
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(self.results, f, indent=2, ensure_ascii=False)
        
        print(f"评估结果已保存到: {output_file}")


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='音频质量评估工具 - 支持PESQ、POLQA、SNR等多种指标',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  python audio_quality_evaluator.py reference.wav encoded.ogg
  python audio_quality_evaluator.py -r reference.wav -d encoded.ogg -s 16000 -o results.json
  
支持的音频格式:
  WAV, OGG, MP3, FLAC, M4A 等 (通过 librosa 支持)
  
评估指标说明:
  SNR: 信噪比，越高表示噪声越小
  PESQ: 感知语音质量评估 (1.0-4.5)
  POLQA: 感知客观听觉质量分析 (1.0-5.0)
  相关系数: 音频相似度 (0-1)
  频谱失真: 频域差异，越小越好
        """
    )
    
    # 位置参数
    parser.add_argument('reference', nargs='?', help='参考音频文件路径')
    parser.add_argument('degraded', nargs='?', help='待测音频文件路径')
    
    # 可选参数
    parser.add_argument('-r', '--reference', dest='ref_file', help='参考音频文件路径')
    parser.add_argument('-d', '--degraded', dest='deg_file', help='待测音频文件路径')
    parser.add_argument('-s', '--sample-rate', type=int, default=16000, 
                       help='目标采样率 (默认: 16000Hz)')
    parser.add_argument('-o', '--output', help='输出JSON结果文件路径')
    parser.add_argument('--no-display', action='store_true', help='不显示结果，只保存到文件')
    
    args = parser.parse_args()
    
    # 确定输入文件
    ref_file = args.reference or args.ref_file
    deg_file = args.degraded or args.deg_file
    
    if not ref_file or not deg_file:
        parser.print_help()
        print("\n错误: 必须指定参考音频文件和待测音频文件")
        sys.exit(1)
    
    try:
        # 创建评估器
        evaluator = AudioQualityEvaluator(sample_rate=args.sample_rate)
        
        # 执行评估
        results = evaluator.evaluate(ref_file, deg_file)
        
        # 显示结果
        if not args.no_display:
            evaluator.print_results()
        
        # 保存结果
        if args.output:
            evaluator.save_results(args.output)
        
    except Exception as e:
        print(f"错误: {str(e)}")
        sys.exit(1)


if __name__ == '__main__':
    main()
