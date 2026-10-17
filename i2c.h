#ifndef __I2C_H__
#define __I2C_H__

#include <reg52.h>

// I2C引脚定义
sbit SCL = P1^0;  // 时钟线
sbit SDA = P1^1;  // 数据线

// I2C函数声明
void I2C_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_SendByte(unsigned char byte);
unsigned char I2C_ReceiveByte(void);
void I2C_Ack(void);
void I2C_NAck(void);
unsigned char I2C_WaitAck(void);

#endif

