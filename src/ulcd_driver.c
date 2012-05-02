#include "ulcd_driver.h"
#include "rs232.h"

#ifdef LINUX
#imclude <unistd.h>
#else
#include <windows.h>
#endif

char errorstr[256];

void sleep(int ms) {
#ifdef LINUX
    usleep(ms*1000);
#else
    Sleep(ms);
#endif
}

unsigned char readchar(int port) {
    unsigned char c;
    while(!PollComport(port, &c, 1)) {
        sleep(1);
    }
    return c;
}

int16_t read_word(ulcd_dev *dev) {
    int16_t v = readchar(dev->port) << 8;
    return v | readchar(dev->port);
}

int check_result(ulcd_dev *dev, const char* errtext) {
    if(readchar(dev->port) != 0x06) {
        sprintf(errorstr, errtext);
        return 0;
    }
    return 1;
}

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

ulcd_dev* ulcd_init(int portno) {
    // Open device
    if(OpenComport(portno, 115200)) {
        sprintf(errorstr, "Error while opening serial port.");
        return 0;
    }

    // Clear buffer
    unsigned char tmp[4096];
    PollComport(portno, tmp, 4096);

    // Allocate memory
    ulcd_dev *dev = (ulcd_dev*)malloc(sizeof(ulcd_dev));
    dev->port = portno;
    memset(dev->name, 0, 16);

    // Init panel
    SendByte(dev->port, 0x55);
    if(!check_result(dev, "Panel initialization failed.")) {
        free(dev);
        return 0;
    }

    // Version information request
    SendByte(dev->port, 0x56);
    SendByte(dev->port, 0x00);

    // Read version information
    dev->type = readchar(dev->port);
    dev->hw_ver = readchar(dev->port) - 6;
    dev->sw_ver = readchar(dev->port) - 6;
    dev->w = get_res_by_code(readchar(dev->port));
    dev->h = get_res_by_code(readchar(dev->port));
    set_devname_by_type(dev);

    // Set touch region
    SendByte(dev->port, 0x59);
    SendByte(dev->port, 0x05);
    SendByte(dev->port, 0x02);
    if(!check_result(dev, "Touch region reset failed.")) {
        free(dev);
        return 0;
    }

    // Enable touch events
    SendByte(dev->port, 0x59);
    SendByte(dev->port, 0x05);
    SendByte(dev->port, 0x00);
    if(!check_result(dev, "Enabling touch events failed.")) {
        free(dev);
        return 0;
    }

    // All done.
    return dev;
}

void ulcd_close(ulcd_dev *dev) {
    if(dev == 0) return;
    CloseComport(dev->port);
    free(dev);
}

int ulcd_clear(ulcd_dev *dev) {
    SendByte(dev->port, 0x45);
    return check_result(dev, "Clear screen failed.");
}

char* ulcd_get_error_str() {
    return errorstr;
}

int ulcd_toggle_power(ulcd_dev *dev, int toggle) {
    SendByte(dev->port, 0x59);
    SendByte(dev->port, 0x03);
    SendByte(dev->port, (toggle > 0) ? 1 : 0);
    return check_result(dev, "Backlight toggling failed.");
}

int ulcd_toggle_backlight(ulcd_dev *dev, int toggle) {
    SendByte(dev->port, 0x59);
    SendByte(dev->port, 0x00);
    SendByte(dev->port, (toggle > 0) ? 1 : 0);
    return check_result(dev, "Backlight toggling failed.");
}

void ulcd_get_event(ulcd_dev *dev, ulcd_event *event) {
    // Get type
    SendByte(dev->port, 0x6F);
    SendByte(dev->port, 0x04);
    event->type = read_word(dev);
    read_word(dev);

    // If type is valid, get coords
    if(event->type > 0) {
        SendByte(dev->port, 0x6F);
        SendByte(dev->port, 0x05);
        event->x = read_word(dev);
        event->y = read_word(dev);
    } else {
        event->x = -1;
        event->y = -1;
    }
}

void ulcd_wait_event(ulcd_dev *dev, ulcd_event *event) {
    // Get coords
    SendByte(dev->port, 0x6F);
    SendByte(dev->port, 0x00);
    event->x = read_word(dev);
    event->y = read_word(dev);

    // Get type
    SendByte(dev->port, 0x6F);
    SendByte(dev->port, 0x04);
    event->type = read_word(dev);
    read_word(dev);
}

// Write stuff to buffer for later rendering

int ulcd_blit_buf(unsigned char* buf,
                  uint16_t x, uint16_t y,
                  uint16_t w, uint16_t h,
                  const unsigned char* data) {
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
    memcpy(buf+10, data, w*h*2);
    return w*h*2 + 10;
}

int ulcd_draw_line_buf(unsigned char *buf,
                       uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1,
                       uint16_t color) {
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
    return 11;
}

int ulcd_draw_rect_buf(unsigned char *buf,
                       uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1,
                       uint16_t color) {
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
    return 11;
}

int ulcd_draw_circle_buf(unsigned char *buf,
                         uint16_t x, uint16_t y,
                         uint16_t radius,
                         uint16_t color) {
    buf[0] = 0x43;
    buf[1] = x >> 8;
    buf[2] = x & 0xFF;
    buf[3] = y >> 8;
    buf[4] = y & 0xFF;
    buf[5] = radius >> 8;
    buf[6] = radius & 0xFF;
    buf[7] = color >> 8;
    buf[8] = color & 0xFF;
    return 9;
}

int ulcd_pen_style_buf(unsigned char* buf, int style) {
    buf[0] = 0x70;
    buf[1] = style;
    return 2;
}

// Draw stuff

int ulcd_blit(ulcd_dev *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const unsigned char* data) {
    // Get command data and send it to panel.
    int sz = w*h*2 + 10;
    unsigned char buf[sz];
    ulcd_blit_buf(buf, x, y, w, h, data);
    SendBuf(dev->port, buf, sz);
    if(!check_result(dev, "Error while blitting.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_line(ulcd_dev *dev,
                   uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1,
                   uint16_t color) {

    // Get command data and send it to panel.
    unsigned char buf[11];
    ulcd_draw_line_buf(buf, x0, y0, x1, y1, color);
    SendBuf(dev->port, buf, 11);
    if(!check_result(dev, "Error while drawing line.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_rect(ulcd_dev *dev,
                   uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1,
                   uint16_t color) {

    // Get command data and send it to panel.
    unsigned char buf[11];
    ulcd_draw_rect_buf(buf, x0, y0, x1, y1, color);
    SendBuf(dev->port, buf, 11);
    if(!check_result(dev, "Error while drawing rectangle.")) {
        return 0;
    }
    return 1;
}

int ulcd_draw_circle(ulcd_dev *dev,
                     uint16_t x, uint16_t y,
                     uint16_t radius,
                     uint16_t color) {
    // Get command data and send it to panel.
    unsigned char buf[9];
    ulcd_draw_circle_buf(buf, x, y, radius, color);
    SendBuf(dev->port, buf, 9);
    if(!check_result(dev, "Error while drawing circle.")) {
        return 0;
    }
    return 1;
}

int ulcd_pen_style(ulcd_dev *dev, int style) {
    // Get command data and send it
    unsigned char buf[2];
    ulcd_pen_style_buf(buf, style);
    SendBuf(dev->port, buf, 2);
    if(!check_result(dev, "Pen style change failed.")) {
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
    uint8_t _r = 32 * clamp(r, 1.0);
    uint8_t _g = 64 * clamp(g, 1.0);
    uint8_t _b = 32 * clamp(b, 1.0);
    out = (_r << 11) | (_g << 6) | (_b);
    return out;
}

