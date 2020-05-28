## Version 1.2.10 Release 05-24-2020:

  * Added Z80 refresh register.
  * Added more accurate Z80 block moves.
  * Added support for Sprinter II/III.
  * Fixed bugs and other improvements.

## Version 1.2.9a Release 04-19-2020:

  * Fixed crash in parity table on 64-bit systems.
  * Fixed crash in SDL cleanup.
  * Fixed crash/garbaled TRS-80 graphics.
  * Fixed mouse selection on window redraw.
  * Toggle joystick emulation with NumLock key.

## Version 1.2.9 Release 04-12-2020:

  * Fixed SDL2 regressions in version 1.2.8.
  * Added display of Z80 speed in window title bar.
  * Added "Turbo Paste" function.
  * Adjust Z80 clock rate on the fly.
  * Fixed bugs and other improvements.

## Version 1.2.8 Release 03-22-2020:

  * Major bugfix release.

## Version 1.2.7 Release 01-25-2020:

  * Fixed bugs and some small improvements.

## Version 1.2.6 Release 12-29-2019:

  * Added CPU panel to show Z80 registers in window title bar.
  * Fixed SDL2 keyboard handling and other bugs.

## Version 1.2.5 Release 12-21-2019:

  * Added function to select and execute CMD files directly.
  * Added function to save screenshot as BMP file.
  * Fixed various bugs.

## Version 1.2.4 Release 11-24-2019:

  * Added function to load and execute Model I CMD files directly.
  * Fixed some bugs.

## Version 1.2.3 Release 10-15-2019:

  * Added character sets for HT-1080Z, Video Genie/System 80/PMC-80.
  * Added support for function keys on Genie II/System 80 Mk II/PMC-81.
  * Added write protection for cassette images.
  * Fixed more bugs.

## Version 1.2.2 Release 08-18-2019:

  * Added support for Exatron Stringy Floppy.
  * Added new key bindings and menu options.
  * Fixed various bugs.

## Version 1.2.1 Release 07-24-2019:

  * Added LED for Turbo Mode.
  * Fixed various bugs.
  * SDL2 port should be stable.

## Version 1.2.0 Release 06-09-2019:

  * Included support for banked memory and Lowe LE18 by Alan Cox.
  * Fixed SegFaults: ROM Selection Menu, Scaling in Fullscreen.
  * New shortcut keys, key bindings and help screen.
  * Ctrl-A, Ctrl-C & Ctrl-V can now be used in the Emulator.
  * Access to real floppy disks works now under Linux.
  * Experimental port to SDL2.

## Version 1.1.0 Release 02-15-2010:

  * Added Turbo Mode - This allows the emulator to run faster than a real
    TRS-80. It is turned on/off using the F11 key. By default, when on, the
    emulator runs 5 times the speed of a normal TRS-80, however, the rate is
    adjustable through the Text GUI or command line/configuration file
    options.
  * Changed Disk Drive Numbering to Start With 0, not 1 - The intial release
    of the emulator numbered floppy drives from 1-8 and hard drives from 1-4.
    However, TRS-80 drives started number at 0, so this was very confusing for
    users. This has been changed in this release, however, note that
    configurtaion files and command line options will need to be changed by
    users to account for the numbering change in -disk and -hard options.
  * Added more keys definitions to key documentation, per bug reports.

## Version 1.0.0 Release 09-17-2009:

  * First Public Release.

## Version 0.2.0 Release 10-08-2006:

  * First Checkin to Sourceforge SVN.