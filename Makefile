# FROM https://www.avrfreaks.net/forum/understanding-fcpu
BIN=vpok861
OBJS=AVRVpokBetScreen.o AVRVpokStrings.o AVRVpokTables.o main.o LCD20x4-AVRAsm.o

CC=avr-gcc
OBJCOPY=avr-objcopy
# F_CPU is frequency; dunno if it's important
MCU=attiny861
# -lc might help with eeprom_read_word and write not being found by linker,
# per https://www.avrfreaks.net/forum/eeprom-functions-have-moved-new-atmel-gcc
# didn't help, read more of that page

CFLAGS=-Os -DF_CPU=16000000UL -mmcu=${MCU}
PORT=/dev/ttyACM0

${BIN}.hex: ${BIN}.elf
	${OBJCOPY} -O ihex -R .eeprom $< $@

${BIN}.elf: ${OBJS}
	# sean adds -mmcu=${MCU} see if it ropes in device eeprom defs - seems to work!
	${CC} -mmcu=${MCU} -o $@ $^

# from https://mithatkonar.com/wiki/doku.php/microcontrollers/avr_makefile_template
# assembler
ASMFLAGS =-I. ${INC} -mmcu=${MCU}    \
	-x assembler-with-cpp
	#            \
	#-Wa
	#,-gstabs,-ahlms=${(firstword   \
	#$(<:.S=.lst) $(<.s=.lst)}

# object from asm
.s.o :
	${CC} ${ASMFLAGS} -c $< -o $@

install: ${BIN}.hex
	# FIX THIS
	avrdude -F -V -c arduino -p ATMEGA328P -P ${PORT} -b 115200 -U flash:w:$<

clean:
	rm -f ${BIN}.elf ${BIN}.hex ${OBJS}
