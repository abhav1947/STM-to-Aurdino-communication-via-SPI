/*
 * 008spi_TxRx_Aurdino.c
 *
 *  Created on: 07-Nov-2021
 *      Author: Abhav S Velidi
 */

/* SPI Master(STM) and SPI Slave(Aurdino) command and response based communication
 * 1. Use SPI3 in Full Duplex
 * ST board be in SPI master and aurdino will be configured for SPI slave mode
 * Use DFF = 0 i.e 8bit data mode
 * Use hardware slave management (SSM = 0)
 * SCLK speed = 2Mhz, fclk = 16Mhz
 * */

/* Requirements
 * 1. CMD_LED_CTRL <pin no>	<value>
 * <pin no> digital pin number of the aurdino board (0 t0 9)
 * <value> 1 = ON, 0 = OFF;
 * Slave Action: Control the digital pin ON or OFF on aurdino
 * Slave returns: None
 *
 * 2. CMD_SENSOR_READ <analog_pin_number>
 * <analog_pin_number> Analog pin number of the aurdino board(A0 to A5)
 * Slave Action: Slave should read the analog value of the supplied pin
 * Slave returns: 1 byte of analog read value (As DFF is set to 1 byte or 8bits mode)
 *
 * 3. CMD_LED_READ <pin no>
 * <pin no> digital pin number of the aurdino board
 * Slave Action: Read the status of the supplied pin number
 * Slave returns: 1 byte of led status. 1 = ON, 0 = OFF
 *
 * 4. CMD_PRINT <len> <message>
 * <len> 1 byte of length information of the message to follow
 * <message> message of 'len' bytes
 * Salve Action: Receive the message and display via serial port
 * Slave returns: Nothing
 */

/*Flow of program
 * 										  Start
 * 									   Enter main()
 * 								   All Initializations
 * 							  Wait till button is pressed
 * 								  Execute CMD_LED_CTRL
 * 						      Wait till button is pressed
 * 								 Execute CMD_SENSOR_READ
 * 							  Wait till button is pressed
 * 								  Execute CMD_LED_READ
 */

/* From Datasheet Alternate function Pins for SPI3
 * SPI3_SCK -> PB3
 * SPI3_MISO -> PB4
 * SPI3_MOSI -> PB5
 * SPI3_NSS -> PA4
 * AF for GPIOB -> AF6
 * AF for GPIOA -> AF6
 */
/* Button for sending data
 * PA0 -> BTN
 */

/*			 														VERY IMP
 * When we send i byte of data from master we receive 1 byte of data from the slave simultaneously so we need to keep clearing the RXNE flag and
 * vice versa
 * */

#include "stm32f302r8.h"
#include<string.h>
#include<stdint.h>

//Command Codes
#define COMMAND_LED_CNTRL		0x50
#define COMMAND_SENSOR_READ		0x51
#define COMMAND_LED_READ		0x52
#define COMMAND_PRINT			0x53
#define COMMAND_ID_READ			0x54

#define LED_ON	1
#define LED_OFF	0

//aurdino analog pins
#define ANALOG_PIN0		0
#define ANALOG_PIN1		1
#define ANALOG_PIN2		2
#define ANALOG_PIN3		3
#define ANALOG_PIN4		4
#define ANALOG_PIN5		5
#define ANALOG_PIN6		6
#define ANALOG_PIN7		7

//aurdino led pins
#define LED_PIN_NO_9	9

void GPIO_InitforSPI(void)
{
	GPIO_Handel_t GPIOHandle, GPIOButtn;
	GPIOHandle.pGPIOX = GPIOA;
	GPIOHandle.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	GPIOHandle.GPIO_PinConfig.GPIO_PinAltFunMode = 6;
	GPIOHandle.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	GPIOHandle.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	GPIOHandle.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_HIGH;

	//Configure NSS as PA4
	GPIOHandle.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_4;
	GPIO_Init(&GPIOHandle);

	GPIOHandle.pGPIOX = GPIOB;
	GPIOHandle.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	GPIOHandle.GPIO_PinConfig.GPIO_PinAltFunMode = 6;
	GPIOHandle.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	GPIOHandle.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	GPIOHandle.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_HIGH;

	//Configure SCLK
	GPIOHandle.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_3;
	GPIO_Init(&GPIOHandle);

	//Configure MOSI
	GPIOHandle.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
	GPIO_Init(&GPIOHandle);

	//Configure MISO
	GPIOHandle.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_4;
	GPIO_Init(&GPIOHandle);

	//Configure Button
	GPIOButtn.pGPIOX = GPIOA;
	GPIOButtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOButtn.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	GPIOButtn.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_HIGH;
	GPIOButtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
	GPIO_Init(&GPIOButtn);

}

void SPI3_Inits(void)
{
	SPI_Handle_t SPIHandle;
	SPIHandle.pSPIx = SPI3;
	SPIHandle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;
	SPIHandle.SPIConfig.SPI_BusConfig = SPI_CONFIG_FD;
	SPIHandle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV8;
	SPIHandle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPIHandle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPIHandle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
	SPIHandle.SPIConfig.SPI_SSM = SPI_SSM_DI;
	SPI_Init(&SPIHandle);

}

//Switch debouncing
void delay(void)
{
	for(uint32_t i = 0; i <50000/2 ; i++);
}

uint8_t SPI_VerifyResponse(uint8_t ackbyte)
{
	if(ackbyte == 0xF5)
	{
		//it is ack
		return 1;
	}else
	{
		//it is nack
		return 0;
	}
}

int main()
{
	uint8_t dummy_write = 0xff;
	uint8_t dummy_read;
	//Initialize the GPIO Pin Alternate Function mode for SPI3
	GPIO_InitforSPI();

	//SPI3 Peripheral Configuration
	SPI3_Inits();

	//When SSOE is 0, NSS toggles according to SPE bit
	SPI_SSOEConfig(SPI3, ENABLE);

	while(1)
	{
			//wait till button is pressed
		    while(!(GPIO_ReadFromInputpin(GPIOA,GPIO_PIN_NO_0)));
			delay(); // For switch debouncing

			//enable SPI peripheral
			SPI_PeripheralControl(SPI3, ENABLE);

			//1. CMD_LED_CNTRL
			uint8_t cmmndcode = COMMAND_LED_CNTRL;
			uint8_t ackbyte;
			uint8_t args[2];

			//send command to be executed
			SPI_SendData(SPI3,&cmmndcode, 1);

			//Read from dummy_read variable to automatically clear RXNE bit or even reset RXNE flag function can be written
			SPI_ReceiveData(SPI3, &dummy_read, 1);

			//If slave supports command it sends ACK
			//Now we have to receive the ACK
			//Send 1 byte of dummy data to fetch the response from the slave
			SPI_SendData(SPI3,&dummy_write,1);
			SPI_ReceiveData(SPI3, &ackbyte, 1);

			//Lets compare weather we have got ACK nor NACK, So lets create a  small function
			if (SPI_VerifyResponse(ackbyte))
			{
				args[0] = LED_PIN_NO_9;
				args[1] = LED_ON;
				//Send data
				SPI_SendData(SPI3, args, 2);
			}
			// end of CMD_LED_CNTRL



			//2. CMD_SENSOR_READ <analog pin number(1)>

			//wait till button is pressed
			while(! GPIO_ReadFromInputpin(GPIOA, GPIO_PIN_NO_0));
			delay(); // For switch debouncing

			cmmndcode = COMMAND_SENSOR_READ;
			//Send the command to the aurdino
			SPI_SendData(SPI3, &cmmndcode, 1);

			//Read from dummy_read variable to automatically clear RXNE bit
			SPI_ReceiveData(SPI3, &dummy_read, 1);

			//Send 1 byte of dummy data to fetch the response from the slave
			SPI_SendData(SPI3,&dummy_write,1);
			//Lets compare weather we have got ACK nor NACK, So lets create a  small function
			SPI_ReceiveData(SPI3, &ackbyte, 1);

			if(SPI_VerifyResponse(ackbyte))
			{
				args[0] = ANALOG_PIN1;
				//Send which pin is to be used for analog read
				SPI_SendData(SPI3, args, 1);

				//Read from dummy_read variable to automatically clear RXNE bit
				SPI_ReceiveData(SPI3, &dummy_read, 1);

				//delay so that slave can be ready with data
				delay();

				//send dummy data to fetch response from slave
				SPI_SendData(SPI3, &dummy_write, 1);

				//Read the analog data sent by the aurdino
				uint8_t analog_read;
				SPI_ReceiveData(SPI2, &analog_read, 1);
			}



			//3. COMMAND_LED_READ

			//wait till button is pressed
			while(!(GPIO_ReadFromInputpin(GPIOA,GPIO_PIN_NO_0)));
			delay(); // For switch Debouncing

			cmmndcode = COMMAND_LED_READ;
			//Send the command to be executed to Aurdino
			SPI_SendData(SPI3,&cmmndcode,1);

			//As 1 byte of data is sent 1 byte of data is received so we do a dummy read to clear RXNE flag automatically
			SPI_ReceiveData(SPI3, &dummy_read, 1);

			//Send 1 byte of dummy data to fetch the response from the slave
			SPI_SendData(SPI3,&dummy_write,1);
			//Lets compare weather we have got ACK nor NACK, So lets create a  small function
			SPI_ReceiveData(SPI3, &ackbyte, 1);

			if(SPI_VerifyResponse(ackbyte))
			{
				args[0] = LED_PIN_NO_9;
				//send pin number to know from which pin, pin status must be read
				SPI_SendData(SPI3, args, 1);

				//As 1 byte of data is sent 1 byte of data is received so we do a dummy read to clear RXNE flag automatically
				SPI_ReceiveData(SPI3, &dummy_read, 1);

				//send dummy data to fetch response from slave
				SPI_SendData(SPI3, &dummy_write, 1);

				//Read the analog data sent by the aurdino
				uint8_t pin_status;
				SPI_ReceiveData(SPI2, &pin_status, 1);
			}


			//4. CMD_PRINT
			//wait till button is pressed
			while(!(GPIO_ReadFromInputpin(GPIOA,GPIO_PIN_NO_0)));
			delay(); // For switch Debouncing

			cmmndcode = COMMAND_LED_READ;
			//Send the command to be executed to Aurdino
			SPI_SendData(SPI3,&cmmndcode,1);

			//As 1 byte of data is sent 1 byte of data is received so we do a dummy read to clear RXNE flag automatically
			SPI_ReceiveData(SPI3, &dummy_read, 1);

			//Send 1 byte of dummy data to fetch the response from the slave
			SPI_SendData(SPI3,&dummy_write,1);
			//Lets compare weather we have got ACK nor NACK, So lets create a  small function
			SPI_ReceiveData(SPI3, &ackbyte, 1);

			uint8_t message[] = "Hello World";
			if(SPI_VerifyResponse(ackbyte))
			{
				//Send string length
				args[0] = strlen((char*)message);

				//send length of string
				SPI_SendData(SPI3, args, 1);

				//As 1 byte of data is sent 1 byte of data is received so we do a dummy read to clear RXNE flag automatically
				SPI_ReceiveData(SPI3, &dummy_read, 1);

				//send string itself
				for(uint32_t i=0; i << args[0]; i++)
				{
					//Sending 1 byte of data or 1 letter at time
					SPI_SendData(SPI3, &message[i], 1);
					//Dummy read to reset RTXE bi
					SPI_ReceiveData(SPI3, &dummy_read, 1);
				}
			}

			//wait for busy flag to reset
			while(SPI_GetFlagStatus(SPI3, SPI_BUSY_FLAG));

			//disable SPI peripheral once the BUSY flag is reset
			SPI_PeripheralControl(SPI3, DISABLE);

	}




	return 0;
}


