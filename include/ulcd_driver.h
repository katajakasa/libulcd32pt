#ifndef DRIVER_H
#define DRIVER_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device stuff

typedef struct serial_port serial_port;

typedef struct {
    serial_port *port;
    char name[16];
    int type;
    int w,h;
    int hw_ver, sw_ver;
} ulcd_dev;

// Drawing stuff

enum PEN_STYLES {
    ULCD_PEN_SOLID = 0x00,
    ULCD_PEN_WIREFRAME = 0x01,
};

// Audio

enum {
    ULCD_VOLUME_MIN = 0x08,
    ULCD_VOLUME_MAX = 0x7F,
    ULCD_VOLUME_UP = 0xFD,
    ULCD_VOLUME_UP_8 = 0xFE,
    ULCD_VOLUME_DOWN = 0x03,
    ULCD_VOLUME_DOWN_8 = 0x01,
    ULCD_VOLUME_MUTE = 0x00,
    ULCD_VOLUME_UNMUTE = 0xFF,
};

// Events

enum EVENT_TYPES {
    ULCD_NO_ACTIVITY = 0,
    ULCD_TOUCH_PRESS,
    ULCD_TOUCH_RELEASE,
    ULCD_TOUCH_MOVING,
};

typedef struct {
    int16_t x,y;
    int16_t type;
} ulcd_event;

// Init and deinit functions

ulcd_dev* ulcd_init(const char* device);
void ulcd_close(ulcd_dev *dev);

// Utility stuff

char* ulcd_get_error_str();

// Panel management

int ulcd_clear(ulcd_dev *dev);
int ulcd_toggle_power(ulcd_dev *dev, int toggle);
int ulcd_toggle_backlight(ulcd_dev *dev, int toggle);

// Events

void ulcd_get_event(ulcd_dev *dev, ulcd_event *event);
void ulcd_wait_event(ulcd_dev *dev, ulcd_event *event);

// Audio

int ulcd_set_volume(ulcd_dev *dev, uint8_t volume);
int ulcd_audio_play(ulcd_dev *dev, const char* file);
int ulcd_audio_stop(ulcd_dev *dev);

// SD Card

int ulcd_sd_write(ulcd_dev *dev, const char *file, const char* data, int len);
int ulcd_sd_read(ulcd_dev *dev, const char *file, char* data, int read);
int ulcd_sd_list(ulcd_dev *dev, const char *filter, char *buffer, int buflen);
int ulcd_sd_erase(ulcd_dev *dev, const char *file);
int ulcd_sd_image_save(ulcd_dev *dev, const char *file, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
int ulcd_sd_image_load(ulcd_dev *dev, const char *file, uint16_t x, uint16_t y);

// Drawing

int ulcd_blit(ulcd_dev *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* data);
int ulcd_draw_pixel(ulcd_dev *dev, uint16_t x, uint16_t y, uint16_t color);
int ulcd_draw_ellipse(ulcd_dev *dev, uint16_t x, uint16_t y, uint16_t xrad, uint16_t yrad, uint16_t color);
int ulcd_draw_line(ulcd_dev *dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
int ulcd_draw_rect(ulcd_dev *dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
int ulcd_draw_circle(ulcd_dev *dev, uint16_t x, uint16_t y, uint16_t radius, uint16_t color);
int ulcd_draw_text(ulcd_dev *dev, const char* text, int x, int y, int font, uint16_t color);
int ulcd_pen_style(ulcd_dev *dev, int style);
uint16_t alloc_color(float r, float g, float b);

uint16_t ulcd_read_pixel(ulcd_dev *dev, uint16_t x, uint16_t y);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
