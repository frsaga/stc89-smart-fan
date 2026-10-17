#ifndef __FAN_H__
#define __FAN_H__

#include <reg52.h>

#define FAN_TICK_MS 50u

typedef enum
{
    MODE_NORMAL = 0,
    MODE_NATURE,
    MODE_SLEEP,
    MODE_MAX
} WIND_MODE;

void Fan_Init(void);
void Fan_PowerToggle(void);
void Fan_SwitchMode(WIND_MODE mode);
void Fan_NextLevel(WIND_MODE mode);
void Fan_UpdateEnvironment(int temp_x10, int hum_x10);
void Fan_UpdatePressure(int press_x100);
void Fan_TaskTick(void);
void Fan_GetStatus(WIND_MODE *mode,
                   unsigned char *level,
                   unsigned char *duty,
                   unsigned char *on,
                   int *t_x10,
                   int *h_x10,
                   int *p_x100);

#endif

