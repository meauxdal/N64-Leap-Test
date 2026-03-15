N64_INST = /n64_toolchain
PROG_NAME = vi_leap_test

# Pass the name into C code as a string macro
CFLAGS += -DAPP_TITLE=\"$(PROG_NAME)\"

all: $(PROG_NAME).z64

OBJS = main.o
$(PROG_NAME)_OBJS = main.o

include $(N64_INST)/include/n64.mk

$(PROG_NAME).elf: $(OBJS)

clean:
	rm -f *.o *.elf *.z64