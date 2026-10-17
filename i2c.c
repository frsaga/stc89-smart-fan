#include "i2c.h"

// 微秒级延时函数（需要根据实际晶振频率调整）
void I2C_Delay(void)
{
    unsigned char i;
    for(i = 0; i < 10; i++);
}

// I2C初始化
void I2C_Init(void)
{
    SCL = 1;
    SDA = 1;
}

// I2C起始信号
void I2C_Start(void)
{
    SDA = 1;
    SCL = 1;
    I2C_Delay();
    SDA = 0;
    I2C_Delay();
    SCL = 0;
}

// I2C停止信号
void I2C_Stop(void)
{
    SDA = 0;
    SCL = 0;
    I2C_Delay();
    SCL = 1;
    I2C_Delay();
    SDA = 1;
    I2C_Delay();
}

// I2C发送应答信号
void I2C_Ack(void)
{
    SDA = 0;
    I2C_Delay();
    SCL = 1;
    I2C_Delay();
    SCL = 0;
    I2C_Delay();
    SDA = 1;
}

// I2C发送非应答信号
void I2C_NAck(void)
{
    SDA = 1;
    I2C_Delay();
    SCL = 1;
    I2C_Delay();
    SCL = 0;
    I2C_Delay();
}

// I2C等待应答信号
unsigned char I2C_WaitAck(void)
{
    unsigned char ack;
    SDA = 1;
    I2C_Delay();
    SCL = 1;
    I2C_Delay();
    ack = SDA;
    SCL = 0;
    I2C_Delay();
    return ack;
}

// I2C发送一个字节
void I2C_SendByte(unsigned char byte)
{
    unsigned char i;
    for(i = 0; i < 8; i++)
    {
        if(byte & 0x80)
            SDA = 1;
        else
            SDA = 0;
        I2C_Delay();
        SCL = 1;
        I2C_Delay();
        SCL = 0;
        I2C_Delay();
        byte <<= 1;
    }
}

// I2C接收一个字节
unsigned char I2C_ReceiveByte(void)
{
    unsigned char i, byte = 0;
    SDA = 1;
    for(i = 0; i < 8; i++)
    {
        byte <<= 1;
        SCL = 1;
        I2C_Delay();
        if(SDA)
            byte |= 0x01;
        SCL = 0;
        I2C_Delay();
    }
    return byte;
}

