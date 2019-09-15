#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_clipboard.h>

/* Emulator specific variables */
static int charCount = 0;
static char *pasteString;
static int pasteStringLength = 0;

/* Extern emulator routines */
extern int trs_paste_started();
extern void trs_end_copy();

int PasteManagerGetChar(unsigned short *character)
{
  if (charCount) {
    *character = pasteString[pasteStringLength - charCount];
    charCount--;
    if (charCount)
      return(1);
    else {
      free(pasteString);
      return(0);
    }
  }
  else {
    free(pasteString);
    return(0);
  }
}

int PasteManagerStartPaste(void)
{ 
  pasteString = SDL_GetClipboardText();
  pasteStringLength = strlen(pasteString);
  charCount = pasteStringLength;

  if (charCount) {
    trs_paste_started();
    return 1;
  } else {
    free(pasteString);
    return 0;
  }
}


void PasteManagerStartCopy(char *string)
{
  SDL_SetClipboardText(string);
  trs_end_copy();
}
