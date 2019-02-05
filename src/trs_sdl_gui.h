/* Copyright (c): 2006, Mark Grebe */

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

#define N_JOYBUTTONS (20)
#define GUI          (1)
#define KEYBRD       (2)
#define SAVE         (3)
#define LOAD         (4)
#define RESET        (5)
#define EXIT         (6)
#define PAUSE        (7)
#define JOYGUI       (8)
#define DISK         (9)
#define HARD         (10)
#define TAPE         (11)
#define SAVE_STATE   (12)
#define LOAD_STATE   (13)
#define READ         (14)
#define WRITE        (15)
#define EMULATOR     (16)
#define INTERFACE    (17)
#define OTHER        (18)

extern int jbutton_map[N_JOYBUTTONS];
extern int jaxis_mapped;

void trs_expand_dir(char *dir, char *expanded_dir);
void trs_gui_display_pause(void);
int  trs_gui_file_browse(char* path, char* filename, int browse_dir, char* type);
void trs_gui_disk_management(void);
void trs_gui_hard_management(void);
void trs_gui_cassette_management(void);
void trs_gui_display_management(void);
void trs_gui_misc_management(void);
void trs_gui_save_state(void);
int  trs_gui_load_state(void);
void trs_gui_write_config(void);
int  trs_gui_read_config(void);
void trs_gui_new_machine(void);
void trs_gui(void);

void trs_gui_refresh();
void trs_gui_clear_screen(void);
void trs_gui_write_char(int position, int char_index, int invert);
void trs_expand_dir(char *dir, char *expanded_dir);

void trs_gui_get_virtual_key(void);
void trs_gui_joy_gui(void);

extern void trs_gui_save_rect(int x, int y, int w, int h);
extern void trs_gui_restore_rect(int x, int y, int w, int h);
