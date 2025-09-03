# Project name
SOURCE          = ship.c
ADDITIONAL      = drivers/dtc.o drivers/eeproml.o drivers/eepromd.o
# Get base name so we can create .elf file
NAME            = $(basename $(SOURCE))
# MSP430 MCU to compile for
CPU             = msp430g2553
# Optimisation level
OPTIMIZATION    = -O0
# Extra variables
CFLAGS          = -mmcu=$(CPU) $(OPTIMIZATION) -std=c99 -Wall -g -fomit-frame-pointer
# Libemb library link flags
LIBEMB          = -lconio -lserial

# Build and link executable
$(NAME).elf: $(SOURCE) $(ADDITIONAL)
	msp430-gcc $(CFLAGS) -o $@ $(SOURCE) $(ADDITIONAL) $(LIBEMB)
	msp430-objdump -D $(NAME).elf > $(NAME).hex

# Flash to board with mspdebug
flash: $(NAME).elf
	mspdebug tilib "prog $(NAME).elf"

# Erase board
erase:
	mspdebug tilib erase

galaxy:
	mspdebug tilib "prog galaxy.elf"

# Clean up temporary files
clean:
	rm -f $(NAME).elf $(NAME).hex

# Remote debug board
debug: $(NAME).elf
	( mspdebug tilib "gdb" 1>/dev/null & ); msp430-gdb $(NAME).elf -ex "target remote :2000"
