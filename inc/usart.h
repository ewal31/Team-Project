#ifndef USART_H
#define USART_H

/*	Note: only USART1 and USART6 are connected to APB2
* 		  the other USARTx are connected to APB1
*/

#define USARTx                           USART6
#define USARTx_CLK                       RCC_APB2Periph_USART6
#define USARTx_CLK_INIT                  RCC_APB2PeriphClockCmd
//#define USARTx_IRQn                      USART3_IRQn
//#define USARTx_IRQHandler                USART3_IRQHandler

#define USARTx_TX_PIN                    GPIO_Pin_6                
#define USARTx_TX_GPIO_PORT              GPIOC                       
#define USARTx_TX_GPIO_CLK               RCC_AHB1Periph_GPIOC
#define USARTx_TX_SOURCE                 GPIO_PinSource6
#define USARTx_TX_AF                     GPIO_AF_USART6

#define USARTx_RX_PIN                    GPIO_Pin_7                
#define USARTx_RX_GPIO_PORT              GPIOC                    
#define USARTx_RX_GPIO_CLK               RCC_AHB1Periph_GPIOC
#define USARTx_RX_SOURCE                 GPIO_PinSource7
#define USARTx_RX_AF                     GPIO_AF_USART6

void init_usart(void);
void usart_send_char(uint8_t tx_char);
void usart_printf(uint8_t* message, int len);
void usart_debug_task(void *param);

#endif
