/* gui.c
 *
 * gui functionality.
 *
 * Written for ENGG4810 Team 30 by:
 * - Mitchell Krome :0
 * - Edward Wall
 *
 * It's a scary place in here.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "controller.h"
#include "data_processing.h"
#include "gui.h"
#include "renderer.h"
#include "rfm73.h"
#include "sd_card.h"
#include "st7735.h"
#include "util.h"

static void update_gui_elements(struct gui_element *elements, int numElements);

static int button1_text_callback(void *elem);
static int button2_text_callback(void *elem);
static int button3_text_callback(void *elem); 
static int button1_rect_callback(void *elem);
static int button2_rect_callback(void *elem);
static int button3_rect_callback(void *elem);
static int speed_text_callback(void *elem);
static int dist_text_callback(void *elem);
static int accel_text_callback(void *elem);
static int power_text_callback(void *elem);
static int time_text_callback(void *elem);
static int date_text_callback(void *elem);
static int radio_symbol_callback(void *elem);
static int sd_symbol_callback(void *elem);
static int entry_text_callback(void *elem);
static int entry_underline_callback(void *elem);
static int selected_text_callback(void *elem);
static int next_text_callback(void *elem);
static int prev_text_callback(void *elem);
static int slider_callback(void *elem);
static int entry_header_text_callback(void *elem);
static int dial_callback(void *elem);
static int pbar_callback(void *elem);
static int settingsButton1_rect_callback(void *elem); 
static int settingsButton2_rect_callback(void *elem);
static int settingsButton3_rect_callback(void *elem);
static int settingsButton4_rect_callback(void *elem);
static int settingsButton5_rect_callback(void *elem);
static int setting1_text_callback(void *elem); 
static int setting2_text_callback(void *elem);
static int setting3_text_callback(void *elem);
static int setting4_text_callback(void *elem);
static int setting1_underline_callback(void *elem); 
static int setting2_underline_callback(void *elem);
static int setting3_underline_callback(void *elem);
static int setting4_underline_callback(void *elem);
static int notification_rect_callback(void *elem);
static int notification_message_text_callback(void *elem);
static int pbar_callback(void *elem);
static int bar_callback(void *elem);

extern const uint16_t symbols[];

/* Storage buffers for the text. */ 
static char smallTime[20] = { 0 };
static char but1[15] = { 0 };
static char but2[15] = { 0 };
static char but3[15] = { 0 };
static char time[20] = { 0 };
static char date[20] = { 0 };
static char speed[15] = { 0 };
static char dist[15] = { 0 };
static char accel[15] = { 0 };
static char power[15] = { 0 };
static char entry[15] = { 0 };
static char *headers[] = { "Enter Time", "Enter Date", "Enter Diam", "Enter Weight" };
static char setbut1[] = "Time"; 
static char setbut2[] = "Date";
static char setbut3[] = "Diam";
static char setbut4[] = "Mass";
static char setbut5[] = "Lite";
static char set1text[15] = { 0 }; 
static char set2text[15] = { 0 };
static char set3text[15] = { 0 };
static char set4text[15] = { 0 };
static char set5text[15] = { 0 };
static char notification[] = "Trip Ended";
static char *tripMessage[] = { "No Errors", "SD Card Error", "Sensor Signal Lost" };

/* Loads of global initialisations! */

/* Status bar items. */
static struct rect statusBar = { .w = 160, .h = 14, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct symb sdStatus = { symbols, DEFAULT_COLOUR };
static struct symb rfStatus = { symbols + 12, DEFAULT_COLOUR };
static struct text statusTime = { .size = 1, .text = smallTime, .colo = DEFAULT_COLOUR};

/* Menu bar items. */
static struct rect button1 = { .w = 53, .h = 18, .t = 1, .b_colo = BORDER_COLOUR, .f_colo = BUTTON_BACKGROUND_COLOUR, .fill = 1 };
static struct text button1Text = { .size = 1, .text = but1, .colo = BUTTON_TEXT_COLOUR };
static struct rect button2 = { .w = 53, .h = 18, .t = 1, .b_colo = BORDER_COLOUR, .f_colo = BUTTON_BACKGROUND_COLOUR, .fill = 1 };
static struct text button2Text = { .size = 1, .text = but2, .colo = BUTTON_TEXT_COLOUR };
static struct rect button3 = { .w = 54, .h = 18, .t = 1, .b_colo = BORDER_COLOUR, .f_colo = BUTTON_BACKGROUND_COLOUR, .fill = 1 };
static struct text button3Text = { .size = 1, .text = but3, .colo = BUTTON_TEXT_COLOUR };

/* Settings menu. */
static struct rect settingButton1 = { .w = 53, .h = 19, .t = 1, .b_colo = BORDER_COLOUR, .f_colo = BUTTON_BACKGROUND_COLOUR, .fill = 1 };
static struct text settingButton1Text = { .size = 1, .text = setbut1, .colo = BUTTON_TEXT_COLOUR };
static struct rect setting1 = { .w = 107, .h = 19, .t = 0, .b_colo = BORDER_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text setting1Text = { .size = 1, .text = set1text, .colo = DEFAULT_COLOUR };
static struct line setting1Underline = { .x1 = 1, .x2 = 2, .y1 = 48, .y2 = 48, .t = 1, .colo = DEFAULT_COLOUR };

static struct rect settingButton2 = { .w = 53, .h = 19, .t = 1, .b_colo = BORDER_COLOUR, .f_colo = BUTTON_BACKGROUND_COLOUR, .fill = 1 };
static struct text settingButton2Text = { .size = 1, .text = setbut2, .colo = BUTTON_TEXT_COLOUR };
static struct rect setting2 = { .w = 107, .h = 19, .t = 0, .b_colo = BORDER_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text setting2Text = { .size = 1, .text = set2text, .colo = DEFAULT_COLOUR };
static struct line setting2Underline = { .x1 = 1, .x2 = 2, .y1 = 67, .y2 = 67, .t = 1, .colo = DEFAULT_COLOUR };

static struct rect settingButton3 = { .w = 53, .h = 19, .t = 1, .b_colo = BORDER_COLOUR, .f_colo = BUTTON_BACKGROUND_COLOUR, .fill = 1 };
static struct text settingButton3Text = { .size = 1, .text = setbut3, .colo = BUTTON_TEXT_COLOUR };
static struct rect setting3 = { .w = 107, .h = 19, .t = 0, .b_colo = BORDER_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text setting3Text = { .size = 1, .text = set3text, .colo = DEFAULT_COLOUR };
static struct line setting3Underline = { .x1 = 1, .x2 = 2, .y1 = 86, .y2 = 86, .t = 1, .colo = DEFAULT_COLOUR };

static struct rect settingButton4 = { .w = 53, .h = 19, .t = 1, .b_colo = BORDER_COLOUR, .f_colo = BUTTON_BACKGROUND_COLOUR, .fill = 1 };
static struct text settingButton4Text = { .size = 1, .text = setbut4, .colo = BUTTON_TEXT_COLOUR };
static struct rect setting4 = { .w = 107, .h = 19, .t = 0, .b_colo = BORDER_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text setting4Text = { .size = 1, .text = set4text, .colo = DEFAULT_COLOUR };
static struct line setting4Underline = { .x1 = 1, .x2 = 2, .y1 = 105, .y2 = 105, .t = 1, .colo = DEFAULT_COLOUR };

static struct rect settingButton5 = { .w = 53, .h = 20, .t = 1, .b_colo = BORDER_COLOUR, .f_colo = BUTTON_BACKGROUND_COLOUR, .fill = 1 };
static struct text settingButton5Text = { .size = 1, .text = setbut5, .colo = BUTTON_TEXT_COLOUR };
static struct rect setting5 = { .w = 107, .h = 20, .t = 0, .b_colo = BORDER_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text setting5Text = { .size = 1, .text = set5text, .colo = DEFAULT_COLOUR };
static struct line setting5Underline = { .x1 = 1, .x2 = 2, .y1 = 124, .y2 = 124, .t = 1, .colo = DEFAULT_COLOUR };

/* Main data area. */
//static struct rect dataArea = { .w = 160, .h = 96, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };

/* Text for time/date screen. */
static struct rect timeArea = { .w = 160, .h = 58, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text timeText = { .size = 3, .text = time, .colo = DEFAULT_COLOUR };

static struct rect dateArea = { .w = 160, .h = 38, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text dateText = { .size = 2, .text = date, .colo = DEFAULT_COLOUR };

/* Text for digital display */
static struct rect speedArea = { .w = 160, .h = 35, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text speedText = { .size = 3, .text = speed, .colo = DEFAULT_COLOUR };

static struct rect distArea = { .w = 160, .h = 30, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text distText = { .size = 2, .text = dist, .colo = DEFAULT_COLOUR };

static struct rect accelPowerArea = { .w = 160, .h = 31, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text accelText = { .size = 2, .text = accel, .colo = DEFAULT_COLOUR };
static struct text powerText = { .size = 2, .text = power, .colo = DEFAULT_COLOUR };

/* For settings menu. */
static struct rect entryHeaderArea = { .w = 160, .h = 33, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text entryHeaderText = { .size = 2, .text = entry, .colo = DEFAULT_COLOUR };

static struct rect entryArea = { .w = 160, .h = 35, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text entryText = { .size = 3, .text = entry, .colo = DEFAULT_COLOUR };
static struct line entryUnderline = { .x1 = 1, .x2 = 2, .y1 = 102, .y2 = 102, .t = 3, .colo = DEFAULT_COLOUR };

/* Padding. */
static struct rect topPaddingArea = { .w = 160, .h = 11, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct rect bottomPaddingArea = { .w = 160, .h = 20, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };

/* Analog display. */
static struct rect analogDistArea = { .w = 110, .h = 10, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text analogDistText = { .size = 1, .text = dist, .colo = DEFAULT_COLOUR };

static struct rect dialArea = { .w = 120, .h = 60, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct circ dial = { .colo = DIAL_COLOUR, .pcolo = POINTER_COLOUR, .speed = 10 };

static struct power_bar powBar = { .power = 20 };
static struct rect powerBarArea = { .w = 120, .h = 25, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct rect accelBarArea  = { .w = 40, .h = 96, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct accel_bar accelBar = { .acceleration = 1 };
static struct rect emptySpaceArea = { .w = 110, .h = 31, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };


/* Trip Ended message. */
static struct rect notificationPadding = { .w = 160, .h = 10, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = TRIP_SUCCESS_COLOUR, .fill = 1 };
static struct rect notificationArea = { .w = 160, .h = 30, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = TRIP_SUCCESS_COLOUR, .fill = 1 };
static struct text notificationText  = { .size = 3, .text = notification, .colo = DEFAULT_COLOUR };
static struct rect notificationMessageArea = { .w = 160, .h = 56, .t = 0, .b_colo = BACKGROUND_COLOUR, .f_colo = BACKGROUND_COLOUR, .fill = 1 };
static struct text notificationMessageText  = { .size = 1, .text = notification, .colo = DEFAULT_COLOUR };


/* Element structure for the status bar. */
static struct gui_element statusElements[] = {
    { .xPos = 0, .yPos = 0, .dirty = 1, .type = RECT, .element = (void *)&statusBar, .callback = NULL},
    { .xPos = 1, .yPos = 1, .dirty = 1, .type = SYMB, .element = (void *)&sdStatus, .callback = sd_symbol_callback},
    { .xPos = 80, .yPos = 7, .dirty = 1, .type = TEXT, .element = (void *)&statusTime, .callback = time_text_callback},
    { .xPos = 149, .yPos = 1, .dirty = 1, .type = SYMB, .element = (void *)&rfStatus, .callback = radio_symbol_callback},
};

/* Element structures for the buttons. */
static struct gui_element button1Elements[] = {
    { .xPos = 0, .yPos = 14, .dirty = 1, .type = RECT, .element = (void *)&button1, .callback = button1_rect_callback},
    { .xPos = 26, .yPos = 23, .dirty = 1, .type = TEXT, .element = (void *)&button1Text, .callback = button1_text_callback},
};

static struct gui_element button2Elements[] = {
    { .xPos = 53, .yPos = 14, .dirty = 1, .type = RECT, .element = (void *)&button2, .callback = button2_rect_callback},
    { .xPos = 78, .yPos = 23, .dirty = 1, .type = TEXT, .element = (void *)&button2Text, .callback = button2_text_callback},
};

static struct gui_element button3Elements[] = {
    { .xPos = 106, .yPos = 14, .dirty = 1, .type = RECT, .element = (void *)&button3, .callback = button3_rect_callback},
    { .xPos = 132, .yPos = 23, .dirty = 1, .type = TEXT, .element = (void *)&button3Text, .callback = button3_text_callback},
};

/* Element structures for the time. */
static struct gui_element timeElements[] = {
    { .xPos = 0, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&timeArea, .callback = NULL},
    { .xPos = 80, .yPos = 67, .dirty = 1, .type = TEXT, .element = (void *)&timeText, .callback = time_text_callback},
};

static struct gui_element dateElements[] = {
    { .xPos = 0, .yPos = 90, .dirty = 1, .type = RECT, .element = (void *)&dateArea, .callback = NULL},
    { .xPos = 80, .yPos = 109, .dirty = 1, .type = TEXT, .element = (void *)&dateText, .callback = date_text_callback},
};

/* Structures for the digital display. */
static struct gui_element speedElements[] = {
    { .xPos = 0, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&speedArea, .callback = NULL},
    { .xPos = 80, .yPos = 52, .dirty = 1, .type = TEXT, .element = (void *)&speedText, .callback = speed_text_callback},
};

static struct gui_element distElements[] = {
    { .xPos = 0, .yPos = 67, .dirty = 1, .type = RECT, .element = (void *)&distArea, .callback = NULL},
    { .xPos = 80, .yPos = 85, .dirty = 1, .type = TEXT, .element = (void *)&distText, .callback = dist_text_callback},
};

static struct gui_element accelPowerElements[] = {
    { .xPos = 0, .yPos = 97, .dirty = 1, .type = RECT, .element = (void *)&accelPowerArea, .callback = NULL},
    { .xPos = 45, .yPos = 113, .dirty = 1, .type = TEXT, .element = (void *)&accelText, .callback = accel_text_callback},
    { .xPos = 125, .yPos = 113, .dirty = 1, .type = TEXT, .element = (void *)&powerText, .callback = power_text_callback},
};

/* Structures for the keypad system. */
static struct gui_element entryHeaderElements[] = {
    { .xPos = 0, .yPos = 40, .dirty = 1, .type = RECT, .element = (void *)&entryHeaderArea, .callback = NULL},
    { .xPos = 80, .yPos = 56, .dirty = 1, .type = TEXT, .element = (void *)&entryHeaderText, .callback = entry_header_text_callback},
};

static struct gui_element entryElements[] = {
    { .xPos = 0, .yPos = 73, .dirty = 1, .type = RECT, .element = (void *)&entryArea, .callback = NULL},
    { .xPos = 80, .yPos = 90, .dirty = 1, .type = TEXT, .element = (void *)&entryText, .callback = entry_text_callback},
    { .xPos = 80, .yPos = 102, .dirty = 1, .type = LINE, .element = (void *)&entryUnderline, .callback = entry_underline_callback},
};

static struct gui_element entryBottomPadding[] = {
    { .xPos = 0, .yPos = 108, .dirty = 1, .type = RECT, .element = (void *)&bottomPaddingArea, .callback = NULL},
};

static struct gui_element entryTopPadding[] = {
    { .xPos = 0, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&topPaddingArea, .callback = NULL},
};

/* Analog display area. */
static struct gui_element emptySpaceElements[] = {
    { .xPos = 25, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&emptySpaceArea, .callback = NULL},
};

static struct gui_element dialElements[] = {
    { .xPos = 0, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&dialArea, .callback = NULL},
    { .xPos = 55, .yPos = 85, .dirty = 1, .type = DIAL, .element = (void *)&dial, .callback = dial_callback},
};

static struct gui_element analogDistElements[] = {
    { .xPos = 0, .yPos = 93, .dirty = 1, .type = RECT, .element = (void *)&analogDistArea, .callback = NULL},
    { .xPos = 55, .yPos = 97, .dirty = 1, .type = TEXT, .element = (void *)&analogDistText, .callback = dist_text_callback},
};

static struct gui_element powerBarElements[] = {
    { .xPos = 0, .yPos = 103, .dirty = 1, .type = RECT, .element = (void *)&powerBarArea, .callback = NULL},
    { .xPos = 0, .yPos = 103, .dirty = 1, .type = HBARS, .element = (void *)&powBar, .callback = bar_callback},
};

static struct gui_element accelBarElements[] = {
    { .xPos = 120, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&accelBarArea, .callback = NULL},
    { .xPos = 120, .yPos = 80, .dirty = 1, .type = BARS, .element = (void *)&accelBar, .callback = pbar_callback},
};

/* For settings menu. */
static struct gui_element settingsButton1Elements[] = {
    { .xPos = 0, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&settingButton1, .callback = settingsButton1_rect_callback},
    { .xPos = 26, .yPos = 42, .dirty = 1, .type = TEXT, .element = (void *)&settingButton1Text, .callback = NULL},
};

static struct gui_element setting1Elements[] = {
    { .xPos = 53, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&setting1, .callback = NULL},
    { .xPos = 108, .yPos = 42, .dirty = 1, .type = TEXT, .element = (void *)&setting1Text, .callback = setting1_text_callback},
    { .xPos = 108, .yPos = 42, .dirty = 1, .type = LINE, .element = (void *)&setting1Underline, .callback = setting1_underline_callback},
};

static struct gui_element settingsButton2Elements[] = {
    { .xPos = 0, .yPos = 51, .dirty = 1, .type = RECT, .element = (void *)&settingButton2, .callback = settingsButton2_rect_callback},
    { .xPos = 26, .yPos = 61, .dirty = 1, .type = TEXT, .element = (void *)&settingButton2Text, .callback = NULL},
};

static struct gui_element setting2Elements[] = {
    { .xPos = 53, .yPos = 51, .dirty = 1, .type = RECT, .element = (void *)&setting2, .callback = NULL},
    { .xPos = 108, .yPos = 61, .dirty = 1, .type = TEXT, .element = (void *)&setting2Text, .callback = setting2_text_callback},
    { .xPos = 108, .yPos = 61, .dirty = 1, .type = LINE, .element = (void *)&setting2Underline, .callback = setting2_underline_callback},
};

static struct gui_element settingsButton3Elements[] = {
    { .xPos = 0, .yPos = 70, .dirty = 1, .type = RECT, .element = (void *)&settingButton3, .callback = settingsButton3_rect_callback},
    { .xPos = 26, .yPos = 80, .dirty = 1, .type = TEXT, .element = (void *)&settingButton3Text, .callback = NULL},
};

static struct gui_element setting3Elements[] = {
    { .xPos = 53, .yPos = 70, .dirty = 1, .type = RECT, .element = (void *)&setting3, .callback = NULL},
    { .xPos = 108, .yPos = 80, .dirty = 1, .type = TEXT, .element = (void *)&setting3Text, .callback = setting3_text_callback},
    { .xPos = 108, .yPos = 80, .dirty = 1, .type = LINE, .element = (void *)&setting3Underline, .callback = setting3_underline_callback},
};

static struct gui_element settingsButton4Elements[] = {
    { .xPos = 0, .yPos = 89, .dirty = 1, .type = RECT, .element = (void *)&settingButton4, .callback = settingsButton4_rect_callback},
    { .xPos = 26, .yPos = 99, .dirty = 1, .type = TEXT, .element = (void *)&settingButton4Text, .callback = NULL},
};

static struct gui_element setting4Elements[] = {
    { .xPos = 53, .yPos = 89, .dirty = 1, .type = RECT, .element = (void *)&setting4, .callback = NULL},
    { .xPos = 108, .yPos = 99, .dirty = 1, .type = TEXT, .element = (void *)&setting4Text, .callback = setting4_text_callback},
    { .xPos = 108, .yPos = 99, .dirty = 1, .type = LINE, .element = (void *)&setting4Underline, .callback = setting4_underline_callback},
};

static struct gui_element settingsButton5Elements[] = {
    { .xPos = 0, .yPos = 108, .dirty = 1, .type = RECT, .element = (void *)&settingButton5, .callback = settingsButton5_rect_callback},
    { .xPos = 26, .yPos = 118, .dirty = 1, .type = TEXT, .element = (void *)&settingButton5Text, .callback = NULL},
};

static struct gui_element setting5Elements[] = {
    { .xPos = 53, .yPos = 108, .dirty = 1, .type = RECT, .element = (void *)&setting5, .callback = NULL},
    { .xPos = 108, .yPos = 118, .dirty = 1, .type = TEXT, .element = (void *)&setting5Text, .callback = NULL},
    { .xPos = 108, .yPos = 118, .dirty = 1, .type = LINE, .element = (void *)&setting5Underline, .callback = NULL},
};

/* End trip screen. */
static struct gui_element notificationPaddingElements[] = {
    { .xPos = 0, .yPos = 32, .dirty = 1, .type = RECT, .element = (void *)&notificationPadding, .callback = NULL},
};

static struct gui_element notificationElements[] = {
    { .xPos = 0, .yPos = 42, .dirty = 1, .type = RECT, .element = (void *)&notificationArea, .callback = notification_rect_callback},
    { .xPos = 80, .yPos = 57, .dirty = 1, .type = TEXT, .element = (void *)&notificationText, .callback = NULL},
};

static struct gui_element notificationMessageElements[] = {
    { .xPos = 0, .yPos = 72, .dirty = 1, .type = RECT, .element = (void *)&notificationMessageArea, .callback = NULL},
    { .xPos = 80, .yPos = 100, .dirty = 1, .type = TEXT, .element = (void *)&notificationMessageText, .callback = notification_message_text_callback},
};

/* Global variables depicting the state of the system. */
static enum display_mode mode = DIGITAL;
static int buttonSelected = 1;
static int sdInserted = 1;
static int radioSignal = 1;
static int allDirty = 0;

static int headerNum = 0;

static int currentSelected = 1;
static int selectedPos = 0;

static int notificationType = 0;
static int tripError = 0;

/* Functions that can be used to modify the state variables in the gui.
 *
 * These explain themself.
 */

void gui_set_notification_type(int type)
{
    notificationType = type;
}

void gui_set_selected_pos(int pos)
{
    selectedPos = pos;
}

void gui_set_header_number(int header)
{
    headerNum = header;
}

void gui_set_keypad_selection(int selected)
{
    currentSelected = selected;
}

void gui_set_radio_status(int stat)
{
    radioSignal = stat;
}

void gui_set_sd_status(int stat)
{
    sdInserted = stat;
}

void gui_set_selected_button(int but)
{
    buttonSelected = but;
}

void gui_switch_modes(enum display_mode newMode)
{
    mode = newMode;
    allDirty = 1;
}

/* Main GUI task. Just loops updating the gui at 30 updates per second. */
void gui_task(void *params)
{
    int lastDirty = 0;
    TickType_t lastRender = 0;

    st7735_init();
    clear_screen();

    lastRender = xTaskGetTickCount();

    while(1) {
        lastDirty = allDirty;
        update_gui();
        if (lastDirty == allDirty) {
            allDirty = 0;
        }
        vTaskDelayUntil(&lastRender, 33);
    }
}

/* Updates all the elements of the gui based on the current operating mode and
 * whether or not the element has actually changed.
 */
void update_gui(void)
{
    /* These ones are always on the screen. */
    update_gui_elements(statusElements, 4);
    update_gui_elements(button1Elements, 2);
    update_gui_elements(button2Elements, 2);
    update_gui_elements(button3Elements, 2);

    /* Update the elemets of the screen that are only active in certain modes.
     */
    if (mode == TIME) {
        update_gui_elements(timeElements, 2);
        update_gui_elements(dateElements, 2);
    } else if (mode == DIGITAL) {
        update_gui_elements(speedElements, 2);
        update_gui_elements(distElements, 2);
        update_gui_elements(accelPowerElements, 3);
    } else if (mode == TEXT_ENTRY) {
        update_gui_elements(entryHeaderElements, 2);
        update_gui_elements(entryElements, 3);
        update_gui_elements(entryBottomPadding, 1);
        update_gui_elements(entryTopPadding, 1);
//        update_gui_elements(keypadElements, 4);
    } else if (mode == ANALOG) {
        update_gui_elements(dialElements, 2);
        update_gui_elements(analogDistElements, 2);
        update_gui_elements(accelBarElements, 2);
        update_gui_elements(powerBarElements, 2);
        update_gui_elements(emptySpaceElements, 1);
    } else if (mode == SETTINGS) {
        update_gui_elements(settingsButton1Elements, 2);
        update_gui_elements(setting1Elements, 2);
        update_gui_elements(settingsButton2Elements, 2);
        update_gui_elements(setting2Elements, 2);
        update_gui_elements(settingsButton3Elements, 2);
        update_gui_elements(setting3Elements, 2);
        update_gui_elements(settingsButton4Elements, 2);
        update_gui_elements(setting4Elements, 2);
        update_gui_elements(settingsButton5Elements, 2);
        update_gui_elements(setting5Elements, 2);
    } else if (mode == NOTIFICATION) {
        update_gui_elements(notificationPaddingElements, 1);
        update_gui_elements(notificationElements, 2);
        update_gui_elements(notificationMessageElements, 2);
    }
}

/* Runs the callback for numElements gui elements in elements and renders them
 * if any are found to be dirty.
 */
static void update_gui_elements(struct gui_element *elements, int numElements)
{
    int dirty = 0;

    for (int i = 0; i < numElements; i++) {
        if (elements[i].callback) {
            elements[i].dirty = elements[i].callback(elements[i].element);
        } else if (allDirty) {
            elements[i].dirty = 1;
        }
        
        if (elements[i].dirty) {
            dirty = 1;
        }
    }

    if (dirty) {
        render_gui_elements(elements, numElements);
    }

//    if (dirty || allDirty) {
//        render_gui_elements(elements, numElements);
//    }
}

/***** Callback Functions *****/

/* Below here are all the callback functions that update each element of the
 * gui. Each one is not documented because they are very simple and you can
 * work out what they do by reading the code.  
 *
 * Each callback must take a void * that will be the correspoding gui element
 * and return whether or not that element is dirty and should be redrawn.
 * 
 * Be warned there are a lot...
 */

static int notification_message_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;

    if (t->text == tripMessage[tripError]) {
        dirty = 0;
    } else {
        t->text = tripMessage[tripError];
    }

    return dirty;
}

static int notification_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (tripError == 0 && r->f_colo != TRIP_SUCCESS_COLOUR) {
        r->f_colo = TRIP_SUCCESS_COLOUR;
        dirty = 1;
    } else if (currentSelected != 0 && r->f_colo != TRIP_ERROR_COLOUR) {
        r->f_colo = TRIP_ERROR_COLOUR;
        dirty = 1;
    }

    return dirty;
}

static int setting1_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;

    /* If this one is being edited use the current entry otherwise just use the
     * value.
     */
    if (currentSelected == 1 && selectedPos >= 0) {
        char *tmp;
        tmp = get_current_entry();
        if (!strcmp(tmp, t->text)) {
            dirty = 0;
        } else {
            strcpy(t->text, tmp);
        }
    } else {
        char tmp[20] = { 0 };
        RTC_TimeTypeDef currentTime;

        RTC_GetTime(RTC_Format_BCD, &currentTime);

        tmp[0] = (currentTime.RTC_Hours>>4) + '0';
        tmp[1] = (currentTime.RTC_Hours & 0x0F) + '0';
        tmp[2] = ':';
        tmp[3] = (currentTime.RTC_Minutes>>4) + '0';
        tmp[4] = (currentTime.RTC_Minutes & 0x0F) + '0';
        tmp[5] = ':';
        tmp[6] = (currentTime.RTC_Seconds>>4) + '0';
        tmp[7] = (currentTime.RTC_Seconds & 0x0F) + '0';
        tmp[8] = '\0';

        if (!strcmp(t->text, tmp)) {
            dirty = 0;
        } else {
            strcpy(t->text, tmp);
        }
    }

    return dirty;
}

static int setting1_underline_callback(void *elem)
{
    struct line *l = elem;
    int dirty = 1;
    int x1, x2, len, start;
    
    if (currentSelected == 1 && selectedPos >= 0) {
        len = strlen(get_current_entry());
        start = 90 - (len * 3);
        x1 = start + (selectedPos * 6);
        x2 = x1 + 5;
        l->colo = DEFAULT_COLOUR;
    } else {
        x1 = l->x1;
        x2 = l->x1;
        l->colo = BACKGROUND_COLOUR;
    }

    if (l->x1 == x1 && l->x2 == x2) {
        dirty = 0;
    } else {
        l->x1 = x1;
        l->x2 = x2;
    }
    
    return dirty;
}

static int setting2_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;

    /* If this one is being edited use the current entry otherwise just use the
     * value.
     */
    if (currentSelected == 2 && selectedPos >= 0) {
        char *tmp;
        tmp = get_current_entry();

        if (!strcmp(tmp, t->text)) {
            dirty = 0;
        } else {
            strcpy(t->text, tmp);
        }
    } else {
        RTC_DateTypeDef currentDate;
        char tmp[20] = { 0 };

        RTC_GetDate(RTC_Format_BCD, &currentDate);

        tmp[0] = (currentDate.RTC_Date>>4) + '0';
        tmp[1] = (currentDate.RTC_Date & 0x0F) + '0';
        tmp[2] = '/';
        tmp[3] = (currentDate.RTC_Month>>4) + '0';
        tmp[4] = (currentDate.RTC_Month & 0x0F) + '0';
        tmp[5] = '/';
        tmp[6] = '2';
        tmp[7] = '0';
        tmp[8] = (currentDate.RTC_Year>>4) + '0';
        tmp[9] = (currentDate.RTC_Year & 0x0F) + '0';
        tmp[10] = '\0';

        if (!strcmp(t->text, tmp)) {
            dirty = 0;
        } else {
            strcpy(t->text, tmp);
        }
    }

    return dirty;
}

static int setting2_underline_callback(void *elem)
{
    struct line *l = elem;
    int dirty = 1;
    int x1, x2, len, start;
    
    if (currentSelected == 2 && selectedPos >= 0) {
        len = strlen(get_current_entry());
        start = 90 - (len * 3);
        x1 = start + (selectedPos * 6);
        x2 = x1 + 5;
        l->colo = DEFAULT_COLOUR;
    } else {
        x1 = l->x1;
        x2 = l->x1;
        l->colo = BACKGROUND_COLOUR;
    }

    if (l->x1 == x1 && l->x2 == x2) {
        dirty = 0;
    } else {
        l->x1 = x1;
        l->x2 = x2;
    }
    
    return dirty;
}

static int setting3_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;

    /* If this one is being edited use the current entry otherwise just use the
     * value.
     */
    if (currentSelected == 3 && selectedPos >= 0) {
        char *tmp;
        tmp = get_current_entry();

        if (!strcmp(tmp, t->text)) {
            dirty = 0;
        } else {
            strcpy(t->text, tmp);
        }
    } else {
        char tmp[20] = { 0 };
        int currentDiam = get_current_wheel_size();
        int len, inc = 0;
        
        if (currentDiam < 100) {
            tmp[0] = '0';
            inc = 1;
        }

        len = write_dec_num(currentDiam, 0, tmp + inc);
        len += inc;
        tmp[len] = 'm';
        tmp[len + 1] = 'm';
        tmp[len + 2] = '\0';

        if (!strcmp(t->text, tmp)) {
            dirty = 0;
        } else {
            strcpy(t->text, tmp);
        }
    }

    return dirty;
}

static int setting3_underline_callback(void *elem)
{
    struct line *l = elem;
    int dirty = 1;
    int x1, x2, len, start;
    
    if (currentSelected == 3 && selectedPos >= 0) {
        len = strlen(get_current_entry());
        start = 90 - (len * 3);
        x1 = start + (selectedPos * 6);
        x2 = x1 + 5;
        l->colo = DEFAULT_COLOUR;
    } else {
        x1 = l->x1;
        x2 = l->x1;
        l->colo = BACKGROUND_COLOUR;
    }

    if (l->x1 == x1 && l->x2 == x2) {
        dirty = 0;
    } else {
        l->x1 = x1;
        l->x2 = x2;
    }
    
    return dirty;
}

static int setting4_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;

    /* If this one is being edited use the current entry otherwise just use the
     * value.
     */
    if (currentSelected == 4 && selectedPos >= 0) {
        char *tmp;
        tmp = get_current_entry();

        if (!strcmp(tmp, t->text)) {
            dirty = 0;
        } else {
            strcpy(t->text, tmp);
        }
    } else {
        char tmp[20] = { 0 };
        int currentWeight = get_current_weight();
        int len, inc = 0;

        if (currentWeight < 1000) {
            tmp[0] = '0';
            inc = 1;
        }

        len = write_dec_num(currentWeight, 0, tmp + inc);
        len += inc;
        tmp[len] = 'k';
        tmp[len + 1] = 'g';
        tmp[len + 2] = '\0';

        if (!strcmp(t->text, tmp)) {
            dirty = 0;
        } else {
            strcpy(t->text, tmp);
        }
    }

    return dirty;
}

static int setting4_underline_callback(void *elem)
{
    struct line *l = elem;
    int dirty = 1;
    int x1, x2, len, start;
    
    if (currentSelected == 4 && selectedPos >= 0) {
        len = strlen(get_current_entry());
        start = 90 - (len * 3);
        x1 = start + (selectedPos * 6);
        x2 = x1 + 5;
        l->colo = DEFAULT_COLOUR;
    } else {
        x1 = l->x1;
        x2 = l->x1;
        l->colo = BACKGROUND_COLOUR;
    }

    if (l->x1 == x1 && l->x2 == x2) {
        dirty = 0;
    } else {
        l->x1 = x1;
        l->x2 = x2;
    }
    
    return dirty;
}

static int settingsButton1_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (currentSelected == 1 && r->f_colo != BUTTON_SELECTED_COLOUR) {
        r->f_colo = BUTTON_SELECTED_COLOUR;
        dirty = 1;
    } else if (currentSelected != 1 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
        r->f_colo = BUTTON_BACKGROUND_COLOUR;
        dirty = 1;
    }

    return dirty;
}

static int settingsButton2_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (currentSelected == 2 && r->f_colo != BUTTON_SELECTED_COLOUR) {
        r->f_colo = BUTTON_SELECTED_COLOUR;
        dirty = 1;
    } else if (currentSelected != 2 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
        r->f_colo = BUTTON_BACKGROUND_COLOUR;
        dirty = 1;
    }

    return dirty;
}

static int settingsButton3_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (currentSelected == 3 && r->f_colo != BUTTON_SELECTED_COLOUR) {
        r->f_colo = BUTTON_SELECTED_COLOUR;
        dirty = 1;
    } else if (currentSelected != 3 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
        r->f_colo = BUTTON_BACKGROUND_COLOUR;
        dirty = 1;
    }

    return dirty;
}

static int settingsButton4_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (currentSelected == 4 && r->f_colo != BUTTON_SELECTED_COLOUR) {
        r->f_colo = BUTTON_SELECTED_COLOUR;
        dirty = 1;
    } else if (currentSelected != 4 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
        r->f_colo = BUTTON_BACKGROUND_COLOUR;
        dirty = 1;
    }

    return dirty;
}

static int settingsButton5_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (currentSelected == 5 && r->f_colo != BUTTON_SELECTED_COLOUR) {
        r->f_colo = BUTTON_SELECTED_COLOUR;
        dirty = 1;
    } else if (currentSelected != 5 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
        r->f_colo = BUTTON_BACKGROUND_COLOUR;
        dirty = 1;
    }

    return dirty;
}

static int dial_callback(void *elem)
{
    struct circ *c = elem;
    int dirty = 1, newSpeed;

    newSpeed = (c->speed + 1) % 100;

    if (newSpeed == c->speed) {
        dirty = 0;
    } else {
        c->speed = newSpeed;
    }

    return dirty;
}

static int pbar_callback(void *elem)
{
    struct accel_bar *ab = elem;
    int dirty = 1, newaccel;

    newaccel = ((ab->acceleration + 21) % 40) - 20;

    if (newaccel == ab->acceleration) {
        dirty = 0;
    } else {
        ab->acceleration = newaccel;
    }

    return dirty;
}

static int bar_callback(void *elem)
{
    struct power_bar *ab = elem;
    int dirty = 1, newaccel;

    newaccel = (ab->power + 1) % 120;

    if (newaccel == ab->power) {
        dirty = 0;
    } else {
        ab->power = newaccel;
    }

    return dirty;
}

static int entry_underline_callback(void *elem)
{
    struct line *l = elem;
    int dirty = 1;
    int x1, x2, len, start;

    len = strlen(get_current_entry());

    start = 80 - (len * 9);

    x1 = start + (selectedPos * 18);
    x2 = x1 + 17;

    if (l->x1 == x1 && l->x2 == x2) {
        dirty = 0;
    } else {
        l->x1 = x1;
        l->x2 = x2;
    }
    
    return dirty;
}

static int entry_text_callback(void *elem)
{
    struct text *t = elem;
    char *tmp;
    int dirty = 1;

    tmp = get_current_entry();

    if (!strcmp(tmp, t->text)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }

    return dirty;
}

static int entry_header_text_callback(void *elem)
{
    struct text *t = elem;
    char *tmp = headers[headerNum];
    int dirty = 1;
    
    if (!strcmp(tmp, t->text)) {
        dirty = 0;
    } else {
        t->text = tmp; 
    }
    
    return dirty;
}

static int slider_callback(void *elem)
{
    struct text_slider *s = elem;
    int dirty = 1;

    if (s->pos != currentSelected) {
        s->pos = currentSelected;
    } else {
        dirty = 0;
    }

    return dirty;
}

static int prev_text_callback(void *elem)
{
    struct text *t = elem;
    char tmp[5] = { 0 };
    int dirty = 1;

    tmp[0] = ((currentSelected - 1) >= 0 ? (currentSelected - 1) : 9) + '0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }
    
    return dirty;
}

static int next_text_callback(void *elem)
{
    struct text *t = elem;
    char tmp[5] = { 0 };
    int dirty = 1;

    tmp[0] = ((currentSelected + 1) % 10) + '0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }
    
    return dirty;
}

static int selected_text_callback(void *elem)
{
    struct text *t = elem;
    char tmp[5] = { 0 };
    int dirty = 1;

    tmp[0] = currentSelected + '0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }
    
    return dirty;
}

static int sd_symbol_callback(void *elem)
{
    struct symb *s = elem;
    int dirty = 1;

    sdInserted = sd_get_status();

    if (sdInserted) {
        if (!memcmp(s->sym, symbols, 12)) {
            dirty = 0;
        } else {
            s->sym = symbols; 
        }
    } else {
        if (!memcmp(s->sym, symbols + 24, 12)) {
            dirty = 0;
        } else {
            s->sym = symbols + 24; 
        }
    }

    return dirty;
}

static int radio_symbol_callback(void *elem)
{
    struct symb *s = elem;
    int dirty = 1;

    radioSignal = get_radio_status();

    if (radioSignal) {
        if (!memcmp(s->sym, symbols + 12, 12)) {
            dirty = 0;
        } else {
            s->sym = symbols + 12; 
        }
    } else {
        if (!memcmp(s->sym, symbols + 24, 12)) {
            dirty = 0;
        } else {
            s->sym = symbols + 24; 
        }
    }

    return dirty;
}

static int date_text_callback(void *elem)
{
    struct text *t = elem;
    RTC_DateTypeDef currentDate;
    char tmp[20] = { 0 };
    int dirty = 1;

    RTC_GetDate(RTC_Format_BCD, &currentDate);

    tmp[0] = (currentDate.RTC_Date>>4) + '0';
    tmp[1] = (currentDate.RTC_Date & 0x0F) + '0';
    tmp[2] = '/';
    tmp[3] = (currentDate.RTC_Month>>4) + '0';
    tmp[4] = (currentDate.RTC_Month & 0x0F) + '0';
    tmp[5] = '/';
    tmp[6] = '2';
    tmp[7] = '0';
    tmp[8] = (currentDate.RTC_Year>>4) + '0';
    tmp[9] = (currentDate.RTC_Year & 0x0F) + '0';
    tmp[10] = '\0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }

    return dirty;
}

static int time_text_callback(void *elem)
{
    struct text *t = elem;
    RTC_TimeTypeDef currentTime;
    char tmp[20] = { 0 };
    int dirty = 1;
    
    RTC_GetTime(RTC_Format_BCD, &currentTime);

    tmp[0] = (currentTime.RTC_Hours>>4) + '0';
    tmp[1] = (currentTime.RTC_Hours & 0x0F) + '0';
    tmp[2] = ':';
    tmp[3] = (currentTime.RTC_Minutes>>4) + '0';
    tmp[4] = (currentTime.RTC_Minutes & 0x0F) + '0';
    tmp[5] = ':';
    tmp[6] = (currentTime.RTC_Seconds>>4) + '0';
    tmp[7] = (currentTime.RTC_Seconds & 0x0F) + '0';
    tmp[8] = '\0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }

    return dirty;
}


static int accel_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;
    int accel = 17;
    char tmp[20] = { 0 };
    int len;

    accel = get_current_accel();

    len = write_dec_num(accel, 0, tmp);

    tmp[len] = 'm';
    tmp[len + 1] = '/';
    tmp[len + 2] = 's';
    tmp[len + 3] = '/';
    tmp[len + 4] = 's';
    tmp[len + 5] = '\0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }

    return dirty;
}

static int power_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;
    int power = 177;
    char tmp[20] = { 0 };
    int len;

    power = get_current_power();

    len = write_dec_num(power, 0, tmp);

    tmp[len] = 'k';
    tmp[len + 1] = 'w';
    tmp[len + 2] = '\0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }

    return dirty;
}

static int dist_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;
    int dist;
    char tmp[20] = { 0 };
    int len;

    dist = get_current_distance();

    len = write_dec_num(dist, 2, tmp);

    tmp[len] = 'k';
    tmp[len + 1] = 'm';
    tmp[len + 2] = '\0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }

    return dirty;
}

static int speed_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;
    int speed = 52;
    char tmp[20] = { 0 };
    int len;

    speed = get_current_speed();

    len = write_dec_num(speed, 0, tmp);

    tmp[len] = 'k';
    tmp[len + 1] = 'm';
    tmp[len + 2] = '/';
    tmp[len + 3] = 'h';
    tmp[len + 4] = '\0';

    if (!strcmp(t->text, tmp)) {
        dirty = 0;
    } else {
        strcpy(t->text, tmp);
    }

    return dirty;
}

static int button1_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (mode == SETTINGS) {
        if (currentSelected == 0 && r->f_colo != BUTTON_SELECTED_COLOUR) {
            r->f_colo = BUTTON_SELECTED_COLOUR;
            dirty = 1;
        } else if (currentSelected != 0 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
            r->f_colo = BUTTON_BACKGROUND_COLOUR;
            dirty = 1;
        }
    } else {
        if (buttonSelected == 1 && r->f_colo != BUTTON_SELECTED_COLOUR) {
            r->f_colo = BUTTON_SELECTED_COLOUR;
            dirty = 1;
        } else if (buttonSelected != 1 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
            r->f_colo = BUTTON_BACKGROUND_COLOUR;
            dirty = 1;
        }
    }

    return dirty;
}

static int button2_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (buttonSelected == 2 && r->f_colo != BUTTON_SELECTED_COLOUR) {
        r->f_colo = BUTTON_SELECTED_COLOUR;
        dirty = 1;
    } else if (buttonSelected != 2 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
        r->f_colo = BUTTON_BACKGROUND_COLOUR;
        dirty = 1;
    }

    return dirty;
}

static int button3_rect_callback(void *elem)
{
    struct rect *r = elem;
    int dirty = 0;

    if (buttonSelected == 3 && r->f_colo != BUTTON_SELECTED_COLOUR) {
        r->f_colo = BUTTON_SELECTED_COLOUR;
        dirty = 1;
    } else if (buttonSelected != 3 && r->f_colo != BUTTON_BACKGROUND_COLOUR) {
        r->f_colo = BUTTON_BACKGROUND_COLOUR;
        dirty = 1;
    }

    return dirty;
}

static int button1_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;

    if (mode == TIME || mode == TEXT_ENTRY) {
        if (!strcmp(t->text, "Settings")) {
            dirty = 0;
        } else {
            strcpy(t->text, "Settings");
        }
    } else if (mode == SETTINGS) {
        if (!strcmp(t->text, "Done")) {
            dirty = 0;
        } else {
            strcpy(t->text, "Done");
        }
    } else {
        if (!strcmp(t->text, "Digital")) {
            dirty = 0;
        } else {
            strcpy(t->text, "Digital");
        }
    }

    return dirty;
}

static int button2_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;

    if (mode == TIME || mode == TEXT_ENTRY || mode == SETTINGS) {
        if (!strcmp(t->text, "Digital")) {
            dirty = 0;
        } else {
            strcpy(t->text, "Set Car");
        }
    } else {
        if (!strcmp(t->text, "Analog")) {
            dirty = 0;
        } else {
            strcpy(t->text, "Analog");
        }
    }

    return dirty;
}

static int button3_text_callback(void *elem)
{
    struct text *t = elem;
    int dirty = 1;

    if (mode == TIME || mode == TEXT_ENTRY || mode == SETTINGS) {
        if (sdInserted && radioSignal) {
            if (!strcmp(t->text, "Start")) {
                dirty = 0;
            } else {
                strcpy(t->text, "Start");
            }
        } else if (!sdInserted) {
            if (!strcmp(t->text, "No SD")) {
                dirty = 0;
            } else {
                strcpy(t->text, "No SD");
            }
        } else {
            if (!strcmp(t->text, "No Sense")) {
                dirty = 0;
            } else {
                strcpy(t->text, "No Sense");
            }
        }
    } else {
        if (!strcmp(t->text, "Stop")) {
            dirty = 0;
        } else {
            strcpy(t->text, "Stop");
        }
    }

    return dirty;
}
