/*
 * Copyright (C) 1992 Clarendon Hill Software.
 *
 * Permission is granted to any individual or institution to use, copy,
 * or redistribute this software, provided this copyright notice is retained.
 *
 * This software is provided "as is" without any expressed or implied
 * warranty.  If this software brings on any sort of damage -- physical,
 * monetary, emotional, or brain -- too bad.  You've got no one to blame
 * but yourself.
 *
 * The software may be modified for your own purposes, but modified versions
 * must retain this notice.
 */

/*
 * Copyright (c) 1996-2020, Timothy P. Mann
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "error.h"
#include "load_cmd.h"
#include "trs.h"
#include "trs_disk.h"
#include "trs_sdl_keyboard.h"
#include "trs_state_save.h"

/* Include ROMs */
#include "trs_rom1.c"
#include "trs_rom3.c"
#include "trs_rom4p.c"
#include "trs_romesf.c"

int trs_model = 1;
char *program_name;

int trs_load_cmd(const char *filename)
{
  FILE *program;
  extern Uchar memory[];
  int entry;

  if ((program = fopen(filename,"rb")) == NULL) {
    error("failed to load CMD file %s: %s", filename, strerror(errno));
    return -1;
  }
  if (load_cmd(program, memory, NULL, 0, NULL, -1, NULL, &entry, 1) == LOAD_CMD_OK) {
    debug("entry point of %s: 0x%x (%d) ...\n", filename, entry, entry);
    if (entry >= 0)
      REG_PC = entry;
  } else {
    error("unknown CMD format");
    fclose(program);
    return -1;
  }
  fclose(program);
  return 0;
}

static int trs_load_rom(const char *filename)
{
  FILE *program;
  int c;

  if (filename[0] == 0)
    return -1;

  if ((program = fopen(filename, "rb")) == NULL) {
    error("failed to load ROM file %s: %s", filename, strerror(errno));
    return -1;
  }
  c = getc(program);
  if (c == ':') {
    /* Assume Intel hex format */
    rewind(program);
    trs_rom_size = load_hex(program);
    fclose(program);
    if (trs_rom_size == -1) {
      error("ROM file %s not in Intel hex format", filename);
      return -1;
    } else
      return 0;
  } else if (c == 1 || c == 5) {
    /* Assume MODELA/III file */
    extern Uchar rom[];
    Uchar loadmap[Z80_ADDRESS_LIMIT];
    rewind(program);
    if (load_cmd(program, rom, loadmap, 0, NULL, -1, NULL, NULL, 1) == LOAD_CMD_OK) {
      trs_rom_size = Z80_ADDRESS_LIMIT;
      while (trs_rom_size > 0) {
        if (loadmap[--trs_rom_size] != 0) {
          trs_rom_size++;
          break;
        }
      }
      fclose(program);
      return 0;
    } else {
      /* Guess it wasn't one */
      rewind(program);
      c = getc(program);
    }
  }
  trs_rom_size = 0;
  while (c != EOF) {
    mem_write_rom(trs_rom_size++, c);
    c = getc(program);
  }
  return 0;
}

static void trs_load_compiled_rom(int address, int size, const unsigned char rom[])
{
  int i;

  trs_rom_size = address + size;
  for (i = 0; i < size; ++i)
    mem_write_rom(address + i, rom[i]);
}

void trs_rom_init(void)
{
  switch (trs_model) {
    case 1:
      if (trs_load_rom(romfile) != 0)
        trs_load_compiled_rom(0, sizeof(trs_rom1), trs_rom1);
      if (stringy)
        trs_load_compiled_rom(0x3000, sizeof(trs_romesf), trs_romesf);
      break;
    case 3:
    case 4:
      if (trs_load_rom(romfile3) != 0)
        trs_load_compiled_rom(0, sizeof(trs_rom3), trs_rom3);
      break;
    case 5:
      if (trs_load_rom(romfile4p) != 0)
        trs_load_compiled_rom(0, sizeof(trs_rom4p), trs_rom4p);
      break;
  }
}

int SDLmain(int argc, char *argv[])
{
  int debug = FALSE;
  wordregister x;

  /* program_name must be set first because the error
   * printing routines use it. */
  program_name = strrchr(argv[0], DIR_SLASH);
  if (program_name == NULL) {
    program_name = argv[0];
  } else {
    program_name++;
  }

  x.byte.low = 1;
  x.byte.high = 0;
  if (x.word != 1)
    fatal("Program compiled with wrong ENDIAN: please recompile for this architecture.");

#if defined(SDL2) && defined(_WIN32)
  SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);
#endif

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    fatal("failed to initialize SDL: %s", SDL_GetError());

#ifndef SDL2
  putenv("SDL_VIDEO_CENTERED=center");
  /* For NumLock on SDL >= 1.2.14 */
  putenv("SDL_DISABLE_LOCK_KEYS=1");

  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  /* Enable Unicode key translations */
  SDL_EnableUNICODE(TRUE);
#endif

  trs_parse_command_line(argc, argv, &debug);
  trs_set_keypad_joystick();
  trs_open_joystick();
  screen_init();
  trs_screen_init();
  trs_reset(1);

  if (trs_state_file[0]) {
    trs_state_load(trs_state_file);
    trs_screen_init();
    trs_screen_refresh();
  }
  if (trs_cmd_file[0])
    trs_load_cmd(trs_cmd_file);

  if (!debug || fullscreen) {
    /* Run continuously until exit or request to enter debugger */
    z80_run(TRUE);
  }
#ifdef ZBX
  printf("Entering debugger.\n");
  debug_init();
  debug_shell();
#endif

  trs_sdl_cleanup();
  printf("Quitting.\n");
  exit(0);
}

int main(int argc, char *argv[])
{
  SDLmain(argc, argv);
  return 0;
}
