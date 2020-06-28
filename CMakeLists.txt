project(sdltrs C)

cmake_minimum_required(VERSION 2.8.5)

include(GNUInstallDirs)
include(TestBigEndian)

set(CMAKE_C_FLAGS
	"-s -Wall -Wextra -Wno-unused-parameter -O2 ${CMAKE_C_FLAGS}"
)

set(SOURCES
	src/blit.c
	src/debug.c
	src/dis.c
	src/error.c
	src/load_cmd.c
	src/load_hex.c
	src/main.c
	src/trs_cassette.c
	src/trs_chars.c
	src/trs_disk.c
	src/trs_hard.c
	src/trs_imp_exp.c
	src/trs_interrupt.c
	src/trs_io.c
	src/trs_memory.c
	src/trs_mkdisk.c
	src/trs_printer.c
	src/trs_rom1.c
	src/trs_rom3.c
	src/trs_rom4p.c
	src/trs_romesf.c
	src/trs_sdl_gui.c
	src/trs_sdl_interface.c
	src/trs_sdl_keyboard.c
	src/trs_state_save.c
	src/trs_stringy.c
	src/trs_uart.c
	src/z80.c
	src/PasteManager.c
)

add_executable(sdltrs ${SOURCES})

test_big_endian(BIGENDIAN)
if (${BIGENDIAN})
	add_definitions(-Dbig_endian)
endif ()
unset(BIGENDIAN)

option(FASTMOVE	"Fast inaccurate Z80 block moves"	OFF)
option(NOX	"Build SDL 1.2 version without X"	OFF)
option(READLINE	"Readline support for zbx debugger"	ON)
option(SDL1	"Use SDL version 1.2 instead of SDL2"	OFF)

if (FASTMOVE)
	add_definitions(-DFAST_MOVE)
endif ()

if (READLINE)
	find_package(Curses)
	find_path(Readline_ROOT_DIR
		NAMES include/readline/readline.h
	)
	find_path(Readline_INCLUDE_DIR
		NAMES readline/readline.h
		HINTS ${Readline_ROOT_DIR}/include
	)
	find_library(Readline_LIBRARY
		NAMES readline
		HINTS ${Readline_ROOT_DIR}/lib
	)
	add_definitions(-DREADLINE)
	include_directories(${Readline_INCLUDE_DIR})
	target_link_libraries(sdltrs ${Readline_LIBRARY})
endif ()

if (SDL1 OR NOX)
	find_package(SDL REQUIRED)
	include_directories(${SDL_INCLUDE_DIR})
	target_link_libraries(sdltrs ${SDL_LIBRARY})
	if (NOX)
		add_definitions(-DNOX)
	else ()
		find_package(X11 REQUIRED)
		include_directories(${X11_INCLUDE_DIR})
		target_link_libraries(sdltrs ${X11_LIBRARIES})
	endif ()
else ()
	find_package(SDL2 QUIET)
	if (${SDL2_FOUND})
		# Comply to policy CMP0004:
		string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
		# Set empty variable (needed for Arch Linux):
		if ("${SDL2_LIBRARIES}" STREQUAL "")
			set(SDL2_LIBRARIES "SDL2::SDL2")
		endif ()
		message("-- Found SDL2: ${SDL2_LIBRARIES}")
		include_directories(${SDL2_INCLUDE_DIRS})
		target_link_libraries(sdltrs ${SDL2_LIBRARIES})
	else ()
		exec_program(sdl2-config ARGS --cflags OUTPUT_VARIABLE SDL2_CFLAGS)
		exec_program(sdl2-config ARGS --libs   OUTPUT_VARIABLE SDL2_LIBS)
		message("-- Found SDL2: ${SDL2_LIBS}")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SDL2_CFLAGS}")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SDL2_LIBS}")
	endif ()
	add_definitions(-DSDL2)
endif ()

install(TARGETS sdltrs		DESTINATION ${CMAKE_INSTALL_BINDIR}/)
install(FILES src/sdltrs.1	DESTINATION ${CMAKE_INSTALL_MANDIR}/)
install(FILES LICENSE		DESTINATION ${CMAKE_INSTALL_DOCDIR}/)
install(DIRECTORY html		DESTINATION ${CMAKE_INSTALL_DOCDIR}/)
install(DIRECTORY diskimages/	DESTINATION ${CMAKE_INSTALL_DATADIR}/sdltrs/)
install(FILES sdltrs.desktop	DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
install(FILES icons/sdltrs.png	DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps/)
install(FILES icons/sdltrs.svg	DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps/)
install(FILES icons/sdltrs.xpm	DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps/)
