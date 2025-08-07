#ifndef ENCODER_INFO_H
#define ENCODER_INFO_H

enum class EncoderType{
    FLAC,
    VORBIS,
    LAME
};

//编码器参数上下文
struct EncoderParamContext{
    EncoderType encoderType;
    int quality;
};



#endif