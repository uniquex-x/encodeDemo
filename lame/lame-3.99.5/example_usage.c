#include <stdio.h>
#include <stdlib.h>
#include "include/lame.h"  // 使用相对路径

int main() {
    lame_global_flags *gfp;
    int ret_code;
    
    // 初始化LAME编码器
    gfp = lame_init();
    if (gfp == NULL) {
        printf("LAME初始化失败\n");
        return -1;
    }
    
    // 设置编码参数
    lame_set_in_samplerate(gfp, 44100);
    lame_set_VBR(gfp, vbr_default);
    lame_set_VBR_quality(gfp, 2);
    
    // 初始化编码器参数
    ret_code = lame_init_params(gfp);
    if (ret_code < 0) {
        printf("LAME参数初始化失败: %d\n", ret_code);
        lame_close(gfp);
        return -1;
    }
    
    // 打印编码器信息
    printf("LAME版本: %s\n", get_lame_version());
    printf("输入采样率: %d Hz\n", lame_get_in_samplerate(gfp));
    printf("输出比特率: %d kbps\n", lame_get_brate(gfp));
    printf("VBR模式: %s\n", lame_get_VBR(gfp) == vbr_off ? "关闭" : "开启");
    
    // 清理资源
    lame_close(gfp);
    
    printf("LAME库集成测试成功！\n");
    return 0;
} 