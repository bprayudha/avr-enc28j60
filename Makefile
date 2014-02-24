#Makefile
MCU=atmega168
F_CPU = 12500000
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-g -mmcu=$(MCU) -Wall -Wstrict-prototypes -mcall-prologues -DF_CPU=$(F_CPU)

PROGRAMMER=stk200
DEVICE=m168
TARGET=web_server

LIBDIR=lib
SRCDIR=sample

all:
	$(CC) $(CFLAGS) -Os -c $(LIBDIR)/enc28j60.c
	$(CC) $(CFLAGS) -Os -c $(LIBDIR)/network.c
	$(CC) $(CFLAGS) -Os -c $(SRCDIR)/main.c
	$(CC) $(CFLAGS) -o web_server.out -Wl,-Map,web_server.map main.o network.o enc28j60.o
	mkdir bin
	$(OBJCOPY) -R .eeprom -O ihex web_server.out bin/web_server.hex
	rm -f *.map *.o *.out

clean:
	rm -rf bin
	
load:
	avrdude -c$(PROGRAMMER) -p$(DEVICE) -U flash:w:bin/$(TARGET).hex