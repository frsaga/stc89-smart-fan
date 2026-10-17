#include "key.h"

// 按键（低电平有效）
sbit KEY_NORMAL = P2^0;
sbit KEY_NATURE = P2^1;
sbit KEY_SLEEP  = P2^2;
sbit KEY_POWER  = P2^3;

// Timer1 扫描得到的按键码（一次按键产生一次非 0 码）
static volatile unsigned char g_key_code = KEY_NONE_CODE;

// 原始按键优先级编码（低电平有效）
static unsigned char Key_GetRaw(void)
{
    if(KEY_POWER == 0)  return KEY_POWER_CODE;
    if(KEY_NORMAL == 0) return KEY_NORMAL_CODE;
    if(KEY_NATURE == 0) return KEY_NATURE_CODE;
    if(KEY_SLEEP == 0)  return KEY_SLEEP_CODE;
    return KEY_NONE_CODE;
}

// Timer1 初始化：约 10ms 中断一次，用于按键扫描
static void Timer1_KeyInit(void)
{
    // 模式1 16位定时
    TMOD &= 0x0F;       // 清除 T1 配置位
    TMOD |= 0x10;       // T1: 模式1

    // 假设 11.0592MHz, 12T，10ms 定时
    // 10ms 需要计数约 9216 次，重装载值 65536-9216=56320=0xDBC0
    TH1 = 0xDB;
    TL1 = 0xC0;

    ET1 = 1;            // 使能 T1 中断
    TR1 = 1;            // 启动 T1
    EA  = 1;            // 开总中断（防止未开）
}

// Timer1 中断服务函数：做按键去抖和边沿触发
void Timer1_ISR(void) interrupt 3
{
    static unsigned char last_sample = KEY_NONE_CODE;
    static unsigned char stable_key  = KEY_NONE_CODE;
    static unsigned char cnt         = 0;

    // 重新装载 10ms
    TH1 = 0xDB;
    TL1 = 0xC0;

    {
        unsigned char cur = Key_GetRaw();

        if(cur == last_sample)
        {
            if(cnt < 255) cnt++;
        }
        else
        {
            cnt = 0;
            last_sample = cur;
        }

        // 连续稳定 2 次（约20ms）认为按键状态稳定
        if(cnt >= 2)
        {
            cnt = 2; // 防止溢出
            if(cur != stable_key)
            {
                // 状态发生变化：只在由无键 -> 某键 时，产生一次键码
                if(stable_key == KEY_NONE_CODE && cur != KEY_NONE_CODE)
                {
                    g_key_code = cur;    // 记录一次有效按键
                }
                stable_key = cur;
            }
        }
    }
}

void Key_Init(void)
{
    // 按键上拉，默认高电平
    P2 |= 0x0F;

    // Timer1：用于按键扫描
    Timer1_KeyInit();
}

unsigned char Key_GetCode(void)
{
    unsigned char key_code = g_key_code;
    g_key_code = KEY_NONE_CODE;
    return key_code;
}


