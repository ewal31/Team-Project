 /* rfm73.c
  *
  * Radio function for RFM73 RF module. 
  *
  * Written for ENGG4810 Team 30 by:
  * - Mitchell Krome
  * - Edward Wall
  *
  */ 

#include "FreeRTOS.h"
#include "task.h"
 
#include "rfm73.h"
#include "spi.h"

#define RADIO_PORT GPIOB

#define RADIO_CSN_PIN GPIO_Pin_8
#define RADIO_CSN_LOW() GPIO_ResetBits(RADIO_PORT, RADIO_CSN_PIN);
#define RADIO_CSN_HIGH() GPIO_SetBits(RADIO_PORT, RADIO_CSN_PIN);

#define RADIO_CE_PIN GPIO_Pin_10
#define RADIO_CE_LOW() GPIO_ResetBits(RADIO_PORT, RADIO_CE_PIN);
#define RADIO_CE_HIGH() GPIO_SetBits(RADIO_PORT, RADIO_CE_PIN);

static void switch_bank(uint8_t bank);

static uint8_t bank0_init[][2] = {
    { (0x20 | 0x00), 0x33 }, // No interupts, no crc, no power, TX mode
    { (0x20 | 0x01), 0x00 }, // No AutoAck
    { (0x20 | 0x02), 0x01 }, // Data pipe 0 enable
    { (0x20 | 0x03), 0x03 }, // Set 5 byte address field
    { (0x20 | 0x04), 0x00 }, // No retransmission
    { (0x20 | 0x05), 0x20 }, // Set rf channel to 0x20 (32)
    { (0x20 | 0x06), 0x05 }, // 2Mbps, 0dBm, low LNA gain
    { (0x20 | 0x11), 0x20 }, // Set 32 byte packets pipe 0
    { (0x20 | 0x1C), 0x00 }, // No dynamic payload
    { (0x20 | 0x1D), 0x00 }, // No features
};

/* Magic numbers from the datasheet + reference code. Have to send from end 
 * to beginning for reverse order.
 */
static uint8_t bank1_init[][5] = {
    { (0x20 | 0x00), 0x40, 0x4B, 0x01, 0xE2 },
    { (0x20 | 0x01), 0xC0, 0x4B, 0x00, 0x00 },
    { (0x20 | 0x02), 0xD0, 0xFC, 0x8C, 0x02 },
    { (0x20 | 0x03), 0x99, 0x00, 0x39, 0x41 },
    { (0x20 | 0x04), 0xd9, 0x96, 0x82, 0x1b }, // normal power
    { (0x20 | 0x05), 0x24, 0x06, 0x7F, 0xA6 }, // No rssi
    { (0x20 | 0x0C), 0x00, 0x12, 0x73, 0x00 }, // 120us settling, byte reversed
    { (0x20 | 0x0D), 0x46, 0xB4, 0x80, 0x00 }, // byte reversed
};

/* Ramp initialiser, byte reversed*/
static uint8_t bank1_ramp_init[12] = {
    (0x20 | 0x0E), 0x41, 0x20, 0x08, 0x04, 0x81, 0x20, 0xCF, 0xF7, 0xFE, 0xFF, 0xFF,
};

void rfm73_init(void) 
{

    /* Enable CE and CSN as outputs and set them high. */
    GPIO_InitTypeDef GPIOInit;
    //Clock B should be enabled already
    GPIOInit.GPIO_Mode = GPIO_Mode_OUT;
    GPIOInit.GPIO_OType = GPIO_OType_PP;
    GPIOInit.GPIO_Speed = GPIO_Fast_Speed;
    GPIOInit.GPIO_PuPd = GPIO_PuPd_UP;

    GPIOInit.GPIO_Pin = RADIO_CSN_PIN;
    GPIO_Init(RADIO_PORT, &GPIOInit);
    GPIOInit.GPIO_Pin = RADIO_CE_PIN;
    GPIO_Init(RADIO_PORT, &GPIOInit);
    
    RADIO_CE_LOW()
    RADIO_CSN_HIGH()
    
    for (uint8_t i = 0; i < 10; ++i) {
            for(volatile int j = 0; j < 1000; ++j); /* Should be about 15 us */
            RADIO_CSN_LOW()
            spi2_send_byte(bank0_init[i][0]);
            spi2_send_byte(bank0_init[i][1]);
            RADIO_CSN_HIGH()
    }
    
    switch_bank(1);
    
    for (uint8_t i = 0; i < 8; ++i) {
            RADIO_CSN_LOW()
            for (uint8_t j = 0; j < 5; ++j) {
                    spi2_send_byte(bank1_init[i][j]);
            }
            RADIO_CSN_HIGH()
            for(volatile int j = 0; j < 1000; ++j); /* Should be about 15 us */
    }
    
    RADIO_CSN_LOW()
    for (uint8_t i = 0; i < 12; ++i) {
            spi2_send_byte(bank1_ramp_init[i]);
    }
    RADIO_CSN_HIGH()

    switch_bank(0);

    RADIO_CE_HIGH()
}

/* Switch the register bank to bank. Must be called with spi enabled. */
static void switch_bank(uint8_t bank)
{
    RADIO_CSN_LOW()
    uint8_t currBank = (spi2_send_byte(RFM73_NOP_CMD) & RFM73_BANK_MASK);
    RADIO_CSN_HIGH()
    
    if (currBank) {
        if (!bank) {
            RADIO_CSN_LOW()
            spi2_send_byte(RFM73_ACTIVATE_CMD);
            spi2_send_byte(RFM73_SWAP_BANK);
            RADIO_CSN_HIGH()
        }
    } else {
        if (bank) {
            RADIO_CSN_LOW()
            spi2_send_byte(RFM73_ACTIVATE_CMD);
            spi2_send_byte(RFM73_SWAP_BANK);
            RADIO_CSN_HIGH()
        }
    }
}

void rfm73_flush_fifo(void)
{
    RADIO_CSN_LOW()
    spi2_send_byte(RFM73_FLUSH_RX);
    RADIO_CSN_HIGH()
}

uint8_t rfm73_check_fifo(void)
{
    uint8_t status;

    RADIO_CSN_LOW()
    status = spi2_send_byte(0xFF);
    RADIO_CSN_HIGH()

    if (status & (1<<6)) {
        return 1;
    }
    return 0;
}

/* Send a single packet. If disableAfter is 1, then after sending the packet
 * the radio will be turned to power down mode.
 */
void rfm73_send_packet(uint8_t *pack)
{
    RADIO_CSN_LOW()

    spi2_send_byte(RFM73_W_TX_FIFO);
    for (uint8_t i = 0; i < 32; ++i) {
        spi2_send_byte(*pack++);
    }

    RADIO_CSN_HIGH()
    
    /* NOTE: We may be able to skip the delay and use the spi_disable call
     * as our delay.
     */
    RADIO_CE_HIGH()
    for(volatile int i = 0; i < 2000; ++i); /* Should be about 15 us */
    RADIO_CE_LOW();
}

/* Receive a packet from the radio and load it into buff. */
void rfm73_receive_packet(uint8_t *buff) 
{
    /* Read out the FIFO. */
    RADIO_CSN_LOW()
    spi2_send_byte(RFM73_W_RX_FIFO);
    for (uint8_t i = 0; i < 32; ++i) {
        buff[i] = spi2_send_byte(0xFF);
    }
    RADIO_CSN_HIGH()

    RADIO_CE_LOW();
    for(volatile int i = 0; i < 10000; ++i); /* Should be about 15 us */

    /* Clear the flag. */
    RADIO_CSN_LOW()
    spi2_send_byte(RFM73_W_REG_CMD | RFM73_BANK0_STATUS);
    spi2_send_byte(0xFF);
    RADIO_CSN_HIGH()

    RADIO_CE_HIGH()
}

