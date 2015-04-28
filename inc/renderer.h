#ifndef RENDERER_H_
#define RENDERER_H_

#include <stdint.h>
#include "gui.h"

#define F_BUFF_SIZE 10000

struct frame_buffer {
    uint8_t width;
    uint8_t height;
    uint8_t xOffset;
    uint8_t yOffset;
    uint16_t buff[F_BUFF_SIZE];
};

struct rect {
    uint16_t b_colo;
    uint16_t f_colo;
    uint8_t w;
    uint8_t h;
    uint8_t t;
    uint8_t fill;
};

struct accel_bar {
    int8_t acceleration; 
};

struct power_bar {
    int8_t power; 
};

struct line {
    uint16_t colo;
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
    uint8_t t;
};

struct triangle {
    uint16_t b_colo;
    uint16_t f_colo;
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
    uint8_t x3;
    uint8_t y3;
    uint8_t fill;
};

struct text_slider {
    uint16_t colo;
    uint8_t textSize;
    uint8_t len;
    uint8_t pos;
    uint8_t xText;
};

struct text {
    char *text;
    uint16_t colo;
    uint16_t size;
};

struct symb {
    uint16_t const *sym;
    uint16_t colo;
};

struct circ {
    uint16_t colo;
    uint16_t pcolo;
    uint8_t speed;
};

void render_gui_elements(struct gui_element *elements, int num);

#endif
