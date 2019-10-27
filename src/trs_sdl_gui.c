/*  Copyright (c): 2006, Mark Grebe */

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
   Modified by Mark Grebe, 2006
   Last modified on Wed May 07 09:12:00 MST 2006 by markgrebe
*/

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <SDL.h>
#include "error.h"
#include "trs.h"
#include "trs_cassette.h"
#include "trs_disk.h"
#include "trs_hard.h"
#include "trs_stringy.h"
#include "trs_uart.h"
#include "trs_state_save.h"
#include "trs_mkdisk.h"
#include "trs_sdl_gui.h"
#include "trs_sdl_keyboard.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define LEFT_VERT_LINE      149
#define RIGHT_VERT_LINE     170
#define TOP_HORIZ_LINE      131
#define BOTTOM_HORIZ_LINE   176
#define TOP_LEFT_CORNER     151
#define TOP_RIGHT_CORNER    171
#define BOTTOM_LEFT_CORNER  181
#define BOTTOM_RIGHT_CORNER 186

#define MENU_NORMAL_TYPE          1
#define MENU_TITLE_TYPE           2
#define MENU_FLOPPY_BROWSE_TYPE   3
#define MENU_HARD_BROWSE_TYPE     4
#define MENU_WAFER_BROWSE_TYPE    5
#define MENU_CASS_BROWSE_TYPE     6

#define CHECK_TIMEOUT (4000)
#define N_KEYS        (52)
#define SHIFT         (39)

static char **filenamelist = NULL;
static int filenamecount = 0;
static int filenamelistsize = 0;

typedef struct menu_entry_type {
  char title[64];
  int type;
} MENU_ENTRY;

static const char *function_choices[8] = {
  "      GUI       ", "Virtual Keyboard",
  "   Save State   ", "   Load State   ",
  "     Reset      ", "      Exit      ",
  "     Pause      ", "  Joystick GUI  "
};

int function_codes[8] = {
  GUI,    KEYBRD,
  SAVE,   LOAD,
  RESET,  EXIT,
  PAUSE,  JOYGUI
};

static const char *key_names[N_KEYS] = {
  " 1 ", " 2 ", " 3 ", " 4 ", " 5 ", " 6 ", " 7 ", " 8 ", " 9 ", " 0 ", " : ", " - ", "BRK",
  " UP", " q ", " w ", " e ", " r ", " t ", " y ", " u ", " i ", " o ", " p ", "LFT", "RGT",
  "DWN", " a ", " s ", " d ", " f ", " g ", " h ", " j ", " k ", " l ", " ; ", "ENT", "CLR",
  "SHF", " z ", " x ", " c ", " v ", " b ", " n ", " m ", " , ", " . ", " / ", " @ ", "SPC"
};
static int key_syms[N_KEYS] = {
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
static int key_syms_shifted[N_KEYS] = {
  SDLK_EXCLAIM, SDLK_QUOTEDBL, SDLK_HASH, SDLK_DOLLAR, 0x25, SDLK_AMPERSAND, SDLK_QUOTE, SDLK_LEFTPAREN, SDLK_RIGHTPAREN, SDLK_UNDERSCORE, SDLK_ASTERISK, SDLK_EQUALS, SDLK_CARET,
  SDLK_TAB, 0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49,      0x4f,         0x50,          0xc4, 0xdc,
  0x7e,     0x41, 0x53, 0x44, 0x46, 0x47, 0x48, 0x4a, 0x4b,      0x4c,         SDLK_PLUS,     0xe4, 0xfc,
  -1,       0x5a, 0x58, 0x43, 0x56, 0x42, 0x4e, 0x4d, SDLK_LESS, SDLK_GREATER, SDLK_QUESTION, 0xd6, 0xf6
};

int jbutton_map[N_JOYBUTTONS]    = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int jbutton_active[N_JOYBUTTONS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int jaxis_mapped = 0;

extern int scanlines;
extern void trs_gui_write_char(int position, int char_index, int invert);
#ifdef SDL2
extern int trs_sdl_sym2upper(int sym);
#endif
static void trs_gui_write_text_len(const char *text, int len, int x, int y, int invert);
static void trs_gui_write_text(const char *text, int x, int y, int invert);
static void trs_gui_write_text_char(const char text, int x, int y, int invert);
static void trs_gui_center_text(const char *text, int y, int invert);
static void trs_gui_frame(int x, int y, int w, int h);
static void trs_gui_clear_rect(int x, int y, int w, int h);
static void trs_gui_limit_string(const char *orig, char *limited, unsigned int limit);
static void trs_add_extension(char *filename, const char *ext);
static int trs_gui_get_key(void);
static void trs_gui_display_message(const char *title, const char *message);
static void trs_gui_create_filename_list();
static void trs_gui_add_to_filename_list(char *filename);
static int trs_gui_filename_cmp(const char *name1, const char *name2);
static void trs_gui_quicksort(char **start, char **end, int (*sort_function) ());
static void trs_gui_delete_filename_list(void);
static int trs_gui_readdirectory(const char *path, const char *mask, int browse_dir);
static int trs_gui_input_string(const char *title, const char *input, char* output,
                                unsigned int limit, int file);
static int trs_gui_display_popup(const char* title, char **entry,
                                 int entry_count, int selection);
static int trs_gui_display_popup_matrix(const char* title, const char **entry,
                                        int rows, int columns, int selection);
static int trs_gui_display_menu(const char* title, MENU_ENTRY *entry, int selection);
static void trs_gui_disk_creation(void);
static void trs_gui_disk_sizes(void);
#ifdef __linux
static void trs_gui_disk_steps(void);
#endif
static void trs_gui_disk_options(void);
static void trs_gui_joystick_management(void);
static void trs_gui_printer_management(void);
static void trs_gui_default_dirs(void);
static void trs_gui_rom_files(void);
static void trs_gui_about_sdltrs(void);
static int trs_gui_config_management(void);
static const char *trs_gui_get_key_name(int key);
static int trs_gui_virtual_keyboard(void);
static int trs_gui_display_question(const char *text);
void trs_gui_keys_sdltrs(void);
void trs_gui_model(void);
int trs_gui_exit_sdltrs(void);

void trs_gui_write_text_len(const char *text, int len, int x, int y, int invert)
{
  int const position = x + y * 64;
  int i;

  if (len > 60)
    len = 60;

  for (i=0;i<len;i++)
    trs_gui_write_char(position+i,text[i],invert);
}

void trs_gui_write_text(const char *text, int x, int y, int invert)
{
  trs_gui_write_text_len(text, strlen(text), x, y, invert);
}

void trs_gui_write_text_char(const char text, int x, int y, int invert)
{
  trs_gui_write_char(x + y * 64,text,invert);
}

void trs_gui_center_text(const char *text, int y, int invert)
{
  int const position = (64-strlen(text))/2 + y * 64;
  int i;

  for (i=0;i<(int)strlen(text);i++)
    trs_gui_write_char(position+i,text[i],invert);
}

void trs_gui_frame(int x, int y, int w, int h)
{
  int i;

  for (i=(x+1)+64*y; i<(x+w-1)+64*y; i++)
    trs_gui_write_char(i,TOP_HORIZ_LINE,0);
  for (i=(x+1)+64*(y+h-1); i<(x+w-1)+64*(y+h-1); i++)
    trs_gui_write_char(i,BOTTOM_HORIZ_LINE,0);
  for (i=x+64*(y+1); i<x+64*(y+h-1); i+=64)
    trs_gui_write_char(i,LEFT_VERT_LINE,0);
  for (i=(x+w-1)+64*(y+1); i<(x+w-1)+64*(y+h-1); i+=64)
    trs_gui_write_char(i,RIGHT_VERT_LINE,0);
  trs_gui_write_char(x+64*y,TOP_LEFT_CORNER,0);
  trs_gui_write_char((x+w-1)+64*y,TOP_RIGHT_CORNER,0);
  trs_gui_write_char(x+64*(y+h-1),BOTTOM_LEFT_CORNER,0);
  trs_gui_write_char((x+w-1)+64*(y+h-1),BOTTOM_RIGHT_CORNER,0);
}

void trs_gui_clear_rect(int x, int y, int w, int h)
{
  int i,j;

  for (i=0;i<h;i++)
    for (j=0;j<w;j++)
      trs_gui_write_text_char(' ', x+j, y+i, 0);
}

void trs_gui_limit_string(const char *orig, char *limited, unsigned int limit)
{
  int len_first_part;
  int pos_second_part;

  if (strlen(orig) > limit) {
    len_first_part = (limit-3)/2;
    pos_second_part = strlen(orig) - (limit - len_first_part - 3);
    strncpy(limited, orig, len_first_part);
    limited[len_first_part] = '\0';
    snprintf(limited + len_first_part, limit, "...%s", orig + pos_second_part);
  } else
    snprintf(limited, limit + 1, "%s", orig);
}

void trs_add_extension(char *filename, const char *ext)
{
  int const flen = strlen(filename);
  int const elen = strlen(ext);

  if (flen > elen)
    if (strcmp(&filename[flen - elen],ext) == 0)
      return;

  snprintf(filename + flen, FILENAME_MAX - 1, "%s", ext);
}

int trs_gui_get_key(void)
{
   SDL_Event event;

   while (1) {
     SDL_WaitEvent(&event);
     switch(event.type) {
       case SDL_QUIT:
         trs_exit(0);
         break;
#ifdef SDL2
       case SDL_WINDOWEVENT:
#else
       case SDL_ACTIVEEVENT:
#endif
         trs_gui_refresh();
         trs_x_flush();
         break;
       case SDL_KEYDOWN:
         if (event.key.keysym.mod & KMOD_ALT) {
           switch (event.key.keysym.sym) {
#ifdef _WIN32
             case SDLK_F4:
#endif
             case SDLK_q:
               trs_exit(2);
               break;
             default:
               break;
           }
         }
         else if (event.key.keysym.sym == SDLK_F8)
           trs_exit(!(event.key.keysym.mod & KMOD_SHIFT) + 1);
#ifdef SDL2
         else if (event.key.keysym.mod & KMOD_SHIFT)
           return trs_sdl_sym2upper(event.key.keysym.sym);
#else
         else if (event.key.keysym.sym < 0x100 &&
             event.key.keysym.unicode >= 0x20 &&
             event.key.keysym.unicode <= 0x7E)
           return(event.key.keysym.unicode);
#endif
         else
           return(event.key.keysym.sym);
         break;
       case SDL_JOYBUTTONDOWN:
         if (event.jbutton.button < N_JOYBUTTONS) {
           int key = jbutton_map[event.jbutton.button];

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
           else if (abs(event.jaxis.value) < JOY_BOUNCE/8)
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

void trs_gui_display_message(const char* title, const char *message)
{
  int key;

  trs_gui_frame(1,6,62,3);
  trs_gui_clear_rect(2,7,60,1);
  trs_gui_write_text(title, 3, 6, 0);
  trs_gui_write_text(message, 5, 7, 0);
  trs_x_flush();

  while (1) {
    key = trs_gui_get_key();
    switch(key) {
      case SDLK_ESCAPE:
      case SDLK_RETURN:
        return;
    }
  }
}

void trs_gui_display_pause()
{
  trs_gui_frame(1,6,62,3);
  trs_gui_clear_rect(2,7,60,1);
  trs_gui_center_text("Emulation Paused", 7, 0);
  trs_x_flush();
}

void trs_gui_create_filename_list()
{
  if (filenamelist == NULL) {
    if ((filenamelist = (char **) malloc(256 * sizeof(char *))) == NULL)
      fatal("Failed to allocate filenamelist");
    filenamelistsize = 256;
  }
}

void trs_gui_add_to_filename_list(char *filename)
{
  filenamelist[filenamecount++] = filename;
  if (filenamecount == filenamelistsize) {
    char **filenamelist_new;

    if ((filenamelist_new = realloc(filenamelist, 2 *
        filenamelistsize * sizeof(char*))) == NULL)
      fatal("Failed to reallocate filenamelist");
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

void trs_gui_quicksort(char **start, char **end, int (*sort_function) ())
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

  for (i=0;i<filenamecount;i++)
    free(filenamelist[i]);
  filenamecount = 0;
}

int trs_gui_readdirectory(const char *path, const char *mask, int browse_dir)
{
  DIR *directory = NULL;
  char pathname[FILENAME_MAX];
  char *filename;
  char *filename_pos;
  int pathlen;
  struct dirent *dir_entry;
  struct stat st;

  snprintf(pathname, FILENAME_MAX, "%s", path);
  pathlen = strlen(path);
  filename_pos = pathname + pathlen;

  directory = opendir(path);
  if (directory) {
    trs_gui_create_filename_list();
    while ((dir_entry = readdir(directory))) {

      if (strcmp(dir_entry->d_name, ".") == 0)
        continue;

      strcpy(filename_pos, dir_entry->d_name);
      stat(pathname, &st);
      if ((st.st_mode & S_IFMT) == S_IFDIR) {
        int dirname_len;

        dirname_len = strlen(dir_entry->d_name);
        if ( (filename = (char *) malloc(dirname_len + 3)) ) {
          filename[0] = '<';
          strcpy(filename + 1, dir_entry->d_name);
          filename[dirname_len + 1] = '>';
          filename[dirname_len + 2] = 0;
        }
      } else if (browse_dir) {
        continue;
      } else {
        if (mask != NULL && strstr(dir_entry->d_name, mask) == NULL)
          continue;
        filename = (char *) strdup(dir_entry->d_name);
      }
      if (!filename)
        return(-1);
      trs_gui_add_to_filename_list(filename);
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

    trs_gui_quicksort(filenamelist, filenamelist+filenamecount,
        trs_gui_filename_cmp);
    return(0);
  }
  return(-1);
}

int trs_gui_file_browse(const char* path, char* filename, const char *mask,
                        int browse_dir, const char* type)
{
  char current_dir[FILENAME_MAX];
  char limited_dir[64];
  char title[64];
  struct stat st;
  const char *new_dir;
  int i,j,key;
  int selection = 0;
  int current_first = 0;
  int done = 0;
  int drawcount;
  int redraw = 1;

  snprintf(current_dir, FILENAME_MAX-1, "%s", path);

  for (i=strlen(current_dir);i>0;i--) {
#ifdef _WIN32
    if (current_dir[i] == '\\') {
#else
    if (current_dir[i] == '/') {
#endif
      current_dir[i+1]=0;
      break;
    }
  }

  stat(current_dir, &st);
  if ((st.st_mode & S_IFMT) != S_IFDIR) {
    if (getcwd(current_dir, FILENAME_MAX) == NULL)
      error("getcwd: %s", current_dir);
  }
#ifdef _WIN32
  if (current_dir[strlen(current_dir)-1] != '\\')
    strcat(current_dir,"\\");
#else
  if (current_dir[strlen(current_dir)-1] != '/')
    strcat(current_dir,"/");
#endif

  trs_gui_clear_screen();
  trs_gui_frame(0,0,64,16);
  if (browse_dir) {
    snprintf(title,63,"Choose%sDirectory",type);
    trs_gui_center_text("TAB selects directory",15,1);
  }
  else
    snprintf(title,63,"Select%sFile To Load",type);
  trs_gui_write_text(title, 2, 0, 0);
  trs_gui_limit_string(current_dir, limited_dir, 58);
  trs_gui_center_text(limited_dir,1,0);
  if (trs_gui_readdirectory(current_dir, mask, browse_dir) == -1)
    return(-1);

  if (filenamecount < 13)
    drawcount = filenamecount;
  else
    drawcount = 13;

  while (!done) {
    if (redraw) {
      trs_gui_clear_rect(2,2,60,13);
      for (i=0;i<drawcount;i++)
        trs_gui_write_text(filenamelist[current_first+i],2,i+2,0);
      redraw = 0;
    }
    trs_gui_write_text(filenamelist[current_first+selection],2,selection+2,1);
    trs_x_flush();
    key = trs_gui_get_key();
    trs_gui_write_text(filenamelist[current_first+selection],2,selection+2,0);
    if (key >= '0' && key <= 'z') {
      i = j = current_first + selection;
      do {
        if (++i > filenamecount - 1)
          i = 0;
      } while (i != j && (tolower((int)*filenamelist[i]) != tolower(key)));
      if (i < 13) {
        current_first = 0;
        selection = i;
      } else if (i + 13 > filenamecount) {
        current_first = filenamecount - 13;
        selection = i - current_first;
      } else {
        current_first = i - selection;
      }
      redraw = 1;
    } else {
      switch(key) {
        case SDLK_DOWN:
        case SDLK_RIGHT:
          if (selection < drawcount-1) {
            selection ++;
          } else {
            if (current_first < filenamecount-drawcount) {
              current_first++;
              redraw = 1;
            }
          }
          break;
        case SDLK_UP:
        case SDLK_LEFT:
          if (selection > 0) {
            selection --;
          }
          else {
            if (current_first > 0) {
              current_first--;
              redraw = 1;
            }
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
          if (current_first > filenamecount-drawcount) {
            current_first = filenamecount-drawcount;
            selection = drawcount-1;
          }
          redraw = 1;
          break;
        case SDLK_HOME:
          selection = current_first = 0;
          redraw = 1;
          break;
        case SDLK_END:
          selection = drawcount-1;
          current_first = filenamecount-drawcount;
          redraw = 1;
          break;
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_TAB:
          if (key == SDLK_TAB && browse_dir)
            done = 1;
          else
          if (*filenamelist[current_first + selection] == '<') {
            new_dir = filenamelist[current_first + selection];
            selection = 0;
            current_first = 0;

            if (new_dir[1] == '.' && new_dir[2] == '.') {
              for (i=strlen(current_dir)-2;i>=0;i--) {
#ifdef _WIN32
                if (current_dir[i] == '\\') {
#else
                  if (current_dir[i] == '/') {
#endif
                    current_dir[i+1]=0;
                    break;
                  }
                }
              } else {
                strcat(current_dir, &new_dir[1]);
#ifdef _WIN32
                current_dir[strlen(current_dir)-1] = '\\';
#else
                current_dir[strlen(current_dir)-1] = '/';
#endif
              }

              trs_gui_clear_rect(1,1,62,14);
              trs_gui_limit_string(current_dir, limited_dir, 58);
              trs_gui_center_text(limited_dir,1,0);

              trs_gui_delete_filename_list();
              if (trs_gui_readdirectory(current_dir, mask, browse_dir) == -1)
                return(-1);

              if (filenamecount < 13)
                drawcount = filenamecount;
              else
                drawcount = 13;

              redraw = 1;
            }
#ifdef _WIN32
            /* Select a new drive */
            else if (*filenamelist[current_first + selection] == '[') {
              new_dir = filenamelist[current_first + selection];
              selection = 0;
              current_first = 0;
              current_dir[0] = new_dir[1];
              current_dir[1] = new_dir[2];
              current_dir[2] = '\\';
              current_dir[3] = 0;

              trs_gui_clear_rect(1,1,62,14);
              trs_gui_limit_string(current_dir, limited_dir, 58);
              trs_gui_center_text(limited_dir,1,0);

              trs_gui_delete_filename_list();
              if (trs_gui_readdirectory(current_dir, mask, browse_dir) == -1)
                return(-1);

              if (filenamecount < 13)
                drawcount = filenamecount;
              else
                drawcount = 13;

              redraw = 1;
            }
#endif
            else
              done = 1;
          break;
        case SDLK_ESCAPE:
          done = 1;
          selection = -1;
          break;
        }
      }
    }

    if (selection != -1) {
      snprintf(filename, FILENAME_MAX, "%s", current_dir);
      if (browse_dir) {
        new_dir = filenamelist[current_first + selection];
        if (new_dir[1] != '.' && new_dir[2] != '.') {
#ifdef _WIN32
          if (new_dir[0] == '[') {
            filename[0] = new_dir[1];
            filename[1] = new_dir[2];
            filename[2] = '\\';
            filename[3] = 0;
          } else
#endif
          {
            strcat(filename, &new_dir[1]);
#ifdef _WIN32
            filename[strlen(filename)-1] = '\\';
#else
            filename[strlen(filename)-1] = '/';
#endif
          }
        }
        trs_gui_clear_rect(1,14,62,1);
      }
      else
        strcat(filename, filenamelist[current_first + selection]);
    }
    trs_gui_delete_filename_list();
    if (selection == -1)
      return(selection);
    else
      return(current_first + selection);
  }

int trs_gui_input_string(const char *title, const char* input, char* output,
                         unsigned int limit, int file)
{
  char directory_name[FILENAME_MAX];
  int key,ret=0;
  int done = 0;
  int insert = 1;
  int invert;
  unsigned int i, pos;
  unsigned int length;
  unsigned int first_disp;

  if (input != output)
    snprintf(output, limit + 1, "%s", input);

  pos = length = strlen(input);
  if (pos > 60)
    first_disp = pos - 59;
  else
    first_disp = 0;

  trs_gui_frame(1,6,62,3);
  trs_gui_clear_rect(2,7,60,1);
  trs_gui_write_text(title, 3, 6, 0);

  while (!done) {
    trs_gui_clear_rect(2,7,60,1);
    for (i=0;i<60;i++) {
      invert = (first_disp + i == pos);
      if (first_disp + i >= length)
        trs_gui_write_text_char(' ',2+i,7,invert);
      else
        trs_gui_write_text_char(output[first_disp+i],2+i,7,invert);
    }
    trs_x_flush();
    key = trs_gui_get_key();
    switch(key) {
      case SDLK_LEFT:
        if (pos>0) {
          if (pos == first_disp)
            first_disp--;
          pos--;
        }
        break;
      case SDLK_RIGHT:
        if (pos<length) {
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
        pos = length;
        if (pos > 60)
          first_disp = pos - 59;
        else
          first_disp = 0;
        break;
      case SDLK_BACKSPACE:
        if (pos>0) {
          for (i=pos;i<length;i++)
            output[i-1] = output[i];
          length--;
          if (pos == first_disp)
            first_disp--;
          pos--;
        }
        break;
      case SDLK_DELETE:
        if (pos<length) {
          for (i=pos;i<length-1;i++)
            output[i] = output[i+1];
          length--;
        }
        break;
      case SDLK_INSERT:
        insert = !insert;
        break;
      case SDLK_RETURN:
        ret = 0;
        done = 1;
        break;
      case SDLK_ESCAPE:
        ret = -1;
        done = 1;
        break;
      case SDLK_TAB:
      case SDLK_UP:
        if (file) {
          if (trs_gui_file_browse(input, directory_name, NULL, 1, " ") != -1) {
            snprintf(output, limit + 1, "%s", directory_name);
            pos = length = strlen(output);
            if (pos > 60)
              first_disp = pos - 59;
            else
              first_disp = 0;
          }
          trs_gui_frame(1,6,62,3);
          trs_gui_write_text(title, 3, 6, 0);
        }
        break;
      default:
        if (key >= 0x20 && key <= 0xFF && pos < limit)  {
          if (insert && length < limit) {
            for (i=length;i>pos;i--)
              output[i] = output[i-1];
            length++;
          }
          output[pos] = (char) key;
          if (pos == first_disp + 59)
            first_disp ++;
          pos++;
          if (pos > length)
            length++;
        }
        break;
    }
  }

  output[length] = 0;
  return(ret);
}

int trs_gui_display_popup(const char* title, char **entry,
                          int entry_count, int selection)
{
  int num = 0, i, key;
  int first_x, first_y;
  int saved_selection = selection;
  unsigned int max_len = 0;

  for (num=0;num<entry_count;num++) {
    if (strlen(entry[num]) > max_len)
      max_len = strlen(entry[num]);
  }
  first_x = (64-max_len)/2;
  first_y = (16-entry_count)/2;

  trs_gui_frame(first_x-1,first_y-1,max_len+2,entry_count+2);
  trs_gui_write_text(title, first_x+1, first_y-1, 0);

  for (num=0;num<entry_count;num++)
    trs_gui_write_text(entry[num], first_x, first_y+num,0);

  while (1) {
    trs_gui_write_text(entry[selection], first_x, selection+first_y,1);
    trs_x_flush();
    key = trs_gui_get_key();
    trs_gui_write_text(entry[selection], first_x, selection+first_y,0);
    if (entry_count == 2) {
      if (tolower(key) == 'n') return 0;
      if (tolower(key) == 'y') return 1;
    }
    if (key >= '0' && key <= 'z') {
      for (i = 0; i < num; i++) {
        if (strchr(entry[i], toupper(key))) {
          selection = i;
          break;
        }
      }
    }
    switch(key) {
      case SDLK_DOWN:
      case SDLK_RIGHT:
        if (selection < entry_count-1)
          selection ++;
        else
          selection = 0;
        break;
      case SDLK_UP:
      case SDLK_LEFT:
        if (selection > 0)
          selection --;
        else
          selection = entry_count-1;
        break;
      case SDLK_HOME:
      case SDLK_PAGEUP:
        selection = 0;
        break;
      case SDLK_END:
      case SDLK_PAGEDOWN:
        selection = entry_count-1;
        break;
      case SDLK_RETURN:
      case SDLK_SPACE:
      case SDLK_TAB:
        return selection;
        break;
      case SDLK_ESCAPE:
        return saved_selection;
        break;
    }
  }
}

int trs_gui_display_menu(const char* title, MENU_ENTRY *entry, int selection)
{
  char filename[FILENAME_MAX];
  int num = 0,i,key;

  trs_gui_frame(0,0,64,16);
  trs_gui_write_text(title, 2, 0, 0);

  while(entry[num].type != 0) {
    trs_gui_write_text(entry[num].title, 2, num+2,0);
    num++;
  }
  num--;

  while (1) {
    trs_gui_write_text(entry[selection].title, 2, selection+2,1);
    trs_x_flush();
    key = trs_gui_get_key();
    trs_gui_write_text(entry[selection].title, 2, selection+2,0);
    if (key >= '0' && key <= '9') {
      key -= '0';
      if (key < num && entry[key].type != MENU_TITLE_TYPE)
        selection = key;
    } else
    if (key >= 'A' && key <= 'z') {
      i = selection;
      do {
        if (++selection > num)
          selection = 0;
      } while (selection != i && (tolower((int)*entry[selection].title) != tolower(key)));
      while(entry[selection].type == MENU_TITLE_TYPE) {
        if (selection < num)
          selection ++;
      }
    } else
    switch(key) {
      case SDLK_DOWN:
      case SDLK_RIGHT:
        do {
          if (selection < num)
            selection ++;
          else
            selection = 0;
        } while(entry[selection].type == MENU_TITLE_TYPE);
        break;
      case SDLK_UP:
      case SDLK_LEFT:
        do {
          if (selection > 0)
            selection --;
          else
            selection = num;
        } while(entry[selection].type == MENU_TITLE_TYPE);
        break;
      case SDLK_HOME:
      case SDLK_PAGEUP:
        selection = 0;
        while(entry[selection].type == MENU_TITLE_TYPE) {
          if (selection < num)
            selection ++;
        }
        break;
      case SDLK_END:
      case SDLK_PAGEDOWN:
        selection = num;
        break;
      case SDLK_DELETE:
      case SDLK_BACKSPACE:
        if ((entry[selection].type == MENU_FLOPPY_BROWSE_TYPE) ||
            (entry[selection].type == MENU_HARD_BROWSE_TYPE) ||
            (entry[selection].type == MENU_WAFER_BROWSE_TYPE) ||
            (entry[selection].type == MENU_CASS_BROWSE_TYPE)) {
          if (entry[selection].type == MENU_FLOPPY_BROWSE_TYPE) {
            trs_disk_remove(selection);
          } else if (entry[selection].type == MENU_HARD_BROWSE_TYPE)  {
            trs_hard_remove(selection);
          } else if (entry[selection].type == MENU_WAFER_BROWSE_TYPE) {
            stringy_remove(selection);
          } else {
            trs_cassette_remove();
          }
          trs_gui_clear_rect(2,selection+2,60,1);
          entry[selection].title[0]=' ';
          return selection;
        }
        break;
      case SDLK_RETURN:
      case SDLK_TAB:
        if ((entry[selection].type == MENU_FLOPPY_BROWSE_TYPE) ||
            (entry[selection].type == MENU_HARD_BROWSE_TYPE) ||
            (entry[selection].type == MENU_WAFER_BROWSE_TYPE) ||
            (entry[selection].type == MENU_CASS_BROWSE_TYPE)) {
          if (entry[selection].type == MENU_FLOPPY_BROWSE_TYPE) {
            if (trs_gui_file_browse(trs_disk_dir, filename, NULL ,0,
                  " Floppy Disk Image ") == -1)
              return selection;
            trs_disk_insert(selection, filename);
          } else if (entry[selection].type == MENU_HARD_BROWSE_TYPE)  {
            if (trs_gui_file_browse(trs_hard_dir, filename, NULL, 0,
                  " Hard Disk Image ") == -1)
              return selection;
            trs_hard_attach(selection, filename);
          } else if (entry[selection].type == MENU_WAFER_BROWSE_TYPE)  {
            if (trs_gui_file_browse(trs_cass_dir, filename, NULL, 0,
                  " Wafer Image ") == -1)
              return selection;
            stringy_insert(selection, filename);
          } else {
            if (trs_gui_file_browse(trs_cass_dir, filename, NULL, 0,
                  " Cassette Image ") == -1)
              return selection;
            trs_cassette_insert(filename);
          }
          trs_gui_clear_rect(2,selection+2,60,1);
        }
        return selection;
        break;
      case SDLK_SPACE:
        if (entry[selection].type == MENU_FLOPPY_BROWSE_TYPE) {
          if (trs_disk_getwriteprotect(selection))
            trs_protect_disk(selection, 0);
          else
            trs_protect_disk(selection, 1);
        } else if (entry[selection].type == MENU_HARD_BROWSE_TYPE) {
          if (trs_hard_getwriteprotect(selection))
            trs_protect_hard(selection, 0);
          else
            trs_protect_hard(selection, 1);
        } else if (entry[selection].type == MENU_CASS_BROWSE_TYPE) {
          if (trs_cass_getwriteprotect())
            trs_protect_cass(0);
          else
            trs_protect_cass(1);
        } else if (entry[selection].type == MENU_WAFER_BROWSE_TYPE) {
          if (stringy_get_writeprotect(selection))
            trs_protect_stringy(selection, 0);
          else
            trs_protect_stringy(selection, 1);
        }
        return selection;
        break;
      case SDLK_ESCAPE:
        return -1;
        break;
    }
  }
}

void trs_gui_disk_creation(void)
{
  MENU_ENTRY disk_creation_menu[] =
  {{"Image Type                                                  ",MENU_NORMAL_TYPE},
   {"Number of Sides                                             ",MENU_NORMAL_TYPE},
   {"Density                                                     ",MENU_NORMAL_TYPE},
   {"Physical Size                                               ",MENU_NORMAL_TYPE},
   {"Ignore Density Flag                                         ",MENU_NORMAL_TYPE},
   {"Insert Created Disk Into This Drive                         ",MENU_NORMAL_TYPE},
   {"Create Disk Image with Above Parameters",MENU_NORMAL_TYPE},
   {"",0}};
  char *image_type_choices[3] = {"   JV1","   JV3","   DMK"};
  char *num_sides_choices[2] =  {"     1","     2"};
  char *density_choices[2] =    {"Single","Double"};
  char *size_choices[2] =       {"5 Inch","8 Inch"};
  char *ignore_choices[2] =     {"    No","   Yes"};
  char *drive_choices[9] =      {"  None","Disk 0","Disk 1","Disk 2",
                                 "Disk 3","Disk 4","Disk 5","Disk 6",
                                 "Disk 7"};
  char filename[FILENAME_MAX];
  int selection = 6;
  int ret;
  static int image_type = 1;
  static int num_sides = 1;
  static int density = 1;
  static int eight = 0;
  static int ignore_density = 0;
  static int drive_insert = 0;

  while (1) {
    snprintf(&disk_creation_menu[0].title[54],7,"%s",image_type_choices[image_type]);
    snprintf(&disk_creation_menu[1].title[54],7,"%s",num_sides_choices[num_sides-1]);
    snprintf(&disk_creation_menu[2].title[54],7,"%s",density_choices[density-1]);
    snprintf(&disk_creation_menu[3].title[54],7,"%s",size_choices[eight]);
    snprintf(&disk_creation_menu[4].title[54],7,"%s",ignore_choices[ignore_density]);
    snprintf(&disk_creation_menu[5].title[54],7,"%s",drive_choices[drive_insert]);
    trs_gui_clear_screen();
    selection = trs_gui_display_menu("SDLTRS Floppy Disk Creation Menu",
        disk_creation_menu, selection);
    switch(selection) {
      case 0:
        image_type = trs_gui_display_popup("Type",image_type_choices,3,
            image_type);
        break;
      case 1:
        num_sides = trs_gui_display_popup("Sides",num_sides_choices,2,
            num_sides - 1) + 1;
        break;
      case 2:
        density = trs_gui_display_popup("Dens",density_choices,2,
            density - 1) + 1;
        break;
      case 3:
        eight = trs_gui_display_popup("Size",size_choices,2,
            eight);
        break;
      case 4:
        ignore_density = trs_gui_display_popup("Ignore",ignore_choices,2,
            ignore_density);
        break;
      case 5:
        drive_insert = trs_gui_display_popup("Disk",drive_choices,9,
            drive_insert);
        break;
      case 6:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Disk Image, TAB selects directory",
              trs_disk_dir,filename,FILENAME_MAX-1,1) == 0) {
          if (image_type == 0)
            ret = trs_create_blank_jv1(filename);
          else if (image_type == 1)
            ret = trs_create_blank_jv3(filename);
          else
            ret = trs_create_blank_dmk(filename, num_sides, density, eight, ignore_density);
          if (ret)
            trs_gui_display_message("Error","Error creating Disk Image");
          else if (drive_insert)
            trs_disk_insert(drive_insert-1, filename);
          return;
        }
        break;
      case -1:
        return;
        break;
    }
  }
}

void trs_gui_disk_sizes(void)
{
  MENU_ENTRY disk_sizes_menu[] =
  {{"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",0,}};
  char *size_choices[2] = {"5 Inch","8 Inch"};
  int selection = 0;
  int gui_disk_sizes[8];
  int i, choice, size;

  for (i=0;i<8;i++)
    gui_disk_sizes[i] = trs_disk_getsize(i);

  while (1) {
    trs_gui_clear_screen();
    for (i=0;i<8;i++) {
      if (gui_disk_sizes[i] == 5)
        choice = 0;
      else
        choice = 1;
      snprintf(disk_sizes_menu[i].title,63,
          "Disk Drive Number %d Size                              %s",
          i,size_choices[choice]);
    }
    selection = trs_gui_display_menu("SDLTRS Floppy Disk Size Menu",
        disk_sizes_menu, selection);
    if (selection == -1)
      return;
    else {
      size = trs_gui_display_popup("Size",size_choices,2,
          gui_disk_sizes[selection]==8);
      if (size == 0)
        gui_disk_sizes[selection] = 5;
      else
        gui_disk_sizes[selection] = 8;
      trs_disk_setsize(selection, gui_disk_sizes[selection]);
    }
  }
}

#ifdef __linux
void trs_gui_disk_steps(void)
{
  MENU_ENTRY disk_steps_menu[] =
  {{"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",MENU_NORMAL_TYPE},
   {"",0}};
  char *step_choices[2] = {"Single","Double"};
  int selection = 0;
  int gui_disk_steps[8];
  int i, choice, step;

  for (i=0;i<8;i++)
    gui_disk_steps[i] = trs_disk_getstep(i);

  while (1) {
    trs_gui_clear_screen();
    for (i=0;i<8;i++) {
      if (gui_disk_steps[i] == 1)
        choice = 0;
      else
        choice = 1;
      snprintf(disk_steps_menu[i].title,63,
          "Disk Drive Number %d Step                              %s",
          i,step_choices[choice]);
    }
    selection = trs_gui_display_menu("SDLTRS Floppy Disk Menu",
        disk_steps_menu, selection);
    if (selection == -1)
      return;
    else {
      step = trs_gui_display_popup("Step",step_choices,2,
          gui_disk_steps[selection]==2);
      if (step == 0)
        gui_disk_steps[selection] = 1;
      else
        gui_disk_steps[selection] = 2;
      trs_disk_setstep(selection, gui_disk_steps[selection]);
    }
  }
}
#endif

void trs_gui_disk_options(void)
{
  MENU_ENTRY disk_menu[] =
  {{"Doubler Type                                                ",MENU_NORMAL_TYPE},
   {"True DAM Emulation                                          ",MENU_NORMAL_TYPE},
   {"Set Drive Sizes",MENU_NORMAL_TYPE},
#ifdef __linux
   {"Set Drive Steps",MENU_NORMAL_TYPE},
#endif
   {"",0,}};
  char *on_off_choices[2] =  {"       Off","        On"};
  char *doubler_choices[4] = {"      None","    Percom","     Tandy","      Both"};
  int selection = 0;

  while (1) {
    snprintf(&disk_menu[0].title[50],11,"%s",doubler_choices[trs_disk_doubler]);
    snprintf(&disk_menu[1].title[50],11,"%s",on_off_choices[trs_disk_truedam]);
    trs_gui_clear_screen();
    selection = trs_gui_display_menu("SDLTRS Floppy Disk Options Menu",disk_menu, selection);
    switch(selection) {
      case 0:
        trs_disk_doubler = trs_gui_display_popup("Doubler",doubler_choices,4,
            trs_disk_doubler);
        break;
      case 1:
        trs_disk_truedam = trs_gui_display_popup("True DAM",on_off_choices,2,
            trs_disk_truedam);
        break;
      case 2:
        trs_gui_disk_sizes();
        break;
#ifdef __linux
      case 3:
        trs_gui_disk_steps();
        break;
#endif
      case -1:
        return;
        break;
    }
  }
}

void trs_gui_disk_management(void)
{
  MENU_ENTRY disk_menu[] =
  {{" Disk 0:",MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 1:",MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 2:",MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 3:",MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 4:",MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 5:",MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 6:",MENU_FLOPPY_BROWSE_TYPE},
   {" Disk 7:",MENU_FLOPPY_BROWSE_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Save Disk Set",MENU_NORMAL_TYPE},
   {"Load Disk Set",MENU_NORMAL_TYPE},
   {"Create Blank Floppy Disk",MENU_NORMAL_TYPE},
   {"Disk Drive Options",MENU_NORMAL_TYPE},
   {"",0}};
  char filename[FILENAME_MAX];
  int selection = 0;
  int i;

  while (1) {
    for (i=0;i<8;i++) {
      const char *diskname = trs_disk_getfilename(i);

      if (diskname[0] == 0)
        snprintf(&disk_menu[i].title[8],6,"%s","Empty");
      else
        trs_gui_limit_string(diskname,&disk_menu[i].title[8],52);
      disk_menu[i].title[0] = trs_disk_getwriteprotect(i) ? '*' : ' ';
    }
    trs_gui_clear_screen();
    selection = trs_gui_display_menu("SDLTRS Floppy Disk Menu",disk_menu, selection);
    switch(selection) {
      case 9:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Disk Set, TAB selects directory",
              trs_disk_set_dir,filename,FILENAME_MAX-5,1) == 0) {
          trs_add_extension(filename,".set");
          if (trs_diskset_save(filename) == -1)
            trs_gui_display_message("Error", "Failed to save Disk Set");
        }
        break;
      case 10:
        if (trs_gui_file_browse(trs_disk_set_dir, filename, ".set", 0," Disk Set ") >= 0) {
          if (trs_diskset_load(filename) == -1)
            trs_gui_display_message("Error", "Failed to load Disk Set");
        }
        break;
      case 11:
        trs_gui_disk_creation();
        break;
      case 12:
        trs_gui_disk_options();
        break;
      case -1:
        return;
        break;
    }
  }
}

void trs_gui_hard_management(void)
{
  MENU_ENTRY hard_menu[] =
  {{" Hard 0:",MENU_HARD_BROWSE_TYPE},
   {" Hard 1:",MENU_HARD_BROWSE_TYPE},
   {" Hard 2:",MENU_HARD_BROWSE_TYPE},
   {" Hard 3:",MENU_HARD_BROWSE_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Save Disk Set",MENU_NORMAL_TYPE},
   {"Load Disk Set",MENU_NORMAL_TYPE},
   {"Cylinder Count                                              ",MENU_NORMAL_TYPE},
   {"Sector Count                                                ",MENU_NORMAL_TYPE},
   {"Granularity                                                 ",MENU_NORMAL_TYPE},
   {"Directory Sector                                            ",MENU_NORMAL_TYPE},
   {"Insert Created Disk Into This Drive                         ",MENU_NORMAL_TYPE},
   {"Create Hard Disk Image with Above Parameters",MENU_NORMAL_TYPE},
   {"",0}};
  static int cylinder_count = 202;
  static int sector_count = 256;
  static int granularity = 8;
  static int dir_sector = 1;
  static int drive_insert = 0;
  char *drive_choices[5] = {"  None","Hard 0","Hard 1","Hard 2","Hard 3"};
  char filename[FILENAME_MAX];
  char input[4];
  int selection = 0;
  int i, value;

  while (1) {
    for (i=0;i<4;i++) {
      const char *diskname = trs_hard_getfilename(i);

      if (diskname[0] == 0)
        snprintf(&hard_menu[i].title[8],6,"%s","Empty");
      else
        trs_gui_limit_string(diskname,&hard_menu[i].title[8],52);
      hard_menu[i].title[0] = trs_hard_getwriteprotect(i) ? '*' : ' ';
    }
    snprintf(&hard_menu[7].title[57],4,"%3d",cylinder_count);
    snprintf(&hard_menu[8].title[57],4,"%3d",sector_count);
    snprintf(&hard_menu[9].title[57],4,"%3d",granularity);
    snprintf(&hard_menu[10].title[57],4,"%3d",dir_sector);
    snprintf(&hard_menu[11].title[54],7,"%6s",drive_choices[drive_insert]);
    trs_gui_clear_screen();
    selection = trs_gui_display_menu("SDLTRS Hard Disk Menu",
        hard_menu, selection);
    switch(selection) {
      case 5:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Disk Set, TAB selects directory",
              trs_disk_set_dir,filename,FILENAME_MAX-5,1) == 0) {
          trs_add_extension(filename,".set");
          if (trs_diskset_save(filename) == -1)
            trs_gui_display_message("Error", "Failed to save Disk Set");
        }
        break;
      case 6:
        if (trs_gui_file_browse(trs_disk_set_dir, filename, ".set", 0," Disk Set ") >= 0) {
          if (trs_diskset_load(filename) == -1)
            trs_gui_display_message("Error", "Failed to load Disk Set");
        }
        break;
      case 7:
        snprintf(input,4,"%d",cylinder_count);
        if (trs_gui_input_string("Enter Cylinder Count",input,input,3,0) == 0) {
          value = atoi(input);
          if (value >=3 && value <= 256) {
            cylinder_count = value;
            if (cylinder_count > 203)
              trs_gui_display_message("Warning",
                  "Cylinder Count > 203 is incompatible with XTRSHARD/DCT");
          } else
            trs_gui_display_message("Error",
                "Cylinder Count must be between 3 and 256");
        }
        break;
      case 8:
        snprintf(input,4,"%d",sector_count);
        if (trs_gui_input_string("Enter Sector Count",input,input,3,0) == 0) {
          value = atoi(input);
          if (value >=4 && value <= 256) {
            sector_count = value;
            if ((sector_count % 32) != 0) {
              trs_gui_display_message("Warning",
                  "Sector not a mult of 32 is incompatible with WD1000/1010");
              if (sector_count > 32)
                trs_gui_display_message("Warning",
                    "Sector is incompatible with Matthew Reed's emulators");
            }
          } else
            trs_gui_display_message("Error",
                "Sector Count must be between 4 and 256");
        }
        break;
      case 9:
        snprintf(input,2,"%d",granularity);
        if (trs_gui_input_string("Enter Granularity",input,input,1,0) == 0) {
          value = atoi(input);
          if (value >= 1 && value <= 8) {
            granularity = value;
          } else
            trs_gui_display_message("Error",
                "Granularity must be between 1 and 8");
        }
        break;
      case 10:
        snprintf(input,4,"%d",dir_sector);
        if (trs_gui_input_string("Enter Directory Sector",input,input,3,0) == 0) {
          value = atoi(input);
          if (value >= 1 && value < cylinder_count) {
            dir_sector = value;
          } else
            trs_gui_display_message("Error",
                "Directory Sector must be between 1 and Cylinder Count-1");
        }
        break;
      case 11:
        drive_insert = trs_gui_display_popup("Hard",drive_choices,5,
            drive_insert);
        break;
      case 12:
        if (sector_count < granularity) {
          trs_gui_display_message("Error",
              "Sector Count must be >= Granularity");
          break;
        }
        if ((sector_count % granularity) != 0) {
          trs_gui_display_message("Error",
              "Sector Count must be multiple of Granularity");
          break;
        }
        if ((sector_count / granularity) > 32) {
          trs_gui_display_message("Error",
              "Sector Count / Granularity must be <= 32");
          break;
        }
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Hard Disk Image, TAB selects directory",
              trs_hard_dir,filename,191,1) == 0) {
          if (trs_create_blank_hard(filename, cylinder_count, sector_count,
                granularity, dir_sector))
            trs_gui_display_message("Error","Error creating Hard Disk Image");
          else if (drive_insert)
            trs_hard_attach(drive_insert-1, filename);
          return;
        }
        break;
      case -1:
        return;
        break;
    }
  }
}

void trs_gui_stringy_management(void)
{
  MENU_ENTRY stringy_menu[] =
  {{" Wafer 0:",MENU_WAFER_BROWSE_TYPE},
   {" Wafer 1:",MENU_WAFER_BROWSE_TYPE},
   {" Wafer 2:",MENU_WAFER_BROWSE_TYPE},
   {" Wafer 3:",MENU_WAFER_BROWSE_TYPE},
   {" Wafer 4:",MENU_WAFER_BROWSE_TYPE},
   {" Wafer 5:",MENU_WAFER_BROWSE_TYPE},
   {" Wafer 6:",MENU_WAFER_BROWSE_TYPE},
   {" Wafer 7:",MENU_WAFER_BROWSE_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Save Disk Set",MENU_NORMAL_TYPE},
   {"Load Disk Set",MENU_NORMAL_TYPE},
   {"Insert Created Image Into This Wafer                       ",MENU_NORMAL_TYPE},
   {"Create Blank Floppy Wafer",MENU_NORMAL_TYPE},
   {"",0}};
  char *wafer_choices[9] = {"   None","Wafer 0","Wafer 1","Wafer 2","Wafer 3",
                            "Wafer 4","Wafer 5","Wafer 6","Wafer 7"};
  char filename[FILENAME_MAX];
  int selection = 0;
  int i;
  static int wafer_insert = 0;

  while (1) {
    for (i=0;i<8;i++) {
      const char *wafername = stringy_get_name(i);

      if (wafername[0] == 0)
        snprintf(&stringy_menu[i].title[9],6,"%s","Empty");
      else
        trs_gui_limit_string(wafername,&stringy_menu[i].title[9],52);
      stringy_menu[i].title[0] = stringy_get_writeprotect(i) ? '*' : ' ';
    }
    snprintf(&stringy_menu[11].title[52],10,"%8s",wafer_choices[wafer_insert]);
    trs_gui_clear_screen();
    selection = trs_gui_display_menu("SDLTRS Stringy Wafer Menu",stringy_menu,selection);
    switch(selection) {
      case 9:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Disk Set, TAB selects directory",
              trs_disk_set_dir,filename,FILENAME_MAX-5,1) == 0) {
          trs_add_extension(filename,".set");
          if (trs_diskset_save(filename) == -1)
            trs_gui_display_message("Error", "Failed to save Disk Set");
        }
        break;
      case 10:
        if (trs_gui_file_browse(trs_disk_set_dir, filename, ".set", 0," Disk Set ") >= 0) {
          if (trs_diskset_load(filename) == -1)
            trs_gui_display_message("Error", "Failed to load Disk Set");
        }
        break;
      case 11:
        wafer_insert = trs_gui_display_popup("Wafer",wafer_choices,9,
            wafer_insert);
        break;
      case 12:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Wafer Image, TAB selects directory",
              trs_cass_dir,filename,FILENAME_MAX-1,1) == 0) {
          if (stringy_create(filename))
            trs_gui_display_message("Error","Error creating Stringy Wafer Image");
          else if (wafer_insert)
            stringy_insert(wafer_insert-1, filename);
          return;
        }
        break;
      case -1:
        return;
        break;
    }
  }
}

void trs_gui_cassette_management(void)
{
  MENU_ENTRY cass_menu[] =
  {{" Cass  :",MENU_CASS_BROWSE_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Cassette Position                                           ",MENU_NORMAL_TYPE},
   {"Cassette Default Sample Rate                                ",MENU_NORMAL_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Image Type                                                  ",MENU_NORMAL_TYPE},
   {"Insert Created Cassette Into Drive                          ",MENU_NORMAL_TYPE},
   {"Create Blank Cassette Image with Above Parameters",MENU_NORMAL_TYPE},
   {"",0}};
  char input[12];
  char *image_type_choices[3] = {"   CAS","   CPT","   WAV"};
  char *drive_choices[2]  =     {"      No","     Yes"};
  char filename[FILENAME_MAX];
  static int image_type = 0;
  static int drive_insert = 1;
  int selection = 0;
  int ret;
  int value;
  FILE *cassette_file;

  while (1) {
    const char *cass_name = trs_cassette_getfilename();

    if (cass_name[0] == 0)
      snprintf(&cass_menu[0].title[8],6,"%s","Empty");
    else
      trs_gui_limit_string(cass_name,&cass_menu[0].title[8],52);
    cass_menu[0].title[0] = trs_cass_getwriteprotect() ? '*' : ' ';

    trs_gui_clear_screen();
    snprintf(&cass_menu[2].title[36],25,"%10d of %10d",trs_get_cassette_position(),trs_get_cassette_length());
    snprintf(&cass_menu[3].title[50],11,"%10d",cassette_default_sample_rate);
    snprintf(&cass_menu[5].title[54],7,"%s",image_type_choices[image_type]);
    snprintf(&cass_menu[6].title[52],9,"%s",drive_choices[drive_insert]);
    selection = trs_gui_display_menu("SDLTRS Cassette Menu",cass_menu, selection);
    switch(selection) {
      case 2:
        snprintf(input,11,"%d",trs_get_cassette_position());
        if (trs_gui_input_string("Enter Cassette Position in Bytes",
              input,input, 10, 0) == 0) {
          value = atoi(input);
          if (value >= 0 && value <= trs_get_cassette_length())
            trs_set_cassette_position(value);
        }
        break;
      case 3:
        snprintf(input,11,"%d",cassette_default_sample_rate);
        if (trs_gui_input_string("Enter Cassette Default Sample Rate",
              input,input, 10, 0) == 0) {
          value = atoi(input);
          if (value < 0 || value > DEFAULT_SAMPLE_RATE)
            value = DEFAULT_SAMPLE_RATE;
          cassette_default_sample_rate = value;
        }
        break;
      case 5:
        image_type = trs_gui_display_popup("Type",image_type_choices,3,
            image_type);
        break;
      case 6:
        drive_insert = trs_gui_display_popup("Insert",drive_choices,2,
            drive_insert);
        break;
      case 7:
        filename[0] = 0;
        if (trs_gui_input_string("Enter Filename for Cassette Image, TAB selects directory",
              trs_cass_dir,filename,FILENAME_MAX-1,1) == 0) {
          if (image_type == 0) {
            trs_add_extension(filename,".cas");
            cassette_file = fopen(filename, "wb");
            if (cassette_file == NULL)
              ret = -1;
            else {
              ret = 0;
              fclose(cassette_file);
            }
          }
          else if (image_type == 1) {
            trs_add_extension(filename,".cpt");
            cassette_file = fopen(filename, "wb");
            if (cassette_file == NULL)
              ret = -1;
            else {
              ret = 0;
              fclose(cassette_file);
            }
          }
          else {
            trs_add_extension(filename,".wav");
            cassette_file = fopen(filename, "wb");
            if (cassette_file == NULL)
              ret = -1;
            else {
              ret = create_wav_header(cassette_file);
              fclose(cassette_file);
            }
          }
          if (ret)
            trs_gui_display_message("Error","Error creating Cassette Image");
          else {
            if (drive_insert)
              trs_cassette_insert(filename);
          }
          return;
        }
        break;
      case -1:
        return;
        break;
    }
  }
}

void trs_gui_display_management(void)
{
  MENU_ENTRY display_menu[] =
  {{"Emulator Background Color                                   ",MENU_NORMAL_TYPE},
   {"Emulator Foreground Color                                   ",MENU_NORMAL_TYPE},
   {"GUI Background Color                                        ",MENU_NORMAL_TYPE},
   {"GUI Foreground Color                                        ",MENU_NORMAL_TYPE},
   {"Model 1 Character Set                                       ",MENU_NORMAL_TYPE},
   {"Model 3 Character Set                                       ",MENU_NORMAL_TYPE},
   {"Model 4/4P Character Set                                    ",MENU_NORMAL_TYPE},
   {"Border Width                                                ",MENU_NORMAL_TYPE},
   {"Resize Window on Mode Change for Model 3                    ",MENU_NORMAL_TYPE},
   {"Resize Window on Mode Change for Model 4                    ",MENU_NORMAL_TYPE},
   {"Scale Factor for Window                                     ",MENU_NORMAL_TYPE},
   {"LED Display for Disks and Turbo Mode                        ",MENU_NORMAL_TYPE},
   {"Display Scanlines to simulate old CRT                       ",MENU_NORMAL_TYPE},
   {"",0,}};
  char input[8];
  char *resize_choices[2] =   {"        No","       Yes"};
  char *disk_led_choices[2] = {" Hide"," Show"};
  char *font1_choices[7] =    {"      Early",
                               "      Stock",
                               "      LCmod",
                               "      Wider",
                               "      Genie",
                               "   HT-1080Z",
                               "Video Genie"};
  char *font34_choices[3] =   {"     Katakana",
                               "International",
                               "         Bold"};
  char *scale_choices[4] =    {"  None","   2 x","   3 x","   4 x"};
  int selection = 0;
  int local_trs_charset1 = trs_charset1;
  int local_trs_charset3 = trs_charset3 - 4;
  int local_trs_charset4 = trs_charset4 - 7;
  unsigned int local_foreground = foreground;
  unsigned int local_background = background;
  unsigned int local_gui_foreground = gui_foreground;
  unsigned int local_gui_background = gui_background;
  int gui_show_led = trs_show_led;
  int gui_resize3 = resize3;
  int gui_resize4 = resize4;
  int gui_scale = scale_x;
  int gui_scanlines = scanlines;
  int gui_border_width = window_border_width;

  if (local_trs_charset1 >= 10)
    local_trs_charset1 -= 6;

  while (1) {
    trs_gui_clear_screen();
    snprintf(&display_menu[0].title[52],9,"0x%06X",local_background);
    snprintf(&display_menu[1].title[52],9,"0x%06X",local_foreground);
    snprintf(&display_menu[2].title[52],9,"0x%06X",local_gui_background);
    snprintf(&display_menu[3].title[52],9,"0x%06X",local_gui_foreground);
    snprintf(&display_menu[4].title[49],12,"%s",font1_choices[local_trs_charset1]);
    snprintf(&display_menu[5].title[47],14,"%s",font34_choices[local_trs_charset3]);
    snprintf(&display_menu[6].title[47],14,"%s",font34_choices[local_trs_charset4]);
    snprintf(&display_menu[7].title[52],9,"%8d",gui_border_width);
    snprintf(&display_menu[8].title[50],11,"%s",resize_choices[gui_resize3]);
    snprintf(&display_menu[9].title[50],11,"%s",resize_choices[gui_resize4]);
    snprintf(&display_menu[10].title[54],7,"%s",scale_choices[gui_scale - 1]);
    snprintf(&display_menu[11].title[55],6,"%s",disk_led_choices[gui_show_led]);
    snprintf(&display_menu[12].title[50],11,"%s",resize_choices[gui_scanlines]);
    selection = trs_gui_display_menu("SDLTRS Display Setting Menu",
        display_menu, selection);
    switch(selection) {
      case 0:
        snprintf(input,7,"%06X",local_background);
        if (trs_gui_input_string("Enter Background RGB color (Hex, RRGGBB)",input,input,6,0) == 0) {
          local_background = strtol(input, NULL, 16);
          if (local_background != background) {
            background = local_background;
            trs_screen_init(0);
            trs_screen_refresh();
          }
        }
        break;
      case 1:
        snprintf(input,7,"%06X",local_foreground);
        if (trs_gui_input_string("Enter Foreground RGB color (Hex, RRGGBB)",input,input,6,0) == 0) {
          local_foreground = strtol(input, NULL, 16);
          if (local_foreground != foreground) {
            foreground = local_foreground;
            trs_screen_init(0);
            trs_screen_refresh();
          }
        }
        break;
      case 2:
        snprintf(input,7,"%06X",local_gui_background);
        if (trs_gui_input_string("Enter GUI Background RGB color (Hex, RRGGBB)",input,input,6,0) == 0) {
          local_gui_background = strtol(input, NULL, 16);
          if (local_gui_background != gui_background) {
            gui_background = local_gui_background;
            trs_screen_init(0);
            trs_screen_refresh();
          }
        }
        break;
      case 3:
        snprintf(input,7,"%06X",local_gui_foreground);
        if (trs_gui_input_string("Enter GUI Foreground RGB color (Hex, RRGGBB)",input,input,6,0) == 0) {
          local_gui_foreground = strtol(input, NULL, 16);
          if (local_gui_foreground != gui_foreground) {
            gui_foreground = local_gui_foreground;
            trs_screen_init(0);
            trs_screen_refresh();
          }
        }
        break;
      case 4:
        local_trs_charset1 = trs_gui_display_popup("Charset 1",font1_choices,7,
            local_trs_charset1);
        break;
      case 5:
        local_trs_charset3 = trs_gui_display_popup("Charset 3",font34_choices,3,
            local_trs_charset3);
        break;
      case 6:
        local_trs_charset4 = trs_gui_display_popup("Charset 4/4P",font34_choices,3,
            local_trs_charset4);
        break;
      case 7:
        snprintf(input,3,"%d",gui_border_width);
        if (trs_gui_input_string("Enter Window border width in pixels",input,input,2,0) == 0)
          gui_border_width = atol(input);
        break;
      case 8:
        gui_resize3 = trs_gui_display_popup("Resize 3",resize_choices,2,
            gui_resize3);
        break;
      case 9:
        gui_resize4 = trs_gui_display_popup("Resize 4",resize_choices,2,
            gui_resize4);
        break;
      case 10:
        gui_scale = trs_gui_display_popup("Scale",scale_choices,4,
            gui_scale - 1) + 1;
        break;
      case 11:
        gui_show_led = trs_gui_display_popup("LED",disk_led_choices,2,
            gui_show_led);
        break;
      case 12:
        gui_scanlines = trs_gui_display_popup("Scanlines",resize_choices,2,
            gui_scanlines);
        break;
      case -1:
        if (local_trs_charset1 >= 4)
          local_trs_charset1 += 6;

        local_trs_charset3 += 4;
        local_trs_charset4 += 7;

        if ((trs_charset1 != local_trs_charset1) ||
            (trs_charset3 != local_trs_charset3) ||
            (trs_charset4 != local_trs_charset4) ||
            (gui_show_led != trs_show_led) ||
            (gui_resize3 != resize3) ||
            (gui_resize4 != resize4) ||
            (gui_scale != scale_x) ||
            (gui_scanlines != scanlines) ||
            (gui_border_width != window_border_width)) {
          trs_charset1 = local_trs_charset1;
          trs_charset3 = local_trs_charset3;
          trs_charset4 = local_trs_charset4;
          trs_show_led = gui_show_led;
          resize3 = gui_resize3;
          resize4 = gui_resize4;
          if (gui_scale != scale_x) {
            fullscreen = 0;
            scale_x = gui_scale;
            scale_y = scale_x*2;
          }
          scanlines = gui_scanlines;
          window_border_width = gui_border_width;
          trs_screen_init(0);
          trs_gui_clear_screen();
          trs_screen_refresh();
        }
      return;
      break;
    }
  }
}

int trs_gui_joystick_get_button(void)
{
  SDL_Event event;

  trs_gui_frame(20, 1, 23, 3);
  trs_gui_write_text("Press Joystick Button", 21, 2, 0);
  trs_x_flush();

  while (1) {
    SDL_WaitEvent(&event);
    switch (event.type) {
      case SDL_QUIT:
        trs_exit(0);
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_F8)
          trs_exit(!(event.key.keysym.mod & KMOD_SHIFT) + 1);
        else if (event.key.keysym.sym == SDLK_ESCAPE)
          return -1;
        break;
      case SDL_JOYBUTTONDOWN:
        if (event.jbutton.button >= N_JOYBUTTONS) {
          trs_gui_display_message("Error", "Unsupported Joystick Button");
          return -1;
        }
        return event.jbutton.button;
        break;
    }
  }
  return -1;
}

void trs_gui_joystick_map_button_to_key(void)
{
  int key, button;

  trs_gui_frame(20, 1, 23, 3);
  trs_gui_write_text("     Select Key      ", 21, 2, 0);
  trs_x_flush();
  if ((key = trs_gui_virtual_keyboard()) == -1)
    return;
  if ((button = trs_gui_joystick_get_button()) != -1)
    jbutton_map[button] = key;
}

void trs_gui_joystick_map_button_to_function(void)
{
  int selection, button;

  trs_gui_frame(20, 1, 23, 3);
  trs_gui_write_text("   Select Function   ", 21, 2, 0);
  trs_x_flush();
  if ((selection = trs_gui_display_popup_matrix("", function_choices, 4, 2, 0)) == -1)
    return;
  if ((button = trs_gui_joystick_get_button()) != -1)
    jbutton_map[button] = function_codes[selection];
}

void trs_gui_joystick_unmap_button(void)
{
  int button = trs_gui_joystick_get_button();

  if (button != -1)
    jbutton_map[button] = -1;
}

void trs_gui_joystick_unmap_all_buttons(void)
{
  if (trs_gui_display_question("Are You Sure?") == 1) {
    int i;

    for (i = 0; i < N_JOYBUTTONS; i++)
      jbutton_map[i] = -1;
 }
}

void trs_gui_joystick_display_map(int show_active)
{
  int row, column, i;
  char text[10];

  for (column = 0; column < 5; column++) {
    for (row = 0; row < 4; row++) {
      i = column * 4 + row;
      snprintf(text, 4,"%2d:", i);
      trs_gui_write_text(text, 2 + column * 12, 11 + row, 0);
      switch (jbutton_map[i]) {
        case -1:     snprintf(text, 9,"---     "); break;
        case GUI:    snprintf(text, 9,"<GUI>   "); break;
        case KEYBRD: snprintf(text, 9,"<KEYBRD>"); break;
        case SAVE:   snprintf(text, 9,"<SAVE>  "); break;
        case LOAD:   snprintf(text, 9,"<LOAD>  "); break;
        case RESET:  snprintf(text, 9,"<RESET> "); break;
        case EXIT:   snprintf(text, 9,"<EXIT>  "); break;
        case PAUSE:  snprintf(text, 9,"<PAUSE> "); break;
        case JOYGUI: snprintf(text, 9,"<JOYGUI>"); break;
        default:
          snprintf(text, 9,"%s", trs_gui_get_key_name(jbutton_map[i]));
          break;
      }
      trs_gui_write_text(text, 5 + column * 12, 11 + row, show_active ? jbutton_active[i] : 0);
    }
  }
}

int trs_gui_display_question(const char *text)
{
  char *answer_choices[] = {
    "          No           ",
    "          Yes          "
  };

  return trs_gui_display_popup(text, answer_choices, 2, 0);
}

void trs_gui_joystick_map_joystick(void)
{
  MENU_ENTRY display_menu[] = {
    {"Map Button to Key",MENU_NORMAL_TYPE},
    {"Map Button to Function",MENU_NORMAL_TYPE},
    {"Unmap Button",MENU_NORMAL_TYPE},
    {"Unmap All Buttons",MENU_NORMAL_TYPE},
    {"Check Button Mapping",MENU_NORMAL_TYPE},
    {"Map Analog Stick to Arrow Keys                              ",MENU_NORMAL_TYPE},
    {"",0}
  };
  char *on_off_choices[2] = {"   Off","    On"};
  int selection = 0, done = 0, checking = 0, counter;

  while (!done) {
    trs_gui_clear_screen();
    trs_gui_joystick_display_map(0);
    snprintf(&display_menu[5].title[54],7,"%s", on_off_choices[jaxis_mapped]);
    selection = trs_gui_display_menu("SDLTRS Map Joystick to Keys/Functions Menu", display_menu, selection);
    switch(selection) {
      case 0:
        trs_gui_joystick_map_button_to_key();
        break;
      case 1:
        trs_gui_joystick_map_button_to_function();
        break;
      case 2:
        trs_gui_joystick_unmap_button();
        break;
      case 3:
        trs_gui_joystick_unmap_all_buttons();
        break;
      case 4:
        checking = 1;
        break;
      case 5:
        jaxis_mapped = trs_gui_display_popup("Stick", on_off_choices, 2, jaxis_mapped);
        break;
      case -1:
        done = 1;
        break;
    }
    counter = CHECK_TIMEOUT;
    while (checking) {
      char text[62];
      int len, first_x;
      SDL_Event event;

      snprintf(text, 60, "Press Joystick Button (%d sec)", counter/1000 + 1);
      len = strlen(text);
      first_x = (64 - len)/2;
      trs_gui_frame(first_x - 1, 1, len + 2, 3);
      trs_gui_write_text(text, first_x, 2, 0);
      trs_x_flush();
      if (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
          trs_exit(0);
          break;
        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_F8)
            trs_exit(!(event.key.keysym.mod & KMOD_SHIFT) + 1);
          else if (event.key.keysym.sym == SDLK_ESCAPE)
            checking = 0;
          break;
        case SDL_JOYBUTTONDOWN:
          if (event.jbutton.button < N_JOYBUTTONS) {
            counter = CHECK_TIMEOUT;
            jbutton_active[event.jbutton.button] = 1;
          }
          break;
        case SDL_JOYBUTTONUP:
          if (event.jbutton.button < N_JOYBUTTONS)
            jbutton_active[event.jbutton.button] = 0;
          break;
        }
        trs_gui_joystick_display_map(1);
      }
      else {
        SDL_Delay(100);
        counter -= 100;
        if (counter <= 0)
          checking = 0;
      }
    }
  }
}

void trs_gui_joystick_management(void)
{
  MENU_ENTRY display_menu[] =
  {{"Use Keypad for Joystick                                     ",MENU_NORMAL_TYPE},
   {"USB Joystick/Gamepad                                        ",MENU_NORMAL_TYPE},
   {"Map Joystick to Keys/Functions",MENU_NORMAL_TYPE},
   {"",0}};
  char *keypad_choices[2] =     {"      No","     Yes"};
  char *joystick_choices[MAX_JOYSTICKS+1];
  char joystick_strings[MAX_JOYSTICKS+1][64];
  int selection = 0;
  int i, num_joysticks, joy_index;
  int gui_keypad_joystick = trs_keypad_joystick;
  int gui_joystick_num = trs_joystick_num;

  for (i=0;i<MAX_JOYSTICKS+1;i++)
    joystick_choices[i] = joystick_strings[i];

  while (1) {
    trs_gui_clear_screen();
    snprintf(&display_menu[0].title[52],9,"%s",keypad_choices[gui_keypad_joystick]);
    if (gui_joystick_num == -1)
      snprintf(&display_menu[1].title[48],13,"        None");
    else
      snprintf(&display_menu[1].title[48],13,"Joystick %1d",gui_joystick_num);
    selection = trs_gui_display_menu("SDLTRS Joystick Setting Menu",
        display_menu, selection);
    switch(selection) {
      case 0:
        gui_keypad_joystick = trs_gui_display_popup("Keypad",keypad_choices,2,
            gui_keypad_joystick);
        break;
      case 1:
        num_joysticks = SDL_NumJoysticks();
        if (num_joysticks > MAX_JOYSTICKS)
          num_joysticks = MAX_JOYSTICKS;
        snprintf(joystick_choices[0],61,"%60s","None");
        for (i=0;i<num_joysticks;i++) {
          snprintf(joystick_choices[i+1],61,"Joystick %1d - %47s",i,
#ifdef SDL2
              SDL_JoystickName(SDL_JoystickOpen(i)));
#else
              SDL_JoystickName(i));
#endif
        }
        if ((gui_joystick_num == -1) || (gui_joystick_num >= num_joysticks))
          joy_index = 0;
        else
          joy_index = gui_joystick_num+1;
        joy_index = trs_gui_display_popup("Joystick",joystick_choices,
            num_joysticks+1,
            joy_index);
        if (joy_index == 0)
          gui_joystick_num = -1;
        else
          gui_joystick_num = joy_index-1;
        break;
      case 2:
        trs_gui_joystick_map_joystick();
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
        break;
    }
  }
}

void trs_gui_misc_management(void)
{
  MENU_ENTRY misc_menu[] =
  {{"Shift Bracket Emulation                                     ",MENU_NORMAL_TYPE},
   {"Sound Output                                                ",MENU_NORMAL_TYPE},
   {"Turbo Mode                                                  ",MENU_NORMAL_TYPE},
   {"Turbo Speed                                                 ",MENU_NORMAL_TYPE},
   {"Keystretch Value                                            ",MENU_NORMAL_TYPE},
   {"Emtsafe                                                     ",MENU_NORMAL_TYPE},
   {"Serial Switches                                             ",MENU_NORMAL_TYPE},
   {"Serial Port Name:                                           ",MENU_TITLE_TYPE},
   {"                                                            ",MENU_NORMAL_TYPE},
   {"",0}};
  char *on_off_choices[2] = {"      Off","       On"};
  char input[FILENAME_MAX];
  int selection = 0;

  while (1) {
    trs_gui_clear_screen();
    snprintf(&misc_menu[0].title[51],10,"%s",on_off_choices[trs_kb_bracket_state]);
    snprintf(&misc_menu[1].title[51],10,"%s",on_off_choices[trs_sound]);
    snprintf(&misc_menu[2].title[51],10,"%s",on_off_choices[timer_overclock]);
    snprintf(&misc_menu[3].title[50],11,"%10d", timer_overclock_rate);
    snprintf(&misc_menu[4].title[50],11,"%10d",stretch_amount);
    snprintf(&misc_menu[5].title[51],10,"%s",on_off_choices[trs_emtsafe]);
    snprintf(&misc_menu[6].title[56],5,"0x%02X",trs_uart_switches);
    trs_gui_limit_string(trs_uart_name,&misc_menu[8].title[2],60);
    selection = trs_gui_display_menu("SDLTRS Misc Settings Menu",
        misc_menu, selection);

    switch(selection) {
      case 0:
        trs_kb_bracket_state = trs_gui_display_popup("Bracket",on_off_choices,2,
            trs_kb_bracket_state);
        break;
      case 1:
        trs_sound = trs_gui_display_popup("Sound",on_off_choices,2,
            trs_sound);
        break;
      case 2:
        timer_overclock = trs_gui_display_popup("Turbo",on_off_choices,2,
            timer_overclock);
        break;
      case 3:
        snprintf(input,11,"%d",timer_overclock_rate);
        if (trs_gui_input_string("Enter Turbo Rate Multiplier",input,input,10,0) == 0) {
          timer_overclock_rate =  atoi(input);
          if (timer_overclock_rate <= 0)
            timer_overclock_rate = 1;
        }
        break;
      case 4:
        snprintf(input,11,"%d",stretch_amount);
        if (trs_gui_input_string("Enter Keystretch in Cycles",input,input,10,0) == 0)
          stretch_amount = atoi(input);
        break;
      case 5:
        trs_emtsafe = trs_gui_display_popup("Emtsafe",on_off_choices,2,
            trs_emtsafe);
        break;
      case 6:
        snprintf(input,3,"%2X",trs_uart_switches);
        if (trs_gui_input_string("Enter Serial Switches (Hex, XX)",input,input,2,0) == 0) {
          trs_uart_switches = strtol(input,NULL,16);
          trs_uart_init(0);
        }
        break;
      case 8:
        if (trs_gui_input_string("Enter Serial Port Name",trs_uart_name,input,FILENAME_MAX-1,0) == 0) {
          snprintf(trs_uart_name, FILENAME_MAX, "%s", input);
          trs_uart_init(0);
        }
        break;
      case -1:
        trs_kb_bracket(trs_kb_bracket_state);
        trs_screen_caption(timer_overclock, trs_sound);
        return;
        break;
    }
  }
}

void trs_gui_printer_management(void)
{
  MENU_ENTRY printer_menu[] =
  {{"Printer Type                                                ",MENU_NORMAL_TYPE},
   {"Close and Reopen Printer Output File",MENU_NORMAL_TYPE},
   {"Printer Command:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"",0}};
  char *printer_choices[2] = {"     None","     Text"};
  char input[FILENAME_MAX];
  int selection = 0;

  while (1) {
    trs_gui_clear_screen();
    snprintf(&printer_menu[0].title[51],10,"%s",printer_choices[trs_printer]);
    trs_gui_limit_string(trs_printer_command,&printer_menu[3].title[2],60);
    selection = trs_gui_display_menu("SDLTRS Printer Management Menu",
        printer_menu, selection);

    switch(selection) {
      case 0:
        trs_printer = trs_gui_display_popup("Printer",printer_choices,2,
            trs_printer);
        break;
      case 1:
        if (trs_printer_reset() != -1)
          trs_gui_display_message("Status","Printer file closed, printer command ran");
        else
          trs_gui_display_message("Warning","No Printer Output in File");
        break;
      case 3:
        if (trs_gui_input_string("Enter Printer Command",trs_printer_command,input, FILENAME_MAX-1,0) == 0)
          snprintf(trs_printer_command, FILENAME_MAX, "%s", input);
        break;
      case -1:
        return;
        break;
    }
  }
}

void trs_gui_model(void)
{
  MENU_ENTRY model_menu[] =
  {{"Model                                                       ",MENU_NORMAL_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Lowercase Modification for Model I                          ",MENU_NORMAL_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Exatron Stringy Floppy Emulation for Model I                ",MENU_NORMAL_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Lowe Electronics LE18 Graphics Emulation                    ",MENU_NORMAL_TYPE},
   {"Micro Labs Grafyx Solution Graphics Emulation               ",MENU_NORMAL_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Dave Huffman (and other) Memory Expansion                   ",MENU_NORMAL_TYPE},
   {"Alpha Technologies HyperMem Memory Expansion                ",MENU_NORMAL_TYPE},
   {"Alpha Technologies SuperMem Memory Expansion                ",MENU_NORMAL_TYPE},
   {"TRS80 Users Society Selector Memory Expansion               ",MENU_NORMAL_TYPE},
   {"",0}};
  char *model_choices[4] =  {"  TRS-80 Model I",
                             "TRS-80 Model III",
                             "  TRS-80 Model 4",
                             " TRS-80 Model 4P"};
  char *on_off_choices[2] = {"        Off","         On"};
  int selection = 0;
  int model_selection = 0;
  int state;
  int local_trs_model = trs_model;

  while (1) {
    trs_gui_clear_screen();
    switch(local_trs_model) {
      case 1:
        model_selection = 0;
        break;
      case 3:
      case 4:
        model_selection = local_trs_model-2;
        break;
      case 5:
        model_selection = 3;
        break;
    }
    snprintf(&model_menu[0].title[44],17,"%s",model_choices[model_selection]);
    snprintf(&model_menu[2].title[49],12,"%s",on_off_choices[lowercase]);
    snprintf(&model_menu[4].title[49],12,"%s",on_off_choices[stringy]);
    snprintf(&model_menu[6].title[49],12,"%s",on_off_choices[lowe_le18]);
    snprintf(&model_menu[7].title[49],12,"%s",on_off_choices[grafyx_get_microlabs()]);
    snprintf(&model_menu[9].title[49],12,"%s",on_off_choices[huffman_ram]);
    snprintf(&model_menu[10].title[49],12,"%s",on_off_choices[hypermem]);
    snprintf(&model_menu[11].title[49],12,"%s",on_off_choices[supermem]);
    snprintf(&model_menu[12].title[49],12,"%s",on_off_choices[selector]);

    selection = trs_gui_display_menu("SDLTRS Emulator Setting Menu",model_menu, selection);
    switch(selection) {
      case -1:
        if (trs_model != local_trs_model) {
          trs_model = local_trs_model;
          trs_gui_new_machine();
        }
        return;
        break;
      case 0:
        model_selection = trs_gui_display_popup("Model",model_choices,4,
            model_selection);
        switch(model_selection) {
          case 0:
            local_trs_model = 1;
            break;
          case 1:
            local_trs_model = 3;
            break;
          case 2:
            local_trs_model = 4;
            break;
          case 3:
            local_trs_model = 5;
            break;
        }
        break;
      case 2:
        lowercase = trs_gui_display_popup("Lowercase",on_off_choices,2,
            lowercase);
        break;
      case 4:
        stringy = trs_gui_display_popup("Stringy",on_off_choices,2,
            stringy);
        break;
      case 6:
        lowe_le18 = trs_gui_display_popup("Lowe LE18",on_off_choices,2,
            lowe_le18);
        break;
      case 7:
        state = trs_gui_display_popup("Grafyx",on_off_choices,2,
            grafyx_get_microlabs());
        grafyx_set_microlabs(state);
        break;
      case 9:
        huffman_ram = trs_gui_display_popup("Huffman",on_off_choices,2,
            huffman_ram);
        if (huffman_ram)
          hypermem = 0;
        break;
      case 10:
        hypermem = trs_gui_display_popup("HyperMem",on_off_choices,2,
            hypermem);
        if (hypermem)
          huffman_ram = 0;
        break;
      case 11:
        supermem = trs_gui_display_popup("SuperMem",on_off_choices,2,
            supermem);
        if (supermem)
          selector = 0;
        break;
      case 12:
        selector = trs_gui_display_popup("Selector",on_off_choices,2,
            selector);
        if (selector)
          supermem = 0;
        break;
    }
  }
}

void trs_gui_default_dirs(void)
{
  MENU_ENTRY default_menu[] =
  {{"Floppy Disk Directory:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"Hard Disk Directory:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"Cassette Directory:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"Disk Set Directory:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"State Directory:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"Printer Output Directory:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"",0}};
  int selection = 1;

  while (1) {
    trs_gui_clear_screen();
    trs_gui_limit_string(trs_disk_dir,&default_menu[1].title[2],59);
    trs_gui_limit_string(trs_hard_dir,&default_menu[3].title[2],59);
    trs_gui_limit_string(trs_cass_dir,&default_menu[5].title[2],59);
    trs_gui_limit_string(trs_disk_set_dir,&default_menu[7].title[2],59);
    trs_gui_limit_string(trs_state_dir,&default_menu[9].title[2],59);
    trs_gui_limit_string(trs_printer_dir,&default_menu[11].title[2],59);
    /* print current defaults */
    selection = trs_gui_display_menu("SDLTRS Default Directory Menu",default_menu, selection);
    switch(selection) {
      case -1:
        return;
        break;
      case 1:
        trs_gui_file_browse(trs_disk_dir, trs_disk_dir, NULL, 1," Floppy Disk ");
        break;
      case 3:
        trs_gui_file_browse(trs_hard_dir, trs_hard_dir, NULL, 1," Hard Disk ");
        break;
      case 5:
        trs_gui_file_browse(trs_cass_dir, trs_cass_dir, NULL, 1," Cassette ");
        break;
      case 7:
        trs_gui_file_browse(trs_disk_set_dir, trs_disk_set_dir, NULL, 1," Disk Set ");
        break;
      case 9:
        trs_gui_file_browse(trs_state_dir, trs_state_dir, NULL, 1," Saved State ");
        break;
      case 11:
        trs_gui_file_browse(trs_printer_dir, trs_printer_dir, NULL, 1," Printer Output ");
        break;
    }
  }
}

void trs_gui_rom_files(void)
{
  MENU_ENTRY romfile_menu[] =
  {{"Model 1 ROM File:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Model 3 ROM File:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"",MENU_TITLE_TYPE},
   {"Model 4P ROM File:",MENU_TITLE_TYPE},
   {"   ",MENU_NORMAL_TYPE},
   {"",0}};
  int selection = 1;

  while (1) {
    trs_gui_clear_screen();
    trs_gui_limit_string(romfile,&romfile_menu[1].title[2],58);
    trs_gui_limit_string(romfile3,&romfile_menu[4].title[2],58);
    trs_gui_limit_string(romfile4p,&romfile_menu[7].title[2],58);
    selection = trs_gui_display_menu("SDLTRS ROM File Selection Menu",romfile_menu, selection);
    switch(selection) {
      case -1:
        return;
        break;
      case 1:
        trs_gui_file_browse(romfile, romfile, NULL, 0," Model 1 ROM ");
        break;
      case 4:
        trs_gui_file_browse(romfile3, romfile3, NULL, 0," Model 3 ROM ");
        break;
      case 7:
        trs_gui_file_browse(romfile4p, romfile4p, NULL, 0," Model 4P ROM ");
        break;
    }
  }
}

void trs_gui_about_sdltrs(void)
{
  trs_gui_clear_screen();
  trs_gui_frame(0,0,64,16);
  trs_gui_write_text("About SDLTRS", 2, 0, 0);

  trs_gui_center_text("SDLTRS", 3, 0);
  trs_gui_center_text("Version 1.2.3", 4, 0);
  trs_gui_center_text("Copyright (C) 2006-2011 Mark Grebe", 5, 0);
  trs_gui_center_text("<http://sdltrs.sourceforge.net>", 6, 0);
  trs_gui_center_text("Patches (2018-2019) by Alan Cox and Jens Guenther", 8, 0);
  trs_gui_center_text("<http://gitlab.com/jengun/sdltrs>", 9, 0);
  trs_gui_center_text("Based on xtrs 4.9d by Tim Mann", 11, 0);
  trs_gui_center_text("xtrs 1.0 Copyright (C) 1992 Clarendon Hill Software", 12, 0);
  trs_gui_center_text("Press Any Key To Return", 15, 1);
  trs_x_flush();

  trs_gui_get_key();
}

void trs_gui_keys_sdltrs(void)
{
  trs_gui_clear_screen();
  trs_gui_frame(0,0,64,16);
  trs_gui_write_text("Keys in SDLTRS", 2, 0, 0);

  trs_gui_write_text("F1-F3: Functions Keys F1/F2/F3  PgUp/PgDn: Left/Right Shift ", 2, 1, 0);
  trs_gui_write_text("F4: F4/CapsLock on TRS-80 4/4P  Insert: TRS-80 Underscore   ", 2, 2, 0);
  trs_gui_write_text("F5/ScrollLock/End: TRS-80 @     Shift UP Arrow: TRS-80 ESC  ", 2, 3, 0);
  trs_gui_write_text("F6: TRS-80 '0' Key (Shifted 0)  Alt PgUp/PgDn: Scale Window ", 2, 4, 0);
  trs_gui_write_text("F7/Alt M: Main Menu of SDLTRS   Alt Enter: Toggle Fullscreen", 2, 5, 0);
  trs_gui_write_text("F8/Shift-F8: Exit/Abort SDLTRS  Alt A/C/V: Select/Copy/Paste", 2, 6, 0);
  trs_gui_write_text("F9/Alt Z: Enter debugger (zbx)  Alt D/F: Floppy Disk Menu   ", 2, 7, 0);
  trs_gui_write_text("F10/Shift-F10: Warm/Cold Reset  Alt H: Hard Disk Menu       ", 2, 8, 0);
  trs_gui_write_text("F11/Alt K: Show this key help   Alt T: Cassette/Tape Menu   ", 2, 9, 0);
  trs_gui_write_text("F12/Alt N: Switch Turbo On/Off  Alt L/S: Load / Save State  ", 2, 10, 0);
  trs_gui_write_text("ESC: TRS-80 Break Key           Alt R/W: Read / Write Config", 2, 11, 0);
  trs_gui_write_text("Home/Clear: TRS-80 Clear Key    Alt P/Pause: Pause Emulator ", 2, 12, 0);
  trs_gui_write_text("Control: TRS-80 4/4P Ctrl Key   Alt 0-7: Insert Disk Drive  ", 2, 13, 0);
  trs_gui_write_text("RightAlt: TRS-80 Shifted Down   Shift Alt 0-7: Remove Disk  ", 2, 14, 0);
  trs_gui_center_text("Press Any Key To Return", 15, 1);
  trs_x_flush();

  trs_gui_get_key();
}

int trs_gui_exit_sdltrs(void)
{
  return trs_gui_display_question("Exit SDLTRS?");
}

void trs_gui_write_config(void)
{
  char filename[FILENAME_MAX];

  if (trs_gui_input_string("Write Configuration to file, TAB selects directory",
                            trs_config_file,filename,FILENAME_MAX-5,1) == 0) {
    trs_add_extension(filename,".t8c");
    if (trs_write_config_file(filename) == -1)
      trs_gui_display_message("Error", "Failed to write Configuration");
  }
}

int trs_gui_read_config(void)
{
  if (trs_gui_file_browse(trs_config_file, trs_config_file, ".t8c", 0," Configuration (.t8c) ") == -1)
    return -1;
  if (trs_load_config_file() == -1) {
    trs_gui_display_message("Error", "Failed to read Configuration");
    return -1;
  }
  trs_gui_new_machine();
  return 0;
}

static int trs_gui_config_management(void)
{
  MENU_ENTRY misc_menu[] =
  {{"Save Emulator State      (ALT-S)",MENU_NORMAL_TYPE},
   {"Load Emulator State      (ALT-L)",MENU_NORMAL_TYPE},
   {"Write Configuration File (ALT-W)",MENU_NORMAL_TYPE},
   {"Read Configuration File  (ALT-R)",MENU_NORMAL_TYPE},
   {"",0}};
  int selection = 0;

  while (1) {
    trs_gui_clear_screen();
    selection = trs_gui_display_menu("SDLTRS Configuration Files Menu",
        misc_menu, selection);

    switch(selection) {
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
        break;
    }
  }
}

void trs_gui_save_state(void)
{
  char filename[FILENAME_MAX];

  filename[0] = 0;
  if (trs_gui_input_string("Save Emulator State to file, TAB selects directory",
      init_state_file[0] != 0 ? init_state_file : trs_state_dir,filename,FILENAME_MAX-5,1) == 0) {
    trs_add_extension(filename,".t8s");
    if (trs_state_save(filename) == -1)
      trs_gui_display_message("Error", "Failed to save State");
  }
}

int trs_gui_load_state(void)
{
  char filename[FILENAME_MAX];

  if (trs_gui_file_browse(trs_state_dir, filename, ".t8s", 0," Saved State (.t8s) ") == -1)
    return -1;
  if (trs_state_load(filename) == -1) {
    trs_gui_display_message("Error", "Failed to load State");
    return -1;
  }
  return 0;
}

void trs_gui_new_machine(void)
{
  trs_screen_var_reset();
  romin = 0;
  mem_init();
  trs_disk_init(0);
  trs_rom_init();
  trs_screen_init(1);
  screen_init();
  trs_timer_init();
  trs_reset(1);
}

void trs_gui(void)
{
  MENU_ENTRY main_menu[] =
  {{"Floppy Disk Management   (ALT-D)",MENU_NORMAL_TYPE},
   {"Hard Disk Management     (ALT-H)",MENU_NORMAL_TYPE},
   {"Cassette Management      (ALT-T)",MENU_NORMAL_TYPE},
   {"Stringy Wafer Management (ALT-G)",MENU_NORMAL_TYPE},
   {"Emulator Settings        (ALT-E)",MENU_NORMAL_TYPE},
   {"Configuration/State File Management",MENU_NORMAL_TYPE},
   {"Printer Management",MENU_NORMAL_TYPE},
   {"Select Default Directories",MENU_NORMAL_TYPE},
   {"ROM File Selection",MENU_NORMAL_TYPE},
   {"Display Settings         (ALT-I)",MENU_NORMAL_TYPE},
   {"Joystick Settings",MENU_NORMAL_TYPE},
   {"Miscellaneous Settings   (ALT-O)",MENU_NORMAL_TYPE},
   {"About SDLTRS",MENU_NORMAL_TYPE},
   {"",0}};
  int selection = 0;

  while (1) {
    trs_gui_clear_screen();
    selection = trs_gui_display_menu("SDLTRS Main Menu",main_menu, selection);
    switch(selection) {
      case -1:
        return;
        break;
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
        trs_gui_model();
        break;
      case 5:
        if (trs_gui_config_management())
          return;
        break;
      case 6:
        trs_gui_printer_management();
        break;
      case 7:
        trs_gui_default_dirs();
        break;
      case 8:
        trs_gui_rom_files();
        trs_rom_init();
        break;
      case 9:
        trs_gui_display_management();
        break;
      case 10:
        trs_gui_joystick_management();
        break;
      case 11:
        trs_gui_misc_management();
        break;
      case 12:
        trs_gui_about_sdltrs();
        break;
    }
  }
}

int trs_gui_display_popup_matrix(const char* title, const char **entry,
  int rows, int columns, int selection)
{
  int row, column;
  int entry_count = rows*columns;
  int num, i, j, key;
  int width, first_x, first_y;
  int done = 0;
  unsigned int max_len = 0;

  if (selection < 0)
    selection = 0;
  else if (selection >= entry_count)
    selection = entry_count - 1;
  row = selection/columns;
  column = selection%columns;
  for (num = 0; num < entry_count; num++)
    if (strlen(entry[num]) > max_len)
      max_len = strlen(entry[num]);
  width = columns*(max_len + 1) - 1;
  first_x = (64 - width)/2;
  first_y = (16 - rows)/2;
  trs_gui_save_rect(first_x - 1, first_y - 1, width + 2, rows + 2);

  trs_gui_frame(first_x - 1, first_y - 1, width + 2, rows + 2);
  trs_gui_write_text(title, first_x + 1, first_y - 1, 0);
  trs_gui_clear_rect(first_x, first_y, width, rows);
  for (i = 0; i < rows; i++)
    for (j = 0; j < columns; j++)
      trs_gui_write_text(entry[i*columns + j], first_x + j*(max_len + 1), first_y + i, 0);

  while (!done) {
    selection = row*columns + column;
    trs_gui_write_text(entry[selection], first_x + column*(max_len + 1), first_y + row, 1);
    trs_x_flush();
    key = trs_gui_get_key();
    trs_gui_write_text(entry[selection], first_x + column*(max_len + 1), first_y + row, 0);
    switch (key) {
      case SDLK_DOWN:
        if (row < rows - 1) row++; else row = 0;
        break;
      case SDLK_UP:
        if (row > 0) row--; else row = rows - 1;
        break;
      case SDLK_RIGHT:
        if (column < columns - 1) column++; else column = 0;
        break;
      case SDLK_LEFT:
        if (column > 0) column--; else column = columns - 1;
        break;
      case SDLK_HOME:
        column = 0;
        break;
      case SDLK_END:
        column = columns - 1;
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
        done = 1;
        break;
      case SDLK_ESCAPE:
        selection = -1;
        done = 1;
        break;
    }
  }
  trs_gui_restore_rect(first_x - 1, first_y - 1, width + 2, rows + 2);

  return selection;
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
    key_index = trs_gui_display_popup_matrix("Virtual Keyboard",
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

  if (key != -1) {
    SDL_Event event;

    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = key;
    event.key.keysym.mod = 0;
    event.key.keysym.scancode = 0;
#ifndef SDL2
    event.key.keysym.unicode = 0;
#endif
    SDL_PushEvent(&event);
  }
}

void trs_gui_joy_gui(void)
{
  int selection = trs_gui_display_popup_matrix("Joystick GUI", function_choices, 3, 2, 0);

  if (selection == -1)
    return;
  trs_screen_refresh();
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
      trs_disk_led(-1, 0);
      trs_hard_led(-1, 0);
      break;
    case EXIT:
      trs_exit(2);
      break;
  }
}
