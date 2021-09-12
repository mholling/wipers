all: wipers.hex

%.elf: %.c
	avr-gcc -Wall -Os -mmcu=attiny85 $< -o $@

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

dump: wipers.elf
	avr-objdump -d $< | less

run: wipers.hex
	micronucleus --run $<

watch:
	ls *.c | entr -d make

clean:
	$(RM) *.hex *.elf
