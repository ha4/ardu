
## General Flags
PROJECT = solder5v
TARGET = $(PROJECT).elf
OP_MHZ = 8
CC = avr-gcc
CPP = avr-g++
CFLAGS = -Wall
MCU = attiny45
PARTNO = t45
PROGRAMMER=usbtiny
PORT=usb
AVRDUDE_BAUD = -b 19200
RMRF=del
#RMRF=rm -rf

MKFILES = Makefile fncs.h config.h
CFLAGS += -mcall-prologues

##General options
#ifeq ($(WITH_LCD_ST7565),1327)
#CFLAGS += -DLCD_ST_TYPE=1327
#endif

CFLAGS += -DF_CPU=$(OP_MHZ)000000UL
CFLAGS += -DF_CPU_HZ=$(OP_MHZ)000000
CFLAGS += -DMHZ_CPU=$(OP_MHZ)

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS += $(COMMON)
CFLAGS += -gdwarf-2 -std=gnu99 -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
# CFLAGS += -maccumulate-args
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,--relax,-Map=$(PROJECT).map

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature
#HEX_FLASH_FLAGS += -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings



## Objects that must be built in order to link
OBJECTS = main.o ser.o adc.o timer.o
#disp.o crcx.o
#ifeq ($(WITH_LCD_ST7565),1)
#OBJECTS += lcd-draw.o
#endif
#ifeq ($(WITH_SamplingADC),1)
#CFLAGS += -DSamplingADC
#OBJECTS += samplingADC.o sampling_cap.o sampling_lc.o sampling_xtal.o
#endif



all: $(TARGET) $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss size

##Compile dependency
ReadADC.o: ReadADC.S $(MKFILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

main.o: main.c $(MKFILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

adc.o: adc.c $(MKFILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

timer.o: timer.c $(MKFILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

crcx.o: crcx.c $(MKFILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

disp.o: disp.c $(MKFILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ser.o: ser.c $(MKFILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

hostproto.o: hostproto.c $(MKFILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	$(AVR_TOOL/PATH)avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@echo $(OP_MHZ) MHz operation configured.
	@$(AVR_TOOL/PATH)avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: size clean steril fuses fuses-crystal fuses-crystal-lp erase upload program flash eeprom
clean:
	-$(RMRF) $(OBJECTS) $(PROJECT).elf dep\* $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss $(PROJECT).map
steril:
	$(MAKE)
	-$(RMRF) $(OBJECTS) $(PROJECT).elf dep\* $(PROJECT).lss $(PROJECT).map

flash:
	avrdude -v -c $(PROGRAMMER) $(AVRDUDE_BAUD) -p $(PARTNO) -P $(PORT) -U flash:w:./$(PROJECT).hex:a 
#defl: efuse:0xFF(noSelfPgm) hfuse:0xDF(noRstDis,noDWen,SPIen,noWD,noEEsv,BOD=7) lfuse:62(8MhzRC,Div8,noCkOut)
deviceinfo:
	avrdude -v -c $(PROGRAMMER) $(AVRDUDE_BAUD) -p $(PARTNO) -P $(PORT) -v
#lfuse:0xE2(8MHzRC,noDiv8,noCkOut)
devicefuse:
	avrdude -v -c $(PROGRAMMER) $(AVRDUDE_BAUD) -p $(PARTNO) -P $(PORT) -U lfuse:w:0xE2:m

