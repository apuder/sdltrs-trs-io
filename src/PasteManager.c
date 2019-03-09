#ifdef SDL2
#include "PasteManager_sdl2.c"
#elif WIN32
#include "PasteManager_win32.c"
#elif !NOX
#include "PasteManager_x11.c"
#endif
