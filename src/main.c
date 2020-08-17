/* SDLTRS version Copyright (c): 2006, Mark Grebe */

/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/
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
   Modified by Timothy Mann, 1996 and later
   $Id: main.c,v 1.16 2009/06/15 23:33:53 mann Exp $
   Modified by Mark Grebe, 2006
   Last modified on Wed May 07 09:12:00 MST 2006 by markgrebe
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <SDL.h>
#include "error.h"
#include "load_cmd.h"
#include "trs.h"
#include "trs_disk.h"
#include "trs_sdl_keyboard.h"
#include "trs_state_save.h"

extern int trs_rom1_size;
extern int trs_rom3_size;
extern int trs_rom4p_size;
extern int trs_romesf_size;
extern unsigned char trs_rom1[];
extern unsigned char trs_rom3[];
extern unsigned char trs_rom4p[];
extern unsigned char trs_romesf[];

int trs_model = 1;
char *program_name;

static void check_endian()
{
  wordregister x;
  x.byte.low = 1;
  x.byte.high = 0;
  if (x.word != 1)
    fatal("Program compiled with wrong ENDIAN value: adjust the Makefile and recompile.");
}

int trs_load_cmd(const char *filename)
{
  FILE *program;
  extern Uchar memory;
  int entry;

  if ((program = fopen(filename,"rb")) == NULL) {
    error("failed to load CMD file %s: %s", filename, strerror(errno));
    return -1;
  }
  if (load_cmd(program, &memory, NULL, 0, NULL, -1, NULL, &entry, 1) == LOAD_CMD_OK) {
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

int trs_load_rom(const char *filename)
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
    extern Uchar *rom; /*!! fixme*/
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

void trs_load_compiled_rom(int size, unsigned char rom[])
{
  int i;

  trs_rom_size = size;
  for (i = 0; i < size; ++i)
    mem_write_rom(i, rom[i]);
}

void trs_rom_init(void)
{
  switch (trs_model) {
    case 1:
      if (trs_load_rom(romfile) != 0)
        trs_load_compiled_rom(trs_rom1_size, trs_rom1);
      break;
    case 3:
    case 4:
      if (trs_load_rom(romfile3) != 0)
        trs_load_compiled_rom(trs_rom3_size, trs_rom3);
      break;
    case 5:
      if (trs_load_rom(romfile4p) != 0)
        trs_load_compiled_rom(trs_rom4p_size, trs_rom4p);
      break;
  }

  if (trs_model == 1 && stringy) {
    int i;

    /* Load ROM for ESF and adjust size */
    for (i = 0; i < trs_romesf_size; ++i)
      mem_write_rom(0x3000 + i, trs_romesf[i]);
    trs_rom_size = 0x3780;
  }
}

int SDLmain(int argc, char *argv[])
{
  int debug = FALSE;
  struct stat st;

  /* program_name must be set first because the error
   * printing routines use it. */
  program_name = strrchr(argv[0], DIR_SLASH);
  if (program_name == NULL) {
    program_name = argv[0];
  } else {
    program_name++;
  }

  check_endian();

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

  if (stat(trs_disk_dir, &st) < 0)
    trs_disk_dir[0] = 0;
  if (stat(trs_hard_dir, &st) < 0)
    trs_hard_dir[0] = 0;
  if (stat(trs_cass_dir, &st) < 0)
    trs_cass_dir[0] = 0;
  if (stat(trs_state_dir, &st) < 0)
    trs_state_dir[0] = 0;
  if (stat(trs_disk_set_dir, &st) < 0)
    trs_disk_set_dir[0] = 0;
  if (stat(trs_printer_dir, &st) < 0)
    trs_printer_dir[0] = 0;

  screen_init();
  trs_screen_init();
  trs_reset(1);

  if (init_state_file[0]) {
    trs_state_load(init_state_file);
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
