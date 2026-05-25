# Einstellung des Projekts
MCU        = atmega16
F_CPU      = 8000000UL
TARGET     = $(notdir $(CURDIR))

# Tools
CC         = avr-gcc
OBJCOPY    = avr-objcopy
AVRDUDE    = avrdude
PROGRAMMER = stk500v2
PORT       = /dev/tty.usbserial-110

CFLAGS     = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Iinc

# Dateien
SRC        = $(wildcard src/*.c)
OBJ        = $(SRC:src/%.c=build/%.o)

# Targets
all: build/$(TARGET).hex

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/$(TARGET).elf: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

build/$(TARGET).hex: build/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: build/$(TARGET).hex
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) -b 115200 -F -U flash:w:$<:i

clean:
	rm -rf build/

.PHONY: all flash clean