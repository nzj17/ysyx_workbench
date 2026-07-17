#include <am.h>
#include <klib-macros.h>
#include <stdlib.h>   // for rand()

#define W 400
#define H 300

// 可选的目标颜色列表
static const uint32_t color_palette[] = {
    0x000000, 0xff0000, 0x00ff00, 0x0000ff,
    0xffff00, 0xff00ff, 0x00ffff, 0xffffff
};
#define PALETTE_SIZE (sizeof(color_palette) / sizeof(color_palette[0]))

// 渐变参数
#define GRADIENT_STEPS  60          // 渐变步数
#define BASE_INTERVAL   20000       // 正常速度下每步间隔（微秒），20ms
#define FAST_INTERVAL   5000        // 加速时的间隔（微秒），5ms

static uint32_t fb[W * H];          // 全屏缓冲区

/* 绘制整个屏幕为单一颜色 */
void draw_solid(uint32_t color) {
    for (int i = 0; i < W * H; i++){
        fb[i] = color;}
    io_write(AM_GPU_FBDRAW, 0, 0, fb, W, H, false);
}

/* 线性插值计算中间颜色 */
uint32_t interpolate_color(uint32_t c0, uint32_t c1, int step, int total_steps) {
    int r0 = (c0 >> 16)&0xff; 
    int g0 = (c0 >> 8) &0xff ;
    int b0 =  c0 &0xff ;
    
    int r1 = (c1 >> 16) &0xff ;
    int g1 = (c1 >> 8)  &0xff ;
    int b1 =  c1        &0xff ;
    
    int r = r0 + (r1 - r0) * step / total_steps;
    int g = g0 + (g1 - g0) * step / total_steps;
    int b = b0 + (b1 - b0) * step / total_steps;
    
    return (r << 16) | (g << 8) | b;
}

int main() {
    ioe_init();
    
    uint32_t current_color = color_palette[0];
    uint32_t target_color  = color_palette[1];
    int gradient_step = 0;
    uint64_t next_update = 0;
//    int speed_multiplier = 1;   // 1 = 正常， >1 = 加速（实际用间隔值）
    int interval = BASE_INTERVAL;
    
    while (1) {
        // 读取键盘事件
        AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
        if (ev.keycode != AM_KEY_NONE) {
            if (ev.keycode == AM_KEY_ESCAPE) {
                halt(0);  // 退出程序
            } else {
                if (ev.keydown) {
                    interval = FAST_INTERVAL;  // 按下任意键加速
                } else {
                    interval = BASE_INTERVAL;  // 释放恢复
                }
            }
        }
        
        uint64_t now = io_read(AM_TIMER_UPTIME).us;
        if (now >= next_update) {
            // 计算当前渐变颜色
            uint32_t color = interpolate_color(current_color, target_color,
                                               gradient_step, GRADIENT_STEPS);
            draw_solid(color);
            
            // 进入下一步
            gradient_step++;
            if (gradient_step >= GRADIENT_STEPS) {
                gradient_step = 0;
                current_color = target_color;
                // 随机选择下一个目标颜色（但不能与当前相同）
                int idx;
                do {
                    idx = rand() % PALETTE_SIZE;
                } while (color_palette[idx] == current_color);
                target_color = color_palette[idx];
            }
            
            next_update = now + interval;
        }
    }
    return 0;
}
