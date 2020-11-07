**SDL(2)TRS** needs the development files of `SDL2`/`SDL 1.2` and
optional `GNU readline` for the integrated Z80 debugger zbx.

On *Debian* and *Ubuntu* based systems these can be installed with:
```sh
sudo apt install libsdl2-dev libreadline-dev
```
or
```sh
sudo apt install libsdl1.2-dev libreadline-dev
```
for the SDL 1.2 version.

For *Win32/64* please install [MinGW] or [MinGW-w64] and the [SDL]
development library with the `mingw.tar.gz` file extension.
It is recommended to use [MSYS2] to make things easier.

---

To build with autotools:
------------------------

Installation of `aclocal`, `autoconf` and `automake` is required:
```sh
sudo apt install autoconf automake autotools-dev
```
on *Debian* and *Ubuntu* systems.

From the main directory, execute:
```sh
./autogen.sh
```
which will generate the `configure` script.

To configure the build system, execute:
```sh
./configure
```

This should autodetect the configuration but some options may be passed:
```sh
./configure --enable-fastmove
```
to enable faster but not accurate Z80 block moves,
```sh
./configure --enable-oldscan
```
to enable old method to display Scanlines,
```sh
./configure --enable-sdl1
```
to build the SDL 1.2 version,
```sh
./configure --enable-sdl1 --without-x
```
to build with SDL 1.2 only (no *X11* and no *PasteManager*),
```
./configure --enable-zbx
```
to build with the integrated Z80 debugger zbx,
```sh
./configure --enable-zbx --enable-readline
```
to enable `readline` support for the zbx debugger.

Start build of the program in the main directory by executing:
```sh
make
```

---

To build with CMake:
--------------------

From the main directory, execute:
```sh
mkdir build
cd build
cmake ..
cmake --build .
```

---

To build with Makefiles:
------------------------

From the `src` directory, execute:
```sh
make sdl2
```
to build the SDL2 version,

```sh
make sdl
```
to build the SDL 1.2 version,

```sh
make nox
```
to build with SDL 1.2 only (no X11: **SDLTRS** will be build without
the *PasteManager* but works on systems with no *X11*-server like
*Haiku* or *BeOS*),

```sh
make bsd
```
(or just `make` on *FreeBSD*/*OpenBSD*) to build on BSD with SDL 1.2.

For *Win32/64* please copy the header files of the SDL library to
`\MinGW\include\SDL2` (or `\MinGW\include\SDL` for SDL 1.2), and
libraries to the `\MinGW\lib\` directory or edit the macros `LIBS` and
`INCS` in `Makefile` to point to the location of the SDL installation:
```sh
mingw32-make wsdl2
```
to build the SDL2 version (`sdl2trs.exe`), or
```sh
mingw32-make win32
```
for the SDL 1.2 version (`sdltrs.exe`).

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
./autogen.sh
./configure --enable-readline
make
```
This will build the executable binary file called `sdltrs`.

[Homebrew]: https://brew.sh
[MinGW]: http://www.mingw.org
[MinGW-w64]: http://mingw-w64.org
[MSYS2]: https://www.msys2.org
[SDL]: https://www.libsdl.org
