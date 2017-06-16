#ifndef ADS1115_H
#define ADS1115_H

#include "ads1115_config.h"
#include "stm32f10x_i2c.h"
#include "stdbool.h"


#define ADS1115_GND_ADDR 0x90
#define ADS1115_POW_ADDR 0x92
#define ADS1115_SCL_ADDR 0x96
#define ADS1115_SDA_ADDR 0x94

#define CONVER_REG_ADDR 0x00
#define CONFIG_REG_ADDR 0x01
#define LO_TRESH_REG_ADDR 0x02
#define HI_TRESH_REG_ADDR 0x03


void ADS1115_Init(void);
void ADS1115_Restart(void);
void ADS1115_Reset(void);
void ADS1115_Timer_Init(void);
void ADS1115_Reset_Slave_I2C(void);
ErrorStatus ADS1115_WriteReg(uint8_t slaveAddr, uint8_t regAddr, const uint8_t* buffer);
ErrorStatus ADS1115_ReadReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t* buffer);


#endif
