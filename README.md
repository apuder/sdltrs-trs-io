Modified version of Mark Grebe's [SDLTRS] with bug fixes and patches by
[EtchedPixels] for running [FUZIX]

![screenshot](fuzix.png)
![screenshot](fuzix2.png)

## Changes

  * Included all patches by Alan Cox: support for banked memory, Lowe LE18 ...
  * Fixed various SegFaults: ROM Selection Menu, Scaling in Fullscreen ...
  * Reworked the Interface: new shortcuts and key bindings, help screen ...
  * Ctrl-A, Ctrl-C & Ctrl-V can now be used in the Emulator (CP/M & WordStar)
  * Access to real floppy disks works now on Linux ...
  * Tried to fix reported bugs to the original version ...
  * Port to SDL2 (see [BUILDING]) ...
  * Support for Exatron Stringy Floppy ...

## Binaries

  * [sdltrs.exe]    32-bit (needs  [SDL.DLL] 32-bit)
  * [sdl2trs.exe]   32-bit (needs [SDL2.DLL] 32-bit)
  * [sdl2trs64.exe] 64-bit (needs [SDL2.DLL] 64-bit)

(Build with Mingw32 & Mingw-w64)

[BUILDING]: BUILDING
[EtchedPixels]: https://www.github.com/EtchedPixels/xtrs
[FUZIX]: https://www.github.com/EtchedPixels/FUZIX
[SDL.DLL]: https://www.libsdl.org/download-1.2.php
[SDL2.DLL]: https://www.libsdl.org/download-2.0.php
[SDLTRS]: http://sdltrs.sourceforge.net
[sdltrs.exe]: bin/sdltrs.exe
[sdl2trs.exe]: bin/sdl2trs.exe
[sdl2trs64.exe]: bin/sdl2trs64.exe
