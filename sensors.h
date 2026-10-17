#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <reg52.h>

void Sensors_Init(void);
bit Sensors_ReadAHT20(int *t_x10, int *h_x10);
bit Sensors_ReadBMP280(int *p_x10);  // 气压值，单位：hPa * 10 (例如：10132 = 1013.2 hPa)

#endif

