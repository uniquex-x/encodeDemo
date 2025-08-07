#ifndef ENCODER_TEST_H
#define ENCODER_TEST_H

#include <memory>
#include <vector>
#include <string>
#include "EncoderInfo.h"
#include "encoder.h"

struct TestResult {
    std::string encoderName;
    EncodeResult result;
    double cpuUsage;
    size_t memoryUsage;
};

class EncoderTest {
public:
    EncoderTest();
    EncoderTest(EncoderType selectedEncoder);
    ~EncoderTest();

    void testEncoder();
    void testAllEncoders();
    void compareEncoders();
    std::vector<TestResult> getResults() const { return results; }

    std::unique_ptr<Encoder> encoderCreate(EncoderType selectedEncoder);
    
private:
    void testSingleEncoder(EncoderType type);
    void printResults();

private:
    std::unique_ptr<Encoder> encoder;
    std::vector<TestResult> results;
    std::string testAudioFile;
};

#endif