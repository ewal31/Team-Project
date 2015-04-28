#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>

#include "data_processing.h"
#include "gui.h"
#include "sd_card.h"
#include "util.h"
#include "mpr121.h"

#define BUTTON_SAMPLE_DELAY 20
#define DOUBLE_TAP_WAIT_SAMPLES 3

//#define BUTTON1_GPIO_PORT GPIOB 
//#define BUTTON1_PIN GPIO_Pin_5
//#define BUTTON2_GPIO_PORT GPIOB
//#define BUTTON2_PIN GPIO_Pin_7

#define BUTTON1_GPIO_PORT GPIOB 
#define BUTTON1_PIN GPIO_Pin_1
#define BUTTON2_GPIO_PORT GPIOA
#define BUTTON2_PIN GPIO_Pin_4

enum press_type {
    BUTTON1,
    BUTTON2,
    DOUBLE_TAP,
    NO_PRESS,
};

enum entry_type {
    TIME_ENTRY = 0,
    DATE_ENTRY,
    DIAM_ENTRY,
    WEIGHT_ENTRY,
    LITE_ENTRY,
    NO_ENTRY
};

static int lastButton1;
static int lastButton2;
static int button1Waiting;
static int button2Waiting;
static int holdTime;

/* GUI control data. */
static int currentSelection = 1;
static enum display_mode currentMode = TIME;
static enum entry_type currentEntry = NO_ENTRY;
static int settingSelection = 0;
static int keypadSelection = 0;
static int entryPos = 0;
static char textEntry[20] = { 0 };
static int radioStatus;
static int sdStatus;
static TickType_t notificationStart;

static SemaphoreHandle_t modeLock;

enum press_type check_button_press(void);
void process_button_press(enum press_type press);
void entry_process_select(void);
int set_date(void);
int set_time(void);
int set_weight(void);
int set_diam(void);
void entry_mode_time(void);
void entry_mode_date(void);
void entry_mode_diam(void);
void entry_mode_weight(void);
void entry_process_scroll(int up);
static int start_journey(void);
static void end_journey(void);

void controller_task(void *param)
{
    enum press_type press;
    TickType_t lastButtonCheck = 0, currentTime = 0;

    gui_switch_modes(currentMode);

    while (1) {
        currentTime = xTaskGetTickCount();

        if (currentTime - lastButtonCheck > BUTTON_SAMPLE_DELAY) {
            press = check_button_press();
            radioStatus = get_radio_status();
            sdStatus = sd_get_status();
            process_button_press(press);
        }

        vTaskDelay(10);
    }
}

void process_button_press(enum press_type press)
{
    TickType_t time;

    /* We need to hold the lock before we can mess about with the mode. */
    xSemaphoreTake(modeLock, portMAX_DELAY);

    time = xTaskGetTickCount();

    if (currentMode == NOTIFICATION && (time > notificationStart + 5000)) {
        currentMode = TIME;
    }

    switch (press) {
        case BUTTON2: // Left
            switch (currentMode) {
                case TEXT_ENTRY:
                case SETTINGS:
                    entry_process_scroll(1);
                    break;
                case NOTIFICATION:
                    currentMode = TIME;
                    gui_switch_modes(currentMode);
                    break;
                default:
                    currentSelection = (currentSelection + 1 < 4) ? (currentSelection + 1) : 1;
                    break;
            }
            break;
        case BUTTON1: // Right
            switch (currentMode) {
                case TEXT_ENTRY:
                case SETTINGS:
                    entry_process_scroll(0);
                    break;
                case NOTIFICATION:
                    currentMode = TIME;
                    gui_switch_modes(currentMode);
                    break;
                default:
                    currentSelection = (currentSelection - 1) ? (currentSelection - 1) : 3;
                    break;
            }
            break;
        case DOUBLE_TAP: 
            switch (currentMode) {
                case TIME:
                    if (currentSelection == 3) {
                        if (start_journey()) {
                            currentMode = DIGITAL;
                            gui_switch_modes(currentMode);
                        }
                    } else if (currentSelection == 2) {
                        //entry_mode_diam();
                        //currentMode = TEXT_ENTRY;
                        //gui_switch_modes(currentMode);
                    } else if (currentSelection == 1) {
                        //entry_mode_time();
                        //currentMode = TEXT_ENTRY;
                        entryPos = -1;
                        settingSelection = 0;
                        currentMode = SETTINGS;
                        currentEntry = NO_ENTRY;
                        gui_switch_modes(currentMode);
                    }
                    break;
                case DIGITAL:
                    if (currentSelection == 3) {
                        end_journey();
                        currentMode = TIME;
                        gui_switch_modes(currentMode);
                    } else if (currentSelection == 2) {
                        currentMode = ANALOG;
                        gui_switch_modes(currentMode);
                    }
                    break;
                case ANALOG:
                    if (currentSelection == 3) {
                        end_journey();
                        currentMode = TIME;
                        gui_switch_modes(currentMode);
                    } else if (currentSelection == 1) {
                        currentMode = DIGITAL;
                        gui_switch_modes(currentMode);
                    }
                    break;
                case TEXT_ENTRY:
                case SETTINGS:
                    entry_process_select();
                    gui_switch_modes(currentMode);
                    break;
                case NOTIFICATION:
                    currentMode = TIME;
                    gui_switch_modes(currentMode);
                    break;
            }
        case NO_PRESS:
            break;
    }

    gui_set_selected_button(currentSelection);
//    gui_set_keypad_selection(keypadSelection);
    gui_set_keypad_selection(settingSelection);
    gui_set_header_number((int)currentEntry);
    gui_set_selected_pos(entryPos);

    xSemaphoreGive(modeLock);
}

void end_journey_request(int source)
{
    xSemaphoreTake(modeLock, portMAX_DELAY);

    if (currentMode == DIGITAL || currentMode == ANALOG) {
        if (source == 1) {
            data_processor_end_recording();
            gui_set_notification_type(1);
        } else {
            sd_stop_recording();
            gui_set_notification_type(2);
        }
//        currentMode = TIME;
        currentMode = NOTIFICATION;
        notificationStart = xTaskGetTickCount();
        gui_switch_modes(currentMode);
    }

    xSemaphoreGive(modeLock);
}

static int start_journey(void)
{
    //if (sdStatus && radioStatus) {
        //sd_start_recording();        
        //data_processor_start_recording();
        return 1;
    //}

    //return 0;
}

static void end_journey(void)
{
    sd_stop_recording();
    data_processor_end_recording();
    notificationStart = xTaskGetTickCount();
    currentMode = NOTIFICATION;
    gui_set_notification_type(0);
    gui_switch_modes(currentMode);
}

void entry_process_scroll(int up)
{
    if ((currentMode == SETTINGS) && (currentEntry == NO_ENTRY)) {
        if (up) {
            settingSelection = (settingSelection + 1) % 6;
        } else {
            settingSelection = ((settingSelection - 1 >= 0) ? settingSelection - 1 : 5);
        }
    } else {
        if (up) {
            keypadSelection = (keypadSelection + 1) % 10;
        } else {
            keypadSelection = ((keypadSelection - 1 >= 0) ? keypadSelection - 1 : 9);
        }

        textEntry[entryPos] = keypadSelection + '0'; 
    }
}

void entry_process_select(void)
{
    switch(currentEntry) {
        case TIME_ENTRY:
            if (keypadSelection == 10) {
                /* User is done. */
                if (set_time()) {
                    currentEntry = NO_ENTRY;
                } else {
                    entry_mode_time();
                }
                break;
            } else {
                textEntry[entryPos] = keypadSelection + '0';
            }
            ++entryPos;
            if (entryPos == 8) {
                /* User is done. */
                if (set_time()) {
                    currentEntry = NO_ENTRY;
                } else {
                    entry_mode_time();
                }
                break;
            } else if (entryPos == 2 || entryPos == 5) {
                ++entryPos;
            }
            break;
        case DATE_ENTRY:
            if (keypadSelection == 10) {
                /* User is done. */
                if (set_date()) {
                    currentEntry = NO_ENTRY;
                } else {
                    entry_mode_date();
                }
                break;
            } else {
                textEntry[entryPos] = keypadSelection + '0';
            }
            ++entryPos;
            if (entryPos == 8) {
                /* User is done. */
                if (set_date()) {
                    currentEntry = NO_ENTRY;
                } else {
                    entry_mode_date();
                }
                break;
            } else if (entryPos == 2 || entryPos == 5) {
                ++entryPos;
            }
            break;
        case DIAM_ENTRY:
            if (keypadSelection == 10) {
                /* User is done. */
                if (set_diam()) {
                    currentEntry = NO_ENTRY;
                } else {
                    entry_mode_diam();
                }
                break;
            } else {
                textEntry[entryPos] = keypadSelection + '0';
            }
            ++entryPos;
            if (entryPos == 3) {
                /* User is done. */
                if (set_diam()) {
                    currentEntry = NO_ENTRY;
                } else {
                    entry_mode_diam();
                }
                break;
            }
            break;
        case WEIGHT_ENTRY:
            if (keypadSelection == 10) {
                /* User is done. */
                if (set_weight()) {
                    currentEntry = NO_ENTRY;
                } else {
                    entry_mode_weight();
                }
                break;
            } else {
                textEntry[entryPos] = keypadSelection + '0';
            }
            ++entryPos;
            if (entryPos == 4) {
                /* User is done. */
                if (set_weight()) {
                    currentEntry = NO_ENTRY;
                } else {
                    entry_mode_weight();
                }
                break;
            }
            break;
        case LITE_ENTRY:
            break;
        case NO_ENTRY:
            switch (settingSelection) {
                case 0:
                    currentMode = TIME;
                    break;
                case 1:
                   entry_mode_time();
                   break;
                case 2:
                   entry_mode_date();
                   break;
                case 3:
                   entry_mode_diam();
                   break;
                case 4:
                   entry_mode_weight();
                   break;
                case 5:
                   break;
                default:
                   break;
            }
            break;
    }
    keypadSelection = textEntry[entryPos] - '0';
    if (currentEntry == NO_ENTRY) {
        entryPos = -1;
    }
}

//void entry_process_select(void)
//{
//    switch(currentEntry) {
//        case TIME_ENTRY:
//            if (keypadSelection == 10) {
//                /* User is done. */
//                if (set_time()) {
//                    entry_mode_date();
//                } else {
//                    entry_mode_time();
//                }
//                break;
//            } else {
//                textEntry[entryPos] = keypadSelection + '0';
//            }
//            ++entryPos;
//            if (entryPos == 8) {
//                /* User is done. */
//                if (set_time()) {
//                    entry_mode_date();
//                } else {
//                    entry_mode_time();
//                }
//                break;
//            } else if (entryPos == 2 || entryPos == 5) {
//                ++entryPos;
//            }
//            break;
//        case DATE_ENTRY:
//            if (keypadSelection == 10) {
//                /* User is done. */
//                if (set_date()) {
//                    currentMode = TIME;
//                } else {
//                    entry_mode_date();
//                }
//                break;
//            } else {
//                textEntry[entryPos] = keypadSelection + '0';
//            }
//            ++entryPos;
//            if (entryPos == 8) {
//                /* User is done. */
//                if (set_date()) {
//                    currentMode = TIME;
//                } else {
//                    entry_mode_date();
//                }
//                break;
//            } else if (entryPos == 2 || entryPos == 5) {
//                ++entryPos;
//            }
//            break;
//        case DIAM_ENTRY:
//            if (keypadSelection == 10) {
//                /* User is done. */
//                if (set_diam()) {
//                    entry_mode_weight();
//                } else {
//                    entry_mode_diam();
//                }
//                break;
//            } else {
//                textEntry[entryPos] = keypadSelection + '0';
//            }
//            ++entryPos;
//            if (entryPos == 3) {
//                /* User is done. */
//                if (set_diam()) {
//                    entry_mode_weight();
//                } else {
//                    entry_mode_diam();
//                }
//                break;
//            }
//            break;
//        case WEIGHT_ENTRY:
//            if (keypadSelection == 10) {
//                /* User is done. */
//                if (set_weight()) {
//                    currentMode = TIME;
//                } else {
//                    entry_mode_weight();
//                }
//                break;
//            } else {
//                textEntry[entryPos] = keypadSelection + '0';
//            }
//            ++entryPos;
//            if (entryPos == 4) {
//                /* User is done. */
//                if (set_weight()) {
//                    currentMode = TIME;
//                } else {
//                    entry_mode_weight();
//                }
//                break;
//            }
//            break;
//        case LITE_ENTRY:
//        case NO_ENTRY:
//            break;
//    }
//    keypadSelection = textEntry[entryPos] - '0';
//}

/* Get the current string being entered. */
char *get_current_entry(void)
{
    return textEntry;
}

/* Try and set the weight. If no able to return 0. */
int set_weight(void)
{
    int weight;

    weight = (textEntry[0] - '0') * 1000 + (textEntry[1] - '0') * 100 + 
           (textEntry[2] - '0') * 10 + (textEntry[3] - '0');

    set_current_weight(weight);

    return 1;
}

/* Try and set the diameter. If no able to return 0. */
int set_diam(void)
{
    int diam;

    diam = (textEntry[0] - '0') * 100 + (textEntry[1] - '0') * 10 + 
           (textEntry[2] - '0');

    if (diam < 500 || diam > 2000) {
        return 0;
    }

    set_current_wheel_size(diam);

    return 1;
}

/* Try and set the date. If no able to return 0. */
int set_date(void)
{
    RTC_DateTypeDef dateInit;

    dateInit.RTC_WeekDay = RTC_Weekday_Monday;
    dateInit.RTC_Date = (textEntry[0] - '0') * 10 + (textEntry[1] - '0');
    dateInit.RTC_Month = ((textEntry[3] - '0') << 4) | (textEntry[4] - '0');
    dateInit.RTC_Year = (textEntry[6] - '0') * 10 + (textEntry[7] - '0');

    if (dateInit.RTC_Date > 31 ||  dateInit.RTC_Month > 0x12 || dateInit.RTC_Year > 99) {
        return 0;
    }

    RTC_SetDate(RTC_Format_BIN, &dateInit);

    return 1;
}

/* Try and set the time. If not able to return 0. */
int set_time(void)
{
    RTC_TimeTypeDef timeInit;

    timeInit.RTC_Hours = (textEntry[0] - '0') * 10 + (textEntry[1] - '0');
    timeInit.RTC_Minutes = (textEntry[3] - '0') * 10 + (textEntry[4] - '0');
    timeInit.RTC_Seconds = (textEntry[6] - '0') * 10 + (textEntry[7] - '0');

    if (timeInit.RTC_Hours > 23 || timeInit.RTC_Minutes > 59 || 
            timeInit.RTC_Seconds > 59) {
        return 0;
    }

    RTC_SetTime(RTC_Format_BIN, &timeInit);

    return 1;
}

/* Go into weight entry mode. */
void entry_mode_weight(void)
{
    int currentWeight = get_current_weight();
    int len, inc = 0;

    if (currentWeight < 1000) {
        textEntry[0] = '0';
        inc = 1;
    }

    len = write_dec_num(currentWeight, 0, textEntry + inc);
    len += inc;
    textEntry[len] = 'k';
    textEntry[len + 1] = 'g';
    textEntry[len + 2] = '\0';

    keypadSelection = textEntry[0] - '0';
    currentEntry = WEIGHT_ENTRY;
    entryPos = 0;
}

/* Go into diameter entry mode. */
void entry_mode_diam(void)
{
    int currentDiam = get_current_wheel_size();
    int len, inc = 0;
    
    if (currentDiam < 100) {
        textEntry[0] = '0';
        inc = 1;
    }

    len = write_dec_num(currentDiam, 0, textEntry + inc);
    len += inc;
    textEntry[len] = 'm';
    textEntry[len + 1] = 'm';
    textEntry[len + 2] = '\0';

    keypadSelection = textEntry[0] - '0';
    currentEntry = DIAM_ENTRY;
    entryPos = 0;
}

/* Go into date entry mode. */
void entry_mode_date(void)
{
    RTC_DateTypeDef currentDate;

    RTC_GetDate(RTC_Format_BCD, &currentDate);

    textEntry[0] = (currentDate.RTC_Date>>4) + '0';
    textEntry[1] = (currentDate.RTC_Date & 0x0F) + '0';
    textEntry[2] = '/';
    textEntry[3] = (currentDate.RTC_Month>>4) + '0';
    textEntry[4] = (currentDate.RTC_Month & 0x0F) + '0';
    textEntry[5] = '/';
    textEntry[6] = (currentDate.RTC_Year>>4) + '0';
    textEntry[7] = (currentDate.RTC_Year & 0x0F) + '0';
    textEntry[9] = '\0';

    keypadSelection = textEntry[0] - '0';
    currentEntry = DATE_ENTRY;
    entryPos = 0;
}

/* Go into time entry mode. */
void entry_mode_time(void)
{
    char tmp[] = "00:00:00";

    strcpy(textEntry, tmp);
    keypadSelection = 0;
    currentEntry = TIME_ENTRY;
    entryPos = 0;
}

enum press_type check_button_press(void)
{
    int button1, button2;
    int cap1 = 0, cap2 = 0;

   // if(mpr121_capStatusChanged()){
   //     uint8_t status = mpr121_getStatus();
   //     cap1 = mpr121_readButton1(status);
   //     cap2 = mpr121_readButton2(status);
   // } else {
   //     cap1 = Bit_RESET;
   //     cap2 = Bit_RESET;
   // }

    button1 = GPIO_ReadInputDataBit(BUTTON1_GPIO_PORT, BUTTON1_PIN);// | cap1;
    button2 = GPIO_ReadInputDataBit(BUTTON2_GPIO_PORT, BUTTON2_PIN);// | cap2;

    if (button1 && !lastButton1) {
        lastButton1 = button1;
    } else {
        lastButton1 = button1;
        button1 = 0;
    }

    if (button2 && !lastButton2) {
        lastButton2 = button2;
    } else {
        lastButton2 = button2;
        button2 = 0;
    }

    if ((button1Waiting && button2) || (button2Waiting && button1)) {
        button1Waiting = 0;
        button2Waiting = 0;
        holdTime = 0;
        return DOUBLE_TAP;
    }

    if (holdTime) {
        --holdTime;
    }
    
    /* holdTime 0 here if we've timed out waiting for a double touch or were
     * never waiting for one. Otherwise do nothing until the next call. 
     */

    if (!holdTime) {
        if (button1Waiting) {
            button1Waiting = 0;
            return BUTTON1;
        } else if (button2Waiting) {
            button2Waiting = 0;
            return BUTTON2;
        } else {
            if (button1 && button2) {
                return DOUBLE_TAP;
            } else if (button1) {
                button1Waiting = 1;
                holdTime = DOUBLE_TAP_WAIT_SAMPLES;
            } else if (button2) {
                button2Waiting = 1;
                holdTime = DOUBLE_TAP_WAIT_SAMPLES;
            } 
            return NO_PRESS;
        }
    }

    return NO_PRESS;
}

void init_button_gpio(void)
{
    GPIO_InitTypeDef GPIOInit;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOA , ENABLE);

    /* Initialise the button input pins. */
    GPIOInit.GPIO_Mode = GPIO_Mode_IN;
    GPIOInit.GPIO_OType = GPIO_OType_PP;
    GPIOInit.GPIO_Speed = GPIO_Fast_Speed;
    GPIOInit.GPIO_PuPd = GPIO_PuPd_DOWN;

    GPIOInit.GPIO_Pin = BUTTON1_PIN;
    GPIO_Init(BUTTON1_GPIO_PORT, &GPIOInit);
    GPIOInit.GPIO_Pin = BUTTON2_PIN;
    GPIO_Init(BUTTON2_GPIO_PORT, &GPIOInit);

    /* Initialise the global mode lock. */
    modeLock = xSemaphoreCreateBinary();
    xSemaphoreGive(modeLock);
}
