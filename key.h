// key.h - 按键扫描 & Timer1 中断处理（独立模块）

#ifndef __KEY_H__
#define __KEY_H__

#include <reg52.h>

// 按键编码
#define KEY_NONE_CODE   0
#define KEY_POWER_CODE  1
#define KEY_NORMAL_CODE 2
#define KEY_NATURE_CODE 3
#define KEY_SLEEP_CODE  4

/**
 * 按键模块初始化
 * - 配置按键引脚默认上拉
 * - 初始化 Timer1 为按键扫描定时器（约 10ms 中断一次）
 * - 使能 Timer1 中断
 */
void Key_Init(void);

/**
 * 读取一次按键事件（边沿触发）
 * - 每次调用最多返回一个按键编码
 * - 若自上次调用以来无新的按键按下，则返回 KEY_NONE_CODE
 */
unsigned char Key_GetCode(void);

#endif


