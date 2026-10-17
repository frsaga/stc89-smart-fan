#include "sensors.h"
#include "i2c.h"
#include "delay.h"

#define AHT20_ADDR_WR 0x70
#define AHT20_ADDR_RD 0x71

static unsigned char idata i2c_buf[6];

static void AHT20_WriteCmd(unsigned char *buf, unsigned char len)
{
    unsigned char i;
    I2C_Start();
    I2C_SendByte(AHT20_ADDR_WR);
    I2C_WaitAck();
    for(i = 0; i < len; i++)
    {
        I2C_SendByte(buf[i]);
        I2C_WaitAck();
    }
    I2C_Stop();
}

void Sensors_Init(void)
{
    unsigned char cmd[3];
    cmd[0] = 0xBA; // soft reset
    AHT20_WriteCmd(cmd, 1);
    Delay1ms(20);
    cmd[0] = 0xE1;
    cmd[1] = 0x08;
    cmd[2] = 0x00;
    AHT20_WriteCmd(cmd, 3);
    Delay1ms(10);
}

bit Sensors_ReadAHT20(int *t_x10, int *h_x10)
{
    unsigned char i;
    unsigned long raw_h, raw_t;
    unsigned char cmd[3] = {0xAC, 0x33, 0x00};

    AHT20_WriteCmd(cmd, 3);
    Delay1ms(100);

    I2C_Start();
    I2C_SendByte(AHT20_ADDR_RD);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 0;
    }

    for(i = 0; i < 6; i++)
    {
        i2c_buf[i] = I2C_ReceiveByte();
        if(i == 5) I2C_NAck();
        else       I2C_Ack();
    }
    I2C_Stop();

    raw_h = ((unsigned long)i2c_buf[1] << 12) | ((unsigned long)i2c_buf[2] << 4) | (i2c_buf[3] >> 4);
    raw_t = (((unsigned long)i2c_buf[3] & 0x0F) << 16) | ((unsigned long)i2c_buf[4] << 8) | i2c_buf[5];

    if(h_x10) *h_x10 = (int)((raw_h * 1000UL) / 1048576UL);
    if(t_x10) *t_x10 = (int)((raw_t * 2000UL) / 1048576UL - 500);
    return 1;
}

bit Sensors_ReadBMP280(int *p_x10)
{
    unsigned char i;
    unsigned long raw_p;
    unsigned char cmd = 0xF7; // 压力数据寄存器地址
    unsigned char idata data_buf[3];

    // 发送读取命令
    I2C_Start();
    I2C_SendByte(0x76 << 1); // BMP280写地址
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 0;
    }
    I2C_SendByte(cmd);
    I2C_WaitAck();
    I2C_Stop();

    Delay1ms(10);

    // 读取3字节压力数据
    I2C_Start();
    I2C_SendByte((0x76 << 1) | 0x01); // BMP280读地址
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 0;
    }

    for(i = 0; i < 3; i++)
    {
        data_buf[i] = I2C_ReceiveByte();
        if(i == 2) I2C_NAck();
        else       I2C_Ack();
    }
    I2C_Stop();

    // BMP280压力数据格式：20位，MSB在前
    raw_p = ((unsigned long)data_buf[0] << 12) | ((unsigned long)data_buf[1] << 4) | (data_buf[2] >> 4);

    // 简化的压力计算（需要校准参数，这里使用简化版本）
    // BMP280压力范围：300-1100 hPa，原始值需要校准
    // 这里输出单位为：hPa * 10（例如：10132 表示 1013.2 hPa）
    // 为了避免 16 位 int 溢出，选择 hPa*10 而不是 hPa*100
    if(p_x10)
    {
        // 简化计算：假设原始值范围0-1048575对应3000-11000 (300.0-1100.0 hPa * 10)
        // 实际应该读取校准参数进行精确计算
        unsigned long p_hpa_x10 = 3000UL + ((raw_p * 8000UL) / 1048575UL);
        *p_x10 = (int)p_hpa_x10;
    }
    return 1;
}

