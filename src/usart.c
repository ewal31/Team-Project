#include <stm32f4xx.h>
#include <stm32f4xx_usart.h>
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"

/**
  * @brief  Initialise USARTx 
  * @param  None
  * @retval None
  */
void init_usart(void) {

	GPIO_InitTypeDef 	GPIO_InitStructure;
	USART_InitTypeDef 	USART_InitStructure;

	/* Enable USARTx clock */
	USARTx_CLK_INIT(USARTx_CLK, ENABLE);
	

	/* Enable TX/RX GPIO clocks */
	RCC_AHB1PeriphClockCmd(USARTx_TX_GPIO_CLK | USARTx_RX_GPIO_CLK, ENABLE);

	USART_OverSampling8Cmd(USARTx, ENABLE);
	/* Configure USART to 115200 baudrate, 8bits, 1 stop bit, no parity, no flow control */
	USART_InitStructure.USART_BaudRate = 115200;			
	//USART_InitStructure.USART_BaudRate = 230400;			
  	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
  	USART_InitStructure.USART_Parity = USART_Parity_No;
  	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
	USART_Init(USARTx, &USART_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  
	GPIO_InitStructure.GPIO_Pin = USARTx_TX_PIN;
	GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = USARTx_RX_PIN;
	GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStructure);

	/* Connect USART TX and RX pins*/
	GPIO_PinAFConfig(USARTx_TX_GPIO_PORT, USARTx_TX_SOURCE, USARTx_TX_AF);
	GPIO_PinAFConfig(USARTx_RX_GPIO_PORT, USARTx_RX_SOURCE, USARTx_RX_AF);
	
	/* Enable USART */
  	USART_Cmd(USARTx, ENABLE);
}

void usart_send_char(uint8_t tx_char){
	USART_SendData(USARTx, tx_char);								
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET){
		//vTaskDelay(1);
	}
	USART_ClearFlag(USARTx, USART_FLAG_TC);	
}

void usart_printf(uint8_t* message, int len){
	for(int i=0; i<len; i++){
		usart_send_char(message[i]);
	}
}

void usart_debug_task(void *param){
	while(1){
		usart_send_char('U');
		//usart_printf( (uint8_t*) "Hello World\n",12);
        vTaskDelay(1000);
	}
}


