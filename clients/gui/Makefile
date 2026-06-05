# GUI Client Makefile
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2 -funsigned-char
INCLUDE := -I../../engine/include
LIB     := ../../engine/build/libaprs_engine.a
LDFLAGS := -lm

SOURCES := gui_main.c renderer.c
TARGET  := aprs_gui

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(SOURCES) $(LIB) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)
