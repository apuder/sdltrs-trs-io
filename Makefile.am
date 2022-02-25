AUTOMAKE_OPTIONS=foreign

AM_CFLAGS=	-Wall -DPB_FIELD_16BIT -ITRS-IO/src/esp/components/retrostore/include -ITRS-IO/src/esp/components/trs-io/include -ITRS-IO/src/esp/components/tcpip/include -ITRS-IO/src/esp/components/frehd/include -ITRS-IO/src/esp/components/trs-fs/include -Imisc
AM_CXXFLAGS=	-Wall -DPB_FIELD_16BIT -ITRS-IO/src/esp/components/retrostore/include -ITRS-IO/src/esp/components/trs-io/include -ITRS-IO/src/esp/components/tcpip/include -ITRS-IO/src/esp/components/frehd/include -ITRS-IO/src/esp/components/trs-fs/include -Imisc

bin_PROGRAMS=	sdltrs
dist_man_MANS=	src/sdltrs.1

sdltrs_SOURCES=	src/blit.c \
		src/debug.c \
		src/dis.c \
		src/error.c \
		src/load_cmd.c \
		src/load_hex.c \
		src/main.c \
		src/trs_cassette.c \
		src/trs_disk.c \
		src/trs_hard.c \
		src/trs_imp_exp.c \
		src/trs_interrupt.c \
		src/trs_io.c \
		src/trs_memory.c \
		src/trs_mkdisk.c \
		src/trs_printer.c \
		src/trs_sdl_gui.c \
		src/trs_sdl_interface.c \
		src/trs_sdl_keyboard.c \
		src/trs_state_save.c \
		src/trs_stringy.c \
		src/trs_uart.c \
		src/z80.c \
		src/PasteManager.c \
		TRS-IO/src/esp/components/trs-io/trs-io.cpp \
		TRS-IO/src/esp/components/trs-io/core.cpp \
		TRS-IO/src/esp/components/trs-fs/trs-fs.cpp \
		TRS-IO/src/esp/components/retrostore/utils.cpp \
		TRS-IO/src/esp/components/retrostore/pb_common.cpp \
		TRS-IO/src/esp/components/retrostore/ApiProtos.pb.cpp \
		TRS-IO/src/esp/components/retrostore/pb_decode.cpp \
		TRS-IO/src/esp/components/retrostore/backend.cpp \
		TRS-IO/src/esp/components/retrostore/retrostore.cpp \
		TRS-IO/src/esp/components/retrostore/esp_mock.cpp \
		TRS-IO/src/esp/components/retrostore/cJSON.cpp \
		gen/loader_basic.c \
		gen/loader_cmd.c \
		gen/rsclient.c \
		TRS-IO/src/esp/components/tcpip/tcpip.cpp \
		TRS-IO/src/esp/components/frehd/trs_hard.c \
		TRS-IO/src/esp/components/frehd/trs_extra.c \
		TRS-IO/src/esp/components/frehd/frehd.c \
		TRS-IO/src/esp/components/frehd/io.c \
		TRS-IO/src/esp/components/frehd/dsk.c \
		misc/fileio.cpp \
		misc/trsio-wrapper.cpp

appicondir=	$(datadir)/icons/hicolor/scalable/apps
appicon_DATA=	icons/sdltrs.svg

diskimagedir=	$(datadir)/@PACKAGE@
diskimage_DATA= diskimages/*.dsk

desktopdir=	$(datadir)/applications
desktop_DATA=	sdltrs.desktop

docdir=		$(datadir)/doc/@PACKAGE@
nobase_doc_DATA=html/*.html html/images/*.png

icondir=	$(datadir)/icons/hicolor/48x48/apps
icon_DATA=	icons/sdltrs.png

pixmapdir=	$(datadir)/pixmaps
pixmap_DATA=	icons/sdltrs.xpm
