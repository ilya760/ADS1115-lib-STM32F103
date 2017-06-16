#ifndef ADS1115_CONFIG_H
#define ADS1115_CONFIG_H



#define ADS1115_I2C                  I2C2
#define ADS1115_I2C_RCC_Periph       RCC_APB1Periph_I2C2
#define ADS1115_I2C_Port             GPIOB
#define ADS1115_I2C_SCL_Pin          GPIO_Pin_10
#define ADS1115_I2C_SDA_Pin          GPIO_Pin_11
#define ADS1115_I2C_RCC_Port         RCC_APB2Periph_GPIOB
#define ADS1115_I2C_Speed            400000 

#endif
