/* spi.c 
 *
 * Basic SPI functions.
 *
 * Written for ENGG4810 Team 30 by:
 * - Mitchell Krome
 * - Edware Wall
 *
 * --- HOW TO USE ---
 *
 * 1. Take the lock for the SPI bus you want!
 * 2. Use the supplied SPI commands. Note that ALL commands, even DMA commands,
 *    will block the task. Non-DMA commands will do a busy wait, DMA commands
 *    will wait on a semaphore.
 * 3. Be nice and give the lock back!
 *
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "spi.h"

static SemaphoreHandle_t SPI1Lock;
static SemaphoreHandle_t DMASPI1Signal;
static SemaphoreHandle_t SPI2Lock;

static DMA_InitTypeDef DMA2Init;
static SPI_InitTypeDef SPI1Init;
static SPI_InitTypeDef SPI2Init;

void spi1_change_prescaler(uint16_t prescaler)
{
    SPI1Init.SPI_BaudRatePrescaler = prescaler;
    SPI_Init(SPI1, &SPI1Init);
}

/* Receive a buffer over spi1 using DMA. Sends 0xFF placeholder. */
void spi1_recv_data_dma(uint8_t *buff, uint16_t len)
{
    uint8_t send = 0xFF; 

    DMA2Init.DMA_BufferSize = len;
    DMA2Init.DMA_MemoryInc = DMA_MemoryInc_Disable;
    DMA2Init.DMA_Memory0BaseAddr =(uint32_t)(&send);
    DMA2Init.DMA_Channel = DMA_Channel_3;
    DMA2Init.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_Init(DMA2_Stream3, &DMA2Init);

    DMA2Init.DMA_BufferSize = len;
    DMA2Init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA2Init.DMA_Memory0BaseAddr =(uint32_t)buff;
    DMA2Init.DMA_Channel = DMA_Channel_3;
    DMA2Init.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_Init(DMA2_Stream2, &DMA2Init);

    DMA_ITConfig(DMA2_Stream3, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA2_Stream3, ENABLE);
    DMA_Cmd(DMA2_Stream2, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);

    /* This should start the DMA transfer. */
    SPI_Cmd(SPI1, ENABLE);

    /* Once we can take the semaphore the transfer must be complete. */
    xSemaphoreTake(DMASPI1Signal, portMAX_DELAY);

    DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3);
    DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2);

    DMA_ITConfig(DMA2_Stream3, DMA_IT_TC, DISABLE);
    DMA_Cmd(DMA2_Stream3, DISABLE);
    DMA_Cmd(DMA2_Stream2, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);

    SPI_Cmd(SPI1, DISABLE);
}

/* Do a DMA transfer on spi1 but send the same thing every time. This is a special
 * command to make blanking the screen very efficient.
 */
void spi1_send_byte_dma(uint8_t data, uint16_t len)
{
    uint8_t result; 

    DMA2Init.DMA_BufferSize = len;
    DMA2Init.DMA_MemoryInc = DMA_MemoryInc_Disable;
    DMA2Init.DMA_Memory0BaseAddr =(uint32_t)(&data);
    DMA2Init.DMA_Channel = DMA_Channel_3;
    DMA2Init.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_Init(DMA2_Stream3, &DMA2Init);

    DMA2Init.DMA_BufferSize = len;
    DMA2Init.DMA_MemoryInc = DMA_MemoryInc_Disable;
    DMA2Init.DMA_Memory0BaseAddr =(uint32_t)(&result);
    DMA2Init.DMA_Channel = DMA_Channel_3;
    DMA2Init.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_Init(DMA2_Stream2, &DMA2Init);

    DMA_ITConfig(DMA2_Stream2, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA2_Stream3, ENABLE);
    DMA_Cmd(DMA2_Stream2, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);

    /* This should start the DMA transfer. */
    SPI_Cmd(SPI1, ENABLE);

    /* Once we can take the semaphore the transfer must be complete. */
    xSemaphoreTake(DMASPI1Signal, portMAX_DELAY);

    DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3);
    DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2);

    DMA_ITConfig(DMA2_Stream2, DMA_IT_TC, DISABLE);
    DMA_Cmd(DMA2_Stream3, DISABLE);
    DMA_Cmd(DMA2_Stream2, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);

    SPI_Cmd(SPI1, DISABLE);
}

/* Send a buffer of data over SPI1 using DMA. */
void spi1_send_data_dma(uint8_t *buff, uint16_t len)
{
    uint8_t result; 

    DMA2Init.DMA_BufferSize = len;
    DMA2Init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA2Init.DMA_Memory0BaseAddr = (uint32_t)buff;
    DMA2Init.DMA_Channel = DMA_Channel_3;
    DMA2Init.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_Init(DMA2_Stream3, &DMA2Init);

    DMA2Init.DMA_BufferSize = len;
    DMA2Init.DMA_MemoryInc = DMA_MemoryInc_Disable;
    DMA2Init.DMA_Memory0BaseAddr =(uint32_t)(&result);
    DMA2Init.DMA_Channel = DMA_Channel_3;
    DMA2Init.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_Init(DMA2_Stream2, &DMA2Init);

    DMA_ITConfig(DMA2_Stream2, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA2_Stream3, ENABLE);
    DMA_Cmd(DMA2_Stream2, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);

    /* This should start the DMA transfer. */
    SPI_Cmd(SPI1, ENABLE);

    /* Once we can take the semaphore the transfer must be complete. */
    xSemaphoreTake(DMASPI1Signal, portMAX_DELAY);

    DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3);
    DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2);

    DMA_ITConfig(DMA2_Stream2, DMA_IT_TC, DISABLE);
    DMA_Cmd(DMA2_Stream3, DISABLE);
    DMA_Cmd(DMA2_Stream2, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);

    SPI_Cmd(SPI1, DISABLE);
}

/* Send and receive a byte of data on spi1 */
uint8_t spi1_send_byte(uint8_t data)
{
    uint8_t ret; 

    SPI_Cmd(SPI1, ENABLE);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    ret = SPI_I2S_ReceiveData(SPI1);

    SPI_Cmd(SPI1, DISABLE);

    return ret;
}

/* Send and optionally recieve buffers of data. */
void spi1_send_data(uint8_t *send, uint8_t *recv, uint32_t num)
{
    SPI_Cmd(SPI1, ENABLE);

    /* Check if they want us to return anything. It's nicer to just do 1
     * comparison then compare each time.
     */
    if (recv) {
        for (uint32_t i = 0; i < num; ++i) {
            while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
            SPI_I2S_SendData(SPI1, *send++);
            while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
            *recv++ = SPI_I2S_ReceiveData(SPI1);
        }
    } else {
        for (uint32_t i = 0; i < num; ++i) {
            while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
            SPI_I2S_SendData(SPI1, *send++);
            while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
            /* Still need this because I don't know how to make it clear the
             * recv flag without reading the data or re-configuring the bus.
             */
            (void)SPI_I2S_ReceiveData(SPI1);
        }
    }

    SPI_Cmd(SPI1, DISABLE);
}

/* Take the lock for SPI1. */
void acquire_spi1(void)
{
    xSemaphoreTake(SPI1Lock, portMAX_DELAY);
}

/* Give the SPI1 lock back. */
void release_spi1(void)
{
    xSemaphoreGive(SPI1Lock);
}
    
/* Interrupt handlers. Only one should be active at a time (transmit or
 * receive) or else the semaphore could be given twice. If you're receiving
 * then the recieve finished interrupt is implicit.
 */
void DMA2_Stream2_IRQHandler(void)
{
    BaseType_t taskWoken = pdFALSE;

    DMA_ClearITPendingBit(DMA2_Stream2, DMA_IT_TCIF2);
    xSemaphoreGiveFromISR(DMASPI1Signal, &taskWoken);

    portYIELD_FROM_ISR(taskWoken);
}

void DMA2_Stream3_IRQHandler(void)
{
    BaseType_t taskWoken = pdFALSE;

    DMA_ClearITPendingBit(DMA2_Stream3, DMA_IT_TCIF3);
    xSemaphoreGiveFromISR(DMASPI1Signal, NULL);

    portYIELD_FROM_ISR(taskWoken);
}

/* Initialise all the peripherals needed for the spi to work. */
void spi1_init(void)
{
    GPIO_InitTypeDef GPIOInit;
    NVIC_InitTypeDef NVICInit;

    /* Turn on all the clocks we'll be using. SPI1 is on Port A, and we will
     * use nearest pins (which are B and C) for SS to keep signals close.
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    /* Configure the SPI1 bus pins. */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

    /* Initialise the SPI1 output GPIO pins. */
    GPIOInit.GPIO_Mode = GPIO_Mode_AF;
    GPIOInit.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIOInit.GPIO_OType = GPIO_OType_PP;
    GPIOInit.GPIO_Speed = GPIO_Fast_Speed;

    GPIOInit.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOA, &GPIOInit);
    GPIOInit.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIOInit);

    GPIOInit.GPIO_PuPd = GPIO_PuPd_UP;
    GPIOInit.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIOInit);

    /* Pack the SPI struct and initialise it. Both interfaces use the same
     * clock polarity and high sample on th first edge.
     */
    SPI1Init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI1Init.SPI_Mode = SPI_Mode_Master;
    SPI1Init.SPI_DataSize = SPI_DataSize_8b;
    SPI1Init.SPI_CPOL = SPI_CPOL_Low;
    SPI1Init.SPI_CPHA = SPI_CPHA_1Edge;
    SPI1Init.SPI_NSS = SPI_NSS_Soft;
    SPI1Init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    SPI1Init.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI1Init.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI1Init);

    /* Pack the DMA struct but don't init it. Some things will use differrent
     * modes!
     */
    DMA2Init.DMA_FIFOMode = DMA_FIFOMode_Disable ;
    DMA2Init.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull ;
    DMA2Init.DMA_MemoryBurst = DMA_MemoryBurst_Single ;
    DMA2Init.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA2Init.DMA_Mode = DMA_Mode_Normal;
    DMA2Init.DMA_PeripheralBaseAddr =(uint32_t) (&(SPI1->DR)) ;
    DMA2Init.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA2Init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA2Init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA2Init.DMA_Priority = DMA_Priority_High;

    /* Configure the NVIC to enable the interrupts for DMA. */
    NVICInit.NVIC_IRQChannelPreemptionPriority = 6;
    NVICInit.NVIC_IRQChannelSubPriority = 0;
    NVICInit.NVIC_IRQChannelCmd = ENABLE;

    NVICInit.NVIC_IRQChannel = DMA2_Stream2_IRQn;
    NVIC_Init(&NVICInit);
    NVICInit.NVIC_IRQChannel = DMA2_Stream3_IRQn;
    NVIC_Init(&NVICInit);

    /* Initialise the lock for SPI1. */
    SPI1Lock = xSemaphoreCreateBinary();
    xSemaphoreGive(SPI1Lock);

    /* Initialise the SPI1 DMA signal semaphore. */
    DMASPI1Signal = xSemaphoreCreateBinary();
}

/* Take the lock for SPI2. */
void acquire_spi2(void)
{
    xSemaphoreTake(SPI2Lock, portMAX_DELAY);
}

/* Give the SPI2 lock back. */
void release_spi2(void)
{
    xSemaphoreGive(SPI2Lock);
}

/* Initialise all the peripherals needed for the spi to work. */
void spi2_init(void)
{
    GPIO_InitTypeDef GPIOInit;

    /* Turn on all the clocks we'll be using. SPI2 is on Port B
     */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* Configure the SPI2 bus pins. */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);

    /* Initialise the SPI2 output GPIO pins. */
    GPIOInit.GPIO_Mode = GPIO_Mode_AF;
    GPIOInit.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIOInit.GPIO_OType = GPIO_OType_PP;
    GPIOInit.GPIO_Speed = GPIO_Fast_Speed;

    GPIOInit.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOB, &GPIOInit);
    GPIOInit.GPIO_Pin = GPIO_Pin_14;
    GPIO_Init(GPIOB, &GPIOInit);
    GPIOInit.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOB, &GPIOInit);

    /* Pack the SPI struct and initialise it. Both interfaces use the same
     * clock polarity and high sample on the first edge.
     */
    SPI2Init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI2Init.SPI_Mode = SPI_Mode_Master;
    SPI2Init.SPI_DataSize = SPI_DataSize_8b;
    SPI2Init.SPI_CPOL = SPI_CPOL_Low;
    SPI2Init.SPI_CPHA = SPI_CPHA_1Edge;
    SPI2Init.SPI_NSS = SPI_NSS_Soft;
    SPI2Init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    SPI2Init.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI2Init.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI2Init);

    /* Initialise the lock for SPI2. */
    SPI2Lock = xSemaphoreCreateBinary();
    xSemaphoreGive(SPI2Lock);

}

/* Send and receive a byte of data on spi2 */
uint8_t spi2_send_byte(uint8_t data)
{
    uint8_t ret; 

    SPI_Cmd(SPI2, ENABLE);

    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI2, data);
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    ret = SPI_I2S_ReceiveData(SPI2);

    SPI_Cmd(SPI2, DISABLE);

    return ret;
}

/* Send and optionally receive buffers of data. */
void spi2_send_data(uint8_t *send, uint8_t *recv, uint32_t num)
{
    SPI_Cmd(SPI2, ENABLE);

    /* Check if they want us to return anything. It's nicer to just do 1
     * comparison then compare each time.
     */
    if (recv) {
        for (uint32_t i = 0; i < num; ++i) {
            while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
            SPI_I2S_SendData(SPI2, *send++);
            while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
            *recv++ = SPI_I2S_ReceiveData(SPI2);
        }
    } else {
        for (uint32_t i = 0; i < num; ++i) {
            while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
            SPI_I2S_SendData(SPI2, *send++);
            while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
            /* Still need this because I don't know how to make it clear the
             * recv flag without reading the data or re-configuring the bus.
             */
            (void)SPI_I2S_ReceiveData(SPI2);
        }
    }

    SPI_Cmd(SPI2, DISABLE);
}

