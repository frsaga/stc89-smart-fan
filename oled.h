#ifndef __OLED_H__
#define __OLED_H__

#include "i2c.h"

// OLED地址（通常为0x78或0x7A，取决于A0引脚）
#define OLED_ADDRESS 0x78

// OLED控制命令
#define OLED_CMD  0x00  // 命令模式
#define OLED_DATA 0x40  // 数据模式

// OLED函数声明
void OLED_Init(void);
void OLED_WriteCmd(unsigned char cmd);
void OLED_WriteData(unsigned char dat);
void OLED_SetPos(unsigned char x, unsigned char y);
void OLED_Clear(void);
void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char chr);
void OLED_ShowString(unsigned char x, unsigned char y, unsigned char *str);
void OLED_ShowNum(unsigned char x, unsigned char y, unsigned int num, unsigned char len);
void OLED_ShowSignedNum(unsigned char x, unsigned char y, int num, unsigned char len);
void OLED_ShowHexNum(unsigned char x, unsigned char y, unsigned int num, unsigned char len);
void OLED_ShowBinNum(unsigned char x, unsigned char y, unsigned int num, unsigned char len);

#endif

