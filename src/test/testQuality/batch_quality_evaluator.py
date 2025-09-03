#!/usr/bin/env python3
"""
批量音频质量评估脚本
支持批量比较原始音频和编码后音频的质量
"""

import os
import sys
import argparse
import json
import glob
from datetime import datetime

# 导入评估器
try:
    from audio_quality_evaluator import AudioQualityEvaluator
    EVALUATOR_TYPE = "advanced"
except ImportError:
    from simple_quality_evaluator import SimpleAudioQualityEvaluator as AudioQualityEvaluator
    EVALUATOR_TYPE = "simple"

class BatchQualityEvaluator:
    """批量音频质量评估器"""
    
    def __init__(self, output_dir="results"):
        """
        初始化批量评估器
        
        Args:
            output_dir: 结果输出目录
        """
        self.output_dir = output_dir
        self.results = []
        
        # 创建输出目录
        os.makedirs(output_dir, exist_ok=True)
    
    def find_audio_pairs(self, reference_dir, degraded_dir, ref_pattern="*.wav", deg_pattern="*.ogg"):
        """
        查找音频文件对
        
        Args:
            reference_dir: 参考音频目录
            degraded_dir: 待测音频目录
            ref_pattern: 参考音频文件模式
            deg_pattern: 待测音频文件模式
            
        Returns:
            list: 音频文件对列表
        """
        pairs = []
        
        # 查找参考音频文件
        ref_files = glob.glob(os.path.join(reference_dir, ref_pattern))
        
        for ref_file in ref_files:
            # 获取基础文件名（不含扩展名）
            base_name = os.path.splitext(os.path.basename(ref_file))[0]
            
            # 查找对应的待测音频文件
            deg_pattern_full = deg_pattern.replace("*", base_name)
            deg_files = glob.glob(os.path.join(degraded_dir, deg_pattern_full))
            
            if deg_files:
                pairs.append((ref_file, deg_files[0]))
            else:
                print(f"警告: 未找到 {base_name} 对应的待测音频文件")
        
        return pairs
    
    def evaluate_single_pair(self, ref_file, deg_file):
        """
        评估单个音频文件对
        
        Args:
            ref_file: 参考音频文件
            deg_file: 待测音频文件
            
        Returns:
            dict: 评估结果
        """
        try:
            if EVALUATOR_TYPE == "advanced":
                evaluator = AudioQualityEvaluator()
            else:
                evaluator = AudioQualityEvaluator()
            
            result = evaluator.evaluate(ref_file, deg_file)
            result['status'] = 'success'
            return result
            
        except Exception as e:
            return {
                'reference_file': ref_file,
                'degraded_file': deg_file,
                'status': 'error',
                'error_message': str(e),
                'timestamp': datetime.now().isoformat()
            }
    
    def evaluate_batch(self, audio_pairs, verbose=True):
        """
        批量评估音频文件对
        
        Args:
            audio_pairs: 音频文件对列表
            verbose: 是否显示详细输出
            
        Returns:
            list: 所有评估结果
        """
        total_pairs = len(audio_pairs)
        print(f"开始批量评估 {total_pairs} 个音频文件对...")
        
        results = []
        
        for i, (ref_file, deg_file) in enumerate(audio_pairs, 1):
            if verbose:
                print(f"\n[{i}/{total_pairs}] 正在评估:")
                print(f"  参考: {os.path.basename(ref_file)}")
                print(f"  测试: {os.path.basename(deg_file)}")
            
            result = self.evaluate_single_pair(ref_file, deg_file)
            results.append(result)
            
            if result['status'] == 'success':
                if verbose:
                    if 'overall_quality_score' in result:
                        score = result['overall_quality_score']
                        snr = result.get('snr_db', 'N/A')
                        print(f"  ✓ 质量评分: {score:.3f}, SNR: {snr}")
                    else:
                        print(f"  ✓ 评估完成")
            else:
                print(f"  ✗ 评估失败: {result['error_message']}")
        
        self.results = results
        return results
    
    def generate_summary_report(self, results):
        """
        生成汇总报告
        
        Args:
            results: 评估结果列表
            
        Returns:
            dict: 汇总报告
        """
        successful_results = [r for r in results if r['status'] == 'success']
        failed_results = [r for r in results if r['status'] == 'error']
        
        if not successful_results:
            return {
                'total_files': len(results),
                'successful_evaluations': 0,
                'failed_evaluations': len(failed_results),
                'error': 'No successful evaluations'
            }
        
        # 计算统计信息
        quality_scores = [r.get('overall_quality_score', 0) for r in successful_results]
        snr_values = [r.get('snr_db', 0) for r in successful_results if r.get('snr_db') != float('inf')]
        correlations = [r.get('correlation', 0) for r in successful_results]
        
        # PESQ值（可能是真实值或估算值）
        pesq_values = []
        for r in successful_results:
            if 'pesq' in r:
                pesq_values.append(r['pesq'])
            elif 'pesq_estimated' in r:
                pesq_values.append(r['pesq_estimated'])
        
        summary = {
            'evaluation_info': {
                'timestamp': datetime.now().isoformat(),
                'evaluator_type': EVALUATOR_TYPE,
                'total_files': len(results),
                'successful_evaluations': len(successful_results),
                'failed_evaluations': len(failed_results)
            },
            'quality_statistics': {
                'quality_score': {
                    'mean': sum(quality_scores) / len(quality_scores) if quality_scores else 0,
                    'min': min(quality_scores) if quality_scores else 0,
                    'max': max(quality_scores) if quality_scores else 0
                },
                'snr_db': {
                    'mean': sum(snr_values) / len(snr_values) if snr_values else 0,
                    'min': min(snr_values) if snr_values else 0,
                    'max': max(snr_values) if snr_values else 0
                },
                'correlation': {
                    'mean': sum(correlations) / len(correlations) if correlations else 0,
                    'min': min(correlations) if correlations else 0,
                    'max': max(correlations) if correlations else 0
                }
            }
        }
        
        if pesq_values:
            summary['quality_statistics']['pesq'] = {
                'mean': sum(pesq_values) / len(pesq_values),
                'min': min(pesq_values),
                'max': max(pesq_values)
            }
        
        # 质量分布
        quality_distribution = {'excellent': 0, 'good': 0, 'fair': 0, 'poor': 0, 'bad': 0}
        for score in quality_scores:
            if score >= 4.5:
                quality_distribution['excellent'] += 1
            elif score >= 4.0:
                quality_distribution['good'] += 1
            elif score >= 3.0:
                quality_distribution['fair'] += 1
            elif score >= 2.0:
                quality_distribution['poor'] += 1
            else:
                quality_distribution['bad'] += 1
        
        summary['quality_distribution'] = quality_distribution
        
        # 失败的文件列表
        if failed_results:
            summary['failed_files'] = [
                {
                    'reference': os.path.basename(r['reference_file']),
                    'degraded': os.path.basename(r['degraded_file']),
                    'error': r['error_message']
                }
                for r in failed_results
            ]
        
        return summary
    
    def save_results(self, results, summary):
        """保存结果到文件"""
        # 保存详细结果
        detailed_results_file = os.path.join(self.output_dir, 'detailed_results.json')
        with open(detailed_results_file, 'w', encoding='utf-8') as f:
            json.dump(results, f, indent=2, ensure_ascii=False)
        
        # 保存汇总报告
        summary_file = os.path.join(self.output_dir, 'summary_report.json')
        with open(summary_file, 'w', encoding='utf-8') as f:
            json.dump(summary, f, indent=2, ensure_ascii=False)
        
        # 生成CSV报告
        csv_file = os.path.join(self.output_dir, 'results.csv')
        self.generate_csv_report(results, csv_file)
        
        print(f"\n结果已保存到:")
        print(f"  详细结果: {detailed_results_file}")
        print(f"  汇总报告: {summary_file}")
        print(f"  CSV报告: {csv_file}")
    
    def generate_csv_report(self, results, csv_file):
        """生成CSV格式的报告"""
        import csv
        
        successful_results = [r for r in results if r['status'] == 'success']
        
        if not successful_results:
            return
        
        with open(csv_file, 'w', newline='', encoding='utf-8') as f:
            # 确定CSV列
            fieldnames = ['reference_file', 'degraded_file', 'quality_score', 'snr_db', 'correlation']
            
            # 检查是否有PESQ数据
            if any('pesq' in r or 'pesq_estimated' in r for r in successful_results):
                fieldnames.append('pesq')
            
            if any('mse' in r for r in successful_results):
                fieldnames.append('mse')
            
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            
            for result in successful_results:
                row = {
                    'reference_file': os.path.basename(result['reference_file']),
                    'degraded_file': os.path.basename(result['degraded_file']),
                    'quality_score': result.get('overall_quality_score', ''),
                    'snr_db': result.get('snr_db', ''),
                    'correlation': result.get('correlation', '')
                }
                
                # 添加PESQ值
                if 'pesq' in result:
                    row['pesq'] = result['pesq']
                elif 'pesq_estimated' in result:
                    row['pesq'] = result['pesq_estimated']
                
                if 'mse' in result:
                    row['mse'] = result['mse']
                
                writer.writerow(row)
    
    def print_summary(self, summary):
        """打印汇总报告"""
        print("\n" + "="*60)
        print("批量音频质量评估汇总报告")
        print("="*60)
        
        info = summary['evaluation_info']
        print(f"评估时间: {info['timestamp']}")
        print(f"评估器类型: {info['evaluator_type']}")
        print(f"总文件数: {info['total_files']}")
        print(f"成功评估: {info['successful_evaluations']}")
        print(f"失败评估: {info['failed_evaluations']}")
        
        if info['successful_evaluations'] > 0:
            stats = summary['quality_statistics']
            print(f"\n质量统计:")
            print(f"  综合质量评分: {stats['quality_score']['mean']:.3f} "
                  f"(范围: {stats['quality_score']['min']:.3f} - {stats['quality_score']['max']:.3f})")
            print(f"  信噪比 (SNR): {stats['snr_db']['mean']:.2f} dB "
                  f"(范围: {stats['snr_db']['min']:.2f} - {stats['snr_db']['max']:.2f})")
            print(f"  相关系数: {stats['correlation']['mean']:.4f} "
                  f"(范围: {stats['correlation']['min']:.4f} - {stats['correlation']['max']:.4f})")
            
            if 'pesq' in stats:
                print(f"  PESQ: {stats['pesq']['mean']:.3f} "
                      f"(范围: {stats['pesq']['min']:.3f} - {stats['pesq']['max']:.3f})")
            
            print(f"\n质量分布:")
            dist = summary['quality_distribution']
            total = sum(dist.values())
            for level, count in dist.items():
                percentage = (count / total * 100) if total > 0 else 0
                print(f"  {level}: {count} ({percentage:.1f}%)")
        
        if summary.get('failed_files'):
            print(f"\n失败的文件:")
            for failed in summary['failed_files']:
                print(f"  {failed['reference']} -> {failed['degraded']}: {failed['error']}")
        
        print("="*60)


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='批量音频质量评估工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  # 批量评估两个目录中的音频文件
  python batch_quality_evaluator.py -r original/ -d encoded/ -o results/
  
  # 指定文件模式
  python batch_quality_evaluator.py -r original/ -d encoded/ --ref-pattern "*.wav" --deg-pattern "*.ogg"
  
  # 评估单个文件
  python batch_quality_evaluator.py --single original.wav encoded.ogg -o results/
        """
    )
    
    # 批量模式参数
    parser.add_argument('-r', '--reference-dir', help='参考音频文件目录')
    parser.add_argument('-d', '--degraded-dir', help='待测音频文件目录')
    parser.add_argument('--ref-pattern', default='*.wav', help='参考音频文件模式 (默认: *.wav)')
    parser.add_argument('--deg-pattern', default='*.ogg', help='待测音频文件模式 (默认: *.ogg)')
    
    # 单文件模式参数
    parser.add_argument('--single', nargs=2, metavar=('REF', 'DEG'), 
                       help='单文件模式: 参考文件 待测文件')
    
    # 输出参数
    parser.add_argument('-o', '--output-dir', default='quality_results', 
                       help='结果输出目录 (默认: quality_results)')
    parser.add_argument('--quiet', action='store_true', help='安静模式，减少输出')
    
    args = parser.parse_args()
    
    # 检查参数
    if not args.single and (not args.reference_dir or not args.degraded_dir):
        parser.print_help()
        print("\n错误: 必须指定批量模式的目录或单文件模式的文件")
        sys.exit(1)
    
    try:
        # 创建批量评估器
        batch_evaluator = BatchQualityEvaluator(args.output_dir)
        
        if args.single:
            # 单文件模式
            ref_file, deg_file = args.single
            audio_pairs = [(ref_file, deg_file)]
        else:
            # 批量模式
            print(f"正在查找音频文件对...")
            print(f"参考目录: {args.reference_dir}")
            print(f"待测目录: {args.degraded_dir}")
            print(f"参考模式: {args.ref_pattern}")
            print(f"待测模式: {args.deg_pattern}")
            
            audio_pairs = batch_evaluator.find_audio_pairs(
                args.reference_dir, args.degraded_dir, 
                args.ref_pattern, args.deg_pattern
            )
            
            if not audio_pairs:
                print("错误: 未找到匹配的音频文件对")
                sys.exit(1)
            
            print(f"找到 {len(audio_pairs)} 个音频文件对")
        
        # 执行批量评估
        results = batch_evaluator.evaluate_batch(audio_pairs, verbose=not args.quiet)
        
        # 生成汇总报告
        summary = batch_evaluator.generate_summary_report(results)
        
        # 显示汇总
        if not args.quiet:
            batch_evaluator.print_summary(summary)
        
        # 保存结果
        batch_evaluator.save_results(results, summary)
        
    except Exception as e:
        print(f"错误: {str(e)}")
        sys.exit(1)


if __name__ == '__main__':
    main()
