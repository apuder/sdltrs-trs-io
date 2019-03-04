.PHONY: all clean bsd linux nox sdl2 win32 win32-clean

all:
	@echo "make (clean|bsd|linux|nox|sdl2|win32|win32-clean)"

clean:
	rm -rf *.d *.o sdltrs sdl2trs

bsd:
	${MAKE} -f Makefile.bsd

linux:
	${MAKE} -f Makefile.linux

nox:
	${MAKE} -f Makefile.nox

sdl2:
	${MAKE} -f Makefile.sdl2

win32:
	${MAKE} -f Makefile.win32

win32-clean:
	del *.d *.o sdltrs.exe
