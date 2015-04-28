/* st7735.c
 *
 * Driver for 1.8" TFT using st7735 chip.
 *
 * Written for ENGG4810 Team 30 by:
 * - Mitchell Krome
 * - Edward Wall
 *
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "spi.h"
#include "st7735.h"

#define DISPLAY_SS_PIN GPIO_Pin_4
#define DISPLAY_SS_PORT GPIOC
#define DISPLAY_SS_LOW() GPIO_ResetBits(DISPLAY_SS_PORT, DISPLAY_SS_PIN);
#define DISPLAY_SS_HIGH() GPIO_SetBits(DISPLAY_SS_PORT, DISPLAY_SS_PIN);

#define DISPLAY_DCX_PIN GPIO_Pin_5
#define DISPLAY_DCX_PORT GPIOC
#define DISPLAY_DCX_LOW() GPIO_ResetBits(DISPLAY_DCX_PORT, DISPLAY_DCX_PIN);
#define DISPLAY_DCX_HIGH() GPIO_SetBits(DISPLAY_DCX_PORT, DISPLAY_DCX_PIN);

#define DISPLAY_LIGHT_PIN GPIO_Pin_1
#define DISPLAY_LIGHT_PORT GPIOB
#define DISPLAY_LIGHT_LOW() GPIO_ResetBits(DISPLAY_LIGHT_PORT, DISPLAY_LIGHT_PIN);
#define DISPLAY_LIGHT_HIGH() GPIO_SetBits(DISPLAY_LIGHT_PORT, DISPLAY_LIGHT_PIN);

#define DELAY 0x80

static void send_parameters(uint8_t *params, uint8_t num);
static void send_command(uint8_t command);

/* Initialisation arrays taken from adafruit example code and modified where
 * necessary.
 */
static uint8_t Rcmd1[] = {                 // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
    150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
    255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
    0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
    0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
    0x01, 0x2C, 0x2D,       //     Dot inversion mode
    0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
    0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
    0xA2,
    0x02,                   //     -4.6V
    0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
    0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
    0x0A,                   //     Opamp current small
    0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
    0x8A,                   //     BCLK/2, Opamp current small & Medium low
    0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
    0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
    0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
    0x60,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
    0x05 
};                 //     16-bit color

static uint8_t Rcmd2red[] = {              // Init for 7735R, part 2 (red tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
    0x00, 0x00,             //     XSTART = 0
    0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
    0x00, 0x00,             //     XSTART = 0
    0x00, 0x9F 
};           //     XEND = 159

static uint8_t Rcmd3[] = {                 // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
    0x02, 0x1c, 0x07, 0x12,
    0x37, 0x32, 0x29, 0x2d,
    0x29, 0x25, 0x2B, 0x39,
    0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
    0x03, 0x1d, 0x07, 0x06,
    0x2E, 0x2C, 0x29, 0x2D,
    0x2E, 0x2E, 0x37, 0x3F,
    0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
    10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
    100 
};                  //     100 ms delay

/* Clear the display back to all black. */
void clear_screen(void)
{
    uint8_t xParam[] = {0x00, 0x00, 0x00, 160};
    uint8_t yParam[] = {0x00, 0x0, 0x00, 128};

    acquire_spi1();

    send_command(ST7735_CASET); // Column addr set
    send_parameters(xParam, 4);
    send_command(ST7735_RASET); // Column addr set
    send_parameters(yParam, 4);
    send_command(ST7735_RAMWR);

    DISPLAY_SS_LOW();
    DISPLAY_DCX_HIGH();
    spi1_send_byte_dma(0x00, 160 * 128 * 2);
    DISPLAY_SS_HIGH();

    release_spi1();

}

/* Draw a buffer of length len to the screen starting at (xs, ys) and ending at
 * (xe, ye)
 */
void draw_buffer(uint16_t *buff, uint16_t len, uint8_t xs, uint8_t xe, uint8_t ys,
        uint8_t ye)
{
    uint8_t xParam[] = {0x00, xs, 0x00, xe};
    uint8_t yParam[] = {0x00, ys, 0x00, ye};

    acquire_spi1();

    send_command(ST7735_CASET); // Column addr set
    send_parameters(xParam, 4);
    send_command(ST7735_RASET); // Column addr set
    send_parameters(yParam, 4);
    send_command(ST7735_RAMWR);

    DISPLAY_SS_LOW();
    DISPLAY_DCX_HIGH();

    spi1_send_data_dma((uint8_t *)buff, len * 2);

    DISPLAY_SS_HIGH();

    release_spi1();
}

/* Write the initialisation commands in a command array to the screen. */
static void write_commands(uint8_t *cmds)
{      
    uint8_t num, args;
    uint16_t del;
    
    acquire_spi1();

    num = *cmds++;
    while (num--) {
        send_command(*cmds++);
        args = *cmds++;
        del = args & DELAY;
        args &= ~DELAY;
        if (args)
            send_parameters(cmds, args);
        cmds += args;
        if (del) {
            del = *cmds++;
            if (del == 255)
                del = 500;
            vTaskDelay(del);
        }
    }

    release_spi1();
}

/* Send some parameters to the screen. */
static void send_parameters(uint8_t *params, uint8_t num)
{
    DISPLAY_SS_LOW();
    DISPLAY_DCX_HIGH();

    spi1_send_data(params, NULL, num);

    DISPLAY_SS_HIGH();
}

/* Send a command to the screen. */
static void send_command(uint8_t command)
{
    DISPLAY_SS_LOW();
    DISPLAY_DCX_LOW();

    spi1_send_byte(command);

    DISPLAY_SS_HIGH();
}

/* Initialise the st7735 */
void st7735_init(void)
{
    write_commands(Rcmd1);
    write_commands(Rcmd2red);
    write_commands(Rcmd3);
}

/* Configure the GPIO for the st7735 */
void st7735_configure_gpio(void)
{
    GPIO_InitTypeDef GPIOInit;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    /* Initialise the output GPIO pins. */
    GPIOInit.GPIO_Mode = GPIO_Mode_OUT;
    GPIOInit.GPIO_OType = GPIO_OType_PP;
    GPIOInit.GPIO_Speed = GPIO_Fast_Speed;
    GPIOInit.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIOInit.GPIO_Pin = DISPLAY_SS_PIN;
    GPIO_Init(DISPLAY_SS_PORT, &GPIOInit);
    GPIOInit.GPIO_Pin = DISPLAY_DCX_PIN;
    GPIO_Init(DISPLAY_DCX_PORT, &GPIOInit);
//    GPIOInit.GPIO_Pin = DISPLAY_LIGHT_PIN;
//    GPIO_Init(DISPLAY_LIGHT_PORT, &GPIOInit);

    DISPLAY_SS_HIGH();
    DISPLAY_DCX_HIGH();
//    DISPLAY_LIGHT_HIGH();
}
