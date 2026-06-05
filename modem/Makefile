# APRS Software Modem Makefile
CC      := gcc
CFLAGS  := -std=gnu99 -Wall -Wextra -O2 -funsigned-char
INCLUDE := -I. -I../engine/include -I../engine/src
TARGET  := aprs-modem

SOURCES := modem_main.c wav.c ax25.c afsk.c afsk_demod.c kiss_tnc.c

# Platform-specific libraries
ifeq ($(OS),Windows_NT)
    LIBS := ../engine/build/libaprs_engine.a -lm -lws2_32
else
    LIBS := ../engine/build/libaprs_engine.a -lm
endif

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCES) modem.h kiss_tnc.h
	$(CC) $(CFLAGS) $(INCLUDE) $(SOURCES) $(LIBS) -o $@
	@echo "Build OK: ./$(TARGET)"

test: $(TARGET)
	@echo "=== TX test: encode beacon ==="
	./$(TARGET) tx "SP3ABC-7>APRS,WIDE1-1:!5210.12N/01655.22E-Modem test" /tmp/test_beacon.wav
	@echo ""
	@echo "=== RX test: decode beacon ==="
	./$(TARGET) rx /tmp/test_beacon.wav
	@echo ""
	@echo "=== File sizes ==="
	@ls -la /tmp/test_beacon.wav

clean:
	rm -f $(TARGET)
