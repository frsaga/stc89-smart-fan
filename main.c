#include <reg52.h>
#include "delay.h"
#include "oled.h"
#include "i2c.h"
#include "sensors.h"
#include "fan.h"
#include "timer2_ctrl.h"
#include "key.h"
//mcu model:STC89C52RC
static void OLED_ShowState(void)
{
    WIND_MODE mode;
    unsigned char level, duty;
    unsigned char on;
    int t_x10, h_x10, p_x100;
    unsigned char code *mode_str;

    Fan_GetStatus(&mode, &level, &duty, &on, &t_x10, &h_x10, &p_x100);

    // -------------------- 模式 --------------------
    switch(mode)
    {
        case MODE_NORMAL: mode_str = "NORMAL"; break;
        case MODE_NATURE: mode_str = "NATURE"; break;  // Nature → A
        case MODE_SLEEP:  mode_str = "SLEEP"; break;
        default:          mode_str = "UNDEF"; break;
    }
    OLED_ShowString(42, 1, mode_str);

    // -------------------- 档位 L --------------------
    OLED_ShowString(42, 2, "    ");    // 清 4 格
    if(level < 5)
    {
        OLED_ShowChar(42, 2, '0' + level);
    }
    else
    {
        OLED_ShowString(42, 2, "AUTO");
    }

    // -------------------- 占空比 D --------------------
    OLED_ShowString(42, 3, "    ");    // 清区
    OLED_ShowNum(42, 3, duty, 3);
    OLED_ShowString(60, 3, "%");

    // -------------------- 温度 T --------------------
    OLED_ShowString(42, 4, "      ");  // 清 6 格
    {
        int temp_abs = (t_x10 < 0) ? -t_x10 : t_x10;
        int temp_int = temp_abs / 10;
        unsigned char temp_dec = temp_abs % 10;
        if(t_x10 < 0)
        {
            OLED_ShowChar(42, 4, '-');
            OLED_ShowNum(48, 4, temp_int, 2);
        }
        else
        {
            OLED_ShowNum(42, 4, temp_int, 2);
        }
        OLED_ShowChar(60, 4, '.');
        OLED_ShowNum(66, 4, temp_dec, 1);
        OLED_ShowChar(72, 4, 'C');
    }

    // -------------------- 湿度 H --------------------
    OLED_ShowString(42, 5, "      ");  // 清 6 格
    OLED_ShowNum(42, 5, h_x10 / 10, 3); // XX
    OLED_ShowChar(60, 5, '.');
    OLED_ShowNum(66, 5, (unsigned char)(h_x10 % 10), 1); // X
    OLED_ShowChar(72, 5, '%');

    // -------------------- 气压 P --------------------
    // 这里 p_x100 实际单位已经改为：hPa * 10（例如 10132 = 1013.2 hPa）
    OLED_ShowString(42, 6, "      ");  // 清 6 格
    OLED_ShowNum(42, 6, p_x100 / 10, 4); // XXXX
    OLED_ShowChar(66, 6, '.');
    OLED_ShowNum(72, 6, (unsigned char)(p_x100 % 10), 1); // X
    OLED_ShowString(78, 6, "hPa");

    // -------------------- 开关状态 S --------------------
    OLED_ShowString(42, 7, "    ");     // 清 4 格
    OLED_ShowString(42, 7, on ? "ON" : "OFF");
}

// ----------------- 主函数 -----------------
void main(void)
{
    unsigned char key;
    int t, h, p;

    // 主循环时间片（毫秒），尽量短以提升按键响应
    // Timer1 中断做去抖，这里只需周期性读取结果即可
    #define LOOP_TICK_MS        10u      // 主循环基准周期：10ms
    #define SENSOR_PERIOD_MS  1000u      // 传感器整体刷新周期：1s
    #define OLED_PERIOD_MS     200u      // OLED 刷新周期：200ms

    unsigned int loop_ms        = 0;
    unsigned int sensor_ms_acc  = 0;
    unsigned int oled_ms_acc    = 0;
    unsigned int fan_ms_acc     = 0;

    // 按键模块初始化（按键上拉 + Timer1 按键扫描中断）
    Key_Init();
    I2C_Init();
    OLED_Init();
    Delay1ms(100);
    OLED_Clear();
	OLED_ShowString(0, 0, "SMART FAN");
    OLED_ShowString(0, 1, "MODE:");
    OLED_ShowString(0, 2, "LEVEL:");
    OLED_ShowString(0, 3, "DUTY:");
    OLED_ShowString(0, 4, "TEMP:");
    OLED_ShowString(0, 5, "HUMI:");
    OLED_ShowString(0, 6, "PRES:");
    OLED_ShowString(0, 7, "POWER:");
    OLED_ShowString(42, 7, "OFF");

    Sensors_Init();
    Fan_Init();
    Timer2_Init();      // Timer2初始化（P1.4控制）
    Timer2_Start();     // 启动Timer2（开始60秒计时）
    OLED_ShowState();

    while(1)
    {
        // -------- 按键处理（高响应） --------
        key = Key_GetCode();       // 从按键模块读取一次按键事件（边沿触发）
        if(key == KEY_POWER_CODE)
        {
            Fan_PowerToggle();
        }
        else if(key == KEY_NORMAL_CODE)
        {
            Fan_SwitchMode(MODE_NORMAL);
            Fan_NextLevel(MODE_NORMAL);
        }
        else if(key == KEY_NATURE_CODE)
        {
            Fan_SwitchMode(MODE_NATURE);
            Fan_NextLevel(MODE_NATURE);
        }
        else if(key == KEY_SLEEP_CODE)
        {
            Fan_SwitchMode(MODE_SLEEP);
            Fan_NextLevel(MODE_SLEEP);
        }

        // -------- 时间片累加 --------
        loop_ms       += LOOP_TICK_MS;
        sensor_ms_acc += LOOP_TICK_MS;
        oled_ms_acc   += LOOP_TICK_MS;
        fan_ms_acc    += LOOP_TICK_MS;

        // -------- 传感器读取：约 1s 一次，集中读取后更新风扇策略 --------
        if(sensor_ms_acc >= SENSOR_PERIOD_MS)
        {
            sensor_ms_acc = 0;

            if(Sensors_ReadAHT20(&t, &h))
            {
                Fan_UpdateEnvironment(t, h);
            }
            if(Sensors_ReadBMP280(&p))
            {
                Fan_UpdatePressure(p);
            }
        }

        // -------- 风扇控制任务：保持原有 50ms Tick --------
        if(fan_ms_acc >= FAN_TICK_MS)
        {
            fan_ms_acc = 0;
            Fan_TaskTick();
        }

        // -------- OLED 刷新：降低频率，减少 I2C+绘制开销 --------
        if(oled_ms_acc >= OLED_PERIOD_MS)
        {
            oled_ms_acc = 0;
            OLED_ShowState();
        }

        // 主循环小延时：保持 10ms 周期，提升按键响应
        Delay1ms(LOOP_TICK_MS);
    }
}

