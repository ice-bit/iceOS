OBJS = stdio.o string.o panic.o time.o
VER := $(shell git rev-parse --short HEAD)

CC = i686-elf-gcc # cross-compiler
CFLAGS = -m32 -DVULCAN_VERSION=$(VER) -fno-stack-protector -ffreestanding -Wall -Wextra -Werror -g -c


all:${OBJS}

%.o: %.c
	@$(CC) $(CFLAGS) $< -o $@
	@echo [CC] Compiling $<...