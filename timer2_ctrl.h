#ifndef __TIMER2_CTRL_H__
#define __TIMER2_CTRL_H__

#include <reg52.h>

// Timer2初始化
void Timer2_Init(void);

// Timer2启动
void Timer2_Start(void);

// Timer2停止
void Timer2_Stop(void);

// 获取当前状态（0=计时中，1=低电平输出中）
unsigned char Timer2_GetState(void);

#endif

