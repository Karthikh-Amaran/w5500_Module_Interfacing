/*
 * w5500_spi.c
 *
 *  Created on: Jul 11, 2023
 *      Author: Karthikh Amaran
 */

#include "stm32f4xx_hal.h"
#include "wizchip_conf.h"
#include "stdio.h"

extern SPI_HandleTypeDef hspi2;
/*
	 * Initialize the two GPIO Pins
	 * RESET -> PC3
	 * CS -> PC0
	 */

// In SPI Communication for every Data byte sent we receive a Data byte from the Slave
// To receive this we are using a return value here.
uint8_t SPIReadWrite(uint8_t data)
{
	// Wait till FIFO has a free slot
	while((hspi2.Instance->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE);

	* ( (__IO uint8_t*)&hspi2.Instance ->DR) = data; // Setting the TX data by the Master
	// Type Casting the Data Register to only 8bits

	// Now wait till data arrives
	while((hspi2.Instance->SR & SPI_FLAG_RXNE) != SPI_FLAG_RXNE);

	return (*(__IO uint8_t *)&hspi2.Instance->DR); // Returning the RX Byte sent by the Slave
}

void wizchip_select(void)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET); // CS Low
}

void wizchip_deselect(void)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET); // CS High
}
/*
 * Read only one Byte (Non Burst Function)
 */
// Only read the data from the Slave
uint8_t wizchip_read()
{
	uint8_t rb;
	rb = SPIReadWrite(0x00); // Dummy Write
	return rb;
}

/*
 *  Write only 1 Byte (Non Burst Function)
 */
// Only Write the Data to the Slave
// The DataByte returns a Byte here too but it would be don't care2
void wizchip_write(uint8_t wb)
{
	SPIReadWrite(wb);
}

/*
 *   Burst Function
 *   These Functions would read and write desired number of bytes
 */
/*
 *  Parameters :
 *  @1: It is an array
 *  @2: It is the length of the array
 */
void wizchip_readburst(uint8_t* pBuf, uint16_t len)
{
	for(uint16_t i=0;i<len;i++)
	{
		*pBuf = SPIReadWrite(0x00);
		pBuf++;
	}
}

void wizchip_writeburst(uint8_t* pBuf, uint16_t len)
{
	for(uint16_t i=0;i<len;i++)
	{
		SPIReadWrite(*pBuf);
		pBuf++;
	}
}
/*
 *  Initialization function to initialize the Reset and CS pins of the Controller
 */
void W5500IOInit()
{
	/*
	 * Initialize the two GPIO Pins
	 * RESET -> PC3
	 * CS -> PC0
	 */
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/*
 *  Top level Initialization
 */

void W5500Init()
{
	__IO uint8_t tmp;
	uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};
	// The module supports 8 independent module simultaneously
	W5500IOInit();
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET); // CS Pin set high by default

	// Send a pulse on the Reset PIN
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
	tmp = 0xff;
	while(tmp--);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
	// Registering our lower level Function to the Chip Driver
	reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
	reg_wizchip_spiburst_cbfunc(wizchip_readburst, wizchip_writeburst);

	/* WIZChip Initialization */
	if(ctlwizchip(CW_INIT_WIZCHIP, (void *) memsize) == -1)
	{
		printf("WIZCHIP Initialization Failed. \r\n");
		while(1);
	}
	printf("WIZCHIP Initialization Successful. \r\n");
}
