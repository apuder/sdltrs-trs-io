To build with autotools:
------------------------

From the main directory, execute:
```sh
./autogen.sh
```
which will generate the `configure` script. Installation of `aclocal`,
`autoconf` and `automake` is needed for this.

To enable faster but not accurate Z80 block moves, execute:
```sh
./configure --enable-fastmem
```
To enable `readline` support for the zbx debugger, execute:
```sh
./configure --enable-readline
```
To build the old SDL 1.2 version (see below) of **SDLTRS**, execute:
```sh
./configure --enable-sdl1
```
To build with SDL 1.2 only (no *X11* and no *PasteManager*), execute:
```sh
./configure --enable-sdl1 --without-x
```
Start build of the program in the main directory by executing:
```sh
make
```

---

To build with SDL2:
-------------------

**SDL2TRS** needs the development files of `SDL2` and `GNU readline`
for the debugger. On *Debian* or *Ubuntu* these can be installed with:
```sh
sudo apt install libsdl2-dev libreadline-dev
```
From the `src` directory, execute:
```sh
make sdl2
```

---

To build with SDL 1.2:
----------------------

**SDLTRS** needs the development files of `libSDL 1.2`, `X11` and `GNU
readline` for the debugger. On *Debian* or *Ubuntu* these can be
installed with:
```sh
sudo apt install libsdl1.2-dev libreadline-dev
```
From the `src` directory, execute:
```sh
make sdl
```

---

To build with SDL 1.2 only (no X11):
------------------------------------

From the `src` directory, execute:
```sh
make nox
```
**SDLTRS** will be build without the *PasteManager* (*Copy & Paste* to
and from the Emulator screen), but works on operating systems with no
*X11*-server like *Haiku* or *BeOS*.

---

To build on FreeBSD/NetBSD/OpenBSD with SDL 1.2:
------------------------------------------------

From the `src` directory, execute:
```sh
make bsd
```

---

To build on macOS:
------------------

Download and install [Homebrew] for macOS first.
```sh
brew install autoconf automake libtool llvm readline sdl2
```
should download and install the required packages to build **SDL2TRS**.
In the main directory of the source, execute the following commands:
```sh
./autogen
./configure --enable-readline
make
```

This will build the executable binary file called `sdltrs`.

---

To build on Win32 with SDL 2.0/SDL 1.2:
---------------------------------------

**SDLTRS** is designed to be build with [MinGW]. The [SDL] development
library is also required.

The runtime library file `SDL2.DLL` or `SDL.DLL` should be copied to the
directory of the **SDLTRS** binary file, the header files of the library
to `\MinGW\include\SDL2` or `\MinGW\include\SDL`, and libraries to the
`\MinGW\lib\` directory or edit the macros `LIBS` and `INCS` in `Makefile`
to point to the location of the SDL installation.

From the `src` directory, execute:
```sh
mingw32-make wsdl2
```
to build the SDL2 version, or
```sh
mingw32-make win32
```
for the old SDL 1.2 version.

[Homebrew]: https://brew.sh
[MinGW]: http://www.mingw.org
[SDL]: https://www.libsdl.org
