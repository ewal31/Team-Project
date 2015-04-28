/*
 * rfm73.h
 *
 * Created: 12/03/2015 6:16:10 PM
 *  Author: Mitch
 */ 


#ifndef RFM73_H_
#define RFM73_H_

/* Bank 0 Registers. */
#define RFM73_BANK0_CONFIG 0x00
#define RFM73_BANK0_EN_AA 0x01
#define RFM73_BANK0_EX_RXADDR 0x02
#define RFM73_BANK0_SETUP_AW 0x03
#define RFM73_BANK0_SETUP_RETR 0x04
#define RFM73_BANK0_RF_CH 0x05
#define RFM73_BANK0_RF_SETUP 0x06
#define RFM73_BANK0_STATUS 0x07
#define RFM73_BANK0_OBSERVE_TX 0x08
#define RFM73_BANK0_CD 0x09
#define RFM73_BANK0_RF_ADDR_P0 0x0A
#define RFM73_BANK0_RF_ADDR_P1 0x0B
#define RFM73_BANK0_RF_ADDR_P2 0x0C
#define RFM73_BANK0_RF_ADDR_P3 0x0D
#define RFM73_BANK0_RF_ADDR_P4 0x0E
#define RFM73_BANK0_RF_ADDR_P5 0x0F
#define RFM73_BANK0_TX_ADDR 0x10
#define RFM73_BANK0_RX_PW_P0 0x11
#define RFM73_BANK0_RX_PW_P1 0x12
#define RFM73_BANK0_RX_PW_P2 0x13
#define RFM73_BANK0_RX_PW_P3 0x14
#define RFM73_BANK0_RX_PW_P4 0x15
#define RFM73_BANK0_RX_PW_P5 0x16
#define RFM73_BANK0_FIFO_STATUS 0x17
#define RFM73_BANK0_DYNPD 0x1C
#define RFM73_BANK0_FEATURE 0x1D

#define RFM73_W_REG_CMD 0x20
#define RFM73_R_REG_CMD 0x00
#define RFM73_W_RX_FIFO 0x61
#define RFM73_W_TX_FIFO 0xA0
#define RFM73_ACTIVATE_CMD 0x50
#define RFM73_NOP_CMD 0xFF
#define RFM73_FLUSH_RX 0xE1

#define RFM73_SWAP_BANK 0x53

#define RFM73_BANK_MASK 0x80

#define RFM73_TX_DELAY 255

void rfm73_send_packet(uint8_t *pack);
void rfm73_init(void);
uint8_t rfm73_check_fifo(void);
void rfm73_receive_packet(uint8_t *buff);
void radio_task(void *param);
void rfm73_flush_fifo(void);

#endif /* RFM73_H_ */
