HA = includes.h F15x22.h app.h aprs-rx.h aprs.h bigsym.h config.h digits.h \
     eeprom.h f12x16.h f15x15.h f19x23.h f20x23.h f23x28.h f7x10.h f8x10.h \
     f8x11.h f8x8.h f9x14.h globals.h gps.h graphics.h keil.h peripherial.h \
     s65lib.h serial.h sta_manager.h sym_mono.h timer.h znaki.h \
     display_backend.h input.h frame_io.h
HH = includes.h
CC = gcc
CFLAGS = -funsigned-char -Wall -Wextra -std=c99

OBJS = globals.o app.o aprs-rx.o config.o eeprom.o gps.o graphics.o \
       peripherial.o serial.o sta_manager.o demo.o timer.o s65lib.o \
       display.o input.o input_backend_x11.o \
       frame_io.o frame_io_simulated.o frame_io_file.o frame_io_udp.o

aprs:		$(OBJS)
		$(CC) -o aprs $(CFLAGS) $(OBJS) -lX11 -lm

clean:
		rm -f aprs *.o

globals.o: 	globals.c $(HH)
		$(CC) -c $(CFLAGS) globals.c

app.o:		app.c $(HH)
		$(CC) -c $(CFLAGS) app.c

aprs-rx.o:	aprs-rx.c $(HH)
		$(CC) -c $(CFLAGS) aprs-rx.c

config.o:	config.c $(HH)
		$(CC) -c $(CFLAGS) config.c

eeprom.o:	eeprom.c $(HH)
		$(CC) -c $(CFLAGS) eeprom.c

gps.o:		gps.c $(HH)
		$(CC) -c $(CFLAGS) gps.c

graphics.o:	graphics.c $(HH)
		$(CC) -c $(CFLAGS) graphics.c

peripherial.o:	peripherial.c $(HH)
		$(CC) -c $(CFLAGS) peripherial.c

serial.o:	serial.c $(HH)
		$(CC) -c $(CFLAGS) serial.c

sta_manager.o:	sta_manager.c $(HH)
		$(CC) -c $(CFLAGS) sta_manager.c

demo.o:		demo.c $(HH)
		$(CC) -c $(CFLAGS) demo.c

timer.o:	timer.c $(HH)
		$(CC) -c $(CFLAGS) timer.c

s65lib.o:	s65lib.c $(HH)
		$(CC) -c $(CFLAGS) s65lib.c

display.o:	display.c $(HH)
		$(CC) -c $(CFLAGS) display.c

input.o:	input.c $(HH)
		$(CC) -c $(CFLAGS) input.c

input_backend_x11.o: input_backend_x11.c $(HH)
		$(CC) -c $(CFLAGS) input_backend_x11.c

frame_io.o:	frame_io.c $(HH)
		$(CC) -c $(CFLAGS) frame_io.c

frame_io_simulated.o: frame_io_simulated.c $(HH)
		$(CC) -c $(CFLAGS) frame_io_simulated.c

frame_io_file.o: frame_io_file.c $(HH)
		$(CC) -c $(CFLAGS) frame_io_file.c

frame_io_udp.o: frame_io_udp.c $(HH)
		$(CC) -c $(CFLAGS) frame_io_udp.c
