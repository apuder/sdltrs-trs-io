/*
 * Copyright (C) 2006-2011, Mark Grebe
 * Copyright (C) 2018-2021, Jens Guenther
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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

#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL.h>
#include "error.h"
#include "trs.h"
#include "trs_cassette.h"
#include "trs_disk.h"
#include "trs_hard.h"
#include "trs_mkdisk.h"
#include "trs_sdl_gui.h"
#include "trs_sdl_keyboard.h"
#include "trs_state_save.h"
#include "trs_stringy.h"
#include "trs_uart.h"

#define MENU_NORMAL_TYPE        1
#define MENU_TITLE_TYPE         2
#define MENU_FLOPPY_BROWSE_TYPE 3
#define MENU_HARD_BROWSE_TYPE   4
#define MENU_WAFER_BROWSE_TYPE  5
#define MENU_CASS_BROWSE_TYPE   6

#define MAX_JOYSTICKS 8

#define N_KEYS        52
#define SHIFT         39

static char filename[FILENAME_MAX];
static char **filenamelist = NULL;
static int filenamecount = 0;
static int filenamelistsize = 0;

typedef struct menu_entry_type {
  char title[64];
  int const type;
} MENU_ENTRY;

static const char *yes_no_choices[2] = {
  "        No",
  "       Yes"
};

static const char *function_choices[8] = {
  "  GUI Menu  ", "  Keyboard  ",
  " Save State ", " Load State ",
  "   Reset    ", "    Quit    ",
  "   Pause    ", "Joystick GUI"
};

static int const function_codes[8] = {
  GUI,   KEYBRD,
  SAVE,  LOAD,
  RESET, EXIT,
  PAUSE, JOYGUI
};

static const char *key_names[N_KEYS] = {
  " 1 ", " 2 ", " 3 ", " 4 ", " 5 ", " 6 ", " 7 ", " 8 ", " 9 ", " 0 ", " : ", " - ", "BRK",
  " UP", " q ", " w ", " e ", " r ", " t ", " y ", " u ", " i ", " o ", " p ", "LFT", "RGT",
  "DWN", " a ", " s ", " d ", " f ", " g ", " h ", " j ", " k ", " l ", " ; ", "ENT", "CLR",
  "SHF", " z ", " x ", " c ", " v ", " b ", " n ", " m ", " , ", " . ", " / ", " @ ", "SPC"
};
static int const key_syms[N_KEYS] = {
  SDLK_1,    SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9,     SDLK_0,      SDLK_COLON,     SDLK_MINUS,  SDLK_ESCAPE,
  SDLK_UP,   SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i,     SDLK_o,      SDLK_p,         SDLK_LEFT,   SDLK_RIGHT,
  SDLK_DOWN, SDLK_a, SDLK_s, SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k,     SDLK_l,      SDLK_SEMICOLON, SDLK_RETURN, SDLK_HOME,
  -1,        SDLK_z, SDLK_x, SDLK_c, SDLK_v, SDLK_b, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH,     SDLK_AT,     SDLK_SPACE
};
static const char *key_names_shifted[N_KEYS] = {
  " ! ", " \" ", " # ", " $ ", " % ", " & ", " ' ", " ( ", " ) ", " _ ", " * ", " = ", " ~ ",
  "TAB", " Q ",  " W ", " E ", " R ", " T ", " Y ", " U ", " I ", " O ", " P ", " [ ", " ] ",
  " ^ ", " A ",  " S ", " D ", " F ", " G ", " H ", " J ", " K ", " L ", " + ", " { ", " } ",
  "SHF", " Z ",  " X ", " C ", " V ", " B ", " N ", " M ", " < ", " > ", " ? ", " \\ ", " | "
};
static int const key_syms_shifted[N_KEYS] = {
  SDLK_EXCLAIM, SDLK_QUOTEDBL, SDLK_HASH, SDLK_DOLLAR, 0x25, SDLK_AMPERSAND, SDLK_QUOTE, SDLK_LEFTPAREN, SDLK_RIGHTPAREN, SDLK_UNDERSCORE, SDLK_ASTERISK, SDLK_EQUALS, SDLK_CARET,
  SDLK_TAB,     0x51,          0x57,      0x45,        0x52, 0x54,           0x59,       0x55,           0x49,            0x4f,            0x50,          0xc4,        0xdc,
  0x7e,         0x41,          0x53,      0x44,        0x46, 0x47,           0x48,       0x4a,           0x4b,            0x4c,            SDLK_PLUS,     0xe4,        0xfc,
  -1,           0x5a,          0x58,      0x43,        0x56, 0x42,           0x4e,       0x4d,           SDLK_LESS,       SDLK_GREATER,    SDLK_QUESTION, 0xd6,        0xf6
};

int jbutton_map[N_JOYBUTTONS] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int jaxis_mapped = 0;

extern int  scanlines;
extern int  scanshade;
#if defined(SDL2) || !defined(NOX)
extern int  turbo_paste;
#endif
extern void trs_gui_clear_rect(int x, int y, int w, int h);
extern int  trs_sdl_savebmp(const char *name);
static void trs_gui_write_text(const char *text, int x, int y, int invert);
static void trs_gui_center_text(const char *text, int y, int invert);
static void trs_gui_frame(int x1, int y1, int x2, int y2);
static void trs_gui_clear_screen(void);
static void trs_gui_limit_string(const char *orig, char *limited, unsigned int limit);
static void trs_add_extension(char *name, const char *ext);
static int  trs_gui_get_key(void);
static void trs_gui_display_error(const char *name);
static void trs_gui_display_message(const char *title, const char *message);
static void trs_gui_create_filename_list(void);
static void trs_gui_add_to_filename_list(char *name);
static int  trs_gui_filename_cmp(const char *name1, const char *name2);
static void trs_gui_quicksort(char **start, char **end,
                              int (*sort_function) (const char *, const char *));
static void trs_gui_delete_filename_list(void);
static int  trs_gui_readdirectory(const char *path, const char *mask, int browse_dir);
static int  trs_gui_input_string(const char *title, const char *input, char* output,
                                 unsigned int limit, int file);
static int  trs_gui_display_menu(const char* title, MENU_ENTRY *entry, int selection);
static int  trs_gui_display_popup(const char *title, const char **entry,
                                  int entry_count, int selection);
static int  trs_gui_display_popup_matrix(const char *title, const char **entry,
                                         int rows, int cols, int selection);
static int  trs_gui_display_question(const char *text);
static int  trs_gui_file_overwrite(void);
static void trs_gui_new_machine(void);
static void trs_gui_disk_creation(void);
#ifdef __linux
static void trs_gui_disk_steps(void);
#endif
static void trs_gui_disk_options(void);
static void trs_gui_diskset_load(void);
static void trs_gui_diskset_save(void);
static void trs_gui_disk_management(void);
static void trs_gui_hard_management(void);
static void trs_gui_stringy_management(void);
static void trs_gui_cassette_management(void);
static void trs_gui_emulator_settings(void);
static void trs_gui_display_settings(void);
static void trs_gui_misc_settings(void);
static int  trs_gui_config_management(void);
static void trs_gui_printer_management(void);
static const char *trs_gui_get_key_name(int key);
static int  trs_gui_virtual_keyboard(void);
static void trs_gui_get_virtual_key(void);
static void trs_gui_joy_gui(void);
static int  trs_gui_joystick_get_button(void);
static void trs_gui_joystick_display_map(int button);
static void trs_gui_joystick_settings(void);
static void trs_gui_default_dirs(void);
static void trs_gui_rom_files(void);
static void trs_gui_about_sdltrs(void);
static void trs_gui_keys_sdltrs(void);
static void trs_gui_exec_cmd(void);
static void trs_gui_save_bmp(void);
static void trs_gui(void);

void trs_gui_write_text(const char *text, int x, int y, int invert)
{
  int const len = strlen(text);
  int i;

  if (len > 62 - x) {
    int const len_first_part = (59 - x) / 2;
    int pos_second_part = len - (59 - x - len_first_part);

    for (i = 0; i < len_first_part; i++)
      trs_gui_write_char(x + i, y, text[i], invert);
    for (; i < len_first_part + 3; i++)
      trs_gui_write_char(x + i, y, '.', invert);
    for (; i < 62 - x; i++)
      trs_gui_write_char(x + i, y, text[pos_second_part++], invert);
  } else {
    for (i = 0; i < len; i++)
      trs_gui_write_char(x + i, y, text[i], invert);
  }
}

void trs_gui_center_text(const char *text, int y, int invert)
{
  trs_gui_write_text(text, (64 - strlen(text)) / 2, y, invert);
}

void trs_gui_frame(int x1, int y1, int x2, int y2)
{
  int i;

  for (i = x1 + 1; i < x2; i++) {
    trs_gui_write_char(i, y1, 131, 0);
    trs_gui_write_char(i, y2, 176, 0);
  }
  for (i = y1 + 1; i < y2; i++) {
    trs_gui_write_char(x1, i, 149, 0);
    trs_gui_write_char(x2, i, 170, 0);
  }
  trs_gui_write_char(x1, y1, 151, 0);
  trs_gui_write_char(x2, y1, 171, 0);
  trs_gui_write_char(x1, y2, 181, 0);
  trs_gui_write_char(x2, y2, 186, 0);
}

void trs_gui_clear_screen(void)
{
  trs_gui_clear_rect(0, 0, 64, 16);
}

void trs_gui_limit_string(const char *orig, char *limited, unsigned int limit)
{
  if (strlen(orig) > limit) {
    int const len_first_part = (limit - 3) / 2;
    int const pos_second_part = strlen(orig) - (limit - len_first_part - 3);

    snprintf(limited, limit + 1, "%.*s...%s", len_first_part, orig,
        orig + pos_second_part);
  } else
    snprintf(limited, limit + 1, "%s", orig);
}

void trs_add_extension(char *name, const char *ext)
{
  int const flen = strlen(name);
  int const elen = strlen(ext);

  if (flen > elen)
    if (strcasecmp(&name[flen - elen], ext) == 0)
      return;

  if (flen && name[flen - 1] != DIR_SLASH)
    snprintf(name + flen, FILENAME_MAX - flen, "%s", ext);
}

int trs_gui_get_key(void)
{
  SDL_Event event;

#ifdef SDL2
  /* Stop Text input first to prevent double chars */
  SDL_StopTextInput();
  SDL_StartTextInput();
#endif

  while (1) {
    SDL_WaitEvent(&event);
    switch (event.type) {
      case SDL_QUIT:
        trs_exit(0);
        break;
#ifdef SDL2
      case SDL_WINDOWEVENT:
        trs_screen_update();
        break;
#endif
      case SDL_MOUSEBUTTONDOWN:
        switch (event.button.button) {
          case SDL_BUTTON_LEFT:
            return SDLK_RETURN;
          case SDL_BUTTON_MIDDLE:
            return SDLK_TAB;
          case SDL_BUTTON_RIGHT:
            return SDLK_ESCAPE;
#ifndef SDL2
          case 4: /* SDL_BUTTON_WHEELUP */
            return SDLK_UP;
          case 5: /* SDL_BUTTON_WHEELDOWN */
            return SDLK_DOWN;
#endif
          default:
            break;
        }
        break;
#ifdef SDL2
      case SDL_MOUSEWHEEL:
        if (event.wheel.y > 0)
          return SDLK_UP;
        if (event.wheel.y < 0)
          return SDLK_DOWN;
        break;

      case SDL_TEXTINPUT:
        SDL_StopTextInput();
        return event.text.text[0];
#endif
      case SDL_KEYDOWN:
        if (event.key.keysym.mod & KMOD_ALT) {
          switch (event.key.keysym.sym) {
#ifdef _WIN32
            case SDLK_F4:
#endif
            case SDLK_q:
              trs_exit(1);
              break;
            case SDLK_BACKSPACE:
              return SDLK_F9;
            case SDLK_DELETE:
              return SDLK_F10;
            default:
              break;
          }
        }
        else if (event.key.keysym.sym == SDLK_F8)
          trs_exit(!(event.key.keysym.mod & KMOD_SHIFT));
#ifdef SDL2
        else if (event.key.keysym.sym < 0x20 ||
                 event.key.keysym.sym > 0x7E)
#else
        else if (event.key.keysym.sym < 0x100 &&
            event.key.keysym.unicode >= 0x20 &&
            event.key.keysym.unicode <= 0x7E)
          return event.key.keysym.unicode;
        else
#endif
          return event.key.keysym.sym;
        break;
      case SDL_JOYBUTTONDOWN:
        if (event.jbutton.button < N_JOYBUTTONS) {
          int const key = jbutton_map[event.jbutton.button];

          if (key >= 0)
            return key;
          else if (key == KEYBRD || key == JOYGUI)
            return trs_gui_virtual_keyboard();
        }
        break;
      case SDL_JOYAXISMOTION:
        if (event.jaxis.axis == 0 || event.jaxis.axis == 1) {
          static int hor_value = 0, ver_value = 0;
          int value = 0, trigger_keydown = 0, key = -1;

          if (event.jaxis.axis == 0)
            value = hor_value;
          else
            value = ver_value;

          if (event.jaxis.value < -JOY_BOUNCE) {
            if (value != -1)
              trigger_keydown = 1;
            value = -1;
          }
          else if (event.jaxis.value > JOY_BOUNCE) {
            if (value != 1)
              trigger_keydown = 1;
            value = 1;
          }
          else if (abs(event.jaxis.value) < JOY_BOUNCE / 8)
            value = 0;

          if (trigger_keydown) {
            if (event.jaxis.axis == 0)
              key = (value == -1 ? SDLK_LEFT : SDLK_RIGHT);
            else
              key = (value == -1 ? SDLK_UP : SDLK_DOWN);
          }

          if (event.jaxis.axis == 0)
            hor_value = value;
          else
            ver_value = value;

          if (key != -1)
            return key;
        }
        break;
    }
  }
}

void trs_gui_display_error(const char *name)
{
  if (errno != 0) {
    char text[60];

    if (snprintf(text, 60, "%s: %s", strerror(errno), name))
      trs_gui_display_message("ERROR", text);
  }
}

void trs_gui_display_message(const char* title, const char *message)
{
  trs_gui_frame(1, 6, 62, 8);
  trs_gui_clear_rect(2, 7, 60, 1);
  trs_gui_write_text(title, 3, 6, 0);
  trs_gui_write_text(message, 3, 7, 0);
  trs_gui_write_text(" Press ENTER to continue ", 36, 8, 1);
  trs_screen_update();

  while (1) {
    switch (trs_gui_get_key()) {
      case SDLK_ESCAPE:
      case SDLK_RETURN:
        return;
    }
  }
}

void trs_gui_create_filename_list(void)
{
  if (filenamelist == NULL) {
    if ((filenamelist = (char **)malloc(256 * sizeof(char *))) == NULL) {
      trs_sdl_cleanup();
      fatal("failed to allocate filenamelist");
    }
    filenamelistsize = 256;
  }
}

void trs_gui_add_to_filename_list(char *name)
{
  filenamelist[filenamecount++] = name;
  if (filenamecount == filenamelistsize) {
    char **filenamelist_new;

    if ((filenamelist_new = realloc(filenamelist, 2 *
        filenamelistsize * sizeof(char*))) == NULL) {
      free(filenamelist);
      trs_sdl_cleanup();
      fatal("failed to reallocate filenamelist");
    }
    filenamelist = filenamelist_new;
    filenamelistsize *= 2;
  }
}

int trs_gui_filename_cmp(const char *name1, const char *name2)
{
#ifdef _WIN32
  /* Sort the drive letters last in the list */
  if (name1[0] == '[') {
    if (name2[0] == '[')
      return -1;
    else
      return 1;
  }
  if (name2[0] == '[')
    return -1;
#endif
  if (name1[0] == '<') {
    if (name2[0] != '<')
      return -1;
    if (name1[1] == '.') {
      if (name2[1] != '.')
        return -1;
    }
    else if (name2[1] == '.')
      return 1;
  }
  else if (name2[0] == '<')
    return 1;

  return strcasecmp(name1, name2);
}

void trs_gui_quicksort(char **start, char **end,
                       int (*sort_function) (const char *, const char *))
{
  while (start + 1 < end) {
    char **left = start + 1;
    char **right = end;
    char *pivot = *start;
    char *tmp;

    while (left < right) {
      if ((*sort_function)(*left, pivot) < 0)
        left++;
      else {
        right--;
           tmp = *left;
         *left = *right;
        *right = tmp;
      }
    }
    left--;
       tmp = *left;
     *left = *start;
    *start = tmp;
    trs_gui_quicksort(start, left, trs_gui_filename_cmp);
    start = right;
  }
}

void trs_gui_delete_filename_list(void)
{
  int i;

  for (i = 0; i < filenamecount; i++)
    free(filenamelist[i]);
  filenamecount = 0;
}

int trs_gui_readdirectory(const char *path, const char *mask, int browse_dir)
{
  DIR *directory = NULL;
  char pathname[FILENAME_MAX];
  char *name;
  int  dirname_len;
  struct dirent *dir_entry;
  struct stat st;

  directory = opendir(path);
  if (directory) {
    trs_gui_create_filename_list();
    while ((dir_entry = readdir(directory))) {

      if (strcmp(dir_entry->d_name, ".") == 0)
        continue;

      if (snprintf(pathname, FILENAME_MAX, "%s%s",
          path, dir_entry->d_name) >= FILENAME_MAX) {
        closedir(directory);
        return -1;
      }
      dirname_len = strlen(dir_entry->d_name);

      stat(pathname, &st);
      if (S_ISDIR(st.st_mode)) {
        if ( (name = (char *)malloc(dirname_len + 3)) )
          snprintf(name, dirname_len + 3, "<%s>", dir_entry->d_name);
      }
      else if (browse_dir) {
        continue;
      } else {
        if (mask != NULL) {
          if (dirname_len < 4)
            continue;
          if (strcasecmp(&dir_entry->d_name[dirname_len - 4], mask) != 0)
            continue;
        }
        name = (char *)strdup(dir_entry->d_name);
      }
      if (!name) {
        closedir(directory);
        return -1;
      }
      trs_gui_add_to_filename_list(name);
    }
    closedir(directory);
#ifdef _WIN32
    {
      char letter;
      DWORD drive_mask = GetLogicalDrives();

      for (letter = 'A'; letter <= 'Z'; letter++) {
        if (drive_mask & 1) {
          static char drive[5] = "[C:]";

          drive[1] = letter;
          trs_gui_add_to_filename_list(strdup(drive));
        }
        drive_mask >>= 1;
      }
    }
#endif

    trs_gui_quicksort(filenamelist, filenamelist + filenamecount,
        trs_gui_filename_cmp);
    return 0;
  }
  return -1;
}

int trs_gui_file_browse(const char *path, char *name, const char *mask,
                        int browse_dir, const char* type)
{
  char current_dir[FILENAME_MAX];
  char title[64];
  struct stat st;
  const char *new_dir;
  int i, j, key;
  int selection;
  int current_first;
  int drawcount;
  int redraw;

  snprintf(current_dir, FILENAME_MAX, "%s", path);

  for (i = strlen(current_dir); i > 0; i--) {
    if (current_dir[i] == DIR_SLASH) {
      current_dir[i + 1] = 0;
      break;
    }
  }

  stat(current_dir, &st);
  if (current_dir[0] == 0 || strcmp(current_dir, ".") == 0 || S_ISDIR(st.st_mode) == 0) {
    if (getcwd(current_dir, FILENAME_MAX) == NULL)
      current_dir[0] = 0;
  }
  if (current_dir[strlen(current_dir) - 1] != DIR_SLASH)
    snprintf(current_dir + strlen(current_dir), FILENAME_MAX - strlen(current_dir),
        "%c", DIR_SLASH);

read_directory:
  trs_gui_delete_filename_list();
  if (trs_gui_readdirectory(current_dir, mask, browse_dir) != 0)
    return -1;

  trs_gui_limit_string(current_dir, title, 58);
  trs_gui_clear_screen();
  trs_gui_frame(0, 0, 63, 15);
  trs_gui_center_text(title, 1, 0);

  if (browse_dir) {
    snprintf(title, 63, "Choose %sDirectory", type);
    trs_gui_center_text(" TAB selects directory ", 15, 1);
  }
  else
    snprintf(title, 63, "Select %s File To Load", type);
  trs_gui_write_text(title, 2, 0, 0);

  drawcount = filenamecount < 13 ? filenamecount : 13;
  current_first = selection = 0;
  redraw = 1;

  while (1) {
    if (redraw) {
      trs_gui_clear_rect(2, 2, 60, 13);
      for (i = 0; i < drawcount; i++)
        trs_gui_write_text(filenamelist[current_first + i], 2, i + 2, 0);
      redraw = 0;
    }
    trs_gui_write_text(filenamelist[current_first + selection], 2, selection + 2, 1);
    trs_screen_update();
    key = trs_gui_get_key();
    trs_gui_write_text(filenamelist[current_first + selection], 2, selection + 2, 0);
    if (key >= '0' && key <= 'z') {
      key = tolower(key);
      i = j = current_first + selection;
      do {
        if (++i > filenamecount - 1)
          i = 0;
      } while (i != j && (tolower((int)*filenamelist[i]) != key));
      if (i < 13) {
        current_first = 0;
        selection = i;
      } else if (i + 13 > filenamecount) {
        current_first = filenamecount - 13;
        selection = i - current_first;
      } else
        current_first = i - selection;
      redraw = 1;
    } else {
      switch (key) {
        case SDLK_DOWN:
        case SDLK_RIGHT:
          if (selection < drawcount - 1)
            selection++;
          else
            if (current_first < filenamecount - drawcount) {
              current_first++;
              redraw = 1;
            }
          break;
        case SDLK_UP:
        case SDLK_LEFT:
          if (selection > 0)
            selection--;
          else
            if (current_first > 0) {
              current_first--;
              redraw = 1;
            }
          break;
        case SDLK_PAGEUP:
          current_first -= drawcount;
          if (current_first < 0)
            current_first = selection = 0;
          redraw = 1;
          break;
        case SDLK_PAGEDOWN:
          current_first += drawcount;
          if (current_first > filenamecount - drawcount) {
            current_first = filenamecount - drawcount;
            selection = drawcount - 1;
          }
          redraw = 1;
          break;
        case SDLK_HOME:
          selection = current_first = 0;
          redraw = 1;
          break;
        case SDLK_END:
          selection = drawcount - 1;
          current_first = filenamecount - drawcount;
          redraw = 1;
          break;
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_TAB:
          if (key == SDLK_TAB && browse_dir)
            goto done;
          if (*filenamelist[current_first + selection] == '<') {
            new_dir = filenamelist[current_first + selection];
            if (new_dir[1] == '.' && new_dir[2] == '.') {
              for (i = strlen(current_dir) - 2; i >= 0; i--) {
                if (current_dir[i] == DIR_SLASH) {
                  current_dir[i + 1] = 0;
                  break;
                }
              }
              if (i < 0 &&
#ifdef _WIN32
                current_dir[2] != DIR_SLASH) {
#else
                current_dir[0] != DIR_SLASH) {
#endif
                if (getcwd(current_dir, FILENAME_MAX) == NULL)
                  current_dir[0] = 0;
                snprintf(current_dir + strlen(current_dir),
                    FILENAME_MAX - strlen(current_dir), "%c", DIR_SLASH);
              }
            } else {
              snprintf(current_dir + strlen(current_dir),
                  FILENAME_MAX - strlen(current_dir), "%s", &new_dir[1]);
              current_dir[strlen(current_dir) - 1] = DIR_SLASH;
            }
            goto read_directory;
          }
#ifdef _WIN32
          /* Select a new drive */
          else if (*filenamelist[current_first + selection] == '[') {
            new_dir = filenamelist[current_first + selection];
            current_dir[0] = new_dir[1];
            current_dir[1] = new_dir[2];
            current_dir[2] = DIR_SLASH;
            current_dir[3] = 0;
            goto read_directory;
          }
#endif
          else
            goto done;
          break;
        case SDLK_ESCAPE:
          selection = -1;
          goto done;
          break;
      }
    }
  }

done:
  if (selection >= 0) {
    selection += current_first;
    snprintf(name, FILENAME_MAX, "%s", current_dir);
    if (browse_dir) {
      new_dir = filenamelist[selection];
      if (new_dir[1] != '.' && new_dir[2] != '.') {
#ifdef _WIN32
        if (new_dir[0] == '[') {
          name[0] = new_dir[1];
          name[1] = new_dir[2];
          name[2] = DIR_SLASH;
          name[3] = 0;
        } else
#endif
        {
          snprintf(name + strlen(name), FILENAME_MAX - strlen(name),
              "%s", &new_dir[1]);
          name[strlen(name) - 1] = DIR_SLASH;
        }
      }
    }
    else
      snprintf(name + strlen(name), FILENAME_MAX - strlen(name),
          "%s", filenamelist[selection]);
  }
  trs_gui_delete_filename_list();
  return selection;
}

int trs_gui_input_string(const char *title, const char* input, char* output,
                         unsigned int limit, int file)
{
  int key;
  int insert = 1;
  unsigned int i, pos;
  unsigned int len;
  unsigned int first_disp;

  if (input != output && strcmp(input, ".") != 0)
    snprintf(output, limit + 1, "%s", input);

  pos = len = strlen(output);

redraw:
  if (pos > 60)
    first_disp = pos - 59;
  else
    first_disp = 0;

  trs_gui_frame(1, 6, 62, 8);
  trs_gui_write_text(title, 3, 6, 0);

  while (1) {
    for (i = 0; i < 60; i++) {
      unsigned int cur_pos = first_disp + i;

      trs_gui_write_char(i + 2, 7,
          (cur_pos >= len) ? ' ' : output[cur_pos],
          (cur_pos == pos));
    }
    trs_gui_write_text((insert ? " INS " : " OVR "), 56, 8, 1);
    trs_screen_update();
    key = trs_gui_get_key();
    switch (key) {
      case SDLK_LEFT:
        if (pos > 0) {
          if (pos == first_disp)
            first_disp--;
          pos--;
        }
        break;
      case SDLK_RIGHT:
        if (pos < len) {
          if (pos == first_disp + 59)
            first_disp++;
          pos++;
        }
        break;
      case SDLK_HOME:
      case SDLK_PAGEUP:
        first_disp = pos = 0;
        break;
      case SDLK_END:
      case SDLK_PAGEDOWN:
        pos = len;
        if (pos > 60)
          first_disp = pos - 59;
        else
          first_disp = 0;
        break;
      case SDLK_BACKSPACE:
        if (pos > 0) {
          for (i = pos; i < len; i++)
            output[i - 1] = output[i];
          len--;
          if (pos == first_disp)
            first_disp--;
          pos--;
        }
        break;
      case SDLK_DELETE:
        if (pos < len) {
          for (i = pos; i < len - 1; i++)
            output[i] = output[i + 1];
          len--;
        }
        break;
      case SDLK_INSERT:
        insert = !insert;
        break;
      case SDLK_RETURN:
        output[len] = 0;
        return 0;
      case SDLK_ESCAPE:
        return -1;
      case SDLK_DOWN:
      case SDLK_TAB:
      case SDLK_UP:
        if (file) {
          char directory_name[FILENAME_MAX];

          if (trs_gui_file_browse(input, directory_name, NULL, 1, "") >= 0) {
            snprintf(output, limit + 1, "%s", directory_name);
            pos = len = strlen(output);
          }
          goto redraw;
        }
        break;
      case SDLK_F9:
        first_disp = len = pos = 0;
        output[0] = 0;
        break;
      case SDLK_F10:
        len = pos;
        output[pos] = 0;
        break;
      default:
        if (key >= 0x20 && key <= 0xFF && pos < limit) {
          if (insert && len < limit) {
            for (i = len; i > pos; i--)
              output[i] = output[i - 1];
            len++;
          }
          output[pos] = (char) key;
          if (pos == first_disp + 59)
            first_disp++;
          pos++;
          if (pos > len)
            len++;
        }
        break;
    }
  }
}

int trs_gui_display_menu(const char *title, MENU_ENTRY *entry, int selection)
{
  int num = 0, i, key;

  trs_gui_frame(0, 0, 63, 15);
  trs_gui_write_text(title, 2, 0, 0);

  while (entry[num].type != 0) {
    trs_gui_write_text(entry[num].title, 2, num + 2, 0);
    num++;
  }
  num--;

  while (1) {
    trs_gui_write_text(entry[selection].title, 2, selection + 2, 1);
    trs_screen_update();
    key = trs_gui_get_key();
    trs_gui_write_text(entry[selection].title, 2, selection + 2, 0);
    if (key >= '0' && key <= '9') {
      key -= '0';
      if (key <= num && entry[key].type != MENU_TITLE_TYPE)
        selection = key;
    } else
    if (key >= 'A' && key <= 'z') {
      key = toupper(key);
      i = selection;
      do {
        if (++selection > num)
          selection = 0;
      } while (selection != i && (int)*entry[selection].title != key);
      while (entry[selection].type == MENU_TITLE_TYPE) {
        if (selection < num)
          selection++;
      }
    } else {
      switch (key) {
        case SDLK_DOWN:
        case SDLK_RIGHT:
          do {
            selection++;
            if (selection > num)
              selection = 0;
          } while (entry[selection].type == MENU_TITLE_TYPE);
          break;
        case SDLK_UP:
        case SDLK_LEFT:
          do {
            selection--;
            if (selection < 0)
              selection = num;
          } while (entry[selection].type == MENU_TITLE_TYPE);
          break;
        case SDLK_HOME:
        case SDLK_PAGEUP:
          selection = 0;
          while (entry[selection].type == MENU_TITLE_TYPE) {
            if (selection < num)
              selection++;
          }
          break;
        case SDLK_END:
        case SDLK_PAGEDOWN:
          selection = num;
          break;
        case SDLK_DELETE:
        case SDLK_BACKSPACE:
          switch (entry[selection].type) {
            case MENU_FLOPPY_BROWSE_TYPE:
              trs_disk_remove(selection);
              break;
            case MENU_HARD_BROWSE_TYPE:
              trs_hard_remove(selection);
              break;
            case MENU_WAFER_BROWSE_TYPE:
              stringy_remove(selection);
              break;
            case MENU_CASS_BROWSE_TYPE:
              trs_cassette_remove();
              break;
          }
          if (entry[selection].type != MENU_NORMAL_TYPE) {
            entry[selection].title[0] = ' ';
            return selection;
          }
          break;
        case SDLK_RETURN:
        case SDLK_TAB:
          switch (entry[selection].type) {
            case MENU_FLOPPY_BROWSE_TYPE:
              if (trs_gui_file_browse(trs_disk_getfilename(selection)[0] == 0 ?
                  trs_disk_dir : trs_disk_getfilename(selection), filename, NULL,
                  0, "Floppy Disk Image") >= 0)
                trs_disk_insert(selection, filename);
              break;
            case MENU_HARD_BROWSE_TYPE:
              if (trs_gui_file_browse(trs_hard_getfilename(selection)[0] == 0 ?
                  trs_hard_dir : trs_hard_getfilename(selection), filename, NULL,
                  0, "Hard Disk Image") >= 0)
                trs_hard_attach(selection, filename);
              break;
            case MENU_WAFER_BROWSE_TYPE:
              if (trs_gui_file_browse(stringy_get_name(selection)[0] == 0 ?
                  trs_cass_dir : stringy_get_name(selection), filename, NULL,
                  0, "Wafer Image") >= 0)
                stringy_insert(selection, filename);
              break;
            case MENU_CASS_BROWSE_TYPE:
              if (trs_gui_file_browse(trs_cassette_getfilename()[0] == 0 ?
                  trs_cass_dir : trs_cassette_getfilename(), filename, NULL,
                  0, "Cassette Image") >= 0)
                trs_cassette_insert(filename);
              break;
          }
          return selection;
        case SDLK_SPACE:
          switch (entry[selection].type) {
            case MENU_FLOPPY_BROWSE_TYPE:
              trs_protect_disk(selection, !trs_disk_getwriteprotect(selection));
              break;
            case MENU_HARD_BROWSE_TYPE:
              trs_protect_hard(selection, !trs_hard_getwriteprotect(selection));
              break;
            case MENU_CASS_BROWSE_TYPE:
              trs_protect_cass(!trs_cass_getwriteprotect());
              break;
            case MENU_WAFER_BROWSE_TYPE:
              trs_protect_stringy(selection, !stringy_get_writeprotect(selection));
              break;
          }
          return selection;
        case SDLK_ESCAPE:
          return -1;
      }
    }
  }
}

int trs_gui_display_popup(const char *title, const char **entry,
                          int entry_count, int selection)
{
  int const entry_len = strlen(entry[0]);
  int const saved_selection = selection;
  int i, key;
  int x, y;

  x = (64 - entry_len) / 2;
  y = (16 - entry_count) / 2;

  trs_gui_frame(x - 1, y - 1, x + entry_len, y + entry_count);
  trs_gui_center_text(title, y - 1, 0);

  for (i = 0; i < entry_count; i++)
    trs_gui_write_text(entry[i], x, y + i, 0);

  while (1) {
    trs_gui_write_text(entry[selection], x, selection + y, 1);
    trs_screen_update();
    key = trs_gui_get_key();
    trs_gui_write_text(entry[selection], x, selection + y, 0);
    if (entry_count == 2) {
      switch (key) {
        case 'n':
        case 'N':
          return 0;
        case 'y':
        case 'Y':
          return 1;
      }
    }
    if (key >= '0' && key <= 'z') {
      key = toupper(key);
      for (i = 0; i < entry_count; i++) {
        if (strchr(entry[i], key)) {
          selection = i;
          break;
        }
      }
    } else {
      switch (key) {
        case SDLK_DOWN:
        case SDLK_RIGHT:
          selection++;
          if (selection > entry_count - 1)
            selection = 0;
          break;
        case SDLK_UP:
        case SDLK_LEFT:
          selection--;
          if (selection < 0)
            selection = entry_count - 1;
          break;
        case SDLK_HOME:
        case SDLK_PAGEUP:
          selection = 0;
          break;
        case SDLK_END:
        case SDLK_PAGEDOWN:
          selection = entry_count - 1;
          break;
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_TAB:
          return selection;
        case SDLK_ESCAPE:
          return saved_selection;
      }
    }
  }
}

int trs_gui_display_popup_matrix(const char* title, const char **entry,
                                 int rows, int cols, int selection)
{
  int const entry_len = strlen(entry[0]) + 1;
  int const entry_count = rows * cols;
  int const width = cols * entry_len - 1;
  int row, col;
  int i, j, key;
  int x, y;

  if (selection < 0)
    selection = 0;
  else if (selection >= entry_count)
    selection = entry_count - 1;

  row = selection / cols;
  col = selection % cols;
  x = (64 - width) / 2;
  y = (16 - rows) / 2;

  trs_gui_frame(x - 1, y - 1, x + width, y + rows);
  trs_gui_clear_rect(x, y, width, rows);
  trs_gui_center_text(title, y - 1, 0);
  for (i = 0; i < rows; i++)
    for (j = 0; j < cols; j++)
      trs_gui_write_text(entry[i * cols + j], x + j * entry_len, y + i, 0);

  while (1) {
    if (col < 0)
      col = cols - 1;
    if (col > cols - 1)
      col = 0;
    if (row < 0)
      row = rows - 1;
    if (row > rows - 1)
       row = 0;
    selection = row * cols + col;
    trs_gui_write_text(entry[selection], x + col * entry_len, y + row, 1);
    trs_screen_update();
    key = trs_gui_get_key();
    trs_gui_write_text(entry[selection], x + col * entry_len, y + row, 0);
    switch (key) {
      case SDLK_DOWN:
        row++;
        if (row > rows - 1)
          col++;
        break;
      case SDLK_UP:
        row--;
        if (row < 0)
          col--;
        break;
      case SDLK_RIGHT:
        col++;
        if (col > cols - 1)
          row++;
        break;
      case SDLK_LEFT:
        col--;
        if (col < 0)
          row--;
        break;
      case SDLK_HOME:
        col = 0;
        break;
      case SDLK_END:
        col = cols - 1;
        break;
      case SDLK_PAGEUP:
        row = 0;
        break;
      case SDLK_PAGEDOWN:
        row = rows - 1;
        break;
      case SDLK_RETURN:
      case SDLK_SPACE:
      case SDLK_TAB:
        return selection;
      case SDLK_ESCAPE:
        return -1;
    }
  }
}

int trs_gui_display_question(const char *text)
{
  return trs_gui_display_popup(text, yes_no_choices, 2, 0);
}

int trs_gui_file_overwrite(void)
{
  struct stat st;

  if (stat(filename, &st) == 0 && S_ISREG(st.st_mode))
    return trs_gui_display_question("Overwrite?");

  return 1;
}

void trs_gui_new_machine(void)
{
  trs_screen_var_reset();
  romin = 0;
  trs_screen_init();
  trs_reset(1);
}

void trs_gui_disk_creation(void)
{
  MENU_ENTRY disk_creation_menu[] =
  {{"Image Type                                                  ", MENU_NORMAL_TYPE},
   {"Number of Sides                                             ", MENU_NORMAL_TYPE},
   {"Density                                                     ", MENU_NORMAL_TYPE},
   {"Physical Size                                               ", MENU_NORMAL_TYPE},
   {"Ignore Density Flag                                         ", MENU_NORMAL_TYPE},
   {"Insert Created Disk Into This Drive                         ", MENU_NORMAL_TYPE},
   {"Create Disk Image with Above Parameters", MENU_NORMAL_TYPE},
   {"", 0}};
  const char *image_type_choices[3] = {"   JV1", "   JV3", "   DMK"};
  const char *num_sides_choices[2] =  {"     1", "     2"};
  const char *density_choices[2] =    {"Single", "Double"};
  const char *size_choices[2] =       {"5 Inch", "8 Inch"};
  const char *drive_choices[9] =      {"  None", "Disk 0", "Disk 1", "Disk 2",
                                       "Disk 3", "Disk 4", "Disk 5", "Disk 6",
                                       "Disk 7"};
  static int image_type = 1;
  static int num_sides = 1;
  static int density = 1;
  static int eight = 0;
  static int ignore_density = 0;
  static int drive_insert = 0;
  int selection = 6;

  while (1) {
    snprintf(&disk_creation_menu[0].title[54], 7, "%s", image_type_choices[image_type]);
    snprintf(&disk_creation_menu[1].title[54], 7, "%s", num_sides_choices[num_sides - 1]);
    snprintf(&disk_creation_menu[2].title[54], 7, "%s", density_choices[density - 1]);
    snprintf(&disk_creation_menu[3].title[54], 7, "%s", size_choices[eight]);
    snprintf(&disk_creation_menu[4].title[50], 11, "%s", yes_no_choices[ignore_density]);
    snprintf(&disk_creation_menu[5].title[54], 7, "%s", drive_choices[drive_insert]);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Floppy Disk Creation", disk_creation_menu, selection);
    switch (selection) {
      case 0:
        image_type = trs_gui_display_popup("Type", image_type_choices, 3, image_type);
        break;
      case 1:
        num_sides = trs_gui_display_popup("Sides", num_sides_choices, 2, num_sides - 1) + 1;
        break;
      case 2:
        density = trs_gui_display_popup("Dens", density_choices, 2, density - 1) + 1;
        break;
      case 3:
        eight = trs_gui_display_popup("Size", size_choices, 2, eight);
        break;
      case 4:
        ignore_density = trs_gui_display_popup("Ignore", yes_no_choices, 2, ignore_density);
        break;
      case 5:
        drive_insert = trs_gui_display_popup("Disk", drive_choices, 9, drive_insert);
        break;
      case 6:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Disk Image, TAB selects directory",
            trs_disk_dir, filename, FILENAME_MAX, 1) == 0) {
          if (trs_gui_file_overwrite()) {
            int ret = 0;

            if (image_type == 0)
              ret = trs_create_blank_jv1(filename);
            else if (image_type == 1)
              ret = trs_create_blank_jv3(filename);
            else
              ret = trs_create_blank_dmk(filename, num_sides, density, eight, ignore_density);
            if (ret)
              trs_gui_display_error(filename);
            else if (drive_insert)
              trs_disk_insert(drive_insert - 1, filename);
            return;
          }
        }
        break;
      case -1:
        return;
    }
  }
}

#ifdef __linux
void trs_gui_disk_steps(void)
{
  MENU_ENTRY disk_steps_menu[] =
  {{"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", 0}};
  const char *step_choices[2] = {"Single", "Double"};
  int selection = 0;
  int i, step;

  while (1) {
    for (i = 0; i < 8; i++) {
      snprintf(disk_steps_menu[i].title, 63,
          "Disk Drive Number %d Step                              %s",
          i, step_choices[trs_disk_getstep(i) == 1 ? 0 : 1]);
    }
    trs_gui_clear_screen();
    if ((selection = trs_gui_display_menu("SDLTRS Floppy Disk Step",
        disk_steps_menu, selection)) == -1)
      return;
    step = trs_gui_display_popup("Step", step_choices, 2, trs_disk_getstep(selection) == 2);
    trs_disk_setstep(selection, step == 0 ? 1 : 2);
  }
}
#endif

void trs_gui_disk_options(void)
{
  MENU_ENTRY disk_menu[] =
  {{"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_NORMAL_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Doubler Type                                                ", MENU_NORMAL_TYPE},
   {"True DAM Emulation                                          ", MENU_NORMAL_TYPE},
#ifdef __linux
   {"", MENU_TITLE_TYPE},
   {"Set Drive Steps", MENU_NORMAL_TYPE},
#endif
   {"", 0}};
  const char *doubler_choices[4] = {"     None", "   Percom", "    Tandy", "     Both"};
  const char *size_choices[2] = {"5 Inch", "8 Inch"};
  int selection = 0;
  int i, size;

  while (1) {
    for (i = 0; i < 8; i++) {
      snprintf(disk_menu[i].title, 63,
          "Disk Drive Number %d Size                              %s",
          i, size_choices[trs_disk_getsize(i) == 5 ? 0 : 1]);
    }
    snprintf(&disk_menu[9].title[51], 10, "%s", doubler_choices[trs_disk_doubler]);
    snprintf(&disk_menu[10].title[50], 11, "%s", yes_no_choices[trs_disk_truedam]);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Floppy Disk Options", disk_menu, selection);
    if (selection >= 0 && selection < 8) {
      size = trs_gui_display_popup("Size", size_choices, 2, trs_disk_getsize(selection) == 8);
      trs_disk_setsize(selection, size == 0 ? 5 : 8);
    }
    else switch (selection) {
      case 9:
        trs_disk_doubler = trs_gui_display_popup("Doubler", doubler_choices, 4, trs_disk_doubler);
        break;
      case 10:
        trs_disk_truedam = trs_gui_display_popup("True DAM", yes_no_choices, 2, trs_disk_truedam);
        break;
#ifdef __linux
      case 12:
        trs_gui_disk_steps();
        break;
#endif
      case -1:
        return;
    }
  }
}

void trs_gui_diskset_load(void)
{
  if (trs_gui_file_browse(trs_disk_set_dir, filename, ".set", 0, "Disk Set") >= 0) {
    if (trs_diskset_load(filename) != 0)
      trs_gui_display_error(filename);
  }
}

void trs_gui_diskset_save(void)
{
  filename[0] = 0;
  if (trs_gui_input_string("Enter Filename for Disk Set, TAB selects directory",
      trs_disk_set_dir, filename, FILENAME_MAX - 5, 1) == 0) {
    trs_add_extension(filename, ".set");
    if (trs_gui_file_overwrite()) {
      if (trs_diskset_save(filename) != 0)
        trs_gui_display_error(filename);
    }
  }
}

void trs_gui_disk_management(void)
{
  MENU_ENTRY disk_menu[] =
  {{" Disk 0:", MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 1:", MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 2:", MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 3:", MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 4:", MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 5:", MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 6:", MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 7:", MENU_FLOPPY_BROWSE_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Save Disk Set", MENU_NORMAL_TYPE},
   {"Load Disk Set", MENU_NORMAL_TYPE},
   {"Create Blank Floppy Disk", MENU_NORMAL_TYPE},
   {"Disk Drive Options", MENU_NORMAL_TYPE},
   {"", 0}};
  int selection = 0;
  int i;

  while (1) {
    for (i = 0; i < 8; i++) {
      const char *diskname = trs_disk_getfilename(i);

      if (diskname[0] == 0)
        snprintf(&disk_menu[i].title[8], 6, "%s", "Empty");
      else
        trs_gui_limit_string(diskname, &disk_menu[i].title[8], 52);
      disk_menu[i].title[0] = trs_disk_getwriteprotect(i) ? '*' : ' ';
    }
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Floppy Disk Management", disk_menu, selection);
    switch (selection) {
      case 9:
        trs_gui_diskset_save();
        break;
      case 10:
        trs_gui_diskset_load();
        break;
      case 11:
        trs_gui_disk_creation();
        break;
      case 12:
        trs_gui_disk_options();
        break;
      case -1:
        return;
    }
  }
}

void trs_gui_hard_management(void)
{
  MENU_ENTRY hard_menu[] =
  {{" Hard 0:", MENU_HARD_BROWSE_TYPE},
   {" Hard 1:", MENU_HARD_BROWSE_TYPE},
   {" Hard 2:", MENU_HARD_BROWSE_TYPE},
   {" Hard 3:", MENU_HARD_BROWSE_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Save Disk Set", MENU_NORMAL_TYPE},
   {"Load Disk Set", MENU_NORMAL_TYPE},
   {"Cylinder Count                                              ", MENU_NORMAL_TYPE},
   {"Sector Count                                                ", MENU_NORMAL_TYPE},
   {"Granularity                                                 ", MENU_NORMAL_TYPE},
   {"Directory Sector                                            ", MENU_NORMAL_TYPE},
   {"Insert Created Disk Into This Drive                         ", MENU_NORMAL_TYPE},
   {"Create Hard Disk Image with Above Parameters", MENU_NORMAL_TYPE},
   {"", 0}};
  const char *drive_choices[5] = {"  None", "Hard 0", "Hard 1", "Hard 2", "Hard 3"};
  static int cylinder_count = 202;
  static int sector_count = 256;
  static int granularity = 8;
  static int dir_sector = 1;
  static int drive_insert = 0;
  char input[4];
  int selection = 0;
  int i, value;

  while (1) {
    for (i = 0; i < 4; i++) {
      const char *diskname = trs_hard_getfilename(i);

      if (diskname[0] == 0)
        snprintf(&hard_menu[i].title[8], 6, "%s", "Empty");
      else
        trs_gui_limit_string(diskname, &hard_menu[i].title[8], 52);
      hard_menu[i].title[0] = trs_hard_getwriteprotect(i) ? '*' : ' ';
    }
    snprintf(&hard_menu[7].title[57], 4, "%3d", cylinder_count);
    snprintf(&hard_menu[8].title[57], 4, "%3d", sector_count);
    snprintf(&hard_menu[9].title[57], 4, "%3d", granularity);
    snprintf(&hard_menu[10].title[57], 4, "%3d", dir_sector);
    snprintf(&hard_menu[11].title[54], 7, "%6s", drive_choices[drive_insert]);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Hard Disk Management", hard_menu, selection);
    switch (selection) {
      case 5:
        trs_gui_diskset_save();
        break;
      case 6:
        trs_gui_diskset_load();
        break;
      case 7:
        snprintf(input, 4, "%d", cylinder_count);
        if (trs_gui_input_string("Enter Cylinder Count", input, input, 3, 0) == 0) {
          value = atoi(input);
          if (value >= 3 && value <= 256) {
            cylinder_count = value;
            if (cylinder_count > 203)
              trs_gui_display_message("Warning",
                  "Cylinder Count > 203 is incompatible with XTRSHARD/DCT");
          } else
            trs_gui_display_message("ERROR",
                "Cylinder Count must be between 3 and 256");
        }
        break;
      case 8:
        snprintf(input, 4, "%d", sector_count);
        if (trs_gui_input_string("Enter Sector Count", input, input, 3, 0) == 0) {
          value = atoi(input);
          if (value >= 4 && value <= 256) {
            sector_count = value;
            if ((sector_count % 32) != 0) {
              trs_gui_display_message("Warning",
                  "Sector not a mult of 32 is incompatible with WD1000/1010");
              if (sector_count > 32)
                trs_gui_display_message("Warning",
                    "Sector is incompatible with Matthew Reed's emulators");
            }
          } else
            trs_gui_display_message("ERROR",
                "Sector Count must be between 4 and 256");
        }
        break;
      case 9:
        snprintf(input, 2, "%d", granularity);
        if (trs_gui_input_string("Enter Granularity", input, input, 1, 0) == 0) {
          value = atoi(input);
          if (value >= 1 && value <= 8)
            granularity = value;
          else
            trs_gui_display_message("ERROR", "Granularity must be between 1 and 8");
        }
        break;
      case 10:
        snprintf(input, 4, "%d", dir_sector);
        if (trs_gui_input_string("Enter Directory Sector", input, input, 3, 0) == 0) {
          value = atoi(input);
          if (value >= 1 && value < cylinder_count)
            dir_sector = value;
          else
            trs_gui_display_message("ERROR",
                "Directory Sector must be between 1 and Cylinder Count - 1");
        }
        break;
      case 11:
        drive_insert = trs_gui_display_popup("Hard", drive_choices, 5, drive_insert);
        break;
      case 12:
        if (sector_count < granularity) {
          trs_gui_display_message("ERROR", "Sector Count must be >= Granularity");
          break;
        }
        if ((sector_count % granularity) != 0) {
          trs_gui_display_message("ERROR", "Sector Count must be multiple of Granularity");
          break;
        }
        if ((sector_count / granularity) > 32) {
          trs_gui_display_message("ERROR", "Sector Count / Granularity must be <= 32");
          break;
        }
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Hard Disk Image, TAB selects directory",
            trs_hard_dir, filename, 191, 1) == 0) {
          if (trs_gui_file_overwrite()) {
            if (trs_create_blank_hard(filename, cylinder_count, sector_count,
                granularity, dir_sector) != 0)
              trs_gui_display_error(filename);
            else if (drive_insert)
              trs_hard_attach(drive_insert - 1, filename);
            return;
          }
        }
        break;
      case -1:
        return;
    }
  }
}

void trs_gui_stringy_management(void)
{
  MENU_ENTRY stringy_menu[] =
  {{" Wafer 0:", MENU_WAFER_BROWSE_TYPE},
   {" Wafer 1:", MENU_WAFER_BROWSE_TYPE},
   {" Wafer 2:", MENU_WAFER_BROWSE_TYPE},
   {" Wafer 3:", MENU_WAFER_BROWSE_TYPE},
   {" Wafer 4:", MENU_WAFER_BROWSE_TYPE},
   {" Wafer 5:", MENU_WAFER_BROWSE_TYPE},
   {" Wafer 6:", MENU_WAFER_BROWSE_TYPE},
   {" Wafer 7:", MENU_WAFER_BROWSE_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Save Disk Set", MENU_NORMAL_TYPE},
   {"Load Disk Set", MENU_NORMAL_TYPE},
   {"Insert Created Image Into This Wafer                       ", MENU_NORMAL_TYPE},
   {"Create Blank Floppy Wafer", MENU_NORMAL_TYPE},
   {"", 0}};
  const char *wafer_choices[9] = {"   None", "Wafer 0", "Wafer 1", "Wafer 2", "Wafer 3",
                                  "Wafer 4", "Wafer 5", "Wafer 6", "Wafer 7"};
  static int wafer_insert = 0;
  int selection = 0;
  int i;

  while (1) {
    for (i = 0; i < 8; i++) {
      const char *wafername = stringy_get_name(i);

      if (wafername[0] == 0)
        snprintf(&stringy_menu[i].title[9], 6, "%s", "Empty");
      else
        trs_gui_limit_string(wafername, &stringy_menu[i].title[9], 52);
      stringy_menu[i].title[0] = stringy_get_writeprotect(i) ? '*' : ' ';
    }
    snprintf(&stringy_menu[11].title[52], 10, "%8s", wafer_choices[wafer_insert]);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Stringy Wafer Management", stringy_menu, selection);
    switch (selection) {
      case 9:
        trs_gui_diskset_save();
        break;
      case 10:
        trs_gui_diskset_load();
        break;
      case 11:
        wafer_insert = trs_gui_display_popup("Wafer", wafer_choices, 9, wafer_insert);
        break;
      case 12:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Wafer Image, TAB selects directory",
            trs_cass_dir, filename, FILENAME_MAX, 1) == 0) {
          if (trs_gui_file_overwrite()) {
            if (stringy_create(filename) != 0)
              trs_gui_display_error(filename);
            else
              if (wafer_insert)
                stringy_insert(wafer_insert - 1, filename);
            return;
          }
        }
        break;
      case -1:
        return;
    }
  }
}

void trs_gui_cassette_management(void)
{
  MENU_ENTRY cass_menu[] =
  {{" Cass  :", MENU_CASS_BROWSE_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Cassette Position                                           ", MENU_NORMAL_TYPE},
   {"Cassette Default Sample Rate                                ", MENU_NORMAL_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Image Type                                                  ", MENU_NORMAL_TYPE},
   {"Insert Created Cassette Into Drive                          ", MENU_NORMAL_TYPE},
   {"Create Blank Cassette Image with Above Parameters", MENU_NORMAL_TYPE},
   {"", 0}};
  const char *image_type_choices[3] = {"   CAS", "   CPT", "   WAV"};
  static int image_type = 0;
  static int drive_insert = 1;
  char input[12];
  int selection = 0;
  int value;

  while (1) {
    const char *cass_name = trs_cassette_getfilename();

    if (cass_name[0] == 0)
      snprintf(&cass_menu[0].title[8], 6, "%s", "Empty");
    else
      trs_gui_limit_string(cass_name, &cass_menu[0].title[8], 52);
    cass_menu[0].title[0] = trs_cass_getwriteprotect() ? '*' : ' ';

    snprintf(&cass_menu[2].title[36], 25, "%10d of %10d", trs_get_cassette_position(), trs_get_cassette_length());
    snprintf(&cass_menu[3].title[50], 11, "%10d", cassette_default_sample_rate);
    snprintf(&cass_menu[5].title[54], 7, "%s", image_type_choices[image_type]);
    snprintf(&cass_menu[6].title[50], 11, "%s", yes_no_choices[drive_insert]);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Cassette Management", cass_menu, selection);
    switch (selection) {
      case 2:
        snprintf(input, 11, "%d", trs_get_cassette_position());
        if (trs_gui_input_string("Enter Cassette Position in Bytes", input, input, 10, 0) == 0) {
          value = atoi(input);
          if (value >= 0 && value <= trs_get_cassette_length())
            trs_set_cassette_position(value);
        }
        break;
      case 3:
        snprintf(input, 11, "%d", cassette_default_sample_rate);
        if (trs_gui_input_string("Enter Cassette Default Sample Rate", input, input, 10, 0) == 0) {
          value = atoi(input);
          if (value >= 0 && value <= DEFAULT_SAMPLE_RATE)
            cassette_default_sample_rate = value;
        }
        break;
      case 5:
        image_type = trs_gui_display_popup("Type", image_type_choices, 3, image_type);
        break;
      case 6:
        drive_insert = trs_gui_display_popup("Insert", yes_no_choices, 2, drive_insert);
        break;
      case 7:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Cassette Image, TAB selects directory",
            trs_cass_dir, filename, FILENAME_MAX, 1) == 0) {
          switch (image_type) {
            case 0:
              trs_add_extension(filename, ".cas");
              break;
            case 1:
              trs_add_extension(filename, ".cpt");
              break;
            default:
              trs_add_extension(filename, ".wav");
              break;
          }
          if (trs_gui_file_overwrite()) {
            FILE *cassette_file = fopen(filename, "wb");

            if (cassette_file) {
              if (image_type == 2) {
                 if (create_wav_header(cassette_file) < 0)
                   trs_gui_display_message("ERROR", "Failed to create WAVE header");
              }
              fclose(cassette_file);
              if (drive_insert)
                trs_cassette_insert(filename);
            } else
              trs_gui_display_error(filename);
          }
        }
        break;
      case -1:
        return;
    }
  }
}

void trs_gui_emulator_settings(void)
{
  MENU_ENTRY model_menu[] =
  {{"Model                                                       ", MENU_NORMAL_TYPE},
   {"CPU Clock Speed                                             ", MENU_NORMAL_TYPE},
   {"Exatron Stringy Floppy Emulation for Model I                ", MENU_NORMAL_TYPE},
   {"Lowercase Modification for Model I                          ", MENU_NORMAL_TYPE},
   {"Speedup Kit Emulation for Model I                           ", MENU_NORMAL_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Grafyx Solution (Micro-Labs) Graphics Emulation             ", MENU_NORMAL_TYPE},
   {"LE18 (Lowe Electronics) Graphics Emulation                  ", MENU_NORMAL_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Dave Huffman (and other) 4/4P Memory Expansion              ", MENU_NORMAL_TYPE},
   {"HyperMem (Anitek Software) 4/4P Memory Expansion            ", MENU_NORMAL_TYPE},
   {"SuperMem (Alpha Technology) I/III Memory Expansion          ", MENU_NORMAL_TYPE},
   {"TRS-80 Users Society Selector I Memory Expansion            ", MENU_NORMAL_TYPE},
   {"", 0}};
  const char *model_choices[4] =  {"  TRS-80 Model I",
                                   "TRS-80 Model III",
                                   "  TRS-80 Model 4",
                                   " TRS-80 Model 4P"};
  const char *speed_choices[3] =  {"       None", "   Archbold", "Sprinter II"};
  float clock_mhz[4] = { clock_mhz_1, clock_mhz_3, clock_mhz_4, clock_mhz_4 };
  char input[8];
  int selection = 0;
  int model_selection = trs_model == 1 ? 0 : trs_model - 2;

  while (1) {
    snprintf(&model_menu[0].title[44], 17, "%s", model_choices[model_selection]);
    snprintf(&model_menu[1].title[50], 11, "%6.2f MHz", clock_mhz[model_selection]);
    snprintf(&model_menu[2].title[50], 11, "%s", yes_no_choices[stringy]);
    snprintf(&model_menu[3].title[50], 11, "%s", yes_no_choices[lowercase]);
    snprintf(&model_menu[4].title[49], 12, "%s", speed_choices[speedup]);
    snprintf(&model_menu[6].title[50], 11, "%s", yes_no_choices[grafyx_get_microlabs()]);
    snprintf(&model_menu[7].title[50], 11, "%s", yes_no_choices[lowe_le18]);
    snprintf(&model_menu[9].title[50], 11, "%s", yes_no_choices[huffman_ram]);
    snprintf(&model_menu[10].title[50], 11, "%s", yes_no_choices[hypermem]);
    snprintf(&model_menu[11].title[50], 11, "%s", yes_no_choices[supermem]);
    snprintf(&model_menu[12].title[50], 11, "%s", yes_no_choices[selector]);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Emulator Settings", model_menu, selection);
    switch (selection) {
      case 0:
        model_selection = trs_gui_display_popup("Model", model_choices, 4, model_selection);
        break;
      case 1:
        snprintf(input, 6, "%.2f", clock_mhz[model_selection]);
        if (trs_gui_input_string("Enter CPU Clock Speed in MHz",
            input, input, 6, 0) == 0) {
          float value = atof(input);

          if (value >= 0.1 && value <= 99.0) {
            clock_mhz[model_selection] = value;
            switch (model_selection) {
              case 0:
              default:
                clock_mhz_1 = value;
                break;
              case 1:
                clock_mhz_3 = value;
                break;
              case 2:
              case 3:
                clock_mhz_4 = value;
                break;
            }
            trs_timer_init();
          }
        }
        break;
      case 2:
        stringy = trs_gui_display_popup("Stringy", yes_no_choices, 2, stringy);
        break;
      case 3:
        lowercase = trs_gui_display_popup("Lowercase", yes_no_choices, 2, lowercase);
        break;
      case 4:
        speedup = trs_gui_display_popup("Speedup", speed_choices, 3, speedup);
        break;
      case 6:
        grafyx_set_microlabs(trs_gui_display_popup("Grafyx", yes_no_choices, 2,
            grafyx_get_microlabs()));
        break;
      case 7:
        lowe_le18 = trs_gui_display_popup("LE18", yes_no_choices, 2, lowe_le18);
        break;
      case 9:
        huffman_ram = trs_gui_display_popup("Huffman", yes_no_choices, 2, huffman_ram);
        if (huffman_ram)
          hypermem = 0;
        break;
      case 10:
        hypermem = trs_gui_display_popup("HyperMem", yes_no_choices, 2, hypermem);
        if (hypermem)
          huffman_ram = 0;
        break;
      case 11:
        supermem = trs_gui_display_popup("SuperMem", yes_no_choices, 2, supermem);
        if (supermem)
          selector = 0;
        break;
      case 12:
        selector = trs_gui_display_popup("Selector", yes_no_choices, 2, selector);
        if (selector)
          supermem = 0;
        break;
      case -1:
        model_selection = (model_selection == 0 ? 1 : model_selection + 2);
        if (trs_model != model_selection) {
          trs_model = model_selection;
          trs_gui_new_machine();
        }
        return;
    }
  }
}

void trs_gui_display_settings(void)
{
  MENU_ENTRY display_menu[] =
  {{"Emulator Background Color                                   ", MENU_NORMAL_TYPE},
   {"Emulator Foreground Color                                   ", MENU_NORMAL_TYPE},
   {"GUI Background Color                                        ", MENU_NORMAL_TYPE},
   {"GUI Foreground Color                                        ", MENU_NORMAL_TYPE},
   {"Model 1 Character Set                                       ", MENU_NORMAL_TYPE},
   {"Model 3 Character Set                                       ", MENU_NORMAL_TYPE},
   {"Model 4/4P Character Set                                    ", MENU_NORMAL_TYPE},
   {"Border Width                                                ", MENU_NORMAL_TYPE},
   {"Resize Window on Mode Change for Model 3                    ", MENU_NORMAL_TYPE},
   {"Resize Window on Mode Change for Model 4                    ", MENU_NORMAL_TYPE},
   {"Scale Factor for Window                                     ", MENU_NORMAL_TYPE},
   {"LED Display for Disks and Turbo Mode                        ", MENU_NORMAL_TYPE},
#ifdef OLD_SCANLINES
   {"Display Scanlines with Background Color                     ", MENU_NORMAL_TYPE},
#else
   {"Display Scanlines with brightness                           ", MENU_NORMAL_TYPE},
#endif
   {"", 0}};
  const char *font1_choices[7] =    {"      Early",
                                     "      Stock",
                                     "      LCmod",
                                     "      Wider",
                                     "      Genie",
                                     "   HT-1080Z",
                                     "Video Genie"};
  const char *font34_choices[3] =   {"     Katakana",
                                     "International",
                                     "         Bold"};
  const char *scale_choices[4] =    {" None", "  2 x", "  3 x", "  4 x"};
  char input[8];
  int redraw = 0;
  int selection = 0;
  int value = 0;
  int gui_charset1 = trs_charset1 >= 10 ? trs_charset1 - 6 : trs_charset1;
  unsigned int rgb_color = 0;

  while (1) {
    snprintf(&display_menu[0].title[52], 9, "0x%06X", background);
    snprintf(&display_menu[1].title[52], 9, "0x%06X", foreground);
    snprintf(&display_menu[2].title[52], 9, "0x%06X", gui_background);
    snprintf(&display_menu[3].title[52], 9, "0x%06X", gui_foreground);
    snprintf(&display_menu[4].title[49], 12, "%s", font1_choices[gui_charset1]);
    snprintf(&display_menu[5].title[47], 14, "%s", font34_choices[trs_charset3 - 4]);
    snprintf(&display_menu[6].title[47], 14, "%s", font34_choices[trs_charset4 - 7]);
    snprintf(&display_menu[7].title[52], 9, "%8d", window_border_width);
    snprintf(&display_menu[8].title[50], 11, "%s", yes_no_choices[resize3]);
    snprintf(&display_menu[9].title[50], 11, "%s", yes_no_choices[resize4]);
    snprintf(&display_menu[10].title[55], 6, "%s", scale_choices[scale - 1]);
    snprintf(&display_menu[11].title[50], 11, "%s", yes_no_choices[trs_show_led]);
#ifdef OLD_SCANLINES
    snprintf(&display_menu[12].title[50], 11, "%s", yes_no_choices[scanlines]);
#else
    snprintf(&display_menu[12].title[34], 27, "%-3d%23s", scanshade, yes_no_choices[scanlines]);
#endif
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Display Settings", display_menu, selection);
    switch (selection) {
      case 0:
        snprintf(input, 7, "%06X", background);
        if (trs_gui_input_string("Enter Background RGB color (Hex, RRGGBB)",
            input, input, 6, 0) == 0) {
          rgb_color = strtol(input, NULL, 16);
          if (rgb_color != background) {
            background = rgb_color;
            redraw = 1;
          }
        }
        break;
      case 1:
        snprintf(input, 7, "%06X", foreground);
        if (trs_gui_input_string("Enter Foreground RGB color (Hex, RRGGBB)",
            input, input, 6, 0) == 0) {
          rgb_color = strtol(input, NULL, 16);
          if (rgb_color != foreground) {
            foreground = rgb_color;
            redraw = 1;
          }
        }
        break;
      case 2:
        snprintf(input, 7, "%06X", gui_background);
        if (trs_gui_input_string("Enter GUI Background RGB color (Hex, RRGGBB)",
            input, input, 6, 0) == 0) {
          rgb_color = strtol(input, NULL, 16);
          if (rgb_color != gui_background) {
            gui_background = rgb_color;
            redraw = 1;
          }
        }
        break;
      case 3:
        snprintf(input, 7, "%06X", gui_foreground);
        if (trs_gui_input_string("Enter GUI Foreground RGB color (Hex, RRGGBB)",
            input, input, 6, 0) == 0) {
          rgb_color = strtol(input, NULL, 16);
          if (rgb_color != gui_foreground) {
            gui_foreground = rgb_color;
            redraw = 1;
          }
        }
        break;
      case 4:
        value = trs_gui_display_popup("Charset 1", font1_choices, 7, gui_charset1);
        if (value != gui_charset1) {
          gui_charset1 = value;
          trs_charset1 = value >= 4 ? value += 6 : value;
          redraw = 1;
        }
        break;
      case 5:
        value = trs_gui_display_popup("Charset 3", font34_choices, 3, trs_charset3 - 4) + 4;
        if (value != trs_charset3) {
          trs_charset3 = value;
          redraw = 1;
        }
        break;
      case 6:
        value = trs_gui_display_popup("Charset 4/4P", font34_choices, 3, trs_charset4 - 7) + 7;
        if (value != trs_charset4) {
          trs_charset4 = value;
          redraw = 1;
        }
        break;
      case 7:
        snprintf(input, 3, "%d", window_border_width);
        if (trs_gui_input_string("Enter Window border width in pixels (0 to 50)",
            input, input, 2, 0) == 0) {
          value = atol(input);
          if (value != window_border_width) {
            window_border_width = value;
            if (window_border_width < 0 || window_border_width > 50)
              window_border_width = 2;
            redraw = 1;
          }
        }
        break;
      case 8:
        value = trs_gui_display_popup("Resize 3", yes_no_choices, 2, resize3);
        if (value != resize3) {
          resize3 = value;
          redraw = 1;
        }
        break;
      case 9:
        value = trs_gui_display_popup("Resize 4", yes_no_choices, 2, resize4);
        if (value != resize4) {
          resize4 = value;
          redraw = 1;
        }
        break;
      case 10:
        value = trs_gui_display_popup("Scale", scale_choices, 4, scale - 1) + 1;
        if (value != scale) {
          scale = value;
          fullscreen = 0;
          redraw = 1;
        }
        break;
      case 11:
        value = trs_gui_display_popup("LEDs", yes_no_choices, 2, trs_show_led);
        if (value != trs_show_led) {
          trs_show_led = value;
          redraw = 1;
        }
        break;
      case 12:
        value = trs_gui_display_popup("Scanlines", yes_no_choices, 2, scanlines);
#ifdef OLD_SCANLINES
        if (value != scanlines)
#else
        if (value) {
          snprintf(input, 4, "%d", scanshade);
          if (trs_gui_input_string("Enter brightness (0 = dark - 255 = light)",
              input, input, 3, 0) == 0)
            scanshade = atoi(input) & 255;
        }
#endif
          redraw = 1;
        scanlines = value;
        break;
      case -1:
        return;
    }

    if (redraw) {
      trs_screen_init();
      redraw = 0;
    }
  }
}

void trs_gui_misc_settings(void)
{
  MENU_ENTRY misc_menu[] =
  {{"Emtsafe                                                     ", MENU_NORMAL_TYPE},
   {"Keystretch Value                                            ", MENU_NORMAL_TYPE},
   {"Serial Port Name:                                           ", MENU_TITLE_TYPE},
   {"                                                            ", MENU_NORMAL_TYPE},
   {"Serial Switches                                             ", MENU_NORMAL_TYPE},
   {"Shift Bracket Emulation                                     ", MENU_NORMAL_TYPE},
   {"Sound Output                                                ", MENU_NORMAL_TYPE},
   {"Turbo Mode                                                  ", MENU_NORMAL_TYPE},
   {"Turbo Speed                                                 ", MENU_NORMAL_TYPE},
#if defined(SDL2) || !defined(NOX)
   {"Turbo Paste                                                 ", MENU_NORMAL_TYPE},
#endif
   {"", 0}};
  char input[12];
  int selection = 0;

  while (1) {
    snprintf(&misc_menu[0].title[50], 11, "%s", yes_no_choices[trs_emtsafe]);
    snprintf(&misc_menu[1].title[50], 11, "%10d", stretch_amount);
    trs_gui_limit_string(trs_uart_name, &misc_menu[3].title[2], 58);
    snprintf(&misc_menu[4].title[56], 5, "0x%02X", trs_uart_switches);
    snprintf(&misc_menu[5].title[50], 11, "%s", yes_no_choices[trs_kb_bracket_state]);
    snprintf(&misc_menu[6].title[50], 11, "%s", yes_no_choices[trs_sound]);
    snprintf(&misc_menu[7].title[50], 11, "%s", yes_no_choices[timer_overclock]);
    snprintf(&misc_menu[8].title[50], 11, "%10d", timer_overclock_rate);
#if defined(SDL2) || !defined(NOX)
    snprintf(&misc_menu[9].title[50], 11, "%s", yes_no_choices[turbo_paste]);
#endif
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Miscellaneous Settings", misc_menu, selection);
    switch (selection) {
      case 0:
        trs_emtsafe = trs_gui_display_popup("Emtsafe", yes_no_choices, 2, trs_emtsafe);
        break;
      case 1:
        snprintf(input, 11, "%d", stretch_amount);
        if (trs_gui_input_string("Enter Keystretch in Cycles", input, input, 10, 0) == 0) {
          stretch_amount = atoi(input);
          if (stretch_amount < 0)
            stretch_amount = STRETCH_AMOUNT;
        }
        break;
      case 3:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Serial Port Name", trs_uart_name,
            filename, FILENAME_MAX, 0) == 0) {
          snprintf(trs_uart_name, FILENAME_MAX, "%s", filename);
          trs_uart_init(0);
        }
        break;
      case 4:
        snprintf(input, 3, "%2X", trs_uart_switches);
        if (trs_gui_input_string("Enter Serial Switches (Hex, XX)", input, input, 2, 0) == 0) {
          trs_uart_switches = strtol(input, NULL, 16);
          trs_uart_init(0);
        }
        break;
      case 5:
        trs_kb_bracket_state = trs_gui_display_popup("Bracket", yes_no_choices, 2, trs_kb_bracket_state);
        break;
      case 6:
        trs_sound = trs_gui_display_popup("Sound", yes_no_choices, 2, trs_sound);
        break;
      case 7:
        timer_overclock = trs_gui_display_popup("Turbo", yes_no_choices, 2, timer_overclock);
        break;
      case 8:
        snprintf(input, 11, "%d", timer_overclock_rate);
        if (trs_gui_input_string("Enter Turbo Rate Multiplier", input, input, 10, 0) == 0) {
          timer_overclock_rate = atoi(input);
          if (timer_overclock_rate <= 0)
            timer_overclock_rate = 1;
        }
        break;
#if defined(SDL2) || !defined(NOX)
      case 9:
        turbo_paste = trs_gui_display_popup("Paste", yes_no_choices, 2, turbo_paste);
        break;
#endif
      case -1:
        trs_kb_bracket(trs_kb_bracket_state);
        trs_turbo_mode(timer_overclock);
        return;
    }
  }
}

void trs_gui_save_state(void)
{
  filename[0] = 0;
  if (trs_gui_input_string("Save Emulator State, TAB selects directory",
      trs_state_file[0] != 0 ? trs_state_file : trs_state_dir, filename,
      FILENAME_MAX - 5, 1) == 0) {
    trs_add_extension(filename, ".t8s");
    if (trs_gui_file_overwrite()) {
      if (trs_state_save(filename) == 0)
        snprintf(trs_state_file, FILENAME_MAX, "%s", filename);
      else
        trs_gui_display_error(filename);
    }
  }
}

int trs_gui_load_state(void)
{
  if (trs_gui_file_browse(trs_state_dir, filename, ".t8s", 0,
      "State (.t8s)") >= 0) {
    if (trs_state_load(filename) == 0) {
      trs_screen_init();
      return 0;
    } else
      trs_gui_display_error(filename);
  }
  return -1;
}

void trs_gui_write_config(void)
{
  filename[0] = 0;
  if (trs_gui_input_string("Write Configuration, TAB selects directory",
      trs_config_file[0] != 0 ? trs_config_file : trs_state_dir, filename,
      FILENAME_MAX - 5, 1) == 0) {
    trs_add_extension(filename, ".t8c");
    if (trs_gui_file_overwrite()) {
      if (trs_write_config_file(filename) == 0)
        snprintf(trs_config_file, FILENAME_MAX, "%s", filename);
      else
        trs_gui_display_error(filename);
    }
  }
}

int trs_gui_read_config(void)
{
  if (trs_gui_file_browse(trs_config_file, trs_config_file, ".t8c", 0,
      "Configuration (.t8c)") >= 0) {
    if (trs_load_config_file() == 0) {
      trs_gui_new_machine();
      return 0;
    }
    trs_gui_display_error(trs_config_file);
  }
  return -1;
}

static int trs_gui_config_management(void)
{
  MENU_ENTRY misc_menu[] =
  {{"Save Emulator State (Alt-S)", MENU_NORMAL_TYPE},
   {"Load Emulator State (Alt-L)", MENU_NORMAL_TYPE},
   {"Write Configuration (Alt-W)", MENU_NORMAL_TYPE},
   {"Read Configuration  (Alt-R)", MENU_NORMAL_TYPE},
   {"", 0}};
  int selection = 0;

  while (1) {
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Configuration/State Files", misc_menu, selection);
    switch (selection) {
      case 0:
        trs_gui_save_state();
        break;
      case 1:
        if (trs_gui_load_state() == 0)
          return 1;
        break;
      case 2:
        trs_gui_write_config();
        break;
      case 3:
        if (trs_gui_read_config() == 0)
          return 1;
        break;
      case -1:
        return 0;
    }
  }
}

void trs_gui_printer_management(void)
{
  MENU_ENTRY printer_menu[] =
  {{"Close and Reopen Printer Output File", MENU_NORMAL_TYPE},
   {"Printer Type                                                ", MENU_NORMAL_TYPE},
   {"Printer Command:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"", 0}};
  const char *printer_choices[2] = {"     None", "     Text"};
  int selection = 0;

  while (1) {
    snprintf(&printer_menu[1].title[51], 10, "%s", printer_choices[trs_printer]);
    trs_gui_limit_string(trs_printer_command, &printer_menu[3].title[2], 58);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Printer Management", printer_menu, selection);
    switch (selection) {
      case 0:
        if (trs_printer_reset() == 0)
          trs_gui_display_message("Status", "Printer file closed, printer command ran");
        else
          trs_gui_display_message("Warning", "No Printer Output in File");
        break;
      case 1:
        trs_printer = trs_gui_display_popup("Printer", printer_choices, 2, trs_printer);
        break;
      case 3:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Printer Command", trs_printer_command,
            filename, FILENAME_MAX, 0) == 0)
          snprintf(trs_printer_command, FILENAME_MAX, "%s", filename);
        break;
      case -1:
        return;
    }
  }
}

const char *trs_gui_get_key_name(int key)
{
  int i, found = 0, shifted = 0;

  for (i = 0; i < N_KEYS && !found; i++)
    if (key_syms[i] == key)
      found = 1;
  if (!found) {
    shifted = 1;
    for (i = 0; i < N_KEYS && !found; i++)
      if (key_syms_shifted[i] == key)
        found = 1;
  }
  if (found)
    return !shifted ? key_names[i - 1] : key_names_shifted[i - 1];
  else
    return "???";
}

int trs_gui_virtual_keyboard(void)
{
  static int saved_selection = 0;
  int key_index = SHIFT, shifted = 0;

  while (key_index == SHIFT || (shifted && key_syms_shifted[key_index] == -1)) {
    key_index = trs_gui_display_popup_matrix("Select Key",
        !shifted ? key_names : key_names_shifted, 4, 13, saved_selection);
    if (key_index == -1)
      return -1;
    if (key_index == SHIFT)
      shifted = 1 - shifted;
    saved_selection = key_index;
  }
  return !shifted ? key_syms[key_index] : key_syms_shifted[key_index];
}

void trs_gui_get_virtual_key(void)
{
  int key = trs_gui_virtual_keyboard();

  if (key != -1)
    trs_xlate_keysym(key);
}

void trs_gui_joy_gui(void)
{
  int selection = trs_gui_display_popup_matrix("Joystick GUI", function_choices, 3, 2, 0);

  if (selection == -1)
    return;

  switch (function_codes[selection]) {
    case GUI:
      trs_gui();
      break;
    case KEYBRD:
      trs_gui_get_virtual_key();
      break;
    case SAVE:
      trs_gui_save_state();
      break;
    case LOAD:
      trs_gui_load_state();
      break;
    case RESET:
      trs_reset(1);
      break;
    case EXIT:
      trs_exit(1);
      break;
  }
}

int trs_gui_joystick_get_button(void)
{
  SDL_Event event;

  trs_gui_frame(25, 7, 38, 9);
  trs_gui_write_text("Press Button", 26, 8, 0);
  trs_screen_update();

  while (1) {
    SDL_WaitEvent(&event);
    switch (event.type) {
      case SDL_QUIT:
        trs_exit(0);
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.mod & KMOD_ALT) {
          switch (event.key.keysym.sym) {
#ifdef _WIN32
            case SDLK_F4:
#endif
            case SDLK_q:
              trs_exit(1);
              break;
            default:
              break;
          }
        }
        else if (event.key.keysym.sym == SDLK_F8)
          trs_exit(!(event.key.keysym.mod & KMOD_SHIFT));
        else if (event.key.keysym.sym == SDLK_ESCAPE)
          return -1;
        break;
      case SDL_JOYBUTTONDOWN:
      case SDL_MOUSEBUTTONDOWN:
        if (event.type == SDL_MOUSEBUTTONDOWN)
          event.jbutton.button = event.button.button;
        if (event.jbutton.button < N_JOYBUTTONS)
          return event.jbutton.button;
        else {
          trs_gui_display_message("ERROR", "Unsupported Button");
          return -1;
        }
    }
  }
}

void trs_gui_joystick_display_map(int button)
{
  int row, col, i;
  char text[10];

  for (col = 0; col < 5; col++) {
    for (row = 0; row < 4; row++) {
      i = col * 4 + row;
      snprintf(text, 4, "%2d:", i);
      trs_gui_write_text(text, 2 + col * 12, 11 + row, 0);
      switch (jbutton_map[i]) {
        case -1:     snprintf(text, 9, "---     "); break;
        case GUI:    snprintf(text, 9, "<GUI>   "); break;
        case KEYBRD: snprintf(text, 9, "<KEYBRD>"); break;
        case SAVE:   snprintf(text, 9, "<SAVE>  "); break;
        case LOAD:   snprintf(text, 9, "<LOAD>  "); break;
        case RESET:  snprintf(text, 9, "<RESET> "); break;
        case EXIT:   snprintf(text, 9, "<EXIT>  "); break;
        case PAUSE:  snprintf(text, 9, "<PAUSE> "); break;
        case JOYGUI: snprintf(text, 9, "<JOYGUI>"); break;
        default:
          snprintf(text, 9, "%s", trs_gui_get_key_name(jbutton_map[i]));
          break;
      }
      trs_gui_write_text(text, 5 + col * 12, 11 + row, button == i);
    }
  }
}

void trs_gui_joystick_settings(void)
{
  MENU_ENTRY display_menu[] =
  {{"Use Keypad for Joystick                                     ", MENU_NORMAL_TYPE},
   {"USB Joystick/Gamepad                                        ", MENU_NORMAL_TYPE},
   {"Map Joystick/Mouse to Arrow Keys                            ", MENU_NORMAL_TYPE},
   {"Map Button to Key", MENU_NORMAL_TYPE},
   {"Map Button to Function", MENU_NORMAL_TYPE},
   {"Unmap Button", MENU_NORMAL_TYPE},
   {"Unmap All Buttons", MENU_NORMAL_TYPE},
   {"Check Button Mapping", MENU_NORMAL_TYPE},
   {"", 0}};
  char *joystick_choices[MAX_JOYSTICKS + 1];
  char joystick_strings[MAX_JOYSTICKS + 1][64];
  int selection = 0;
  int button, key;
  int i, num_joysticks, joy_index;
  int gui_keypad_joystick = trs_keypad_joystick;
  int gui_joystick_num = trs_joystick_num;

  for (i = 0; i < MAX_JOYSTICKS + 1; i++)
    joystick_choices[i] = joystick_strings[i];

  while (1) {
    snprintf(&display_menu[0].title[50], 11, "%s", yes_no_choices[gui_keypad_joystick]);
    if (gui_joystick_num == -1)
      snprintf(&display_menu[1].title[50], 11, "      None");
    else
      snprintf(&display_menu[1].title[50], 11, "Joystick %1d", gui_joystick_num);
    snprintf(&display_menu[2].title[50], 11, "%s", yes_no_choices[jaxis_mapped]);
    trs_gui_clear_screen();
    trs_gui_joystick_display_map(-1);

    selection = trs_gui_display_menu("SDLTRS Joystick Settings", display_menu, selection);
    switch (selection) {
      case 0:
        gui_keypad_joystick = trs_gui_display_popup("Keypad", yes_no_choices, 2, gui_keypad_joystick);
        break;
      case 1:
        num_joysticks = SDL_NumJoysticks();
        if (num_joysticks > MAX_JOYSTICKS)
          num_joysticks = MAX_JOYSTICKS;
        snprintf(joystick_choices[0], 61, "%60s", "None");
        for (i = 0; i < num_joysticks; i++) {
          snprintf(joystick_choices[i + 1], 61, "Joystick %1d - %47s", i,
#ifdef SDL2
              SDL_JoystickName(SDL_JoystickOpen(i)));
#else
              SDL_JoystickName(i));
#endif
        }
        if ((gui_joystick_num == -1) || (gui_joystick_num >= num_joysticks))
          joy_index = 0;
        else
          joy_index = gui_joystick_num + 1;
        joy_index = trs_gui_display_popup("Joystick", (const char**)joystick_choices,
            num_joysticks + 1, joy_index);
        gui_joystick_num = joy_index - 1;
        break;
      case 2:
        jaxis_mapped = trs_gui_display_popup("Arrow", yes_no_choices, 2, jaxis_mapped);
        break;
      case 3:
        if ((key = trs_gui_virtual_keyboard()) != -1) {
          if ((button = trs_gui_joystick_get_button()) != -1)
            jbutton_map[button] = key;
        }
        break;
      case 4:
        if ((key = trs_gui_display_popup_matrix("Select Function", function_choices,
            4, 2, 0)) != -1) {
          if ((button = trs_gui_joystick_get_button()) != -1)
            jbutton_map[button] = function_codes[key];
        }
        break;
      case 5:
        if ((button = trs_gui_joystick_get_button()) != -1)
          jbutton_map[button] = -1;
        break;
      case 6:
        if (trs_gui_display_question("Sure?") == 1) {
          for (i = 0; i < N_JOYBUTTONS; i++)
            jbutton_map[i] = -1;
         }
        break;
      case 7:
        if ((button = trs_gui_joystick_get_button()) != -1) {
          trs_gui_joystick_display_map(button);
          trs_screen_update();
          SDL_Delay(1000);
        }
        break;
      case -1:
        if (trs_keypad_joystick != gui_keypad_joystick) {
          trs_keypad_joystick = gui_keypad_joystick;
          trs_set_keypad_joystick();
        }
        if (trs_joystick_num != gui_joystick_num) {
          trs_joystick_num = gui_joystick_num;
          trs_open_joystick();
        }
        return;
    }
  }
}

void trs_gui_default_dirs(void)
{
  MENU_ENTRY default_menu[] =
  {{"Floppy Disk Directory:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"Hard Disk Directory:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"Cassette/Wafer Directory:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"Disk Set Directory:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"State/Configuration Directory:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"Printer Output/Screenshot Directory:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"", 0}};
  int selection = 1;

  while (1) {
    trs_gui_limit_string(trs_disk_dir, &default_menu[1].title[2], 58);
    trs_gui_limit_string(trs_hard_dir, &default_menu[3].title[2], 58);
    trs_gui_limit_string(trs_cass_dir, &default_menu[5].title[2], 58);
    trs_gui_limit_string(trs_disk_set_dir, &default_menu[7].title[2], 58);
    trs_gui_limit_string(trs_state_dir, &default_menu[9].title[2], 58);
    trs_gui_limit_string(trs_printer_dir, &default_menu[11].title[2], 58);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Default Directories", default_menu, selection);
    switch (selection) {
      case 1:
        trs_gui_file_browse(trs_disk_dir, trs_disk_dir, NULL, 1, "Floppy Disk ");
        break;
      case 3:
        trs_gui_file_browse(trs_hard_dir, trs_hard_dir, NULL, 1, "Hard Disk ");
        break;
      case 5:
        trs_gui_file_browse(trs_cass_dir, trs_cass_dir, NULL, 1, "Cassette/Wafer ");
        break;
      case 7:
        trs_gui_file_browse(trs_disk_set_dir, trs_disk_set_dir, NULL, 1, "Disk Set ");
        break;
      case 9:
        trs_gui_file_browse(trs_state_dir, trs_state_dir, NULL, 1, "State/Configuration ");
        break;
      case 11:
        trs_gui_file_browse(trs_printer_dir, trs_printer_dir, NULL, 1, "Printer Output ");
        break;
      case -1:
        return;
    }
  }
}

void trs_gui_rom_files(void)
{
  MENU_ENTRY romfile_menu[] =
  {{"Model 1 ROM File:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Model 3 ROM File:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"", MENU_TITLE_TYPE},
   {"Model 4P ROM File:", MENU_TITLE_TYPE},
   {"   ", MENU_NORMAL_TYPE},
   {"", 0}};
  int selection = 1;

  while (1) {
    trs_gui_limit_string(romfile, &romfile_menu[1].title[2], 58);
    trs_gui_limit_string(romfile3, &romfile_menu[4].title[2], 58);
    trs_gui_limit_string(romfile4p, &romfile_menu[7].title[2], 58);
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS ROM File Selection", romfile_menu, selection);
    switch (selection) {
      case 1:
        trs_gui_file_browse(romfile, romfile, NULL, 0, "Model 1 ROM");
        break;
      case 4:
        trs_gui_file_browse(romfile3, romfile3, NULL, 0, "Model 3 ROM");
        break;
      case 7:
        trs_gui_file_browse(romfile4p, romfile4p, NULL, 0, "Model 4P ROM");
        break;
      case -1:
        return;
    }
  }
}

void trs_gui_about_sdltrs(void)
{
  trs_gui_clear_screen();
  trs_gui_frame(0, 0, 63, 15);
  trs_gui_write_text("About SDLTRS", 2, 0, 0);
  trs_gui_center_text("SDLTRS", 3, 0);
  trs_gui_center_text("Version 1.2.15", 4, 0);
  trs_gui_center_text("Copyright (C) 2006-2011 Mark Grebe", 5, 0);
  trs_gui_center_text("<http://sdltrs.sourceforge.net>", 6, 0);
  trs_gui_center_text("Patches (2018-2021) by Alan Cox and Jens Guenther", 7, 0);
  trs_gui_center_text("<http://gitlab.com/jengun/sdltrs>", 8, 0);
  trs_gui_center_text("Based on xtrs 4.9d by Tim Mann", 10, 0);
  trs_gui_center_text("<http://www.tim-mann.org/xtrs>", 11, 0);
  trs_gui_center_text("xtrs 1.0 Copyright (C) 1992 Clarendon Hill Software", 12, 0);
  trs_gui_center_text(" Press Any Key To Return ", 15, 1);
  trs_screen_update();
  trs_gui_get_key();
}

void trs_gui_keys_sdltrs(void)
{
  trs_gui_clear_screen();
  trs_gui_frame(0, 0, 63, 15);
  trs_gui_write_text("Keys in SDLTRS", 2, 0, 0);
  trs_gui_write_text("F1-F3: Functions Keys F1/F2/F3  PgUp/PgDn: Left/Right Shift ", 2, 1, 0);
  trs_gui_write_text("F4: F4/CapsLock on TRS-80 4/4P  Insert: TRS-80 Underscore   ", 2, 2, 0);
  trs_gui_write_text("F5/ScrollLock: TRS-80 @ Key     Shift-Up Arrow: TRS-80 ESC  ", 2, 3, 0);
  trs_gui_write_text("F6: TRS-80 '0' Key (Shifted 0)  Alt-PgUp/PgDn: Scale Window ", 2, 4, 0);
  trs_gui_write_text("F7/Alt-M: Main Menu of SDLTRS   Alt-Enter: Toggle Fullscreen", 2, 5, 0);
  trs_gui_write_text("F8/Shift-F8: Quit/Abort SDLTRS  Alt-A/C/V: Select/Copy/Paste", 2, 6, 0);
  trs_gui_write_text("F9/Alt-Z:"
#ifdef ZBX
  " Enter debugger (zbx)  "
#else
  " Toggle Fullscreen     "
#endif
  "Alt-D/F: Floppy Disk Menu   ", 2, 7, 0);
  trs_gui_write_text("F10/Shift-F10: Warm/Cold Reset  Alt-H: Hard Disk Menu       ", 2, 8, 0);
  trs_gui_write_text("F11/Alt-K: Show this key help   Alt-T: Cassette/Tape Menu   ", 2, 9, 0);
  trs_gui_write_text("F12/Alt-N: Switch Turbo On/Off  Alt-L/S: Load / Save State  ", 2, 10, 0);
  trs_gui_write_text("ESC: TRS-80 Break Key           Alt-R/W: Read / Write Config", 2, 11, 0);
  trs_gui_write_text("Home/Clear: TRS-80 Clear Key    Alt-P/Pause: Pause Emulator ", 2, 12, 0);
  trs_gui_write_text("End: TRS-80 Shifted Down Arrow  Alt-0...7: Insert Disk Drive", 2, 13, 0);
  trs_gui_write_text("Control: TRS-80 4/4P Ctrl Key   Shift-Alt-0...7: Remove Disk", 2, 14, 0);
  trs_gui_center_text(" Press Any Key To Return ", 15, 1);
  trs_screen_update();
  trs_gui_get_key();
}

void trs_gui_display_pause(void)
{
  trs_gui_frame(1, 6, 62, 8);
  trs_gui_clear_rect(2, 7, 60, 1);
  trs_gui_center_text("Emulation Paused", 7, 0);
  trs_screen_update();
}

void trs_gui_exec_cmd(void)
{
  if (trs_gui_file_browse(trs_cmd_file, trs_cmd_file, ".cmd", 0, "CMD (.cmd)") >= 0) {
    if (trs_load_cmd(trs_cmd_file) != 0)
      trs_gui_display_message("ERROR", "Failed to load CMD file");
  }
}

int trs_gui_exit_sdltrs(void)
{
  return trs_gui_display_question("Quit?");
}

void trs_gui_save_bmp(void)
{
  filename[0] = 0;
  if (trs_gui_input_string("Save Screenshot, TAB selects directory",
      trs_printer_dir, filename, FILENAME_MAX - 5, 1) == 0) {
    trs_add_extension(filename, ".bmp");
    trs_screen_refresh();
    if (trs_gui_file_overwrite()) {
      if (trs_sdl_savebmp(filename) != 0)
        trs_gui_display_error(filename);
    }
  }
}

void trs_gui(void)
{
  MENU_ENTRY main_menu[] =
  {{"Floppy Disk Management   (Alt-D)", MENU_NORMAL_TYPE},
   {"Hard Disk Management     (Alt-H)", MENU_NORMAL_TYPE},
   {"Cassette Management      (Alt-T)", MENU_NORMAL_TYPE},
   {"Stringy Wafer Management (Alt-G)", MENU_NORMAL_TYPE},
   {"Emulator Settings        (Alt-E)", MENU_NORMAL_TYPE},
   {"Display Settings         (Alt-I)", MENU_NORMAL_TYPE},
   {"Miscellaneous Settings   (Alt-O)", MENU_NORMAL_TYPE},
   {"Configuration/State Files", MENU_NORMAL_TYPE},
   {"Printer Management", MENU_NORMAL_TYPE},
   {"Joystick Settings", MENU_NORMAL_TYPE},
   {"Default Directories", MENU_NORMAL_TYPE},
   {"ROM File Selection", MENU_NORMAL_TYPE},
   {"About SDLTRS", MENU_NORMAL_TYPE},
   {"", 0}};
  int selection = 0;

  while (1) {
    trs_gui_clear_screen();

    selection = trs_gui_display_menu("SDLTRS Main Menu", main_menu, selection);
    switch (selection) {
      case 0:
        trs_gui_disk_management();
        break;
      case 1:
        trs_gui_hard_management();
        break;
      case 2:
        trs_gui_cassette_management();
        break;
      case 3:
        trs_gui_stringy_management();
        break;
      case 4:
        trs_gui_emulator_settings();
        break;
      case 5:
        trs_gui_display_settings();
        break;
      case 6:
        trs_gui_misc_settings();
        break;
      case 7:
        if (trs_gui_config_management())
          return;
        break;
      case 8:
        trs_gui_printer_management();
        break;
      case 9:
        trs_gui_joystick_settings();
        break;
      case 10:
        trs_gui_default_dirs();
        break;
      case 11:
        trs_gui_rom_files();
        break;
      case 12:
        trs_gui_about_sdltrs();
        break;
      case -1:
        return;
    }
  }
}

void call_function(int function)
{
  SDL_PauseAudio(1);
  if (function == PAUSE) {
    trs_paused = !trs_paused;
    trs_screen_caption();
    if (!trs_paused)
      trs_screen_refresh();
  }
  else if (function == RESET)
    trs_reset(1);
  else if (function == EXIT)
    trs_exit(0);
  else {
    switch (function) {
      case GUI:
        trs_gui();
        break;
      case JOYGUI:
        trs_gui_joy_gui();
        break;
      case KEYBRD:
        trs_gui_get_virtual_key();
        break;
      case SAVE:
        trs_gui_save_state();
        break;
      case LOAD:
        trs_gui_load_state();
        break;
      case DISK:
        trs_gui_disk_management();
        break;
      case HARD:
        trs_gui_hard_management();
        break;
      case STRINGY:
        trs_gui_stringy_management();
        break;
      case TAPE:
        trs_gui_cassette_management();
        break;
      case WRITE:
        trs_gui_write_config();
        break;
      case READ:
        trs_gui_read_config();
        break;
      case EMULATOR:
        trs_gui_emulator_settings();
        break;
      case INTERFACE:
        trs_gui_display_settings();
        break;
      case OTHER:
        trs_gui_misc_settings();
        break;
      case KEYS:
        trs_gui_keys_sdltrs();
        break;
      case EXEC:
        trs_gui_exec_cmd();
        break;
      case SAVE_BMP:
        trs_gui_save_bmp();
        break;
    }
    trs_screen_refresh();
  }
  SDL_PauseAudio(0);
}
