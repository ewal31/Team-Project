#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "api_mdriver.h"
#include "fat_sl.h"

#include "controller.h"
#include "sd_card.h"
#include "spi.h"
#include "util.h"

#define NUM_SECTORS 4194304UL

#define MAX_BUSY_TIME 500U
#define MIN_SPACE_NEEDED 100000UL

#define SD_CARD_SS_PIN GPIO_Pin_0
#define SD_CARD_SS_PORT GPIOB
#define SD_CARD_SS_LOW() GPIO_ResetBits(SD_CARD_SS_PORT, SD_CARD_SS_PIN);
#define SD_CARD_SS_HIGH() GPIO_SetBits(SD_CARD_SS_PORT, SD_CARD_SS_PIN);

static F_DRIVER driver;

static int sdPresent = 0;
static int sdRemoved = 0;
static int statusChecked = 0;

static int internalSdPresent = 0;

static int recording = 0;
static int needInit = 1;
static int stopRecording = 0;
static F_FILE *currentFile;
static int currentTrip = 0;

QueueHandle_t dataQueue;

static int sd_wait_for_busy(void);
static int write_data_to_card(struct car_data *data);
static F_FILE *init_new_journey(void);
static uint8_t sd_init(void);
static uint8_t sd_write_block(uint8_t *buff, uint32_t blockNum);
static uint8_t sd_read_block(uint8_t *buff, uint32_t blockNum);
static uint8_t sd_send_command(uint8_t command, uint32_t data, uint8_t crc, 
        uint32_t *resp, uint8_t resp_bytes, uint8_t skip);
int sd_get_phy(F_DRIVER *drv, F_PHY *phy);
long sd_status(F_DRIVER *drv);
int sd_read_sector(F_DRIVER *drv, void *buff, unsigned long sector);
int sd_write_sector(F_DRIVER *drv, void *buff, unsigned long sector);
F_DRIVER *driver_init(unsigned long param);

int sd_get_status(void)
{
    return sdPresent;
}

void sd_task(void *params)
{
    struct car_data currentData;
    int writeCalls = 0;

    dataQueue = xQueueCreate(10, sizeof(struct car_data));

    vTaskDelay(1000);

    while(1) {
        if (!recording) {
            vTaskDelay(500);
            sdPresent = sd_init();
            internalSdPresent= sdPresent;
        } else {
            /* Check all these in a specific order because I'm too lazy to add
             * mutexes for the functions that call in from other tasks. This
             * order should ensure weirdness like fast calls to stop() followed
             * by start() still end up making a new file and recording.
             */

            if (stopRecording) {
                recording = 0;
                stopRecording = 0;
                f_flush(currentFile);
                f_close(currentFile);
                f_delvolume();
            }

            if (needInit) {
                currentFile = init_new_journey();
                xQueueReset(dataQueue);
                writeCalls = 0;
            }
            
            /* We need to re-check this condition because a stop() could
             * cause the first check not to be true any more.
             */
            if (recording) {
                /* Check for samples. */
                if (xQueueReceive(dataQueue, &currentData, 20) == pdTRUE) {
                    writeCalls = (writeCalls + 1) % 10;
                    if (write_data_to_card(&currentData) || ((writeCalls == 9) && f_flush(currentFile))) {
                        /* Write failed. Cancel trip. */
                        end_journey_request(1);
                        recording = 0;
                        stopRecording = 0;
                        sdPresent = 0;
                        f_close(currentFile);
                        f_delvolume();
                    }
                }
            }
        }
    }
}

static int write_data_to_card(struct car_data *data)
{
    char tmp[20];
    int len;

    len = write_dec_num(data->speed, 0, tmp);
    if (f_write(tmp, 1, len, currentFile) != len) {
        return 1;
    }

    if (f_putc(',', currentFile) != ',') {
        return 1;
    }

    len = write_dec_num(data->distance, 2, tmp);
    if (f_write(tmp, 1, len, currentFile) != len) {
        return 1;
    }

    if (f_putc(',', currentFile) != ',') {
        return 1;
    }

    len = write_dec_num(data->accel, 0, tmp);
    if (f_write(tmp, 1, len, currentFile) != len) {
        return 1;
    }

    if (f_putc(',', currentFile) != ',') {
        return 1;
    }

    len = write_dec_num(data->power, 0, tmp);
    if (f_write(tmp, 1, len, currentFile) != len) {
        return 1;
    }

    if (f_putc('\n', currentFile) != '\n') {
        return 1;
    }
    
    return 0;
}

void sd_stop_recording(void)
{
    if (recording) {
        stopRecording = 1;
    }
}

int sd_start_recording(void)
{
    if (sdPresent) {
        recording = 1;
        needInit = 1;
        return 0;
    }

    return 1;
}

static F_FILE *init_new_journey(void)
{
    F_FILE *f;
    char timeStamp[20];  
    char fileName[15];
    int i;

    RTC_DateTypeDef currentDate;
    RTC_TimeTypeDef currentTime;
    
    needInit = 0;
    recording = 1; 
    
    RTC_GetTime(RTC_Format_BCD, &currentTime);
    RTC_GetDate(RTC_Format_BCD, &currentDate);

    if (f_initvolume(driver_init) != F_NO_ERROR) {
        goto out_bad_sd;
    }

    strcpy(fileName, "TRIP0001.trp");

    timeStamp[0] = '2';
    timeStamp[1] = '0';
    timeStamp[2] = (currentDate.RTC_Year>>4) + '0';
    timeStamp[3] = (currentDate.RTC_Year & 0x0F) + '0';
    timeStamp[4] = (currentDate.RTC_Month>>4) + '0';
    timeStamp[5] = (currentDate.RTC_Month & 0x0F) + '0';
    timeStamp[6] = (currentDate.RTC_Date>>4) + '0';
    timeStamp[7] = (currentDate.RTC_Date & 0x0F) + '0';
    timeStamp[8] = 'T';
    timeStamp[9] = (currentTime.RTC_Hours>>4) + '0';
    timeStamp[10] = (currentTime.RTC_Hours & 0x0F) + '0';
    timeStamp[11] = (currentTime.RTC_Minutes>>4) + '0';
    timeStamp[12] = (currentTime.RTC_Minutes & 0x0F) + '0';
    timeStamp[13] = (currentTime.RTC_Seconds>>4) + '0';
    timeStamp[14] = (currentTime.RTC_Seconds & 0x0F) + '0';
    timeStamp[15] = '\n';
    timeStamp[16] = '\0';

    for (i = currentTrip + 1; i < 10000; ++i) {
        fileName[4] = (i / 1000) + '0';
        fileName[5] = ((i / 100) % 10) + '0';
        fileName[6] = ((i / 10) % 10) + '0';
        fileName[7] = (i % 10) + '0';
        f = f_open(fileName, "r");
        if (f) {
            f_close(f);
        } else {
            break;
        }
    }

    if (i == 10000) {
        goto out_bad_sd;
    } else {
        currentTrip = i;
    }

    f = f_open(fileName, "w");

    if (!f) {
        goto out_bad_sd;
    }

    if (f_write(timeStamp, 1, strlen(timeStamp), f) <= 0) {
        f_close(f);
        goto out_bad_sd;
    }

    f_flush(f);

    return f;

out_bad_sd:
    /* Notify everybody to stop recording. */
    recording = 0;
    sdPresent = 0;
    f_delvolume();
    end_journey_request(1);
    return NULL;
}

long sd_status(F_DRIVER *drv)
{
    statusChecked = 1;

    if (!internalSdPresent) {
        /* There is no sd card inserted. */
        return F_ST_MISSING;
    } else if (sdPresent && sdRemoved) {
        /* An sd card was removed between now and the last check. */
        sdRemoved = 0;
        return F_ST_CHANGED;
    }

    /* All good. */
    return 0;
}

F_DRIVER *driver_init(unsigned long param)
{
    driver.getstatus = sd_status;
    driver.getphy = sd_get_phy;
    driver.release = NULL;
    driver.readsector = sd_read_sector;
    driver.writesector = sd_write_sector;
    return &driver;
}

int sd_get_phy(F_DRIVER *drv, F_PHY *phy)
{
    phy->number_of_sectors = NUM_SECTORS;
    phy->bytes_per_sector = 512;
    phy->media_descriptor = F_MEDIADESC_REMOVABLE;

    return 0;
}

int sd_read_sector(F_DRIVER *drv, void *buff, unsigned long sector)
{
    int ret;

    ret = sd_read_block((uint8_t *)buff, sector * 512);

    if (ret) {
        internalSdPresent = 0;
    }

    return ret;
}

int sd_write_sector(F_DRIVER *drv, void *buff, unsigned long sector)
{
    int ret;

    ret = sd_write_block((uint8_t *)buff, sector * 512);

    if (ret) {
        internalSdPresent = 0;
    }

    return ret;
}

/* Returns 1 if a valid SD card is detected. */
static uint8_t sd_init(void)
{
    uint8_t response;
    uint32_t cmdData, cmdResponse;
    int tries = 0;

    acquire_spi1();

    spi1_change_prescaler(SPI_BaudRatePrescaler_256);

    /* First thing we need to do is send >74 clocks with SS high. */
    for (int i = 0; i < 10; ++i) {
        spi1_send_byte(0xFF);
    }

    /* Now we must send cmd 0 with SS low. */ 
    do {
        response = sd_send_command(CMD0, 0, 0x95, NULL, 0, 1);
        tries++;
    } while (!IDLE_STATE(response) && tries < 100);

    if (tries == 100) {
        goto out_bad_card;
    }


    /* The card should be in SPI mode now. We send CMD8 and check the response
     * to see if the card is there.
     */
    cmdData = 0x000001AA; // Voltage Host Supply = 0x01, Check Patter = 0xA7
    response = sd_send_command(CMD8, cmdData, 0x00, &cmdResponse, 4, 0);
    if (ILLEGAL_COMMAND(response)) {
        /* We don't want to deal with v1.0 cards. */
        goto out_bad_card;
    }

    /* Now the card should be turning on, we send ACMD41 until the card isn't
     * in idle mode any more.
     */
    do {
        /* To send ACMD we first need to send a CMD55 */
        sd_send_command(CMD55, 0, 0, NULL, 0, 0);
        cmdData = 0x40000000;
        response = sd_send_command(ACMD41, cmdData, 0x00, NULL, 0, 0);
        if (ILLEGAL_COMMAND(response)) {
            goto out_bad_card;
        }
    } while (IDLE_STATE(response));

    /* Now we need to get the OCR register data. */
    response = sd_send_command(CMD58, 0, 0, &cmdResponse, 4, 0);
    if (ILLEGAL_COMMAND(response)) {
        goto out_bad_card;
    }
    
    if (!((cmdResponse >> 30) & 0x01)) {
        /* Low capacity card. Too much effort. */
        goto out_bad_card;
    }

    /* Now the card SHOULD be in data mode. */
    spi1_change_prescaler(SPI_BaudRatePrescaler_8);
    release_spi1();
    return 1;

out_bad_card:
    spi1_change_prescaler(SPI_BaudRatePrescaler_8);
    release_spi1();
    return 0;
}

static uint8_t sd_write_block(uint8_t *buff, uint32_t blockNum)
{
    uint8_t cmd[6] = { 0x40 | CMD24, blockNum >> (3 * 8), 
                     (blockNum >> (2 * 8)) & 0xFF, (blockNum >> (1 * 8)) & 0xFF, 
                      blockNum & 0xFF, 0x01 };
    uint8_t response;
    int tries = 0;
    
    if (sd_wait_for_busy()) {
        return 1; // Failure
    }

    /* We have SPI bus here and SS is low. */

    for (int i = 0; i < 6; ++i) {
       spi1_send_byte(cmd[i]);
    }

    do {
        response = spi1_send_byte(0xFF);
        ++tries;
    } while (!VALID_RESPONSE(response) && tries <= 8);

    if (tries > 8 || ILLEGAL_COMMAND(response) || ADDRESS_ERROR(response)) {
        SD_CARD_SS_HIGH();
        release_spi1();
        return 1; // Failure
    }

    tries = 0;
    do {
        response = spi1_send_byte(0xFF);
        ++tries;
    } while (BUSY(response) && tries <= 1000);

    if (tries > 1000) {
        SD_CARD_SS_HIGH();
        release_spi1();
        return 1; // Failure
    }

    /* Send block token, data then fake CRC's. */
    spi1_send_byte(0xFE);
    spi1_send_data_dma(buff, 512);
    spi1_send_byte(0xFF);
    spi1_send_byte(0xFF);

    response = spi1_send_byte(0xFF);

    SD_CARD_SS_HIGH();
    release_spi1();
    
    if (!DATA_ACCEPTED(response)) {
        SD_CARD_SS_HIGH();
        release_spi1();
        return 1; // Failure
    }

    return 0;
}

static uint8_t sd_read_block(uint8_t *buff, uint32_t blockNum)
{
    uint8_t cmd[6] = { 0x40 | CMD17, blockNum >> (3 * 8), 
                     (blockNum >> (2 * 8)) & 0xFF, (blockNum >> (1 * 8)) & 0xFF, 
                      blockNum & 0xFF, 0x01 };

    uint8_t response;
    int tries = 0;

    if (sd_wait_for_busy()) {
        return 1; // Failure
    }

    /* We have SPI bus here and SS is low. */
    
    for (int i = 0; i < 6; ++i) {
       spi1_send_byte(cmd[i]);
    }

    do {
        response = spi1_send_byte(0xFF);
        ++tries;
    } while (!VALID_RESPONSE(response) && tries <= 8);

    if (tries > 8 || ILLEGAL_COMMAND(response) || ADDRESS_ERROR(response)) {
        SD_CARD_SS_HIGH();
        release_spi1();
        return 1; // Failure
    }

    tries = 0;
    do {
        response = spi1_send_byte(0xFF);
        ++tries;
    } while (!READ_TOKEN(response) && tries <= 1000);

    if (!READ_TOKEN(response) || tries > 1000) {
        SD_CARD_SS_HIGH();
        release_spi1();
        return 1; // Failure
    }

    /* Send the Data. */
    spi1_recv_data_dma(buff, 512);
    spi1_send_byte(0xFF);
    spi1_send_byte(0xFF);

    SD_CARD_SS_HIGH();
    release_spi1();

    return 0;
}

static uint8_t sd_send_command(uint8_t command, uint32_t data, uint8_t crc, 
        uint32_t *resp, uint8_t resp_bytes, uint8_t skip)
{
    uint8_t cmd[6] = { 0x40 | command, data >> (3 * 8), 
                     (data >> (2 * 8)) & 0xFF, (data >> (1 * 8)) & 0xFF, 
                      data & 0xFF, crc | 0x01 };
    uint8_t response = 0;;
    uint32_t tmp;
    int tries = 0;

    SD_CARD_SS_LOW();

    if (!skip) {
        do {
            response = spi1_send_byte(0xFF);
            ++tries;
        } while (BUSY(response) && tries < 1000);

        if (tries > 1000) {
            return 0xFF;
        }
    }

    for (int i = 0; i < 6; ++i) {
       spi1_send_byte(cmd[i]);
    }

    do {
        response = 0;
        response = spi1_send_byte(0xFF);
        ++tries;
    } while (!VALID_RESPONSE(response) && tries <= 8);

    if (tries > 8 || ILLEGAL_COMMAND(response)) {
        SD_CARD_SS_HIGH();
        return response;
    }

    /* Fill in response from the higher bytes first. */
    for (int i = 0; i < resp_bytes; ++i) {
        tmp = spi1_send_byte(0xFF);
        *resp |= (tmp << ((3 - i) * 8));
    }

    SD_CARD_SS_HIGH();

    return response;
}

static int sd_wait_for_busy(void)
{
    uint8_t response;
    TickType_t startTime = xTaskGetTickCount(), currentTime;

    while (1) {
        currentTime = xTaskGetTickCount();
        if (currentTime - startTime > MAX_BUSY_TIME) {
            return 1;
        }

        acquire_spi1();
        SD_CARD_SS_LOW();

        response = spi1_send_byte(0xFF);

        if (!BUSY(response)) {
            return 0;
        }

        SD_CARD_SS_HIGH();
        release_spi1();
        vTaskDelay(5);
    }
}

/* Configre the sd cards needed gpio. */
void sd_gpio_configure()
{
    GPIO_InitTypeDef GPIOInit;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    /* Initialise the output GPIO pins. */
    GPIOInit.GPIO_Mode = GPIO_Mode_OUT;
    GPIOInit.GPIO_OType = GPIO_OType_PP;
    GPIOInit.GPIO_Speed = GPIO_Fast_Speed;
    GPIOInit.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIOInit.GPIO_Pin = SD_CARD_SS_PIN;
    GPIO_Init(SD_CARD_SS_PORT, &GPIOInit);

    SD_CARD_SS_HIGH();
}
