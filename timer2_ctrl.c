#include "timer2_ctrl.h"

// P1.4引脚控制
sbit P14_OUT = P1^4;

// 状态定义
#define STATE_COUNTING   0  // 计时中
#define STATE_OUTPUT_LOW 1  // 输出低电平中

// 定时器配置（假设11.0592MHz晶振，12T模式）
// 50ms中断一次
// 机器周期 = 12 / 11.0592MHz ≈ 1.085μs
// 50ms = 50000μs，需要计数：50000 / 1.085 ≈ 46083
// 重装载值：65536 - 46083 = 19453 = 0x4BFD

static unsigned char timer2_state = STATE_COUNTING;
static unsigned char counter_50ms = 0;   // 50ms计数器（每20次中断=1秒）
static unsigned int seconds_count = 0;    // 秒计数器

// Timer2初始化（配置为16位自动重装载模式，50ms中断）
void Timer2_Init(void)
{
    // P1.4初始化为高电平（推挽输出）
    P14_OUT = 1;
    P1 |= 0x10;  // 确保P1.4初始为高
    
    // Timer2配置
    // T2CON寄存器：
    // TF2: 溢出标志（只读）
    // EXF2: 外部标志（只读）
    // RCLK: 接收时钟（0=Timer1，1=Timer2）
    // TCLK: 发送时钟（0=Timer1，1=Timer2）
    // EXEN2: 外部使能（0=禁止，1=允许）
    // TR2: 启动位（1=启动）
    // C/T2: 定时/计数选择（0=定时，1=计数）
    // CP/RL2: 捕获/重装载选择（0=重装载，1=捕获）
    
    // 配置为16位自动重装载定时模式
    // CP/RL2=0 (自动重装载), C/T2=0 (定时器模式), TR2=0 (先不启动)
    T2CON = 0x00;
    
    // 假设11.0592MHz晶振，12T模式
    // 50ms中断：需要计数约46083次
    // 重装载值：65536 - 46083 = 19453 = 0x4BFD
    RCAP2H = 0x4B;
    RCAP2L = 0xFD;
    TH2 = 0x4B;
    TL2 = 0xFD;
    
    // 使能Timer2中断
    ET2 = 1;
    
    // 初始化状态
    timer2_state = STATE_COUNTING;
    counter_50ms = 0;
    seconds_count = 0;
}

// Timer2启动
void Timer2_Start(void)
{
    // 重置状态
    timer2_state = STATE_COUNTING;
    counter_50ms = 0;
    seconds_count = 0;
    P14_OUT = 1;  // 初始高电平
    
    // 重新装载定时器值
    TH2 = RCAP2H;
    TL2 = RCAP2L;
    
    // 启动Timer2
    TR2 = 1;
    EA = 1;  // 确保全局中断开启
}

// Timer2停止
void Timer2_Stop(void)
{
    TR2 = 0;
    P14_OUT = 1;  // 停止时恢复高电平
}

// 获取当前状态
unsigned char Timer2_GetState(void)
{
    return timer2_state;
}

// Timer2中断服务程序（interrupt 5）
void Timer2_ISR(void) interrupt 5
{
    // 清除溢出标志
    TF2 = 0;
    EXF2 = 0;  // 也清除外部标志（如果设置了）
    
    // 每20次中断 = 1秒（50ms × 20 = 1000ms）
    counter_50ms++;
    if(counter_50ms >= 20)
    {
        counter_50ms = 0;
        
        if(timer2_state == STATE_COUNTING)
        {
            // 计时状态：每秒计数一次
            seconds_count++;
            
            if(seconds_count >= 60)
            {
                // 60秒到，切换到低电平输出状态
                timer2_state = STATE_OUTPUT_LOW;
                P14_OUT = 0;  // 输出低电平
                seconds_count = 0;  // 重置计数器，用于10秒计时
            }
        }
        else if(timer2_state == STATE_OUTPUT_LOW)
        {
            // 低电平输出状态：计时10秒
            seconds_count++;
            
            if(seconds_count >= 10)
            {
                // 10秒到，恢复计时状态
                timer2_state = STATE_COUNTING;
                P14_OUT = 1;  // 恢复高电平
                seconds_count = 0;  // 重新开始60秒计时
            }
        }
    }
}

