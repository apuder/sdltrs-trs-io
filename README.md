Modified version of Mark Grebe's [SDLTRS] with bug fixes and patches by
[EtchedPixels] for running [FUZIX]

![screenshot](fuzix.png)
![screenshot](fuzix2.png)

## Changes

  * Included all patches by Alan Cox: support for banked memory, Lowe LE18 ...
  * Fixed various SegFaults: ROM Selection Menu, Scaling in Fullscreen ...
  * Reworked the Interface: new shortcut keys and key bindings, help screen ...
  * Ctrl-A, Ctrl-C & Ctrl-V can now be used in the Emulator (CP/M & WordStar)
  * Access to real floppy disks works now under Linux ...
  * Tried to fix reported bugs to the original version ...
  * Experimental port to SDL2 (see [BUILDING]) ...

[BUILDING]: BUILDING
[EtchedPixels]: https://www.github.com/EtchedPixels/xtrs
[FUZIX]: https://www.github.com/EtchedPixels/FUZIX
[SDLTRS]: http://sdltrs.sourceforge.net
