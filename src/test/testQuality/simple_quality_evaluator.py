#!/usr/bin/env python3
"""
简化版音频质量评估工具
基于基础Python库实现，减少外部依赖
"""

import argparse
import os
import sys
import json
import wave
import struct
import math
from datetime import datetime

class SimpleAudioQualityEvaluator:
    """简化版音频质量评估器"""
    
    def __init__(self):
        self.results = {}
    
    def read_wav_file(self, file_path):
        """
        读取WAV文件
        
        Args:
            file_path: WAV文件路径
            
        Returns:
            tuple: (音频数据列表, 采样率, 声道数)
        """
        try:
            with wave.open(file_path, 'rb') as wav_file:
                # 获取音频参数
                frames = wav_file.getnframes()
                sample_rate = wav_file.getframerate()
                channels = wav_file.getnchannels()
                sample_width = wav_file.getsampwidth()
                
                # 读取音频数据
                audio_data = wav_file.readframes(frames)
                
                # 根据采样位深度解析数据
                if sample_width == 1:
                    # 8位无符号
                    audio_samples = [x - 128 for x in audio_data]
                elif sample_width == 2:
                    # 16位有符号
                    audio_samples = list(struct.unpack(f'<{frames * channels}h', audio_data))
                elif sample_width == 4:
                    # 32位有符号
                    audio_samples = list(struct.unpack(f'<{frames * channels}i', audio_data))
                else:
                    raise ValueError(f"不支持的采样位深度: {sample_width}")
                
                # 转换为浮点数 (-1.0 到 1.0)
                if sample_width == 1:
                    audio_samples = [x / 128.0 for x in audio_samples]
                elif sample_width == 2:
                    audio_samples = [x / 32768.0 for x in audio_samples]
                elif sample_width == 4:
                    audio_samples = [x / 2147483648.0 for x in audio_samples]
                
                # 如果是立体声，转换为单声道（取平均）
                if channels == 2:
                    mono_samples = []
                    for i in range(0, len(audio_samples), 2):
                        mono_samples.append((audio_samples[i] + audio_samples[i+1]) / 2.0)
                    audio_samples = mono_samples
                
                return audio_samples, sample_rate, channels
                
        except Exception as e:
            raise ValueError(f"无法读取WAV文件 {file_path}: {str(e)}")
    
    def convert_ogg_to_wav(self, ogg_file, temp_wav_file):
        """
        使用ffmpeg将OGG文件转换为WAV文件
        
        Args:
            ogg_file: OGG文件路径
            temp_wav_file: 临时WAV文件路径
            
        Returns:
            bool: 转换是否成功
        """
        import subprocess
        
        try:
            # 使用ffmpeg转换
            cmd = [
                'ffmpeg', '-i', ogg_file, 
                '-ar', '16000',  # 采样率
                '-ac', '1',      # 单声道
                '-y',            # 覆盖输出文件
                temp_wav_file
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                return True
            else:
                print(f"ffmpeg转换失败: {result.stderr}")
                return False
                
        except FileNotFoundError:
            print("错误: 未找到ffmpeg，请安装ffmpeg或使用WAV格式的音频文件")
            return False
        except Exception as e:
            print(f"转换过程出错: {str(e)}")
            return False
    
    def load_audio_file(self, file_path):
        """
        加载音频文件（支持WAV和OGG）
        
        Args:
            file_path: 音频文件路径
            
        Returns:
            tuple: (音频数据, 采样率)
        """
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"音频文件不存在: {file_path}")
        
        file_ext = os.path.splitext(file_path)[1].lower()
        
        if file_ext == '.wav':
            audio_data, sample_rate, _ = self.read_wav_file(file_path)
            return audio_data, sample_rate
        
        elif file_ext in ['.ogg', '.oga']:
            # 创建临时WAV文件
            temp_wav = file_path + '_temp.wav'
            
            try:
                if self.convert_ogg_to_wav(file_path, temp_wav):
                    audio_data, sample_rate, _ = self.read_wav_file(temp_wav)
                    # 删除临时文件
                    os.remove(temp_wav)
                    return audio_data, sample_rate
                else:
                    raise ValueError("OGG文件转换失败")
            except Exception as e:
                # 清理临时文件
                if os.path.exists(temp_wav):
                    os.remove(temp_wav)
                raise e
        
        else:
            raise ValueError(f"不支持的音频格式: {file_ext}")
    
    def align_audio_length(self, ref_audio, deg_audio):
        """对齐两个音频的长度"""
        min_len = min(len(ref_audio), len(deg_audio))
        return ref_audio[:min_len], deg_audio[:min_len]
    
    def calculate_rms(self, audio_data):
        """计算RMS值"""
        if not audio_data:
            return 0.0
        return math.sqrt(sum(x*x for x in audio_data) / len(audio_data))
    
    def calculate_snr(self, ref_audio, deg_audio):
        """计算信噪比"""
        ref_aligned, deg_aligned = self.align_audio_length(ref_audio, deg_audio)
        
        # 计算噪声（差值）
        noise = [ref_aligned[i] - deg_aligned[i] for i in range(len(ref_aligned))]
        
        # 计算信号功率和噪声功率
        signal_power = sum(x*x for x in ref_aligned) / len(ref_aligned)
        noise_power = sum(x*x for x in noise) / len(noise)
        
        if noise_power == 0:
            return float('inf')
        
        snr = 10 * math.log10(signal_power / noise_power)
        return snr
    
    def calculate_correlation(self, ref_audio, deg_audio):
        """计算相关系数"""
        ref_aligned, deg_aligned = self.align_audio_length(ref_audio, deg_audio)
        
        if not ref_aligned:
            return 0.0
        
        # 计算均值
        ref_mean = sum(ref_aligned) / len(ref_aligned)
        deg_mean = sum(deg_aligned) / len(deg_aligned)
        
        # 计算协方差和标准差
        numerator = sum((ref_aligned[i] - ref_mean) * (deg_aligned[i] - deg_mean) 
                       for i in range(len(ref_aligned)))
        
        ref_var = sum((x - ref_mean)**2 for x in ref_aligned)
        deg_var = sum((x - deg_mean)**2 for x in deg_aligned)
        
        denominator = math.sqrt(ref_var * deg_var)
        
        if denominator == 0:
            return 0.0
        
        return numerator / denominator
    
    def calculate_mse(self, ref_audio, deg_audio):
        """计算均方误差"""
        ref_aligned, deg_aligned = self.align_audio_length(ref_audio, deg_audio)
        
        if not ref_aligned:
            return 0.0
        
        mse = sum((ref_aligned[i] - deg_aligned[i])**2 for i in range(len(ref_aligned)))
        return mse / len(ref_aligned)
    
    def calculate_dynamic_range(self, audio_data):
        """计算动态范围"""
        if not audio_data:
            return 0.0
        
        max_val = max(abs(x) for x in audio_data)
        
        # 计算噪底（最小的非零值的RMS）
        non_zero_samples = [abs(x) for x in audio_data if abs(x) > 1e-6]
        if not non_zero_samples:
            return 0.0
        
        min_val = min(non_zero_samples)
        
        if min_val == 0:
            return float('inf')
        
        dynamic_range = 20 * math.log10(max_val / min_val)
        return dynamic_range
    
    def estimate_pesq(self, ref_audio, deg_audio):
        """基于基础指标估算PESQ值"""
        snr = self.calculate_snr(ref_audio, deg_audio)
        correlation = self.calculate_correlation(ref_audio, deg_audio)
        
        if snr == float('inf'):
            pesq_est = 4.5
        else:
            # 简化的PESQ映射
            pesq_est = 1.0 + 3.5 * (1 / (1 + math.exp(-(snr - 20) / 5))) * max(0, correlation)
        
        return max(1.0, min(4.5, pesq_est))
    
    def calculate_quality_score(self, ref_audio, deg_audio):
        """计算综合质量评分"""
        snr = self.calculate_snr(ref_audio, deg_audio)
        correlation = self.calculate_correlation(ref_audio, deg_audio)
        mse = self.calculate_mse(ref_audio, deg_audio)
        
        # SNR评分
        if snr == float('inf'):
            snr_score = 5.0
        else:
            snr_score = max(1.0, min(5.0, 1 + 4 * (1 / (1 + math.exp(-(snr - 15) / 5)))))
        
        # 相关性评分
        corr_score = max(1.0, correlation * 5.0)
        
        # MSE评分
        mse_score = max(1.0, 5.0 - math.log10(mse + 1e-10))
        
        # 综合评分
        overall_score = 0.4 * snr_score + 0.3 * corr_score + 0.3 * mse_score
        return max(1.0, min(5.0, overall_score))
    
    def evaluate(self, reference_file, degraded_file):
        """执行音频质量评估"""
        print(f"正在评估音频质量...")
        print(f"参考文件: {reference_file}")
        print(f"测试文件: {degraded_file}")
        
        try:
            # 加载音频文件
            print("正在加载音频文件...")
            ref_audio, ref_sr = self.load_audio_file(reference_file)
            deg_audio, deg_sr = self.load_audio_file(degraded_file)
            
            # 检查采样率
            if ref_sr != deg_sr:
                print(f"警告: 采样率不一致 (参考: {ref_sr}Hz, 测试: {deg_sr}Hz)")
            
            print("正在计算质量指标...")
            
            # 计算各种指标
            results = {
                'timestamp': datetime.now().isoformat(),
                'reference_file': reference_file,
                'degraded_file': degraded_file,
                'reference_sample_rate': ref_sr,
                'degraded_sample_rate': deg_sr,
                'reference_duration': len(ref_audio) / ref_sr,
                'degraded_duration': len(deg_audio) / deg_sr,
                'reference_samples': len(ref_audio),
                'degraded_samples': len(deg_audio),
            }
            
            # 基础统计
            results['reference_rms'] = self.calculate_rms(ref_audio)
            results['degraded_rms'] = self.calculate_rms(deg_audio)
            results['reference_dynamic_range'] = self.calculate_dynamic_range(ref_audio)
            results['degraded_dynamic_range'] = self.calculate_dynamic_range(deg_audio)
            
            # 质量指标
            results['snr_db'] = self.calculate_snr(ref_audio, deg_audio)
            results['correlation'] = self.calculate_correlation(ref_audio, deg_audio)
            results['mse'] = self.calculate_mse(ref_audio, deg_audio)
            results['pesq_estimated'] = self.estimate_pesq(ref_audio, deg_audio)
            results['overall_quality_score'] = self.calculate_quality_score(ref_audio, deg_audio)
            
            self.results = results
            return results
            
        except Exception as e:
            raise ValueError(f"评估过程出错: {str(e)}")
    
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
        
        print(f"\n文件信息:")
        print(f"  参考文件采样率: {self.results['reference_sample_rate']} Hz")
        print(f"  测试文件采样率: {self.results['degraded_sample_rate']} Hz")
        print(f"  参考音频时长: {self.results['reference_duration']:.2f} 秒")
        print(f"  测试音频时长: {self.results['degraded_duration']:.2f} 秒")
        
        print(f"\n音频特性:")
        print(f"  参考文件RMS: {self.results['reference_rms']:.4f}")
        print(f"  测试文件RMS: {self.results['degraded_rms']:.4f}")
        print(f"  参考文件动态范围: {self.results['reference_dynamic_range']:.2f} dB")
        print(f"  测试文件动态范围: {self.results['degraded_dynamic_range']:.2f} dB")
        
        print(f"\n质量指标:")
        print(f"  信噪比 (SNR): {self.results['snr_db']:.2f} dB")
        print(f"  相关系数: {self.results['correlation']:.4f}")
        print(f"  均方误差 (MSE): {self.results['mse']:.6f}")
        print(f"  PESQ (估算): {self.results['pesq_estimated']:.3f} (1.0-4.5)")
        print(f"  综合质量评分: {self.results['overall_quality_score']:.3f} (1.0-5.0)")
        
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
        
        # SNR评级
        snr = self.results['snr_db']
        if snr == float('inf'):
            snr_level = "完美"
        elif snr >= 40:
            snr_level = "优秀"
        elif snr >= 30:
            snr_level = "良好"
        elif snr >= 20:
            snr_level = "一般"
        elif snr >= 10:
            snr_level = "较差"
        else:
            snr_level = "很差"
        
        print(f"  SNR等级: {snr_level}")
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
        description='简化版音频质量评估工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  python simple_quality_evaluator.py reference.wav encoded.ogg
  python simple_quality_evaluator.py -r reference.wav -d encoded.ogg -o results.json
  
支持的音频格式:
  WAV (原生支持)
  OGG (需要ffmpeg)
  
注意事项:
  - 对于OGG文件，需要安装ffmpeg
  - 此工具提供基础的质量评估，不依赖复杂的外部库
  - PESQ值为估算值，真实PESQ需要专业软件
        """
    )
    
    # 位置参数
    parser.add_argument('reference', nargs='?', help='参考音频文件路径')
    parser.add_argument('degraded', nargs='?', help='待测音频文件路径')
    
    # 可选参数
    parser.add_argument('-r', '--reference', dest='ref_file', help='参考音频文件路径')
    parser.add_argument('-d', '--degraded', dest='deg_file', help='待测音频文件路径')
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
        evaluator = SimpleAudioQualityEvaluator()
        
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
