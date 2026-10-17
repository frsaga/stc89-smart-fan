#include <reg52.h>

// 约1ms延时（需按实际晶振微调）
void Delay1ms(unsigned int count)
{
    unsigned int i, j;
    for(i = 0; i < count; i++)
    {
        for(j = 0; j < 110; j++);
    }
}

