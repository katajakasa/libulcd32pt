/*
 * A Cross platform serial port communication library
 *
 * Copyright (c) 2012, Tuomas Virtanen
 * license: MIT License. Please read LICENSE for more information.
*/

#ifndef __SERIAL_H
#define __SERIAL_H

#ifndef LINUX
#include <windows.h>
#endif

enum SERIAL_SPEED {
    SERIAL_1200,
    SERIAL_2400,
    SERIAL_4800,
    SERIAL_9600,
    SERIAL_14400,
    SERIAL_19200,
    SERIAL_38400,
    SERIAL_56000,
    SERIAL_57600,
    SERIAL_115200,
    SERIAL_128000,
    SERIAL_230400,
    SERIAL_256000,
};

typedef struct serial_port {
    int ok;
#ifdef LINUX
    int handle;
#else
    HANDLE handle;
#endif
} serial_port;

char* serial_get_error_str();
serial_port* serial_open(const char* port, int speed);
void serial_close(serial_port *port);
int serial_read(serial_port *port, char* buffer, int len);
int serial_write(serial_port *port, const char* buffer, int len);

#endif // __SERIAL_H
