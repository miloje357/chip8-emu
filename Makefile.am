bin_PROGRAMS = chip8_emu
chip8_emu_SOURCES = \
	src/main.c\
	src/chip8.c\
	src/debugger.c\
	src/graphics.c\
	include/chip8.h\
	include/debugger.h\
	include/graphics.h
chip8_emu_CFLAGS = -g -Wall -Werror -O3\
		    -I$(top_srcdir)/include\
		    -lncurses

CLEANFILES = config.log config.status
MAINTAINERCLEANFILES = aclocal.m4 configure Makefile.in
