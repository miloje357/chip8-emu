bin_PROGRAMS = chip8_emu chip8_dasm

chip8_emu_SOURCES = \
	src/main.c\
	src/chip8.c\
	src/debugger.c\
	src/game_graphics.c\
	src/tui.c\
	src/dasm.c\
	include/chip8.h\
	include/debugger.h\
	include/game_graphics.h\
	include/tui.h\
	include/dasm.h
chip8_emu_CFLAGS = -g -Wall -Werror -O3\
		    -I$(top_srcdir)/include\
		    -lncurses

chip8_dasm_SOURCES = \
	src/dasm.c\
	src/dasm_main.c\
	include/dasm.h\
	include/chip8.h
chip8_dasm_CFLAGS = -g -Wall -Werror -O3\
		    -I$(top_srcdir)/include

CLEANFILES = config.log config.status
MAINTAINERCLEANFILES = aclocal.m4 configure Makefile.in
