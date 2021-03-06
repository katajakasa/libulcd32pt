#include "ulcd_driver.h"
#include "serial.h"

#ifdef LINUX
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <malloc.h>
#include <stdio.h>
#include <string.h>

char errorstr[256];

// Helper functions for serial port stuff

unsigned char read_char(ulcd_dev *dev) {
    unsigned char c;
    while(serial_read(dev->port, (char*)&c, 1) <= 0) {
#ifdef LINUX
        usleep(1000);
#else
        Sleep(1);
#endif
    }
    return c;
}

void write_char(ulcd_dev *dev, unsigned char c) {
    serial_write(dev->port, (char*)&c, 1);
}

uint16_t read_word(ulcd_dev *dev) {
    uint16_t result;
    result = (read_char(dev) << 8) | read_char(dev);
    return result;
}

void write_word(ulcd_dev *dev, uint16_t word) {
    char buf[2];
    buf[0] = word >> 8;
    buf[1] = word & 0xFF;
    serial_write(dev->port, buf, 2);
}

int check_result(ulcd_dev *dev, const char* errtext) {
    if(read_char(dev) != 0x06) {
        memcpy(errorstr, errtext, strlen(errtext));
        return 0;
    }
    return 1;
}

// Helper functions for interpreting the display stuff

int get_res_by_code(unsigned char code) {
    switch(code) {
        case 0x22: return 220;
        case 0x24: return 240;
        case 0x28: return 128;
        case 0x32: return 320;
        case 0x60: return 160;
        case 0x64: return 64;
        case 0x76: return 176;
        case 0x96: return 96;
        default:
        return 0;
    }
    return 0;
}

void set_devname_by_type(ulcd_dev *dev) {
    switch(dev->type) {
        case 0x00: sprintf(dev->name, "micro-OLED"); return;
        case 0x01: sprintf(dev->name, "micro-LCD"); return;
        case 0x02: sprintf(dev->name, "micro-VGA"); return;
    }
    sprintf(dev->name, "Unknown device");
}

// The LCD control functions

ulcd_dev* ulcd_init(const char* device) {
    // Open device
    serial_port *ser = serial_open(device, SERIAL_115200);
    if(!ser) {
        sprintf(errorstr, "Error while opening serial port.");
        return 0;
    }

    // Clear buffer
    char tmp[4096];
    serial_read(ser, tmp, 4096);

    // Allocate memory
    ulcd_dev *dev = (ulcd_dev*)malloc(sizeof(ulcd_dev));
    dev->port = ser;
    memset(dev->name, 0, 16);

    // Init panel
    write_char(dev, 0x55);
    if(!check_result(dev, "Panel initialization failed.")) {
        free(dev);
        return 0;
    }

    // Version information request
    write_char(dev, 0x56);
    write_char(dev, 0x00);

    // Read version information
    dev->type = read_char(dev);
    dev->hw_ver = read_char(dev) - 6;
    dev->sw_ver = read_char(dev) - 6;
    dev->w = get_res_by_code(read_char(dev));
    dev->h = get_res_by_code(read_char(dev));
    set_devname_by_type(dev);

    // Set touch region
    write_char(dev, 0x59);
    write_char(dev, 0x05);
    write_char(dev, 0x02);
    if(!check_result(dev, "Touch region reset failed.")) {
        free(dev);
        return 0;
    }

    // Enable touch events
    write_char(dev, 0x59);
    write_char(dev, 0x05);
    write_char(dev, 0x00);
    if(!check_result(dev, "Enabling touch events failed.")) {
        free(dev);
        return 0;
    }

    // All done.
    return dev;
}

void ulcd_close(ulcd_dev *dev) {
    if(dev == 0) return;
    serial_close(dev->port);
    free(dev);
}

int ulcd_clear(ulcd_dev *dev) {
    write_char(dev, 0x45);
    return check_result(dev, "Clear screen failed.");
}

char* ulcd_get_error_str() {
    return errorstr;
}

int ulcd_toggle_power(ulcd_dev *dev, int toggle) {
    write_char(dev, 0x59);
    write_char(dev, 0x03);
    write_char(dev, (toggle > 0) ? 1 : 0);
    return check_result(dev, "Backlight toggling failed.");
}

int ulcd_toggle_backlight(ulcd_dev *dev, int toggle) {
    write_char(dev, 0x59);
    write_char(dev, 0x00);
    write_char(dev, (toggle > 0) ? 1 : 0);
    return check_result(dev, "Backlight toggling failed.");
}

void ulcd_get_event(ulcd_dev *dev, ulcd_event *event) {
    // Get type
    write_char(dev, 0x6F);
    write_char(dev, 0x04);
    event->type = read_word(dev);
    read_word(dev);

    // If type is valid, get coords
    if(event->type > 0) {
        write_char(dev, 0x6F);
        write_char(dev, 0x05);
        event->x = read_word(dev);
        event->y = read_word(dev);
    } else {
        event->x = -1;
        event->y = -1;
    }
}

void ulcd_wait_event(ulcd_dev *dev, ulcd_event *event) {
    // Get coords
    write_char(dev, 0x6F);
    write_char(dev, 0x00);
    event->x = read_word(dev);
    event->y = read_word(dev);

    // Get type
    write_char(dev, 0x6F);
    write_char(dev, 0x04);
    event->type = read_word(dev);
    read_word(dev);
}

// Draw stuff

int ulcd_blit(ulcd_dev *dev,
              uint16_t x, uint16_t y,
              uint16_t w, uint16_t h,
              const char* data) {

    char buf[10];

    buf[0] = 0x49;
    buf[1] = x >> 8;
    buf[2] = x & 0xFF;
    buf[3] = y >> 8;
    buf[4] = y & 0xFF;
    buf[5] = w >> 8;
    buf[6] = w & 0xFF;
    buf[7] = h >> 8;
    buf[8] = h & 0xFF;
    buf[9] = 0x10;

    serial_write(dev->port, buf, 10);
    serial_write(dev->port, data, w*h*2);

    if(!check_result(dev, "Error while blitting.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_line(ulcd_dev *dev,
                   uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1,
                   uint16_t color) {

    char buf[11];

    buf[0] = 0x4C;
    buf[1] = x0 >> 8;
    buf[2] = x0 & 0xFF;
    buf[3] = y0 >> 8;
    buf[4] = y0 & 0xFF;
    buf[5] = x1 >> 8;
    buf[6] = x1 & 0xFF;
    buf[7] = y1 >> 8;
    buf[8] = y1 & 0xFF;
    buf[9] = color >> 8;
    buf[10] = color & 0xFF;

    serial_write(dev->port, buf, 11);
    if(!check_result(dev, "Error while drawing line.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_rect(ulcd_dev *dev,
                   uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1,
                   uint16_t color) {

    char buf[11];

    buf[0] = 0x72;
    buf[1] = x0 >> 8;
    buf[2] = x0 & 0xFF;
    buf[3] = y0 >> 8;
    buf[4] = y0 & 0xFF;
    buf[5] = x1 >> 8;
    buf[6] = x1 & 0xFF;
    buf[7] = y1 >> 8;
    buf[8] = y1 & 0xFF;
    buf[9] = color >> 8;
    buf[10] = color & 0xFF;

    serial_write(dev->port, buf, 11);
    if(!check_result(dev, "Error while drawing rectangle.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_circle(ulcd_dev *dev,
                     uint16_t x, uint16_t y,
                     uint16_t radius,
                     uint16_t color) {
    char buf[9];
    buf[0] = 0x43;
    buf[1] = x >> 8;
    buf[2] = x & 0xFF;
    buf[3] = y >> 8;
    buf[4] = y & 0xFF;
    buf[5] = radius >> 8;
    buf[6] = radius & 0xFF;
    buf[7] = color >> 8;
    buf[8] = color & 0xFF;
    serial_write(dev->port, buf, 9);
    if(!check_result(dev, "Error while drawing circle.")) {
        return 0;
    }
    return 1;
}

int ulcd_pen_style(ulcd_dev *dev, int style) {
    char buf[2];

    buf[0] = 0x70;
    buf[1] = style;

    serial_write(dev->port, buf, 2);
    if(!check_result(dev, "Pen style change failed.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_pixel(ulcd_dev *dev,
                    uint16_t x, uint16_t y,
                    uint16_t color) {
    char buf[7];
    buf[0] = 0x50;
    buf[1] = x >> 8;
    buf[2] = x & 0xFF;
    buf[3] = y >> 8;
    buf[4] = y & 0xFF;
    buf[5] = color >> 8;
    buf[6] = color & 0xFF;
    serial_write(dev->port, buf, 7);
    if(!check_result(dev, "Error while drawing pixel.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_ellipse(ulcd_dev *dev,
                      uint16_t x, uint16_t y,
                      uint16_t xrad, uint16_t yrad,
                      uint16_t color) {
    char buf[11];
    buf[0] = 0x65;
    buf[1] = x >> 8;
    buf[2] = x & 0xFF;
    buf[3] = y >> 8;
    buf[4] = y & 0xFF;
    buf[5] = xrad >> 8;
    buf[6] = xrad & 0xFF;
    buf[7] = yrad >> 8;
    buf[8] = yrad & 0xFF;
    buf[9] = color >> 8;
    buf[10] = color & 0xFF;
    serial_write(dev->port, buf, 11);
    if(!check_result(dev, "Error while drawing pixel.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_text(ulcd_dev *dev,
                   const char* text,
                   int x, int y,
                   int font,
                   uint16_t color) {

    // Init
    int textlen = strlen(text);
    char buf[10];

    // Commands
    buf[0] = 0x53;
    buf[1] = x >> 8;
    buf[2] = x & 0xFF;
    buf[3] = y >> 8;
    buf[4] = y & 0xFF;
    buf[5] = font & 0xFF;
    buf[6] = color >> 8;
    buf[7] = color & 0xFF;
    buf[8] = 0x01;
    buf[9] = 0x01;

    // Send data
    serial_write(dev->port, buf, 10);
    serial_write(dev->port, text, textlen);
    write_char(dev, 0x00);

    // Check results
    if(!check_result(dev, "Text drawing failed.")) {
        return 0;
    }
    return 1;
}

uint16_t ulcd_read_pixel(ulcd_dev *dev, uint16_t x, uint16_t y) {
    char buf[5];
    buf[0] = 0x52;
    buf[1] = x >> 8;
    buf[2] = x & 0xFF;
    buf[3] = y >> 8;
    buf[4] = y & 0xFF;
    serial_write(dev->port, buf, 11);
    return read_word(dev);
}

// Audio

int ulcd_set_volume(ulcd_dev *dev, uint8_t volume) {
    // Commands
    write_char(dev, 0x76);
    write_char(dev, volume);

    // Check results
    if(!check_result(dev, "Sound volume setting failed.")) {
        return 0;
    }
    return 1;
}

int ulcd_audio_play(ulcd_dev *dev, const char* file) {
    // Commands
    write_char(dev, 0x40);
    write_char(dev, 0x6C);
    write_char(dev, 0x01);
    serial_write(dev->port, file, strlen(file));
    write_char(dev, 0x00);

    // Check results
    if(!check_result(dev, "Sound playback failed.")) {
        return 0;
    }
    return 1;
}

int ulcd_audio_stop(ulcd_dev *dev) {
    // Commands
    write_char(dev, 0x40);
    write_char(dev, 0x6C);
    write_char(dev, 0x02);
    write_char(dev, 0x00);

    // Check results
    if(!check_result(dev, "Sound playback stop failed.")) {
        return 0;
    }
    return 1;
}

// SD Card

int ulcd_sd_init(ulcd_dev *dev) {
    // Commands
    write_char(dev, 0x40);
    write_char(dev, 0x69);

    // Check results
    if(!check_result(dev, "Could not initialize SD card.")) {
        return 0;
    }
    return 1;
}

// TODO: Ugly, might want to revisit this ...
int ulcd_sd_list(ulcd_dev *dev, const char *filter, char *buffer, int buflen) {
    // Commands
    write_char(dev, 0x40);
    write_char(dev, 0x64);
    serial_write(dev->port, filter, strlen(filter));
    write_char(dev, 0x00);

    // Some vars
    int run = 1;
    int pos = 0;
    char last = 0;
    char in;

    // Get much data! MMMMmmmm.... daaaataaaaa....
    while(run) {
        if(pos > buflen) {
            sprintf(errorstr, "Directory listing too long.");
            break;
        }

        in = read_char(dev);
        if(in == 0x06 && last == 0) {
            return pos;
        }
        if(in == 0x06 && last == 0x0A) {
            return pos;
        }
        if(in == 0x15 && (last == 0x0A || last == 0)) {
            sprintf(errorstr, "Directory listing failed.");
            return pos;
        }
        if(in == 0x0A) {
            buffer[pos++] = ',';
            last = in;
            continue;
        }
        buffer[pos++] = in;
        last = in;
    }

    return pos;
}

int ulcd_sd_erase(ulcd_dev *dev, const char *file) {
    // Send command
    write_char(dev, 0x40);
    write_char(dev, 0x65);
    serial_write(dev->port, file, strlen(file));
    write_char(dev, 0x00);

    // Check results
    if(!check_result(dev, "File erasing failed.")) {
        return 0;
    }
    return 1;
}

// TODO: Implement this.
int ulcd_sd_write(ulcd_dev *dev, const char *file, const char *data, int len) {
    return 0;
}

// TODO: Implement this.
int ulcd_sd_read(ulcd_dev *dev, const char *file, char *data, int read) {
    return 0;
}

int ulcd_sd_image_save(ulcd_dev *dev, const char *file, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    // Commands
    char buf[10];
    buf[0] = 0x40;
    buf[1] = 0x64;
    buf[2] = x >> 8;
    buf[3] = x & 0xFF;
    buf[4] = y >> 8;
    buf[5] = y & 0xFF;
    buf[6] = w >> 8;
    buf[7] = w & 0xFF;
    buf[8] = h >> 8;
    buf[9] = h & 0xFF;
    serial_write(dev->port, buf, 10);
    serial_write(dev->port, file, strlen(file));
    write_char(dev, 0x00);

    // Check results
    if(!check_result(dev, "Image copy+save failed.")) {
        return 0;
    }
    return 1;
}

int ulcd_sd_image_load(ulcd_dev *dev, const char *file, uint16_t x, uint16_t y) {
    // Write commands
    write_char(dev, 0x40);
    write_char(dev, 0x6D);
    serial_write(dev->port, file, strlen(file));
    write_char(dev, 0x00);
    write_word(dev, x);
    write_word(dev, y);
    write_word(dev, 0);

    // Check results
    if(!check_result(dev, "Image load+show failed.")) {
        return 0;
    }
    return 1;
}

// Some utility stuff

float clamp(float v, float to) {
    if(v > to) {
        return to;
    }
    return v;
}

uint16_t alloc_color(float r, float g, float b) {
    uint16_t out;
    uint8_t _r = 31 * clamp(r, 1.0);
    uint8_t _g = 63 * clamp(g, 1.0);
    uint8_t _b = 31 * clamp(b, 1.0);
    out = (_r << 11) | (_g << 5) | (_b);
    return out;
}

