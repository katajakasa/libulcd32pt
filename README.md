libulcd32pt
===========

About
-----
A simple library for running uLCD-32PT LCD touch screen. Note: requires library from 
http://www.teuniz.net/RS-232/

License
-------
MIT. Read LICENSE for more detailed information.

Example
-------

    #include <stdio.h>
    #include <ulcd_driver.h>

    int main(int argc, char** argv) {
        ulcd_dev *dev;

        // Initialize LCD on com port 3
        dev = ulcd_init(2);
        if(!dev) {
            printf("Error while initializing display: %s\n", ulcd_get_error_str());
            return 0;
        }

        // Print some information about the display
        printf("Size: %ix%i\n", dev->w, dev->h);
        printf("Hardware version: %i\n", dev->hw_ver);
        printf("Software version: %i\n", dev->sw_ver);
        printf("LCD Type: %i (%s)\n", dev->type, dev->name);

        // Clear screen
        if(!ulcd_clear(dev)) {
            printf("Error: %s\n", ulcd_get_error_str());
        }

        // Try to play stuff from the MMC
        ulcd_set_volume(dev, ULCD_VOLUME_MAX);
        if(!ulcd_audio_play(dev, "ONLINE.WAV")) {
            printf("Error: %s\n", ulcd_get_error_str());
        }
        
        // Test drawing
        ulcd_pen_style(dev, ULCD_PEN_WIREFRAME);
        ulcd_draw_text(dev, "OMG!", 0, 200, 2, alloc_color(0.9, 0.9, 0.9));
        ulcd_draw_text(dev, "It works!", 0, 220, 2, alloc_color(0.6, 0.8, 0.3));
        ulcd_draw_rect(dev, 18, 18, 128, 64, alloc_color(0.5, 1.0, 1.0));

        // Test events
        int run = 1;
        ulcd_event ev;
        while(run) {
            ulcd_get_event(dev, &ev);
            if(ev.type != 0) {
                printf("Event: %i,%i = %i\n", ev.x, ev.y, ev.type);
            }
        }

        // Close device
        ulcd_close(dev);
        return 0;
    }
