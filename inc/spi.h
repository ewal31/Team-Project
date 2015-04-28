#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

/* General Functions */
void spi1_init(void);
void spi2_init(void);

/* SPI1 Functions. */
void acquire_spi1(void);
void release_spi1(void);
void spi1_send_data(uint8_t *send, uint8_t *recv, uint32_t num);
uint8_t spi1_send_byte(uint8_t data);
void spi1_send_data_dma(uint8_t *buff, uint16_t len);
void spi1_send_byte_dma(uint8_t data, uint16_t len);
void spi1_recv_data_dma(uint8_t *buff, uint16_t len);
void spi1_change_prescaler(uint16_t prescaler);

void acquire_spi2(void);
void release_spi2(void);
uint8_t spi2_send_byte(uint8_t data);
void spi2_send_data(uint8_t *send, uint8_t *recv, uint32_t num);

#endif
