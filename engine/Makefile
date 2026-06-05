# APRS Engine Makefile
# Builds the core engine as a static library

CC      := gcc
AR      := ar
CFLAGS  := -std=c99 -Wall -Wextra -O2 -funsigned-char
INCLUDE := -I./include -I./src
SRCDIR  := src
BUILD   := build
LIB     := $(BUILD)/libaprs_engine.a

SOURCES := $(SRCDIR)/event_bus.c \
           $(SRCDIR)/frame_ingest.c \
           $(SRCDIR)/station_manager.c \
           $(SRCDIR)/position_manager.c \
           $(SRCDIR)/beacon_manager.c \
           $(SRCDIR)/message_manager.c \
           $(SRCDIR)/digi_engine.c \
           $(SRCDIR)/gps_manager.c \
           $(SRCDIR)/stats_engine.c \
           $(SRCDIR)/raw_frame_editor.c \
           $(SRCDIR)/remote_control.c \
           $(SRCDIR)/trx_config.c \
           $(SRCDIR)/notification_mgr.c \
           $(SRCDIR)/config_persist.c \
           $(SRCDIR)/aprs_engine.c

OBJECTS := $(patsubst $(SRCDIR)/%.c,$(BUILD)/%.o,$(SOURCES))

.PHONY: all clean

all: $(BUILD) $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRCDIR)/%.c $(SRCDIR)/internal.h include/aprs_engine_v2.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(LIB): $(OBJECTS)
	$(AR) rcs $@ $^

clean:
	rm -rf $(BUILD)
