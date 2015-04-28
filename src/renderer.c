/* renderer.c
 *
 * rendering graphics primatives.
 *
 * Written for ENGG4810 Team 30 by:
 * - Mitchell Krome
 * - Edward Wall
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "gui.h"
#include "renderer.h"
#include "st7735.h"

/* Bar Graph Colours */

static uint16_t colours[10] = 
{
    CONVERT_24_TO_16_BIT(0, 255, 0),
    CONVERT_24_TO_16_BIT(0, 255, 0),
    CONVERT_24_TO_16_BIT(145,255,0),
    CONVERT_24_TO_16_BIT(145,255,0),
    CONVERT_24_TO_16_BIT(255,255,0),
    CONVERT_24_TO_16_BIT(255,255,0),
    CONVERT_24_TO_16_BIT(255,145,0),
    CONVERT_24_TO_16_BIT(255,145,0),
    CONVERT_24_TO_16_BIT(255,0,0),
    CONVERT_24_TO_16_BIT(255,0,0)
};

/* End points for speeds 0-120km on the dial. This saves us using expensive
 * function calls to work them out later.
 */

static int8_t xEndPoints[] =
{
    45,45,45,45,45,45,44,44,44,44,43,43,43,42,42,42,41,41,40,40,39,38,38,37,36,36,35,34,33,33,32,31,30,29,28,
    27,26,25,25,24,22,21,20,19,18,17,16,15,14,13,12,11,9,8,7,6,5,4,2,1,0,-1,-2,-4,-5,-6,-7,-8,-9,-11,-12,-13,
    -14,-15,-16,-17,-18,-19,-20,-21,-22,-24,-25,-25,-26,-27,-28,-29,-30,-31,-32,-33,-33,-34,-35,-36,-36,-37,
    -38,-38,-39,-40,-40,-41,-41,-42,-42,-42,-43,-43,-43,-44,-44,-44,-44,-45,-45,-45,-45,-45,-45
};

static int8_t yEndPoints[] =
{
    0,1,2,4,5,6,7,8,9,11,12,13,14,15,16,17,18,19,20,21,22,24,25,25,26,27,28,29,30,31,32,33,33,34,35,36,36,37,
    38,38,39,40,40,41,41,42,42,42,43,43,43,44,44,44,44,45,45,45,45,45,45,45,45,45,45,45,44,44,44,44,43,43,43,
    42,42,42,41,41,40,40,39,38,38,37,36,36,35,34,33,33,32,31,30,29,28,27,26,25,25,24,22,21,20,19,18,17,16,15,
    14,13,12,11,9,8,7,6,5,4,2,1,0
};

extern const uint8_t font[];

/* The frame buffer we will use for all our drawing. */
static struct frame_buffer fBuff;

/* This macro makes the linear frame buffer appear as a 2d array for ease of
 * use. Also a debug version that prints where is being written to.
 */
#define B_POS(x, y) (fBuff.buff[(((y) * (fBuff.width)) + (x))])
//#define P_POS(x, y) do {printf("Writing to index %d, %d. Index %d\n", (x), (y), (((y) * (fBuff.width)) + (x)));} while (0)

static void set_frame(char c);
static void set_frame_size(uint16_t w, uint16_t h, uint16_t x, uint16_t y);
static void render_rect(uint16_t xPos, uint16_t yPos, struct rect *r);
static void render_line(struct line *l);
static void render_triangle(struct triangle *t);
static void render_text(int16_t xPos, int16_t yPos, struct text *text);
static void render_symbol(uint16_t xPos, uint16_t yPos, struct symb *symb);
static void render_semi_circle(uint16_t xPos, uint16_t yPos, struct circ *circ);
static void render_line_direct(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t colo);
static void render_slider(uint16_t yPos, struct text_slider *s);
static void render_bars(uint16_t xPos, uint16_t yPos, struct accel_bar *ab);
static void render_hbars(uint16_t xPos, uint16_t yPos, struct power_bar *ab);

/* Set the size of the current frame buffer in preperation for rendering the
 * next frame. This will need to be done in order for the renderer functions to
 * work properly.
 */
static void set_frame_size(uint16_t w, uint16_t h, uint16_t x, uint16_t y)
{
    fBuff.width = w;
    fBuff.height = h;
    fBuff.xOffset = x;
    fBuff.yOffset = y;
}

/* Fill the frame buffer up with a single character. */
void set_frame(char c)
{
    memset(fBuff.buff, c, fBuff.width * fBuff.height * sizeof(uint16_t));
}

/* Render the all the graphics objects in elements. num elements will be
 * rendered, in the order they appear in the array. The first element should be
 * a rectangle that encapsulates the whole area to be re-drawn. The rectangle
 * should be sized so that w x h is less then F_BUFF_SIZE or else bad stuff.
 */
void render_gui_elements(struct gui_element *elements, int num)
{
    struct gui_element *curr = elements;
    struct rect *border = curr->element;

    set_frame_size(border->w, border->h, curr->xPos, curr->yPos);

    for (int i = 0; i < num; ++i) {
        switch(curr->type) {
            case RECT:
                render_rect(curr->xPos, curr->yPos, curr->element);
                break;
            case SYMB:
                render_symbol(curr->xPos, curr->yPos, curr->element);
                break;
            case LINE:
                render_line(curr->element);
                break;
            case TEXT:
                render_text(curr->xPos, curr->yPos, curr->element);
                break;
            case DIAL:
                render_semi_circle(curr->xPos, curr->yPos, curr->element);
                break;
            case SLIDER:
                render_slider(curr->yPos, curr->element);
                break;
	    case BARS:
		render_bars(curr->xPos, curr->yPos, curr->element);
		break;
	    case HBARS:
		render_hbars(curr->xPos, curr->yPos, curr->element);
		break;
            default:
                break;
        }
        curr->dirty = 0;
        ++curr;
    }

    /* All the elements have been rendered, now we need to send them to the
     * screen.
     */
    draw_buffer(fBuff.buff, fBuff.width * fBuff.height, fBuff.xOffset, 
            fBuff.xOffset + fBuff.width - 1, fBuff.yOffset, 
            fBuff.yOffset + fBuff.height - 1);
}

/* Render a rectangle to the frame buffer. */
static void render_rect(uint16_t xPos, uint16_t yPos, struct rect *r)
{
    uint16_t x = xPos - fBuff.xOffset;
    uint16_t y = yPos - fBuff.yOffset;
    uint8_t bord = 0;

    for (uint16_t cy = 0; cy < r->h; ++cy) {
        if (cy < r->t || cy >= (r->h - r->t))
            bord = 1;
        else
            bord = 0;
        for (uint16_t cx = 0; cx < r->w; ++cx) {
            if (bord || cx < r->t || cx >= (r->w - r->t))
                B_POS(x + cx, y + cy) = r->b_colo;
            else if (r->fill)
                B_POS(x + cx, y + cy) = r->f_colo;
        }
    }
}

/* Render the rectagles for the bar graphs */
static void render_bars(uint16_t xPos, uint16_t yPos, struct accel_bar *ab)
{

    struct rect r;
    r.b_colo = BACKGROUND_COLOUR; /* Border Color Black */
    r.f_colo = BUTTON_TEXT_COLOUR; /* Rectangle Color Changes */
    r.w = 20;
    r.h = 5;
    r.t = 1;
    r.fill = 1;

    struct text txt = { .text = "0", .colo=BUTTON_TEXT_COLOUR, .size=1};
    render_text(xPos+14, yPos, &txt);

    txt.text = "20";
    render_text(xPos+10, yPos-38, &txt);

    txt.text = "-20";
    render_text(xPos+8, yPos+40, &txt);

    xPos += 19;

    if (ab->acceleration > 0) { //make positive
	for (uint8_t i = 4, j = 0; i <= (4*(ab -> acceleration))/2; i+=4, j++) {
	    r.f_colo = colours[j];
	    render_rect(xPos, yPos - i, &r);
	}
    } else {
	for (uint8_t i = 0, j = 0; i <= (-4*(ab -> acceleration))/2-4; i+=4, j++) {
	    r.f_colo = colours[j];
	    render_rect(xPos, yPos + i, &r);
	}
    }

    render_line_direct(xPos-fBuff.xOffset, xPos+20-fBuff.xOffset, yPos-fBuff.yOffset, yPos-fBuff.yOffset, BUTTON_TEXT_COLOUR);
    render_line_direct(xPos-fBuff.xOffset, xPos+5-fBuff.xOffset, yPos+40-fBuff.yOffset, yPos+40-fBuff.yOffset, BUTTON_TEXT_COLOUR);
    render_line_direct(xPos-fBuff.xOffset, xPos+5-fBuff.xOffset, yPos-40-fBuff.yOffset, yPos-40-fBuff.yOffset, BUTTON_TEXT_COLOUR);
    render_line_direct(xPos-fBuff.xOffset, xPos-fBuff.xOffset, yPos+40-fBuff.yOffset, yPos-40-fBuff.yOffset, BUTTON_TEXT_COLOUR);
}

/* Render the rectagles for the bar graphs */
static void render_hbars(uint16_t xPos, uint16_t yPos, struct power_bar *ab)
{

    struct rect r;
    r.b_colo = BACKGROUND_COLOUR; /* Border Color Black */
    r.f_colo = BUTTON_TEXT_COLOUR; /* Rectangle Color Changes */
    r.w = ab->power;
    r.h = 15;
    r.t = 1;
    r.fill = 1;

    struct text txt = { .text = "0", .colo=BUTTON_TEXT_COLOUR, .size=1};
    render_text(xPos+3, yPos+5, &txt);

    txt.text = "100";
    render_text(xPos+100, yPos+5, &txt);
    txt.text = "50";
    render_text(xPos+50, yPos+5, &txt);

    r.f_colo = CONVERT_24_TO_16_BIT(255, (ab->power)*2, 0),
    render_rect(xPos, yPos + 10, &r); 

    render_line_direct(xPos-fBuff.xOffset, xPos+120-fBuff.xOffset, yPos+9-fBuff.yOffset, yPos+9-fBuff.yOffset, CONVERT_24_TO_16_BIT(255,0,0));
    render_line_direct(xPos+119-fBuff.xOffset, xPos+119-fBuff.xOffset, yPos+9-fBuff.yOffset, yPos+20-fBuff.yOffset, CONVERT_24_TO_16_BIT(255,0,0));
    render_line_direct(xPos+60-fBuff.xOffset, xPos+60-fBuff.xOffset, yPos+9-fBuff.yOffset, yPos+20-fBuff.yOffset, CONVERT_24_TO_16_BIT(255,0,0));

}

static void render_slider(uint16_t yPos, struct text_slider *s)
{
    uint16_t y = yPos - fBuff.yOffset;
    uint16_t x = s->xText - fBuff.xOffset; // Middle
    uint16_t h = s->textSize * 10;
    uint16_t w = s->textSize * 7;

    x -= (((s->len * 6 * s->textSize - s->textSize) / 2) + 1);
    x += (s->pos * s->textSize * 6 - (s->textSize / 2));

    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            B_POS(x + cx, y + cy) = s->colo;
        }
    }
}

/* Render a line to the frame buffer. */
static void render_line(struct line *l)
{
    int16_t x1 = l->x1 - fBuff.xOffset;  
    int16_t x2 = l->x2 - fBuff.xOffset;  
    int16_t y1 = l->y1 - fBuff.yOffset;  
    int16_t y2 = l->y2 - fBuff.yOffset;  

    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1; 
    int dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1; 
    int err1 = (dx > dy ? dx : -dy) / 2, err2;

    /* Do some optimisations for straight lines and allow thickness option. */
    if (dx == 0) {
        while (y1 <= y2) {
            for (int i = 0; i < l->t; ++i) {
                B_POS(x1 + i, y1) = l->colo;
            }
            ++y1;
        }
    } else if (dy == 0) {
        while (x1 <= x2) {
            for (int i = 0; i < l->t; ++i) {
                B_POS(x1, y1 + i) = l->colo;
            }
            ++x1;
        }
    } else {
        B_POS(x1, y1) = l->colo;
        while (!(x1 == x2 && y1 == y2)) {
            B_POS(x1, y1) = l->colo;
            err2 = err1;
            if (err2 >-dx) { 
                err1 -= dy; 
                x1 += sx;
            }
            if (err2 < dy) { 
                err1 += dx; 
                y1 += sy;
            }
        }
    }
}

/* Helper macro for rendering triangle. */
#define swap(x, y) do {uint16_t tmp = x; x = y; y = tmp;} while (0)

/* Render a triangle to the frame buffer. */
static void render_triangle(struct triangle *t)
{
    int16_t x1 = t->x1 - fBuff.xOffset;  
    int16_t x2 = t->x2 - fBuff.xOffset;  
    int16_t x3 = t->x3 - fBuff.xOffset;  
    int16_t y1 = t->y1 - fBuff.yOffset;  
    int16_t y2 = t->y2 - fBuff.yOffset;  
    int16_t y3 = t->y3 - fBuff.yOffset;  

    if (y2 < y1) {
        swap(y1, y2);
        swap(x1, x2);
    }
    if (y3 < y2) {
        swap(y2, y3);
        swap(x2, x3);
    }
    if (y2 < y1) {
        swap(y1, y2);
        swap(x1, x2);
    }

    if (y1 == y3) {
        /* Straight line. */
        return;
    }

    int dx_a = abs(x3 - x1), sx_a = x1 < x3 ? 1 : -1; 
    int dy_a = abs(y3 - y1), sy_a = y1 < y3 ? 1 : -1; 
    int x1_a = x1, y1_a = y1;
    int err1_a = (dx_a > dy_a ? dx_a : -dy_a) / 2, err2_a;

    int dx_b = abs(x2 - x1), sx_b = x1 < x2 ? 1 : -1; 
    int dy_b = abs(y2 - y1), sy_b = y1 < y2 ? 1 : -1; 
    int x1_b = x1, y1_b = y1, x2_b = x2, y2_b = y2;
    int err1_b = (dx_b > dy_b ? dx_b : -dy_b) / 2, err2_b;

    int last_a = y1;
    int last_b = y1;
    int first = 1;

    B_POS(x1, y1) = t->b_colo;

repeat: 

    while (!(x1_b == x2_b && y1_b == y2_b)) {
        while ((last_b == y1_b) && !(x1_b == x2_b && y1_b == y2_b)) {
            B_POS(x1_b, y1_b) = t->b_colo;
            err2_b = err1_b;
            if (err2_b >-dx_b) { 
                err1_b -= dy_b; 
                x1_b += sx_b;
            }
            if (err2_b < dy_b) { 
                err1_b += dx_b; 
                y1_b += sy_b;
            }
        }

        while (last_a == y1_a) {
            B_POS(x1_a, y1_a) = t->b_colo;
            err2_a = err1_a;
            if (err2_a >-dx_a) { 
                err1_a -= dy_a; 
                x1_a += sx_a;
            }
            if (err2_a < dy_a) { 
                err1_a += dx_a; 
                y1_a += sy_a;
            }
        }

        last_a = y1_a;
        last_b = y1_b;
        
        if (x1_a == x1_b) {
            B_POS(x1_a, y1_a) = t->b_colo;
        } else if (t->fill) {
            int inc = x1_a < x1_b ? 1 : -1;
            for (int x = x1_a + inc; x != x1_b; x += inc) {
                B_POS(x, y1_a) = t->f_colo;
            }
        }
    }

    if (first) {
        dx_b = abs(x3 - x2), sx_b = x2 < x3 ? 1 : -1; 
        dy_b = abs(y3 - y2), sy_b = y2 < y3 ? 1 : -1; 
        x1_b = x2; 
        y1_b = y2;
        x2_b = x3;
        y2_b = y3;
        err1_b = (dx_b > dy_b ? dx_b : -dy_b) / 2;
        first = 0;
        goto repeat;
    }
}

/* Render text to the frame buffer. */
static void render_text(int16_t xPos, int16_t yPos, struct text *text)
{
    int len = strlen(text->text);
    int size = text->size;
//    int width = len * 5 * size + (len * size) - 1;
    int width = len * 5 * size + ((len - 1) * size);
    int x = xPos - width/2 - fBuff.xOffset;
    int y = yPos - (4 * size) - fBuff.yOffset;
    char *t = text->text;

    for (int i = 0; i < len; ++i) {
        int offset = (*t++ - 32) * 5;
        uint8_t b1 = font[offset];
        uint8_t b2 = font[offset + 1];
        uint8_t b3 = font[offset + 2];
        uint8_t b4 = font[offset + 3];
        uint8_t b5 = font[offset + 4];
        for (int j = 0; j < (8 * size); j += size) {
            if (b1 & 0x01) {
                for (int k = 0; k < size; ++k) {
                    for (int l = 0; l < size; ++l) {
                        B_POS(x + l, y + j + k) = text->colo;
                    }
                }
            }
            if (b2 & 0x01) {
                for (int k = 0; k < size; ++k) {
                    for (int l = 0; l < size; ++l) {
                        B_POS(x + (1 * size) + l, y + j + k) = text->colo;
                    }
                }
            }
            if (b3 & 0x01) {
                for (int k = 0; k < size; ++k) {
                    for (int l = 0; l < size; ++l) {
                        B_POS(x + (2 * size) + l, y + j + k) = text->colo;
                    }
                }
            }
            if (b4 & 0x01) {
                for (int k = 0; k < size; ++k) {
                    for (int l = 0; l < size; ++l) {
                        B_POS(x + (3 * size) + l, y + j + k) = text->colo;
                    }
                }
            }
            if (b5 & 0x01) {
                for (int k = 0; k < size; ++k) {
                    for (int l = 0; l < size; ++l) {
                        B_POS(x + (4 * size) + l, y + j + k) = text->colo;
                    }
                }
            }

            b1 >>= 1;
            b2 >>= 1;
            b3 >>= 1;
            b4 >>= 1;
            b5 >>= 1;
        }

        x += (6 * size);
    }
}

/* Renders a small 10x12 symbol. */
static void render_symbol(uint16_t xPos, uint16_t yPos, struct symb *symb)
{
    int x = xPos - fBuff.xOffset;
    int y = yPos - fBuff.yOffset;
    uint16_t const *s = symb->sym;

    for (int i = 0; i < 12; ++i) {
        uint16_t data = s[i];
        for (int j = 9; j >= 0; --j) {
            if (data & 0x01) {
                B_POS(x + j, y + i) = symb->colo;
            }
            data >>= 1;
        }
    }
}

/* Render Filled Circle */
static void render_circle(uint16_t xPos, uint16_t yPos, uint16_t colo, uint16_t pcolo, int16_t radius)
{
    int16_t x = radius; /* radius */
    int16_t x0 = xPos - fBuff.xOffset; /* Center Point*/
    int16_t y0 = yPos - fBuff.yOffset; /* Bottom of Buffer */
    int16_t y = 0;
    int16_t radiusError = 0;

    while(x >= y) {
	
	render_line_direct(x+x0, -x+x0, y+y0, y+y0, colo);
	render_line_direct(y+x0, -y+x0, x+y0, x+y0, colo);
	render_line_direct(-y+x0, y+x0, -x+y0, -x+y0, colo);
	render_line_direct(-x+x0, x+x0, -y+y0, -y+y0, colo);
	
	B_POS(x + x0, y + y0) = pcolo;
        B_POS(y + x0, x + y0) = pcolo;
        B_POS(-x + x0, y + y0) = pcolo;
        B_POS(-y + x0, x + y0) = pcolo;
        B_POS(-x + x0, -y + y0) = pcolo;
        B_POS(-y + x0, -x + y0) = pcolo;
        B_POS(x + x0, -y + y0) = pcolo;
        B_POS(y + x0, -x + y0) = pcolo;


        y += 1;

        if(radiusError < 0)
        {
            radiusError += 2 * y + 1;
        } else {
            x = x - 1;
            radiusError += 2 * (y-x) + 1;
        }
    }

}

/* Render Filled Circle */
static void render_scircle(uint16_t xPos, uint16_t yPos, uint16_t colo,  int16_t radius)
{
    int16_t x = radius; /* radius */
    int16_t x0 = xPos - fBuff.xOffset; /* Center Point*/
    int16_t y0 = yPos - fBuff.yOffset; /* Bottom of Buffer */
    int16_t y = 0;
    int16_t radiusError = 0;

    while(x >= y) {
	
	render_line_direct(-x+x0, x+x0, -y+y0, -y+y0, colo);
	render_line_direct(-y+x0, y+x0, -x+y0, -x+y0, colo);



        B_POS(x + x0, -y + y0) = colo;
        B_POS(y + x0, -x + y0) = colo;
        B_POS(-x + x0, -y + y0) = colo;
        B_POS(-y + x0, -x + y0) = colo;

        y += 1;

        if(radiusError < 0)
        {
            radiusError += 2 * y + 1;
        } else {
            x = x - 1;
            radiusError += 2 * (y-x) + 1;
        }
    }

}

/* Create the Semi-circle for depicting the odometer */
static void render_semi_circle(uint16_t xPos, uint16_t yPos, struct circ *circ)
{
    int16_t x = 51; /* radius */
    int16_t xb = 49;
    int16_t x0 = xPos - fBuff.xOffset; /* Center Point*/
    int16_t y0 = yPos - fBuff.yOffset; /* Bottom of Buffer */
    int16_t y = 0;
    int16_t radiusError = 0;
    struct line l = { .x1 = xPos, .y1 = yPos, 
            .x2 = xPos - xEndPoints[circ->speed], 
            .y2 = yPos -yEndPoints[circ->speed], .colo = circ->colo };

    struct triangle t = { .x1 = xPos - 2, .y1 = yPos-2, .x2 = xPos + 2, .y2 = yPos-2, 
            .x3 = xPos - xEndPoints[circ->speed], 
            .y3 = yPos -yEndPoints[circ->speed], 
            .b_colo = circ->colo, .f_colo = circ->pcolo, .fill = 1 };

    if (circ->speed < 30 || circ->speed > 90) {
        t.x1 = xPos;
        t.x2 = xPos;
        t.y1 = yPos + 2 -2;
        t.y2 = yPos -2 -2;
    }


    render_scircle(xPos, yPos, circ->colo,  50);
    render_scircle(xPos, yPos, BACKGROUND_COLOUR,  47);

    struct text txt = { .text = "60", .colo=BUTTON_TEXT_COLOUR, .size=1};
    render_text(xPos, yPos-40, &txt);

    txt.text = "120";
    render_text(xPos+36, yPos-2, &txt);
    txt.text = "0";
    render_text(xPos-42, yPos-2, &txt);
    txt.text = "30";
    render_text(xPos-28, yPos-28, &txt);
    txt.text = "90";
    render_text(xPos+28, yPos-28, &txt);

    render_triangle(&t);
    render_circle(xPos, yPos-2, circ->pcolo, circ->colo, 5);

    //render_line(&l);
    //render_line_direct(x0 - 3, xPos - xdial[circ->speed] - 3, y0 + 2, yPos - ydial[circ->speed] + 2, circ->colo);
}

static void render_line_direct(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t colo)
{
    //x2 -= fBuff.xOffset;  
    //y2 -= fBuff.yOffset;  

    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1; 
    int dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1; 
    int err1 = (dx > dy ? dx : -dy) / 2, err2;

    B_POS(x1, y1) = colo;
    while (!(x1 == x2 && y1 == y2)) {
	B_POS(x1, y1) = colo;
	err2 = err1;
	if (err2 >-dx) { 
	    err1 -= dy; 
	    x1 += sx;
	}
	if (err2 < dy) { 
	    err1 += dx; 
	    y1 += sy;
	}
    }
}
