all: sinus.elf

sinus.elf: ../../sinus.h

MCU = attiny85
F_CPU = 8000000

PROGRAMMER = usbasp-clone # Broke ass

SRC = sinus.c
OBJ = $(SRC:.c=.o)
LST = $(SRC:.c=.lst)

CC := avr-gcc
OBJCOPY := avr-objcopy
SIZE := avr-size

CFLAGS = -DF_CPU=$(F_CPU)UL
CFLAGS += -Os
CFLAGS += -mmcu=$(MCU)
CFLAGS += -std=gnu99
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -flto -ffunction-sections -fdata-sections -Wl,--gc-sections
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += -Wa,-adhlns=$(<:.c=.lst)
CFLAGS += -I../../

LDFLAGS = -Wl,--gc-sections
LDFLAGS += -Wl,--print-gc-sections

MSG_COMPILING = Compiling:
MSG_LINKING = Linking:
MSG_FLASH = Preparing hex file:

# %.hex: %.elf
# 	@echo
# 	@echo $(MSG_FLASH) $@
# 	# $(OBJCOPY) -O ihex -j .text -j .data $< $@
# 	$(OBJCOPY) -O ihex -R ..eeprom $< $@

%.elf: $(OBJ)
	@echo
	@echo $(MSG_LINKING) $@
	$(CC) -mmcu=$(MCU) $(LDFLAGS) $^ --output $(@F)

%.o : %.c
	@echo $(MSG_COMPILING) $<
	$(CC) $(CFLAGS) -c $< -o $(@F)

clean:
	rm -f *.hex *.elf *.o *.lst
