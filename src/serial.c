/*
 * A Cross platform serial port communication library
 *
 * Copyright (c) 2012, Tuomas Virtanen
 * license: MIT License. Please read LICENSE for more information.
*/

#include "serial.h"

#ifdef LINUX
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#endif

#include <stdio.h>
#include <string.h>
#include <malloc.h>

char error_str[256];

char* serial_get_error_str() {
    return error_str;
}

#ifdef LINUX
void print_linux_error() {
    sprintf(error_str, "Error %d: %s\n", errno, strerror(errno));
}
#else
void print_windows_error() {
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  error_str,
                  256,
                  NULL);
}
#endif

/**
  * Read serial port.
  * @param port A Valid serial_port object
  * @param buffer A Buffer to read into
  * @param len Amount of bytes to read
  * @return -1 on error, 0 or larger in success (read bytes).
  */
int serial_read(serial_port *port, char* buffer, int len) {
    int got = 0;
#ifdef LINUX
    got = read(port->handle, buffer, len);
    if(got < 0) {
        print_linux_error();
        got = -1;
    }
#else
    if(!ReadFile(port->handle, buffer, len, (PDWORD)&got, 0)) {
        print_windows_error();
        got = -1;
    }
#endif
    return got;
}

/**
  * Writes to serial port. Blocks!
  * @param port A Valid serial_port object
  * @param buffer Buffer to write
  * @param len Amount of bytes to write
  * @return -1 on error, 0 or larger on success (written bytes).
  */
int serial_write(serial_port *port, const char* buffer, int len) {
    int wrote = 0;
#ifdef LINUX
    wrote = write(port->handle, buffer, len);
    if(wrote < 0) {
        print_linux_error();
        wrote = -1;
    }
#else
    if(!WriteFile(port->handle, buffer, len, (PDWORD)&wrote, 0)) {
        print_windows_error();
        wrote = -1;
    }
#endif
    return wrote;
}

/**
  * Opens the serial port
  * @param settings Filled SerialSettings struct
  * @return serial_port object, or 0 in case of failure.
  */
serial_port* serial_open(const char* device, int speed) {
    serial_port *port;

#ifdef LINUX
    // Open linux serial port
    int fd = open(device, O_RDWR|O_NOCTTY|O_NDELAY);
    struct termios tio;
    if(fd < 0) {
        print_linux_error();
        return 0;
    }

    // Select speed
    speed_t spd = 0;
    switch(speed) {
        case SERIAL_1200: spd = B1200; break;
        case SERIAL_2400: spd = B2400; break;
        case SERIAL_4800: spd = B4800; break;
        case SERIAL_9600: spd = B9600; break;
        case SERIAL_19200: spd = B19200; break;
        case SERIAL_38400: spd = B38400; break;
#ifdef B5600
        case SERIAL_56000:  spd = B56000;  break;
#endif
        case SERIAL_57600: spd = B57600; break;
        case SERIAL_115200: spd = B115200; break;
#ifdef B128000
        case SERIAL_128000:  spd = B128000;  break;
#endif
        case SERIAL_230400: spd = B230400; break;
#ifdef B256000
        case SERIAL_256000:  spd = B256000;  break;
#endif
        default:
            sprintf(error_str, "Speed not supported!");
            return 0;
    }

    // Get port attributes
    if(tcgetattr(fd, &tio) != 0) {
        print_linux_error();
        return 0;
    }

    // Set flags ...
    tio.c_cflag &= ~(PARENB|CSTOPB|CSIZE|CRTSCTS); // No flow control
    tio.c_cflag |= CS8|CREAD|CLOCAL; // 8bits, read on, ignore ctrl lines
    tio.c_lflag = 0;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 0;

    // Attempt to set line speed for input & output
    if(cfsetispeed(&tio, spd) != 0) {
        print_linux_error();
        return 0;
    }
    if(cfsetospeed(&tio, spd) != 0) {
        print_linux_error();
        return 0;
    }

    // Attempt to set configuration
    if(tcsetattr(fd, TCSANOW, &tio) != 0) {
        print_linux_error();
        return 0;
    }
#else
    // Open windows serial port
    HANDLE wsid = CreateFile(device,
                             GENERIC_READ|GENERIC_WRITE,
                             0,
                             0,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             0);
    if(wsid == INVALID_HANDLE_VALUE) {
        print_windows_error();
        return 0;
    }

    // Get params
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(wsid, &dcbSerialParams)) {
        print_windows_error();
        return 0;
    }

    // Set new params. Remember to disable flow control etc.
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fOutxCtsFlow = FALSE;
    dcbSerialParams.fOutxDsrFlow = FALSE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;

    // Select speed
    switch(speed) {
        case SERIAL_1200: dcbSerialParams.BaudRate = CBR_1200; break;
        case SERIAL_2400: dcbSerialParams.BaudRate = CBR_2400; break;
        case SERIAL_4800: dcbSerialParams.BaudRate = CBR_4800; break;
        case SERIAL_9600: dcbSerialParams.BaudRate = CBR_9600; break;
        case SERIAL_14400: dcbSerialParams.BaudRate = CBR_14400; break;
        case SERIAL_19200: dcbSerialParams.BaudRate = CBR_19200; break;
        case SERIAL_38400: dcbSerialParams.BaudRate = CBR_38400; break;
        case SERIAL_56000: dcbSerialParams.BaudRate = CBR_56000; break;
        case SERIAL_57600: dcbSerialParams.BaudRate = CBR_57600; break;
        case SERIAL_115200: dcbSerialParams.BaudRate = CBR_115200; break;
        case SERIAL_128000: dcbSerialParams.BaudRate = CBR_128000; break;
        case SERIAL_256000: dcbSerialParams.BaudRate = CBR_256000; break;
        default:
            sprintf(error_str, "Speed not supported!");
            return 0;
    }

    // Set serial port settings
    if(!SetCommState(wsid, &dcbSerialParams)) {
        print_windows_error();
        return 0;
    }

    // Set serial port timeouts
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    // Set timeouts
    if(!SetCommTimeouts(wsid, &timeouts)) {
        print_windows_error();
        return 0;
    }
#endif

    // Reserve serial port stuff
    port = malloc(sizeof(serial_port));
#ifdef LINUX
    port->handle = fd;
#else
    port->handle = wsid;
#endif
    port->ok = 1;
    return port;
}

/**
  * Closes the serial port
  * @param port serial_port object to close.
  */
void serial_close(serial_port *port) {
    if(port==0) return;

    port->ok = 0;
#ifdef LINUX
    close(port->handle);
#else
    CloseHandle(port->handle);
#endif
    free(port);
}
