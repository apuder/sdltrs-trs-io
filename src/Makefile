PROG	 = sdltrs

SRCS	+= blit.c
SRCS	+= debug.c
SRCS	+= dis.c
SRCS	+= error.c
SRCS	+= load_cmd.c
SRCS	+= load_hex.c
SRCS	+= main.c
SRCS	+= trs_cassette.c
SRCS	+= trs_chars.c
SRCS	+= trs_disk.c
SRCS	+= trs_hard.c
SRCS	+= trs_imp_exp.c
SRCS	+= trs_interrupt.c
SRCS	+= trs_io.c
SRCS	+= trs_memory.c
SRCS	+= trs_mkdisk.c
SRCS	+= trs_printer.c
SRCS	+= trs_rom1.c
SRCS	+= trs_rom3.c
SRCS	+= trs_rom4p.c
SRCS	+= trs_romesf.c
SRCS	+= trs_sdl_gui.c
SRCS	+= trs_sdl_interface.c
SRCS	+= trs_sdl_keyboard.c
SRCS	+= trs_state_save.c
SRCS	+= trs_stringy.c
SRCS	+= trs_uart.c
SRCS	+= z80.c
SRCS	+= PasteManager.c

OBJS	 = ${SRCS:.c=.o}

.PHONY: all bsd clean clean-win nox sdl sdl2 win32 win64 wsdl2

all:
	@echo "make (bsd|clean|clean-win|depend|nox|sdl|sdl2|win32|win64|wsdl2)"

bsd:
	make -f BSDMakefile

clean:
	rm -f ${OBJS} ${PROG} sdl2trs

clean-win:
	del *.o sdltrs.exe sdl2trs.exe sdl2trs64.exe

depend:
	makedepend -Y -- ${CFLAGS} -- ${SRCS} 2>&1 | \
		(egrep -v 'cannot find|not in' || true)

nox:	ENDIAN	 = $(shell echo "ab" | od -x | grep "6261" > /dev/null || echo "-Dbig_endian")
nox:	LIBS	 = $(shell sdl-config --libs) -lreadline
nox:	INCS	 = $(shell sdl-config --cflags)
nox:	MACROS	 = -DNOX -DREADLINE
nox:	${PROG}

sdl:	ENDIAN	 = $(shell echo "ab" | od -x | grep "6261" > /dev/null || echo "-Dbig_endian")
sdl:	LIBS	 = $(shell sdl-config --libs) -lX11 -lreadline
sdl:	INCS	 = $(shell sdl-config --cflags)
sdl:	MACROS	 = -DREADLINE
sdl:	${PROG}

sdl2:	ENDIAN	 = $(shell echo "ab" | od -x | grep "6261" > /dev/null || echo "-Dbig_endian")
sdl2:	LIBS	 = $(shell sdl2-config --libs) -lreadline
sdl2:	INCS	 = $(shell sdl2-config --cflags)
sdl2:	MACROS	 = -DSDL2 -DREADLINE
sdl2:	PROG	 = sdl2trs
sdl2:	${PROG}

win32:	MINGW	 = C:\MinGW
win32:	LIBS	 = -L${MINGW}\lib -lmingw32 -lSDLmain -lSDL
win32:	INCS	 = -I${MINGW}\include\SDL
win32:	MACROS	 = -DWIN32
win32:	CC	 = ${MINGW}\bin\gcc.exe
win32:	PROG	 = sdltrs.exe
win32:	${PROG}

win64:	MINGW64	 = C:\MinGW64
win64:	LIBS	 = -L${MINGW64}\lib -lmingw32 -lSDL2main -lSDL2
win64:	INCS	 = -I${MINGW64}\include\SDL2
win64:	MACROS	 = -DWIN32 -DSDL2
win64:	CC       = ${MINGW64}\bin\gcc.exe
win64:	PROG	 = sdl2trs64.exe
win64:	${PROG}

wsdl2:	MINGW	 = C:\MinGW
wsdl2:	LIBS	 = -L${MINGW}\lib -lmingw32 -lSDL2main -lSDL2
wsdl2:	INCS	 = -I${MINGW}\include\SDL2
wsdl2:	MACROS	 = -DSDL2 -DWIN32
wsdl2:	CC	 = ${MINGW}\bin\gcc.exe
wsdl2:	PROG	 = sdl2trs.exe
wsdl2:	${PROG}

CFLAGS		 = -g -Wall -Wextra -Wno-unused-parameter -O2 ${INCS} ${ENDIAN} ${MACROS}

${PROG}: ${OBJS}
	${CC} -o ${PROG} ${OBJS} ${LIBS} ${LDFLAGS}
