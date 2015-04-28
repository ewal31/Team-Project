#ifndef GUI_H_
#define GUI_H_

#include <stdint.h>

#define CONVERT_24_TO_16_BIT(r,g,b) ((((((g >> 2) & 0x07) << 5) | (b >> 3)) << 8) | ((r & 0xF1) | (g >> 5)))

//#define BACKGROUND_COLOUR (CONVERT_24_TO_16_BIT(235, 235, 235))
#define BACKGROUND_COLOUR (CONVERT_24_TO_16_BIT(0, 0, 0))

#define BORDER_COLOUR (CONVERT_24_TO_16_BIT(51, 214, 255))
#define BUTTON_BACKGROUND_COLOUR (CONVERT_24_TO_16_BIT(60, 205, 255))
#define BUTTON_SELECTED_COLOUR (CONVERT_24_TO_16_BIT(30, 140, 245))
#define BUTTON_TEXT_COLOUR (CONVERT_24_TO_16_BIT(255, 255, 255))

#define TRIP_SUCCESS_COLOUR (CONVERT_24_TO_16_BIT(0, 255, 0))
#define TRIP_ERROR_COLOUR (CONVERT_24_TO_16_BIT(255, 0, 0))

#define POINTER_COLOUR (CONVERT_24_TO_16_BIT(255,0,0))
#define DIAL_COLOUR (CONVERT_24_TO_16_BIT(255,174,0))

#define DEFAULT_COLOUR 0xFFFF
//#define DEFAULT_COLOUR 0x0000
#define SELECTED_COLOUR 0xA0A0
#define UNSELECTED_COLOUR DEFAULT_COLOUR

typedef enum {
    RECT,
    LINE,
    CIRC,
    TEXT,
    SYMB,
    DIAL,
    SLIDER,
    BARS,
    HBARS,
} gui_element_type;

enum display_mode {
    TIME,
    DIGITAL,
    ANALOG,
    TEXT_ENTRY,
    SETTINGS,
    NOTIFICATION,
};

struct gui_element {
    int (*callback) (void *);
    void *element;
    gui_element_type type;
    uint8_t dirty;
    uint8_t xPos;
    uint8_t yPos;
};

void gui_task(void *params);
void update_gui(void);
void gui_set_selected_button(int but);
void gui_switch_modes(enum display_mode newMode);
void gui_set_keypad_selection(int selected);
void gui_set_header_number(int header);
void gui_set_selected_pos(int pos);
void gui_set_notification_type(int type);

#endif
