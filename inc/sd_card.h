#ifndef SD_CARD_H_
#define SD_CARD_H_

struct car_data {
    int speed;
    int distance; // 2 Decimals.
    int accel;
    int power;
};

void sd_gpio_configure();
void sd_task(void *params);
int sd_get_status(void);
int sd_start_recording(void);
void sd_stop_recording(void);

/* Commands for SD card. */
#define CMD0 0U
#define CMD8 8U
#define CMD9 9U
#define CMD10 10U
#define CMD13 13U
#define CMD17 17U
#define CMD24 24U
#define CMD25 25U
#define CMD32 32U
#define CMD33 33U
#define CMD38 38U
#define CMD55 55U
#define CMD58 58U
#define ACMD23 23U
#define ACMD41 41U

/* R1 Reply macros */
#define BUSY(x) (x != 0xFF)
#define IDLE_STATE(x) (x & 0x01)
#define ERASE_RESTART(x) ((x >> 1) & 0x01)
#define ILLEGAL_COMMAND(x) ((x >> 2) & 0x01)
#define CRC_ERROR(x) ((x >> 3) & 0x01)
#define ERASE_ERROR(x) ((x >> 4) & 0x01)
#define ADDRESS_ERROR(x) ((x >> 5) & 0x01)
#define PARAM_ERROR(x) ((x >> 6) & 0x01)
#define VALID_RESPONSE(x) (!((x>>7) & 0x01))

/* Data transfer tokens. */
#define READ_TOKEN(x) (x == 0xFE)
#define DATA_ACCEPTED(x) ((x & 0x1F) == 0x05)

#endif
