#include "fan.h"
#include "delay.h"

// AT8236 接线（默认正转）
sbit MOTOR_IN1 = P1^2;
sbit MOTOR_IN2 = P1^3;

typedef enum
{
    DECAY_FAST = 0,
    DECAY_SLOW
} DECAY_MODE;

static unsigned char pwm_duty = 0;           // 0~100
static unsigned char pwm_counter = 0;
static WIND_MODE current_mode = MODE_NORMAL;
static unsigned char mode_level[MODE_MAX] = {1, 1, 1}; // 1~5，5为智能
static bit fan_on = 0;
static DECAY_MODE decay_mode = DECAY_FAST;
static bit in_stop_phase = 0;
static unsigned int phase_counter = 0;       // tick = FAN_TICK_MS
static int env_temp_x10 = 250;               // 25.0℃
static int env_hum_x10  = 500;               // 50.0%RH
static int env_press_x100 = 10132;           // 1013.2 hPa，单位：hPa * 10

// 周期表放入CODE区，节省DATA
static const unsigned int code run_ticks[MODE_MAX]  = {40u, 2u, 1u}; // 2.0s / 1.6s / 1.2s
static const unsigned int code stop_ticks[MODE_MAX] = {0u,  1u, 2u};  // 0s   / 0.4s / 1.5s

// ----------------- 内部函数 -----------------
static unsigned char Level_To_Duty(unsigned char level, int temp_x10)
{
    // AUTO 档（5 档及以上）：根据环境温度自动调整占空比
    // 这里做一个简单线性映射：
    //  - 20.0℃ 及以下：约 30% 占空比
    //  - 35.0℃ 及以上：约 90% 占空比
    //  - 中间温度在线性插值
    if(level >= 5)
    {
        int duty;
        int t = temp_x10;   // 单位：0.1℃

        if(t <= 200)        // ≤ 20.0℃
        {
            duty = 30;
        }
        else if(t >= 350)   // ≥ 35.0℃
        {
            duty = 90;
        }
        else
        {
            // 在 20.0℃~35.0℃ 之间线性：每 1℃ 增加约 4% 占空比
            // duty = 30 + (t - 200)/10 * 4
            duty = 30 + ((t - 200) / 10) * 4;
        }

        if(duty < 0)   duty = 0;
        if(duty > 100) duty = 100;
        return (unsigned char)duty;
    }

    switch(level)
    {
        case 1: return 25;
        case 2: return 45;
        case 3: return 70;
        case 4: return 90;
        default:return 45;
    }
}

static void Motor_Output(bit pwm_on)
{
    // 当占空比为 0% 时，强制关闭电机，避免慢衰减模式下 IN1=1,IN2=0 造成全速转
    if(pwm_duty == 0)
    {
        MOTOR_IN1 = 0;
        MOTOR_IN2 = 0;
        return;
    }

    if(decay_mode == DECAY_FAST)
    {
        // 正转 + 快衰减：IN1 = PWM, IN2 = 0
        MOTOR_IN1 = pwm_on;
        MOTOR_IN2 = 0;
    }
    else
    {
        // 正转 + 慢衰减：IN1 = 1, IN2 = PWM
        MOTOR_IN1 = 1;
        MOTOR_IN2 = pwm_on;
    }
}

static void Motor_SetDuty(unsigned char duty)
{
    if(duty > 100) duty = 100;
    pwm_duty = duty;
}

static void Motor_Stop(void)
{
    Motor_SetDuty(0);
    MOTOR_IN1 = 0;
    MOTOR_IN2 = 0;
}

// ----------------- 中断：Timer0 PWM -----------------
void Timer0_Init(void)
{
    TMOD &= 0xF0;
    TMOD |= 0x01;   // 模式1 16位
    TH0 = 0xFF;
    TL0 = 0x9C;     // 100us
    ET0 = 1;
    EA = 1;
    TR0 = 1;
}



void Timer0_ISR(void) interrupt 1
{
    TH0 = 0xFF;
    TL0 = 0x9C;

    if(!fan_on)   // 新增
    {
        MOTOR_IN1 = 0;
        MOTOR_IN2 = 0;
        return;
    }

    pwm_counter++;
    if(pwm_counter >= 100)
        pwm_counter = 0;

    Motor_Output(pwm_counter < pwm_duty);
}


// ----------------- 对外API -----------------
void Fan_Init(void)
{
    fan_on = 0;
    current_mode = MODE_NORMAL;
    mode_level[MODE_NORMAL] = 1;
    mode_level[MODE_NATURE] = 1;
    mode_level[MODE_SLEEP]  = 1;
    decay_mode = DECAY_FAST;
    in_stop_phase = 0;
    phase_counter = 0;
    Motor_Stop();
    Timer0_Init();
}

void Fan_PowerToggle(void)
{
    fan_on = !fan_on;
    in_stop_phase = 0;
    phase_counter = 0;
    if(!fan_on)
    {
        Motor_Stop();
    }
}

void Fan_SwitchMode(WIND_MODE mode)
{
    if(mode >= MODE_MAX) return;
    current_mode = mode;
    in_stop_phase = 0;
    phase_counter = 0;
}

void Fan_NextLevel(WIND_MODE mode)
{
    if(mode >= MODE_MAX) return;
    mode_level[mode] = mode_level[mode] + 1;
    if(mode_level[mode] > 5) mode_level[mode] = 1;
    in_stop_phase = 0;
    phase_counter = 0;
}

void Fan_UpdateEnvironment(int temp_x10, int hum_x10)
{
    env_temp_x10 = temp_x10;
    env_hum_x10 = hum_x10;
}

void Fan_UpdatePressure(int press_x100)
{
    env_press_x100 = press_x100;
}

void Fan_TaskTick(void)
{
    unsigned char level = mode_level[current_mode];
    unsigned char duty  = Level_To_Duty(level, env_temp_x10);

    if(level <= 2) decay_mode = DECAY_SLOW;
    else           decay_mode = DECAY_FAST;

    if(!fan_on)
    {
        Motor_Stop();
        in_stop_phase = 0;
        phase_counter = 0;
        return;
    }

    if(in_stop_phase)
    {
        Motor_SetDuty(0);
        phase_counter++;
        if(phase_counter >= stop_ticks[current_mode])
        {
            in_stop_phase = 0;
            phase_counter = 0;
        }
    }
    else
    {
        Motor_SetDuty(duty);
        phase_counter++;
        if(phase_counter >= run_ticks[current_mode])
        {
            if(stop_ticks[current_mode] > 0)
            {
                in_stop_phase = 1;
            }
            phase_counter = 0;
        }
    }
}

void Fan_GetStatus(WIND_MODE *mode,
                   unsigned char *level,
                   unsigned char *duty,
                   unsigned char *on,
                   int *t_x10,
                   int *h_x10,
                   int *p_x100)
{
    if(mode)  *mode  = current_mode;
    if(level) *level = mode_level[current_mode];
    if(duty)  *duty  = pwm_duty;
    if(on)    *on    = (unsigned char)(fan_on ? 1 : 0);
    if(t_x10) *t_x10 = env_temp_x10;
    if(h_x10) *h_x10 = env_hum_x10;
    if(p_x100) *p_x100 = env_press_x100;
}

