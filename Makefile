.PHONY: all clean bsd linux nox sdl2 win32 win32-clean

all:
	@echo "make (clean | bsd | linux | nox | sdl2 | win32 | win32-clean)"

clean:
	rm -rf src/*.d src/*.o src/sdltrs src/sdl2trs

bsd:
	${MAKE} -C src -f Makefile.bsd

linux:
	${MAKE} -C src -f Makefile.linux

nox:
	${MAKE} -C src -f Makefile.nox

sdl2:
	${MAKE} -C src -f Makefile.sdl2

win32:
	${MAKE} -C src -f Makefile.win32

win32-clean:
	del src\*.d src\*.o src\sdltrs.exe
