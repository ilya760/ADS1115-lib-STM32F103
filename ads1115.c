#include "ads1115.h"

#define SLAVE_DEINIT_TIMER_PRESCALER 18-1
#define SLAVE_DEINIT_TIMER_AUTORELOAD_VALUE 10
#define SLAVE_DEINIT_SCL_PULSE_NUM 11


void ADS1115_Timer_Init()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitTypeDef timer;
	TIM_TimeBaseStructInit(&timer);
	timer.TIM_Prescaler = SLAVE_DEINIT_TIMER_PRESCALER;
	timer.TIM_CounterMode = TIM_CounterMode_Up;
	timer.TIM_ClockDivision = 0;
	timer.TIM_Period = SLAVE_DEINIT_TIMER_AUTORELOAD_VALUE;
	TIM_TimeBaseInit(TIM3, &timer);
};


void ADS1115_Reset_Slave_I2C()
{
		static bool pulse = true;
		
		uint8_t pulse_num = SLAVE_DEINIT_SCL_PULSE_NUM + 1;

		/*Generate pulses*/	
		while(pulse_num != 0)
		{
			//start timer in one-pulse-mode
			TIM3->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
			while ((TIM3->CR1 & TIM_CR1_CEN) != 0);

			if(pulse)
			{	
				GPIO_SetBits(GPIOB, ADS1115_I2C_SCL_Pin);
				GPIO_ResetBits(GPIOB, ADS1115_I2C_SDA_Pin);
				--pulse_num;
			}
			else
			{
				GPIO_ResetBits(GPIOB, ADS1115_I2C_SCL_Pin);
				GPIO_SetBits(GPIOB, ADS1115_I2C_SDA_Pin);					
			};		
			pulse = !pulse;	
		};
};


ErrorStatus EVENT_TIMEOUT(uint32_t I2C_EVENT)
{
	uint32_t tics = 500;
	
	/*Check given event until it occurs or tics = 0*/
	while (!I2C_CheckEvent(ADS1115_I2C, I2C_EVENT) && (tics != 0))
		--tics;
	
	/*Try to reset I2C*/
	if(tics == 0)
	{
		ADS1115_Reset();
		return ERROR;		
	};
	
	return SUCCESS;
};


ErrorStatus FLAG_TIMEOUT(uint32_t I2C_FLAG)
{
	uint32_t tics = 500;
	
	/*Check given event until it occurs or tics = 0*/
	while (I2C_GetFlagStatus(ADS1115_I2C, I2C_FLAG) && (tics != 0))
		--tics;
	
	/*Try to reset I2C*/
	if(tics == 0)
	{
		ADS1115_Reset();
		return ERROR;		
	};
	
	return SUCCESS;	
};


void ADS1115_Restart()
{
    I2C_InitTypeDef I2C_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable I2C and GPIO clocks */
    RCC_APB1PeriphClockCmd(ADS1115_I2C_RCC_Periph, ENABLE);
    RCC_APB2PeriphClockCmd(ADS1115_I2C_RCC_Port, ENABLE);

    /* Configure I2C pins: SCL and SDA */
    GPIO_InitStructure.GPIO_Pin = ADS1115_I2C_SCL_Pin | ADS1115_I2C_SDA_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(ADS1115_I2C_Port, &GPIO_InitStructure);

    /* I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = ADS1115_I2C_Speed;

    I2C_Init(ADS1115_I2C, &I2C_InitStructure);
    /* I2C Peripheral Enable */
    I2C_Cmd(ADS1115_I2C, ENABLE);		
};


void ADS1115_Reset(void)
{
	/*Deinit I2C*/
	I2C_DeInit(ADS1115_I2C);	
	
	/*Reinit ADS1115_I2C SCL as GPIO*/
	RCC_APB2PeriphClockCmd(ADS1115_I2C_RCC_Port, ENABLE);
	GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_StructInit(&GPIO_InitStruct);
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStruct.GPIO_Pin = ADS1115_I2C_SCL_Pin | ADS1115_I2C_SDA_Pin ;
  GPIO_Init(GPIOB, &GPIO_InitStruct);	
	
	/*11 pulses on SCL to deinit all slaves */
	ADS1115_Reset_Slave_I2C();
	

	/*Restart I2C*/		
	I2C_DeInit(ADS1115_I2C);
	ADS1115_Restart();		
};


void ADS1115_Init(void)
{
	ADS1115_Restart();
	ADS1115_Timer_Init();	
};


ErrorStatus ADS1115_I2C_Write(uint8_t slaveAddr, uint8_t regAddr, const uint8_t* buffer)
{
	/* While the bus is busy */
	if(FLAG_TIMEOUT(I2C_FLAG_BUSY) == ERROR) return ERROR;

	/* Send START condition */
	I2C_GenerateSTART(ADS1115_I2C, ENABLE);

	/* Test on EV5 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_MODE_SELECT) == ERROR) return ERROR;

	/* Send ADS1115 address for write */
	I2C_Send7bitAddress(ADS1115_I2C, slaveAddr, I2C_Direction_Transmitter);

	/* Test on EV6 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == ERROR) return ERROR;

	/* Send the ADS1115's internal register address to write to */
	I2C_SendData(ADS1115_I2C, regAddr);

	/* Test on EV8 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_BYTE_TRANSMITTED) == ERROR) return ERROR;

	/* Send the first byte to be written */
	I2C_SendData(ADS1115_I2C, *buffer);
	
	/* Test on EV8 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_BYTE_TRANSMITTED) == ERROR) return ERROR;

	/* Point to the next location from where the byte will be transmitted*/
	++buffer;

	/* Send the second byte to be written */
	I2C_SendData(ADS1115_I2C, *buffer);

	/* Test on EV8 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_BYTE_TRANSMITTED) == ERROR) return ERROR;

	/* Send STOP condition */
	I2C_GenerateSTOP(ADS1115_I2C, ENABLE);


	return SUCCESS;
};


void ADS1115_I2C_ReadSetup(uint8_t slaveAddr, uint8_t regAddr)
{
	
};


ErrorStatus ADS1115_I2C_Read(uint8_t slaveAddr, uint8_t regAddr, uint8_t* buffer)
{
	// ENTR_CRT_SECTION();
	
	/* While the bus is busy */
	if(FLAG_TIMEOUT(I2C_FLAG_BUSY) == ERROR) return ERROR;
	
	/* Send START condition */
	I2C_GenerateSTART(ADS1115_I2C, ENABLE);

	/* Test on EV5 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_MODE_SELECT) == ERROR) return ERROR;

	/* Send ADS1115 address for write */
	I2C_Send7bitAddress(ADS1115_I2C, slaveAddr, I2C_Direction_Transmitter);

	/* Test on EV6 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == ERROR) return ERROR;

	/* Clear EV6 by setting again the PE bit */
	I2C_Cmd(ADS1115_I2C, ENABLE);

	/* Send the ADS1115's internal address to write to */
	I2C_SendData(ADS1115_I2C, regAddr);

	/* Test on EV8 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_BYTE_TRANSMITTED) == ERROR) return ERROR;

	I2C_GenerateSTOP(ADS1115_I2C, ENABLE);
	while((ADS1115_I2C->CR1 & I2C_CR1_STOP) == 0);

	/* Send STRAT condition a second time */
	I2C_GenerateSTART(ADS1115_I2C, ENABLE);

	/* Test on EV5 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_MODE_SELECT) == ERROR) return ERROR;

	/* Send ADS1115 address for read */
	I2C_Send7bitAddress(ADS1115_I2C, slaveAddr, I2C_Direction_Receiver);

	/* Test on EV6 and clear it */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) == ERROR) return ERROR;

	//		/* Read a byte from the ADS1115*/
	//		*buffer = I2C_ReceiveData(ADS1115_I2C);
	//		
	//		/* Point to the next location where the byte read will be saved */
	//		buffer++;
	//		
	//		/* Disable Acknowledgement */
	//		I2C_AcknowledgeConfig(ADS1115_I2C, DISABLE);

	//		/* Send STOP Condition */
	//		I2C_GenerateSTOP(ADS1115_I2C, ENABLE);
	//		
	//		/* Read a byte from the ADS1115*/
	//		*buffer = I2C_ReceiveData(ADS1115_I2C);
	//		
	//		I2C_AcknowledgeConfig(ADS1115_I2C, ENABLE);		
	//			
	//    // EXT_CRT_SECTION();

	/* EV6_1: The acknowledge disable should be done just after EV6,
	that is after ADDR is cleared, so disable all active IRQs around ADDR clearing and
	ACK clearing */

	// Vedi "AN2824"@16
	I2C_NACKPositionConfig(ADS1115_I2C, I2C_NACKPosition_Next);
	__disable_irq();
	/* Clear ADDR by reading SR2 register  */
	int rubbish = ADS1115_I2C->SR2;

	/* Clear ACK bit */
	I2C_AcknowledgeConfig(ADS1115_I2C, DISABLE);

	/*Re-enable IRQs */
	__enable_irq();

	while(I2C_GetFlagStatus(ADS1115_I2C, I2C_FLAG_BTF) == SET);

	/* Disable IRQs around STOP programming and data reading because of the limitation ?*/
	__disable_irq();

	/* Read first data */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_BYTE_RECEIVED) == ERROR) return ERROR;			
	buffer[0] = I2C_ReceiveData(ADS1115_I2C);

	/* Program the STOP */
	I2C_GenerateSTOP(ADS1115_I2C, ENABLE);

	/* Re-enable IRQs */
	__enable_irq();

	/* Read second data */
	if(EVENT_TIMEOUT(I2C_EVENT_MASTER_BYTE_RECEIVED) == ERROR) return ERROR;
	buffer[1] = I2C_ReceiveData(ADS1115_I2C);

	/* Make sure that the STOP bit is cleared by Hardware before CR1 write access */
	while((ADS1115_I2C->CR1 & I2C_CR1_STOP) == 0);

	/* Enable Acknowledgement to be ready for another reception */
	I2C_AcknowledgeConfig(ADS1115_I2C, ENABLE);

	/* Clear POS bit */
	I2C_NACKPositionConfig(ADS1115_I2C, I2C_NACKPosition_Current);

	return SUCCESS;
};


ErrorStatus ADS1115_WriteReg(uint8_t slaveAddr, uint8_t regAddr, const uint8_t* buffer)
{
	return ADS1115_I2C_Write(slaveAddr, regAddr, buffer);
};	


ErrorStatus ADS1115_ReadReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t* buffer)
{
	return	ADS1115_I2C_Read(slaveAddr, regAddr, buffer);
};

