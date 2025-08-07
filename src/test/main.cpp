#include <iostream>
#include "EncoderInfo.h"
#include "encoderTest.h"

int main() {
    std::cout << "音频编码算法性能对比测试程序" << std::endl;
    std::cout << "对比测试 FLAC、Vorbis、MP3(LAME) 三个编码算法性能" << std::endl;
    std::cout << std::string(60, '=') << std::endl << std::endl;

    std::string inputFile = "samples/natureStory.wav";

    // 创建测试实例并运行所有编码器测试
    EncoderTest encoderTest(EncoderType::FLAC, inputFile); // 初始化时需要一个类型，但会测试所有类型
    
    // 测试所有编码器并进行性能对比
    encoderTest.testAllEncoders();
    
    std::cout << std::endl << "测试完成!" << std::endl;
    return 0;
}