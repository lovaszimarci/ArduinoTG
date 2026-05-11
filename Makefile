MCU = atmega328p
F_CPU = 16000000UL
PORT = /dev/ttyACM0
PROGRAMMER = arduino
BAUD = 115200

CC = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS = -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU) -Wall
TARGET = Timegrapher

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET).hex

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

upload: $(TARGET).hex
	avrdude -c $(PROGRAMMER) -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$<

clean:
	rm -f *.o *.elf *.hex
