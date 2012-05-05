# A simple Makefile for libulcd32pt

# Installation stuff
LIBNAME=libulcd32pt.so
INSTALL_LIBDIR=/usr/lib
INSTALL_INCDIR=/usr/include

# Internal paths
OBJDIR=obj
LIBDIR=lib
INCDIR=include

# Tools
CC=gcc
RM=rm -f
MKDIR=mkdir -p
MV=mv
CP=cp

# Stuff for compilation
FILES := \
    src/serial.c \
    src/ulcd_driver.c
    
CFLAGS=-I include/ -fPIC -O2 -Wall -W -DLINUX
LDFLAGS=-shared

all: 
	$(MKDIR) $(LIBDIR)
	$(MKDIR) $(OBJDIR)
	$(CC) $(CFLAGS) -c $(FILES)
	$(MV) *.o $(OBJDIR)/
	$(CC) $(LDFLAGS) -Wl,-soname,$(LIBNAME) -o $(LIBDIR)/$(LIBNAME) $(OBJDIR)/*.o
	@echo "Make done. To install, run make install."

clean:
	$(RM) $(OBJDIR)/*.o
	$(RM) $(LIBDIR)/*

install:
	$(CP) $(LIBDIR)/$(LIBNAME) $(INSTALL_LIBDIR)
	$(CP) $(INCDIR)/ulcd_driver.h $(INSTALL_INCDIR)/
	@echo "Install done."

uninstall:
	$(RM) $(INSTALL_LIBDIR)/$(LIBNAME)
	$(RM) $(INSTALL_INCDIR)/ulcd_driver.h
	@echo "Uninstalled."