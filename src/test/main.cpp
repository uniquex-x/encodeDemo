#include <iostream>
#include <limits>
#include "EncoderInfo.h"
#include "EncoderManager.h"

void showMainMenu() {
    std::cout << "\n=== 音频编码算法性能对比测试程序 ===" << std::endl;
    std::cout << "1. 测试所有编码器 (默认参数)" << std::endl;
    std::cout << "2. 测试单个编码器 (指定参数)" << std::endl;
    std::cout << "3. 编码器参数范围测试" << std::endl;
    std::cout << "4. 显示编码器信息" << std::endl;
    std::cout << "5. 测试改进的MP3编码器" << std::endl;
    std::cout << "0. 退出程序" << std::endl;
    std::cout << "请选择操作 (0-5): ";
}

void showEncoderMenu() {
    std::cout << "\n选择编码器:" << std::endl;
    std::cout << "1. FLAC" << std::endl;
    std::cout << "2. Vorbis" << std::endl;
    std::cout << "3. MP3 (LAME)" << std::endl;
    std::cout << "请选择 (1-3): ";
}

EncoderType getEncoderType(int choice) {
    switch (choice) {
        case 1: return EncoderType::FLAC;
        case 2: return EncoderType::VORBIS;
        case 3: return EncoderType::LAME;
        default: return EncoderType::FLAC;
    }
}

int getIntInput(const std::string& prompt, int min, int max) {
    int value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value && value >= min && value <= max) {
            return value;
        } else {
            std::cout << "无效输入，请输入 " << min << " 到 " << max << " 之间的数字。" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}

void testAllEncoders(EncoderManager& encoderManager) {
    std::cout << "\n=== 开始测试所有编码器 ===" << std::endl;
    encoderManager.testAllEncoders();
}

void testSingleEncoder(EncoderManager& encoderManager) {
    showEncoderMenu();
    int encoderChoice = getIntInput("", 1, 3);
    EncoderType type = getEncoderType(encoderChoice);
    
    // 显示编码器信息
    encoderManager.showEncoderInfo(type);
    
    // 获取编码器实例以获取质量范围
    auto tempEncoder = encoderManager.encoderCreate(type);
    if (!tempEncoder) {
        std::cout << "无法创建编码器" << std::endl;
        return;
    }
    
    int minQuality = tempEncoder->getMinQuality();
    int maxQuality = tempEncoder->getMaxQuality();
    
    int quality = getIntInput("请输入质量参数 (" + std::to_string(minQuality) + 
                             "-" + std::to_string(maxQuality) + "): ", 
                             minQuality, maxQuality);
    
    std::cout << "\n=== 开始测试单个编码器 ===" << std::endl;
    encoderManager.testSingleEncoderWithParams(type, quality);
}

void testQualityRange(EncoderManager& encoderManager) {
    showEncoderMenu();
    int encoderChoice = getIntInput("", 1, 3);
    EncoderType type = getEncoderType(encoderChoice);
    
    // 显示编码器信息
    encoderManager.showEncoderInfo(type);
    
    // 获取编码器实例以获取质量范围
    auto tempEncoder = encoderManager.encoderCreate(type);
    if (!tempEncoder) {
        std::cout << "无法创建编码器" << std::endl;
        return;
    }
    
    int minQuality = tempEncoder->getMinQuality();
    int maxQuality = tempEncoder->getMaxQuality();
    
    std::cout << "当前编码器质量范围: " << minQuality << " - " << maxQuality << std::endl;
    
    int testMin = getIntInput("请输入测试起始质量: ", minQuality, maxQuality);
    int testMax = getIntInput("请输入测试结束质量: ", testMin, maxQuality);
    int step = getIntInput("请输入测试步长 (默认1): ", 1, testMax - testMin + 1);
    
    std::cout << "\n=== 开始参数范围测试 ===" << std::endl;
    encoderManager.testEncoderQualityRange(type, testMin, testMax, step);
}

void showEncodersInfo(EncoderManager& encoderManager) {
    encoderManager.showAllEncodersInfo();
}

int main(int argc, char* argv[]) {
    std::cout << "音频编码算法性能对比测试程序" << std::endl;
    std::cout << "对比测试 FLAC、Vorbis、MP3(LAME) 三个编码算法性能" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    if (argc < 2) {
        std::cout << "请提供输入音频文件路径。" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::cout << "测试音频文件: " << inputFile << std::endl;

    // 创建测试实例
    EncoderManager encoderManager(inputFile);
    
    int choice;
    do {
        showMainMenu();
        choice = getIntInput("", 0, 5);
        
        switch (choice) {
            case 1:
                testAllEncoders(encoderManager);
                break;
            case 2:
                testSingleEncoder(encoderManager);
                break;
            case 3:
                testQualityRange(encoderManager);
                break;
            case 4:
                showEncodersInfo(encoderManager);
                break;
            case 5:
                // 测试改进的MP3编码器
                std::cout << "\n=== 开始测试改进的MP3编码器 ===" << std::endl;
                encoderManager.testImprovedLameEncoder();
                break;
            case 0:
                std::cout << "程序退出。" << std::endl;
                break;
            default:
                std::cout << "无效选择，请重试。" << std::endl;
                break;
        }
        
        if (choice != 0) {
            std::cout << "\n按回车键继续...";
            std::cin.ignore();
            std::cin.get();
        }
        
    } while (choice != 0);
    
    return 0;
}