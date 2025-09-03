#ifndef QUALITY_REPORT_HELPER_H
#define QUALITY_REPORT_HELPER_H

#include "AudioQualityAnalyzer.h"
#include <string>

/**
 * 音质分析报告辅助类
 * 提供格式化输出和评级功能
 */
class QualityReportHelper {
public:
    /**
     * 音质等级枚举
     */
    enum class QualityGrade {
        EXCELLENT,  // 优秀 (90-100)
        GOOD,       // 良好 (75-90)
        FAIR,       // 一般 (60-75)
        POOR,       // 较差 (40-60)
        BAD         // 很差 (<40)
    };

    /**
     * 打印完整的音质分析报告
     */
    static void printDetailedReport(const AudioQualityAnalyzer::QualityAnalysisResult& result);
    
    /**
     * 打印简化的音质摘要
     */
    static void printSummaryReport(const AudioQualityAnalyzer::QualityAnalysisResult& result);
    
    /**
     * 打印比较报告
     */
    static void printComparisonReport(const AudioQualityAnalyzer::QualityAnalysisResult& original,
                                     const AudioQualityAnalyzer::QualityAnalysisResult& encoded,
                                     const std::string& encoderName);
    
    /**
     * 根据综合评分获取质量等级
     */
    static QualityGrade getQualityGrade(double score);
    
    /**
     * 获取质量等级的中文描述
     */
    static std::string getQualityGradeString(QualityGrade grade);
    
    /**
     * 获取质量等级的颜色代码（用于终端输出）
     */
    static std::string getQualityGradeColor(QualityGrade grade);
    
    /**
     * 生成CSV格式的报告头
     */
    static std::string getCSVHeader();
    
    /**
     * 生成CSV格式的数据行
     */
    static std::string getCSVRow(const AudioQualityAnalyzer::QualityAnalysisResult& result,
                                const std::string& encoderName = "",
                                int quality = 0);

private:
    /**
     * 格式化数值输出
     */
    static std::string formatValue(double value, int precision = 2);
    
    /**
     * 获取指标的评价文字
     */
    static std::string getMetricEvaluation(const std::string& metricName, double value);
    
    /**
     * 打印分割线
     */
    static void printSeparator(const std::string& title = "");
};

#endif // QUALITY_REPORT_HELPER_H
