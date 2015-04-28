#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>

#include "usart.h"
#include "mpr121.h"

#include "controller.h"
#include "st7735.h"
#include "sd_card.h"
#include "gui.h"
#include "spi.h"
#include "rfm73.h"
#include "data_processing.h"

void blinker(void *params);
void blinker1(void *params);
void init(void);
void RTC_setup(void);

int main() {
    RTC_setup();
    init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    spi1_init();
    spi2_init();
    sd_gpio_configure();
    st7735_configure_gpio();
    init_button_gpio();

//    init_mpr121();
//    init_usart();

    
//    xTaskCreate(mpr121_debug_task, "mpr", 500, NULL, 1, NULL);
//    xTaskCreate(controller_task, "mpr", 500, NULL, 1, NULL);

    xTaskCreate(controller_task, "contr", 500, NULL, 1, NULL);
    xTaskCreate(gui_task, "gui", 500, NULL, 3, NULL);
    xTaskCreate(processing_task, "proc", 500, NULL, 4, NULL);
    xTaskCreate(sd_task, "sd", 500, NULL, 2, NULL);
//
//    while(1);
    vTaskStartScheduler();

    return 1;
}

void blinker(void *params)
{
    while(1) {
        GPIO_SetBits(GPIOD, GPIO_Pin_13);
        vTaskDelay(500);
        GPIO_ResetBits(GPIOD, GPIO_Pin_13);
        vTaskDelay(500);
    }
}

void blinker1(void *params)
{
    while(1) {
        GPIO_SetBits(GPIOD, GPIO_Pin_14);
        vTaskDelay(500);
        GPIO_ResetBits(GPIOD, GPIO_Pin_14);
        vTaskDelay(500);
    }
}

void init(void)
{
    GPIO_InitTypeDef GPIOInit;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE);

    /* Initialise the output GPIO pins. */
    GPIOInit.GPIO_Mode = GPIO_Mode_OUT;
    GPIOInit.GPIO_OType = GPIO_OType_PP;
    GPIOInit.GPIO_Speed = GPIO_Fast_Speed;
    GPIOInit.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIOInit.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_Init(GPIOD, &GPIOInit);
    GPIO_SetBits(GPIOD, GPIO_Pin_13 | GPIO_Pin_14);

    GPIOInit.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOE, &GPIOInit);
    GPIO_SetBits(GPIOE, GPIO_Pin_3);
}

void RTC_setup(void)
{
    RTC_InitTypeDef RTCInit;
    RTC_TimeTypeDef timeInit;
    RTC_DateTypeDef dateInit;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);


    RCC_LSICmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

    /* check if the RTC is already initialised. */
    if (RTC_GetFlagStatus(RTC_FLAG_INITS) == SET) {
        RTC_WaitForSynchro();
        return;
    }

    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    RCC_RTCCLKCmd(ENABLE);

    RTC_WaitForSynchro();

    RTCInit.RTC_HourFormat = RTC_HourFormat_24 ;
    RTCInit.RTC_AsynchPrediv = 127;
    RTCInit.RTC_SynchPrediv = 255;

    RTC_Init(&RTCInit);

    timeInit.RTC_Hours = 0;
    timeInit.RTC_Minutes = 0;
    timeInit.RTC_Seconds = 0;
    RTC_SetTime(RTC_Format_BIN, &timeInit);

    dateInit.RTC_WeekDay = RTC_Weekday_Monday;
    dateInit.RTC_Month = RTC_Month_March;
    dateInit.RTC_Date = 28;
    dateInit.RTC_Year = 99;
    RTC_SetDate(RTC_Format_BIN, &dateInit);
}
