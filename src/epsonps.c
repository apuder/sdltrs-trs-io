/*
 *  epsonps.c
 *  version 1.00   19910507
 *
 *  IBM Graphics printer to postscript translator.
 *
 *	Copyright (c) 1988, Jonathan Greenblatt,
 *		<jonnyg@rover.umd.edu> (128.8.2.73)
 *		<jonnyg@umd5.umd.edu> (128.8.10.5)
 *		<pcproj@gymble.umd.edu> (128.8.128.16)
 *      This program may be redistributed in source form,
 *      provided no fee is charged and this copyright notice is preserved.
 *
 *  DESCRIPTION:
 *	This program converts epson printer codes from an input file to
 *	PostScript on standard output.  Unknown, ignored or invalid epson
 *	codes are output to standard error output.
 *	This program is an excellent ASCII listing printer.
 *
 *  SUPPORTS:
 *	All Epson LX-800 and Star NL-10 codes except
 * 		1: Download characters <ESC> %, <ESC> &
 *		2: Alignment <ESC> a
 *		3: Proportional printing <ESC> p
 *	Most Epson LQ-800 printer codes.
 *
 *  CONTRIBUTIONS:
 *
 *	The Following was contributed by:
 *		 Mark Alexander <uunet!amdahl!drivax!alexande@umd5.UMD.EDU>
 *
 *  - Added 1/72 line-spacing support (ESC-A).  This required changing
 *    the scaling in the Y dimension to 792, so that we have 72 points
 *    per inch in Y, instead of 36.
 *  - Changed the BNDY definition to compute page breaks correctly
 *    when we're right at the page break.
 *  - If variable line spacing is being used to slew past the end
 *    of page, don't reset cy to the top of page; instead, set it
 *    to top of page + n, where n is the position it would have
 *    been on continuous paper in an Epson.
 *  - Allowed spaces to be underlined.
 *  - Added support for emphasized mode.  It's treated exactly
 *    the same way as doublestrike.
 *  - Added support for italics.  Italics-bold hasn't been tested, though.
 *  - Print unknown ESC codes on standard error.
 *  - Allow an optional input filename to specified on the command line.
 *  - Changed the indentation to make it more readable to me; you
 *    may not like my style at all (sorry).
 *
 *
 *  BUGS:
 *	1: Seems to timeout the apple laserwriter when I send large prinouts
 *	   through the tty line using no special line control.
 *	2: Underlining works, but the lines chop right through the descenders
 *	   on some lower case characters.  I haven't tried to fix this.
 *
 *  CONTRIBUTIONS:
 *
 *	The Following was contributed by:
 *		Russell Lang <rjl@monu1.cc.monash.edu.au>
 *		Graham Holmes <eln127v@monu1.cc.monash.edu.au>
 *		19910507
 *  - Compiles with Turbo C 2.0, Turbo C++ 1.01, MSC 5.10, cc.
 *  - Conforms with PostScript document structuring conventions.
 *  - Input file is now binary, input filename is now required.
 *  - Added output file.
 *  - Added many many "Ignoring ..." warnings.
 *  - Added paper type selection - default is A4  (80 chars * 70 lines).
 *    11" paper type not tested - all laserprinters here are A4!
 *  - Revised font selection.  Most font combinations are valid.
 *  - Corrected horizontal tab spacings.
 *  - Added graphic escapes.
 *  - Added margins, vertical tabs, macro, reset printer.
 *  - Added elite, condensed and enlarged modes.
 *  - Added input options 
 *  - Added 1/216" line spacing (and others).
 *  - Escape codes are a combination of Epson LX-800, Epson LQ-800, 
 *    Star NL-10 and Star NX-1000.  There are probably some incompatiblities.
 *  - Changed to single module source - set.c, set.h included in epsonps.c
 *  - Added IBM extended characters and international character sets.
 *  - Added Epson LQ-800 codes.
 *  - Added literal codes.
 *  - Added IBM screen dump mode.
 */

 /* prototypes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int debug = 1;		/* Print bad things to stderr */
static int auto_lf = 0;		/* add line feed after carriage return */
static int auto_cr = 0;		/* add carriage return after line feed */
static int paper_type = 0;	/* default paper type a4 = 0, 11" = 4 */
static int country_num = 0;	/* International character set */
static int expand_print = 1;	/* print characters in range 0x80-0x9f */
static int ibm_graphic = 1;	/* print IBM graphic characters in range 0x80-0xff */
static int lq_mode = 0;		/* 0 = epson lx mode, 1 = epson lq mode */
static int screendump_mode = 0;	/* IBM screen dump mode */

#define READBIN "rb"	/* binary read mode for fopen() */

#define INTS_IN_SET	16
typedef unsigned int char_set[INTS_IN_SET];

int epsonps(const char *inputfile, const char *outputfile);
static void new_page(void);
static void end_page(void);
static void set_font(void);
static void eject_page(void);
static void check_page(void);
static void clear_set(char_set);
static int in_set(char_set, unsigned char);
static void str_add_set(char_set, const char*);
static void init_sets(void);
static int get_pointx(int);
static int get_pointy(int);
static void init_tabs(void);
static void reset_printer(void);
static void pchar(unsigned char);
static void putline(void);
static void newline(void);
static void ignore(char*);
static void invalid(char*);
static void hexout(unsigned char);
static void debug_state(char*, unsigned char);
static void dochar(unsigned char);
static void init_printer(void);

/* Bounding Boxes */
#define A4_LLX 22
#define A4_LLY 16
#define A4_URX 574
#define A4_URY 826

/* Letter (11"*8.5") bounding box NOT TESTED 19901123 */
#define LTR_LLX 22
#define LTR_LLY 16
#define LTR_URX 590
#define LTR_URY 784

struct page_format {
	char *name;
	int llx,lly,urx,ury;	/* bounding box */
	int nlines;		/* lines per page */
	int nchars;		/* number of PICA characers across page */
	int rotate;		/* rotate output ? */
};

static struct page_format page_formats[] = {
  {"a4",    A4_LLX,A4_LLY,A4_URX,A4_URY,70,80,0},   /* 297mm x 210mm */
  {"a4-15w",A4_LLX,A4_LLY,A4_URX,A4_URY,66,132,1},  /* 11"*15" -> a4 rotated */
  {"a4-12", A4_LLX,A4_LLY,A4_URX,A4_URY,72,80,0},   /* 12"*8.5" -> a4 */
  {"a4-11", A4_LLX,A4_LLY,A4_URX,A4_URY,66,80,0},   /* 11"*8.5" -> a4 */
  {"letter", LTR_LLX,LTR_LLY,LTR_URX,LTR_URY,66,80,0}, /* 11"*8.5" */
  {"letter-15w",LTR_LLX,LTR_LLY,LTR_URX,LTR_URY,66,80,1}, /* 11"*15" -> 8.5"*11" */
  {(char *)NULL,0,0,0,0,0,0,0}
};

static FILE *infile = NULL;
static FILE *outfile = NULL;

static int left = 0;		/* left margin (in PICA characters) */
static int maxx, maxy;
static int xstart, xstop, ystart, ystop, page_ystop;
#define YSTART line_space;

static int cx, cy;
static int point_sizex, point_sizey, line_space;
static int dotposition;
static int page;
static int eight_bit = 0;
static int lcount;

#define	BNDX	(cx > (xstop - point_sizex))
#define	BNDY	(cy > ystop)

#define	NUM_TABS	28
static int tabs[NUM_TABS+1];
static int tabindex;
#define	NUM_VTABS	16
#define NUM_VCHANNELS	8
static int vtabs[NUM_VCHANNELS][NUM_VTABS+1];
static int vtabchannel = 0;
static int vsettabchannel = 0;

#define MACROMAX 32
static char macro[MACROMAX+1];
static int mindex;

/* character sets */
char_set printable_set; /* set of printable chararacters in normal font */
char_set needesc_set;	/* set of characters that need to be \ quoted */
char_set ibmgraph_set;	/* set of IBM graphic characters in ibmgraphfont */
char_set ibmextend_set; /* set of IBM extended characters in normal font */
static const char *printables =
" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890`,./\\[]=-!@#$%^&*()_+|{}:\"<>?~;'";
static const char *needesc = "()\\";
static const char *ibmextend1 =
	"\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217";
static const char *ibmextend2 =
	"\220\221\222\223\224\225\226\227\230\231\232\233\234\235\237";
static const char *ibmextend3 =
	"\240\241\242\243\244\245\246\247\250\255\256\257";
static const char *ibmgraph1 =
	"\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017";
static const char *ibmgraph2 =
	"\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037";
static const char *ibmgraph3 =
	"\177\236\251\252\253\254";
static const char *ibmgraph4 =
	"\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277";
static const char *ibmgraph5 =
	"\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317";
static const char *ibmgraph6 =
	"\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337";
static const char *ibmgraph7 = 
	"\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357";
static const char *ibmgraph8 = 
	"\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377";

static const char *font_name[] = {
	"normalfont","boldfont","italicfont","bolditalicfont","ibmgraphfont"};
static int font_type = 0;
typedef enum {NORMAL, IBMGRAPH, ITALIC} extend_modes;
extend_modes extend_mode = NORMAL;

#define MAX_COUNTRY 12
static const char *international[] = { 
	"USA","France","Germany","UnitedKingdom","DenmarkI","Sweden", 
	"Italy","SpainI","Japan","Norway","DenmarkII","SpainII",
	"LatinAmerica"};

/* width of fonts in characters per inch */
#define PICA 10
#define ELITE 12
#define FIFTEEN 15
/* height of font in characters per inch */
#define FONTHEIGHT 6

/* font variations */
static int underline = 0;
static int subscript =  0;
static int doublewidth = 0;
static int oneline_doublewidth = 0;
static int highlight = 0;
static int emphasize = 0;
static int italic = 0;
static int elite = 0;
static int fifteen = 0;
static int condensed = 0;
static int doubleheight = 0;
static int quadheight = 0;
static int nlq_mode = 0;

/* graphics */
static int gcount;
static int g_dpi;
static int g_dpi_table[] = {60,120,120,240,80,72,90,180,360};
static int g_mode;
static int g_mode_k = 0;
static int g_mode_l = 1;
static int g_mode_y = 2;
static int g_mode_z = 3;
static int gwidth;


static void
new_page(void)
{
	page++;
	cx = xstart;
	cy = ystart;
	fprintf(outfile,"%%%%Page: %d %d\n",page,page);
	fputs("/saveobj2 save def\n",outfile);
	set_font();
}

static void
end_page(void)
{
	fputs("showpage\nsaveobj2 restore\n",outfile);
}

static void
set_font(void)
{
	int	boldface = highlight || emphasize;

	font_type = 0;
	if (italic)
		font_type |= 2;
	if (boldface)
		font_type |= 1;
	if (extend_mode == ITALIC)
		font_type |= 2;
	if (extend_mode == IBMGRAPH)
		font_type = 4;

	point_sizey = get_pointy(FONTHEIGHT);

	if (elite) {
		point_sizex = get_pointx(ELITE);
		fputs("elite ",outfile);
	}
	else if (fifteen) {
		point_sizex = get_pointx(FIFTEEN);
		fputs("fifteen ",outfile);
	}
	else {
		point_sizex = get_pointx(PICA);
		fputs("pica ",outfile);
	}
	if (condensed) {
		point_sizex = point_sizex * 3 / 5;
		fputs("condensed ",outfile);
	}
	if (doublewidth || oneline_doublewidth) {
		point_sizex = point_sizex * 2;
		fputs("doublewidth ",outfile);
	}
	else if (doubleheight) {
		fputs("doubleheight ",outfile);
		point_sizex = point_sizex * 2;
		point_sizey = point_sizey * 2;
	}
	else if (quadheight) {
		fputs("quadheight ",outfile);
		point_sizex = point_sizex * 4;
		point_sizey = point_sizey * 4;
	}

	fprintf(outfile,"%s\n",font_name[font_type]);
}

static void
eject_page(void)
{
	end_page();
	new_page();
}

/* Check if cy has gone past a page boundary, and skip to a new
 * page if necessary.  Set the new y-position not to the top of
 * page, but the point it would have been had this been a real
 * Epson printer with continuous forms.
 */
static void
check_page(void)
{
	int oldcy;

	if (BNDY) {
		oldcy = cy;
		eject_page();
		cy = oldcy - ystop + ystart - YSTART;
	}
}

static void
clear_set(char_set set)
{
	int i;

	for (i = 0; i < INTS_IN_SET; i++) set[i] = 0;
}

static int
in_set(char_set set, unsigned char c)
{
	c &= 0xff;
	return ((set[c >> 4] & (1 << (c & 15))) != 0);
}

static void
str_add_set(char_set set, const char *s)
{
	unsigned char c;

	while (*s) {
		c = (unsigned char)*s++;
		c &= 0xff;
		set[c >> 4] |= 1 << (c & 15);
	}
}

static void
init_sets(void)
{
	clear_set(printable_set);
	clear_set(needesc_set);
	clear_set(ibmextend_set);
	clear_set(ibmgraph_set);
	str_add_set(printable_set,printables);
	str_add_set(needesc_set,needesc);
	str_add_set(ibmextend_set,ibmextend1);
	str_add_set(ibmextend_set,ibmextend2);
	str_add_set(ibmextend_set,ibmextend3);
	str_add_set(ibmgraph_set,ibmgraph1);
	str_add_set(ibmgraph_set,ibmgraph2);
	str_add_set(ibmgraph_set,ibmgraph3);
	str_add_set(ibmgraph_set,ibmgraph4);
	str_add_set(ibmgraph_set,ibmgraph5);
	str_add_set(ibmgraph_set,ibmgraph6);
	str_add_set(ibmgraph_set,ibmgraph7);
	str_add_set(ibmgraph_set,ibmgraph8);
}

static int
get_pointx(int char_per_inch)
{
	if (lq_mode)
		return(360/char_per_inch);
	return(240/char_per_inch);
}

static int
get_pointy(int char_per_inch)
{
	if (lq_mode)
		return(360/char_per_inch);
	return(216/char_per_inch);
}

static void
init_tabs(void)
{
	int i;

	for (i=0; i<NUM_TABS; i++)
		tabs[i] = (i+1) * 8;
	tabs[NUM_TABS] = -1;
	for (vtabchannel=0; vtabchannel<NUM_VCHANNELS; vtabchannel++)
		for (i=0; i<=NUM_VTABS; i++)
			vtabs[vtabchannel][i] = 0;
	vtabchannel = 0;
}

static void
reset_printer(void)
{
	end_page();

	/* margins */
	left = 0;
	xstart = left * get_pointx(PICA);
	xstop = maxx;
	ystart = YSTART;
	ystop = maxy;
	page_ystop = ystop;

	/* line spacing */
	line_space = get_pointy(FONTHEIGHT);

	/* current position */
	cx = xstart;
	cy = ystart;

	/* fonts */
	font_type = 0;
	underline = 0;
	subscript =  0;
	doublewidth = 0;
	oneline_doublewidth = 0;
	highlight = 0;
	emphasize = 0;
	italic = 0;
	elite = 0;
	fifteen = 0;
	condensed = 0;
	doubleheight = 0;
	quadheight = 0;

	/* graphics */
	g_mode_k = 0;
	g_mode_l = 1;
	g_mode_y = 2;
	g_mode_z = 3;

	/* tabs */
	init_tabs();

	/* other */
	eight_bit = 0;
	ibm_graphic = 1;
	expand_print = 1;
	country_num = 0;

	new_page();
	fprintf(outfile,"%s InternationalSet\n",international[country_num]);
}

#define	LINE_SIZE	256
static unsigned char pline[LINE_SIZE];
static char outline[LINE_SIZE+5];
static int ptr = 0;

static void
pchar(unsigned char c)
{
	if (eight_bit) {
		if (eight_bit == 1)
			c |= 0x80;
		else
			c &= 0x7f;
	}
	if (in_set(printable_set,c)) {
		if (extend_mode != NORMAL) {
			putline();
			extend_mode = NORMAL;
			set_font();
		}
		pline[ptr++] = c;
	}
	else if (ibm_graphic && in_set(ibmextend_set,c)) {
		if (extend_mode != NORMAL) {
			putline();
			extend_mode = NORMAL;
			set_font();
		}
		pline[ptr++] = c;
	}
	else if (ibm_graphic && in_set(ibmgraph_set,c)) {
		if (extend_mode != IBMGRAPH) {
			putline();
			extend_mode = IBMGRAPH;
			set_font();
		}
		pline[ptr++] = c;
	}
	else if (!ibm_graphic && in_set(printable_set,c&0x7f)) {
		if (extend_mode != ITALIC) {
			putline();
			extend_mode = ITALIC;
			set_font();
		}
		pline[ptr++] = c&0x7f;
	}
	else {
		if (debug)
		    fprintf(stderr,
		      "Character 0x%02x is not printable\n",((int)c)&0xff);
		if (extend_mode != NORMAL) {
			putline();
			extend_mode = NORMAL;
			set_font();
		}
		pline[ptr++] = ' ';
	}
}

static void
putline(void)
{
	unsigned char *s = pline;
	int p;
	int lx;

	check_page();

	/* Skip over leading spaces, unless underlining is enabled
	 * (we want to underline spaces)
	 */
	if (underline == 0) {
	    while (*s == ' ' && ptr > 0) {
		s++;
		ptr--;
		if (BNDX) {
			cx = xstart;
			newline();
		}
		cx += point_sizex;
	    }
	}

	while (ptr > 0) {
	    p = 0;
	    check_page();
	    if (BNDX) {
		cx = xstart;
		newline();
	    }
	    lx = cx;
	    while ((lx+point_sizex-1) < xstop && ptr > 0 && p < LINE_SIZE) {
		if (*s>0x7f || *s<0x20) {
			outline[p++] = '\\';
			(void)sprintf(&outline[p],"%03o",*s++);
			p+=3;
		}
		else {
			if (in_set(needesc_set,*s))
				outline[p++] = '\\';
			outline[p++] = *s++;
		}
		ptr--;
		lx += point_sizex;
	    }
	    outline[p] = 0;
	    if (p > 0) {
		int i = strlen(outline);

		/* Forward slash won't work at the end of a string */
		if (outline[i-1] == '/') {
			outline[i] = ' ';
			outline[i+1] = '\0';
		}
		if (subscript) {
		  if (subscript == 1) /* Subscript */
		    fprintf(outfile,"%d %d M gsave 1 .5 scale (%s) S grestore\n"
			,cx,cy,outline);
		  else   /* Superscript */
		    fprintf(outfile,"%d %d M gsave 1 .5 scale (%s) S grestore\n"
			,cx,(cy-(point_sizey>>1)),outline);
		}
		else /* Normal printing */
		    fprintf(outfile,"%d %d M (%s) S\n",cx,cy,outline);
		if (underline)
		    fprintf(outfile,"%d %d M %d %d L stroke\n",
			cx,cy+3,lx-1,cy+3);
	    }
	    cx = lx;
	}
}

static void
newline(void)
{
	cy += line_space;
	if (doubleheight)
		cy += get_pointy(FONTHEIGHT) * 2 / 3;
	else if (quadheight)
		cy += get_pointy(FONTHEIGHT) * 2;
	check_page();
}

static void
ignore(str)
char *str;
{
	if (debug)
		fprintf(stderr,"Ignoring %s sequence\n",str);
}

static void
invalid(str)
char *str;
{
	if (debug)
		fprintf(stderr,"Invalid %s sequence\n",str);
}

char hextable[] = {'0','1','2','3','4','5','6','7','8','9',
		'A','B','C','D','E','F'};

static void
hexout(unsigned char c)
{
	(void)fputc(hextable[ ((int)c>>4) & 0x0f ],outfile);
	(void)fputc(hextable[ (int)c & 0x0f ],outfile);
}

static void
debug_state(char *str, unsigned char c)
{
	if (debug>=2)
		fprintf(stderr,"%s: 0x%02x %c\n", str, c,
			(c>=32 && c<127) ? c : ' ');
}

typedef enum {S_BASE,S_ESC,S_UNDERLINE,S_SUBSCRIPT,S_DOUBLE,S_NLQMODE,
	S_LINESPACE,S_FINE_LINESPACE,S_LINEFEED,S_REV_LINEFEED,
	S_IGNORE1,S_IGNORE2,S_IGNORE3,S_MASTER,S_ENLARGE,S_DOUBLEHEIGHT,
	S_SETTAB,S_SETVTAB1,S_SETVTAB2,S_VTABCHANNEL,S_VSETTABCHANNEL,
	S_TABINC,S_HTABINC,S_VTABINC,S_SKIP,S_HSKIP,S_VSKIP,
	S_PAGELENGTH,S_PAGEINCH,S_BOTTOM,S_TOP,S_RIGHT,S_LEFT,
	S_ABSOLUTE1,S_ABSOLUTE2,S_RELATIVE1,S_RELATIVE2,
	S_MACRO,S_SETMACRO,S_IBM_MODE,S_INTERNATIONAL,
	S_LITERAL,S_LITERAL1,S_LITERAL2,S_LITERAL_COUNT,S_SCREENDUMP,
	S_GRAPHIC_MODE,S_GRAPHIC1,S_GRAPHIC2,S_GRAPHIC,
	S_DEF_GRAPHIC,S_DEF_K,S_DEF_L,S_DEF_Y,S_DEF_Z,
	S_GRAPHIC9_MODE,S_GRAPHIC9_1,S_GRAPHIC9_2,
	S_GRAPHIC9_TOP,S_GRAPHIC9_BOTTOM,S_GRAPHIC24_2, 
	S_GRAPHIC24_TOP,S_GRAPHIC24_MIDDLE,S_GRAPHIC24_BOTTOM} epson_states;

static epson_states state = S_BASE;

static void
dochar(unsigned char c)
{
	c &= 0xff;
	if (ptr >= LINE_SIZE) putline();
	switch (state)
	{
	case S_UNDERLINE:
		debug_state("S_UNDERLINE",c);
		state = S_BASE;
		if (c == '\0' || c == '0')	underline = 0;
		else if (c == '\1' || c == '1')	underline = 1;
		break;
	case S_DOUBLE:
		debug_state("S_DOUBLE",c);
		state = S_BASE;
		if (c == '\0' || c == '0')	doublewidth = 0;
		else if (c == '\1' || c == '1')	doublewidth = 1;
		set_font();
		break;
	case S_SUBSCRIPT:
		debug_state("S_SUBSCRIPT",c);
		state = S_BASE;
		if (c == '\0' || c == '0')	subscript = 2;
		else if (c == '\1' || c == '1')	subscript = 1;
		break;
	case S_NLQMODE:
		debug_state("S_NLQMODE",c);
		state = S_BASE;
		if (c == '\0' || c == '0')	nlq_mode = 0;
		else if (c == '\1' || c == '1')	nlq_mode = 1;
		break;
	case S_LINESPACE:
		debug_state("S_LINESPACE",c);
		state = S_BASE;
		if (lq_mode)
			line_space = c * get_pointy(60); /* c/60 " */
		else
			line_space = 3 * c;		 /* c/72 " */
		break;
	case S_FINE_LINESPACE:
		debug_state("S_FINE_LINESPACE",c);
		state = S_BASE;
		if (lq_mode)
			line_space = c * get_pointy(180); /* c/180 " */
		else
			line_space = c;			 /* c/216 " */
		break;
	case S_LINEFEED:
		debug_state("S_LINEFEED",c);
		state = S_BASE;
		if (lq_mode)
			cy += c * get_pointy(180);
		else
			cy += c;
		break;
	case S_REV_LINEFEED:
		debug_state("S_REV_LINEFEED",c);
		state = S_BASE;
		if (lq_mode)
			cy -= c * get_pointy(180);
		else
			cy -= c;
		break;
	case S_IGNORE1:
		debug_state("S_IGNORE1",c);
		state = S_BASE;
		break;
	case S_IGNORE2:
		debug_state("S_IGNORE2",c);
		state = S_IGNORE1;
		break;
	case S_IGNORE3:
		debug_state("S_IGNORE3",c);
		state = S_IGNORE2;
		break;
	case S_MASTER:
		debug_state("S_MASTER",c);
		state = S_BASE;
		elite = c & 0x01;
		/* ignore proportional c & 0x02 */
		condensed = c & 0x04;
		emphasize = c & 0x08;
		highlight = c & 0x10;
		doublewidth = c & 0x20;
		italic = c & 0x40;
		underline = c & 0x80;
		set_font();
		break;
	case S_ENLARGE:
		debug_state("S_ENLARGE",c);
		state = S_BASE;
		switch(c) {
			case 0:
				doubleheight = 0;
				quadheight = 0;
				set_font();
				break;
			case 1:
				doubleheight = 1;
				quadheight = 0;
				cy += get_pointy(FONTHEIGHT) * 2 / 3;
				set_font();
				break;
			case 2:
				doubleheight = 0;
				quadheight = 1;
				cy += get_pointy(FONTHEIGHT) * 2;
				set_font();
				break;
			default:
				ignore("<ESC> h n");
				break;
		}
		break;
	case S_DOUBLEHEIGHT:
		debug_state("S_DOUBLEHEIGHT",c);
		state = S_BASE;
		if (c == '\0' || c == '0')	doubleheight = 0;
		else if (c == '\1' || c == '1')	doubleheight = 1;
		set_font();
		break;
	case S_SETTAB:
		debug_state("S_SETTAB",c);
		if (c == 0) {
			state = S_BASE;
			for (; tabindex <= NUM_TABS; tabindex++)
				tabs[tabindex] = -1;
		}
		else {
			if ( tabindex < NUM_TABS )
				tabs[tabindex++] = c;
		}
		break;
	case S_VSETTABCHANNEL:
		debug_state("S_VSETTABCHANNEL",c);
		state = S_SETVTAB1;
		if (c > 7) {
			invalid("<ESC> b n");
			vsettabchannel = 0;
		}
		else
			vsettabchannel = c;
		break;
	case S_SETVTAB1:
		debug_state("S_SETVTAB1",c);
		if (c == 0) { /* no vtabs */
			state = S_BASE;
			vtabs[vsettabchannel][tabindex] = 0;
		}
		else {
			state = S_SETVTAB2;
			vtabs[vsettabchannel][tabindex++] = c;
		}
		break;
	case S_SETVTAB2:
		debug_state("S_SETVTAB2",c);
		if (c == 0) {
			state = S_BASE;
			for (; tabindex <= NUM_VTABS; tabindex++)
				vtabs[vsettabchannel][tabindex] = -1;
		}
		else {
			if ( tabindex < NUM_VTABS )
				vtabs[vsettabchannel][tabindex++] = c;
		}
		break;
	case S_VTABCHANNEL:
		debug_state("S_VTABCHANNEL",c);
		state = S_BASE;
		if (c > 7)
			invalid("<ESC> / n");
		else
			vtabchannel = c;
		break;
	case S_TABINC:
		debug_state("S_TABINC",c);
		if (c == '\0' || c == '0')
			state = S_HTABINC;
		else if (c == '\1' || c == '1')	
			state = S_VTABINC;
		else {
			state = S_IGNORE1;
			invalid("<ESC> e n");
		}
	case S_HTABINC:
		debug_state("S_HTABINC",c);
		state = S_BASE;
		{
			int i;

			for (i=0; i<NUM_TABS; i++)
				tabs[i] = (i+1) * c;
			tabs[NUM_TABS] = -1;
		}
		break;
	case S_VTABINC:
		debug_state("S_VTABINC",c);
		state = S_BASE;
		{
			int i;

			for (i=0; i<NUM_VTABS; i++)
				vtabs[vtabchannel][i] = (i+1) * c;
			vtabs[vtabchannel][NUM_VTABS] = -1;
		}
		break;
	case S_SKIP:
		debug_state("S_SKIP",c);
		if (c == '\0' || c == '0')
			state = S_HSKIP;
		else if (c == '\1' || c == '1')	
			state = S_VSKIP;
		else {
			state = S_IGNORE1;
			invalid("<ESC> f n");
		}
	case S_HSKIP:
		debug_state("S_HSKIP",c);
		state = S_BASE;
		{
			int i;

			for (i=0; i<c; i++)
				dochar(' ');
		}
		break;
	case S_VSKIP:
		debug_state("S_VSKIP",c);
		state = S_BASE;
		{
			int i;

			for (i=0; i<c; i++)
				newline();
		}
		break;
	case S_PAGELENGTH:
		debug_state("S_PAGELENGTH",c);
		if (c !=0) {
			state = S_BASE;
			ystop = c * line_space;
			page_ystop = ystop;
		}
		else {
			state = S_PAGEINCH;
		}
		break;
	case S_PAGEINCH:
		debug_state("S_PAGEINCH",c);
		state = S_BASE;
		ystop = c * get_pointy(1);
		page_ystop = ystop;
		break;
	case S_BOTTOM:
		debug_state("S_BOTTOM",c);
		state = S_BASE;
		{
			int temp = page_ystop - c * line_space;

			if (ystart + line_space < temp)
				ystop = temp;
			else
				invalid("<ESC> N n");
		}
		break;
	case S_TOP:
		debug_state("S_TOP",c);
		state = S_BASE;
		ystart = c * line_space + YSTART ;
		{
			int temp = (c-1) * line_space + YSTART ;

			if (temp + line_space < ystop)
				ystart = temp;
			else
				invalid("<ESC> r n or <ESC> c n");
		}
		break;
	case S_RIGHT:
		debug_state("S_RIGHT",c);
		state = S_BASE;
		{
			int temp =  c * point_sizex;

			if (xstart + point_sizex < temp)
				xstop = temp;
			else
				invalid("<ESC> Q n");
		}
		break;
	case S_LEFT:
		debug_state("S_LEFT",c);
		state = S_BASE;
		{
			int temp = c * point_sizex;

			if (temp + point_sizex < xstop)
				xstart = temp;
			else
				invalid("<ESC> l n");
		}
		cx = xstart;
		break;
	case S_ABSOLUTE1:
		debug_state("S_ABSOLUTE1",c);
		state = S_ABSOLUTE2;
		dotposition = c;
		break;
	case S_ABSOLUTE2:
		debug_state("S_ABSOLUTE2",c);
		state = S_BASE;
		dotposition = (c*256 + dotposition) * get_pointx(60);
		if (dotposition < xstop)
			cx = dotposition;
		break;
	case S_RELATIVE1:
		debug_state("S_RELATIVE1",c);
		state = S_RELATIVE2;
		dotposition = c;
		break;
	case S_RELATIVE2:
		debug_state("S_RELATIVE2",c);
		state = S_BASE;
		dotposition = c*256 + dotposition;
		if ((unsigned int)dotposition > (unsigned int)32767) {
			dotposition = (int)(dotposition - 65536L);
		}
		if (lq_mode) {
			if (nlq_mode)
				dotposition = dotposition * get_pointx(180);
			else
				dotposition = dotposition * get_pointx(120);
			if (cx + dotposition < xstop)
				cx += dotposition;
		}
		else
			invalid("<ESC> \\");
		break;
	case S_MACRO:
		debug_state("S_MACRO",c);
		if (c == 1) {
			state = S_BASE;
			for (mindex=0; macro[mindex]!=30; mindex++)
				dochar((unsigned char)macro[mindex]);
			break;
		}
		else {
			state = S_SETMACRO;
			mindex = 0;
		}
		/* no break */
	case S_SETMACRO:
		debug_state("S_SETMACRO",c);
		if (mindex<MACROMAX)
			macro[mindex++] = c;
		if (c == 30) {
			macro[mindex] = c;
			state = S_BASE;
		}
		break;
	case S_IBM_MODE:
		debug_state("S_IBM_MODE",c);
		state = S_BASE;
		if (c == '\0' || c == '0')	ibm_graphic = 0;
		else if (c == '\1' || c == '1')	ibm_graphic = 1;
		else invalid("<ESC> t n");
		break;
	case S_INTERNATIONAL:
		debug_state("S_INTERNATIONAL",c);
		state = S_BASE;
		if (c > MAX_COUNTRY)
			invalid("<ESC> R n");
		else {
			country_num = c;
			fprintf(outfile,"%s InternationalSet\n",international[country_num]);
		}
		break;
	case S_LITERAL:
		debug_state("S_LITERAL",c);
		state = S_BASE;
		pchar(c);
		break;
	case S_LITERAL1:
		debug_state("S_LITERAL1",c);
		state = S_LITERAL2;
		lcount = c;
		break;
	case S_LITERAL2:
		debug_state("S_LITERAL2",c);
		state = S_LITERAL_COUNT;
		lcount = 256 * c + lcount;
		if (lcount == 0)
			state = S_BASE;
		break;
	case S_LITERAL_COUNT:
		debug_state("S_LITERAL_COUNT",c);
		pchar(c);
		lcount--;
		if (lcount == 0) {
			state = S_BASE;
		}
		break;
	case S_SCREENDUMP:
		debug_state("S_SCREENDUMP",c);
		/* recognise only \r and \n */
		switch(c) {
		case '\n':
			putline();
			newline();
			if (auto_cr)
				cx = xstart;
			break;
		case '\r':
			putline();
			cx = xstart;
			if (auto_lf)
				newline();
			break;
		default:
			pchar(c);
			break;
		}
		break;
	case S_GRAPHIC_MODE:
		debug_state("S_GRAPHIC_MODE",c);
		state = S_GRAPHIC1;
		if ( (c <= 6) || (c>=32 && c<=40) )
			g_mode = c;
		else
			invalid("<ESC> *");
		break;
	case S_GRAPHIC1:
		debug_state("S_GRAPHIC1",c);
		state = S_GRAPHIC2;
		gcount = c;
		if (g_mode >= 32) {
			g_dpi = g_dpi_table[g_mode-32];
			state = S_GRAPHIC24_2;
		}
		else {
			g_dpi = g_dpi_table[g_mode];
		}
		break;
	case S_GRAPHIC2:
		debug_state("S_GRAPHIC2",c);
		state = S_GRAPHIC;
		gcount = 256 * c + gcount ;
		if (gcount == 0) {
			state = S_BASE;
			break;
		}
		gwidth = (int)( (long)get_pointx(1)*gcount/g_dpi);
		if (lq_mode) {
		    fprintf(outfile,"gsave\n%d %d translate\n",cx,cy+6*get_pointy(180));
		    fprintf(outfile,"%d %d scale\n", get_pointy(180)*24, gwidth);
		}
		else {
		    fprintf(outfile,"gsave\n%d %d translate\n",cx,cy+get_pointy(72));
		    fprintf(outfile,"%d %d scale\n", get_pointy(72)*8, gwidth);
		}
		fprintf(outfile,"8 %d true [0 8 %d 0 0 8]", gcount, gcount);
		fprintf(outfile," {currentfile graphicbuf readhexstring pop} \nimagemask\n");
		cx += gwidth;
		break;
	case S_GRAPHIC:
		debug_state("S_GRAPHIC",c);
		hexout(c);
		gcount--;
		if ( (gcount & 0x1f) == 0 )
			(void)fputc('\n',outfile);
		if (gcount == 0) {
			fputs("grestore\n",outfile);
			state = S_BASE;
		}
		break;
	case S_DEF_GRAPHIC:
		debug_state("S_DEF_GRAPHIC",c);
		switch(c)
		{
		case 'K':
			state = S_DEF_K;
			break;
		case 'L':
			state = S_DEF_L;
			break;
		case 'Y':
			state = S_DEF_Y;
			break;
		case 'Z':
			state = S_DEF_Z;
			break;
		default:
			state = S_IGNORE1;
			invalid("ESC ? n0 n1");
			break;
		}
		break;
	case S_DEF_K:
		debug_state("S_DEF_K",c);
		state = S_BASE;
		if ( (c <= 6) || (c>=32 && c<=40) )
			g_mode_k = c;
		else
			invalid("<ESC> ? K n");
		break;
	case S_DEF_L:
		debug_state("S_DEF_L",c);
		state = S_BASE;
		if ( (c <= 6) || (c>=32 && c<=40) )
			g_mode_l = c;
		else
			invalid("<ESC> ? L n");
		break;
	case S_DEF_Y:
		debug_state("S_DEF_Y",c);
		state = S_BASE;
		if ( (c <= 6) || (c>=32 && c<=40) )
			g_mode_y = c;
		else
			invalid("<ESC> ? Y n");
		break;
	case S_DEF_Z:
		debug_state("S_DEF_Z",c);
		state = S_BASE;
		if ( (c<= 6) || (c>=32 && c<=40) )
			g_mode_z = c;
		else
			invalid("<ESC> ? Z n");
		break;
	case S_GRAPHIC9_MODE:
		debug_state("S_GRAPHIC9_MODE",c);
		state = S_GRAPHIC9_1;
		if (c > 6) {
			invalid("<ESC> ^");
			g_dpi = g_dpi_table[0];
		}
		else {
			g_dpi = g_dpi_table[c];
			g_mode = c;
		}
		break;
	case S_GRAPHIC9_1:
		debug_state("S_GRAPHIC9_1",c);
		state = S_GRAPHIC9_2;
		gcount = c;
		break;
	case S_GRAPHIC9_2:
		debug_state("S_GRAPHIC9_2",c);
		state = S_GRAPHIC9_TOP;
		gcount = 256 * c + gcount ;
		if (gcount == 0) {
			state = S_BASE;
			break;
		}
		gwidth = (int)( (long)get_pointx(1)*gcount/g_dpi);
		if (lq_mode) {
		    invalid("<ESC> ^");
		    fprintf(outfile,"gsave\n%d %d translate\n",cx,cy+9*get_pointy(60));
		    fprintf(outfile,"%d %d scale\n", get_pointy(60)*16, gwidth);
		}
		else {
		    fprintf(outfile,"gsave\n%d %d translate\n",cx,cy+9*get_pointy(72));
		    fprintf(outfile,"%d %d scale\n", get_pointy(72)*16, gwidth);
		}
		fprintf(outfile,"16 %d true [0 16 %d 0 0 16]", gcount, gcount);
		fprintf(outfile," {currentfile graphicbuf readhexstring pop} \nimagemask\n");
		cx += gwidth;
		break;
	case S_GRAPHIC9_TOP:
		debug_state("S_GRAPHIC9_TOP",c);
		state = S_GRAPHIC9_BOTTOM;
		hexout(c);
		break;
	case S_GRAPHIC9_BOTTOM:
		debug_state("S_GRAPHIC9_BOTTOM",c);
		state = S_GRAPHIC9_TOP;
		c &= 0x80;
		hexout(c);
		gcount--;
		if ( (gcount & 0x0f) == 0 )
			(void)fputc('\n',outfile);
		if (gcount == 0) {
			fputs("grestore\n",outfile);
			state = S_BASE;
		}
		break;
	case S_GRAPHIC24_2:
		debug_state("S_GRAPHIC24_2",c);
		state = S_GRAPHIC24_TOP;
		gcount = 256 * c + gcount ;
		if (gcount == 0) {
			state = S_BASE;
			break;
		}
		gwidth = (int)( (long)get_pointx(1)*gcount/g_dpi);
		if (lq_mode) {
		    fprintf(outfile,"gsave\n%d %d translate\n",cx,cy+6*get_pointy(180));
		    fprintf(outfile,"%d %d scale\n", get_pointy(180)*24, gwidth);
		}
		else {
		    fprintf(outfile,"gsave\n%d %d translate\n",cx,cy+get_pointy(72));
		    fprintf(outfile,"%d %d scale\n", get_pointy(72)*8, gwidth);
		}
		fprintf(outfile,"24 %d true [0 24 %d 0 0 24]", gcount, gcount);
		fprintf(outfile," {currentfile graphicbuf readhexstring pop} \nimagemask\n");
		cx += gwidth;
		break;
	case S_GRAPHIC24_TOP:
		debug_state("S_GRAPHIC24_TOP",c);
		state = S_GRAPHIC24_MIDDLE;
		hexout(c);
		break;
	case S_GRAPHIC24_MIDDLE:
		debug_state("S_GRAPHIC24_MIDDLE",c);
		state = S_GRAPHIC24_BOTTOM;
		hexout(c);
		break;
	case S_GRAPHIC24_BOTTOM:
		debug_state("S_GRAPHIC24_BOTTOM",c);
		state = S_GRAPHIC24_TOP;
		hexout(c);
		gcount--;
		if ( (gcount & 0x07) == 0 )
			(void)fputc('\n',outfile);
		if (gcount == 0) {
			fputs("grestore\n",outfile);
			state = S_BASE;
		}
		break;
	case S_ESC:
		debug_state("S_ESC",c);
		state = S_BASE;
		switch (c)
		{
		case '\n':	/* reverse line feed - Star NL-10 */
			cy -= line_space;
			break;
		case '\f':	/* reverse form feed - Star NL-10 */
			cy = ystart;
			break;
		case 14:	/* 0x0e - select one line expanded */
			oneline_doublewidth = 1;
			set_font();
			break;
		case 15:	/* select condensed */
			condensed = 1;
			set_font();
			break;
		case 25:	/* paper controls */
			ignore("<ESC> <EM> n");
			state = S_IGNORE1;
			break;
		case '!':	/* master print mode */
			state = S_MASTER;
			break;
		case '#':	/* accept eight bit */
			eight_bit = 0;
			break;
		case '$':	/* absolute dot position - LQ-800 */
			state = S_ABSOLUTE1;
			break;
		case '%':	/* select download character set */
			ignore("<ESC> % n");
			state = S_IGNORE1;
			break;
		case '*':	/* select graphic mode */
			state = S_GRAPHIC_MODE;
			break;
		case '+':	/* macro - Star NL-10 */
			state = S_MACRO;
			break;
		case '-':
			state = S_UNDERLINE;
			break;
		case '/':	/* select vertical tab channel */
			state = S_VTABCHANNEL;
			break;
		case '0':	/* set line spacing to 1/8 inch */
			line_space = get_pointy(8);
			break;
		case '1':	/* set line spacing to 7/72 inch */
			line_space = 7 * get_pointy(72);
			break;
		case '2':	/* set line spacing to 1/6 inch */
			line_space = get_pointy(6);
			break;
		case '3':	/* set line spacing to n/216 or n/180 inch */
			state = S_FINE_LINESPACE;
			break;
		case '4':
			italic = 1;
			set_font();
			break;
		case '5':
			italic = 0;
			set_font();
			break;
		case '6':	/* expand printable area */
			expand_print = 1;
			break;
		case '7':	/* cancels expand printable area */
			expand_print = 0;
			break;
		case '8':	/* disable paper-out detector */
			ignore("<ESC> 8");
			break;
		case '9':	/* enable paper-out detector */
			ignore("<ESC> 9");
			break;
		case ':':	/* copy ROM characters to user defined */
			state = S_IGNORE3;
			ignore("<ESC> :");
			break;
		case '<':
			if (lq_mode)
				cx = 0; /* print head to home */
			else
				/* select one line uni-directional printing */
				ignore("<ESC> <");
			break;
		case '=':	/* set eighth bit to 0 */
			eight_bit = -1;
			break;
		case '>':	/* set eighth bit to 1 */
			eight_bit = 1;
			break;
		case '?':
			state = S_DEF_GRAPHIC;
			break;
		case '@':
			reset_printer();
			break;
		case 'A':
			state = S_LINESPACE;
			break;
		case 'B':
			state = S_SETVTAB1;
			vsettabchannel = 0;
			tabindex = 0;
			break;
		case 'C':
			state = S_PAGELENGTH;
			break;
		case 'D':
			state = S_SETTAB;
			tabindex = 0;
			break;
		case 'E':
			emphasize = 1;
			set_font();
			break;
		case 'F':
			emphasize = 0;
			set_font();
			break;
		case 'G':
			highlight = 1;
			set_font();
			break;
		case 'H':
			highlight = 0;
			set_font();
			break;
		case 'I':  /* allow use of characters 0x80-0x9f - Star NL-10 */
			state = S_IGNORE1;
			ignore("<ESC> I n");
			break;
		case 'J':
			state = S_LINEFEED;
			break;
		case 'K':
			state = S_GRAPHIC1;
			g_mode = g_mode_k;
			break;
		case 'L':
			state = S_GRAPHIC1;
			g_mode = g_mode_l;
			break;
		case 'M':
			elite = 1;
			fifteen = 0;
			set_font();
			break;
		case 'N':
			state = S_BOTTOM;
			break;
		case 'O':
			ystart = YSTART;
			ystop = page_ystop;
			break;
		case 'P':
			elite = 0;
			fifteen = 0;
			set_font();
			break;
		case 'Q':
			state = S_RIGHT;
			break;
		case 'R':	/* international character set */
			state = S_INTERNATIONAL;
			break;
		case 'S':
			state = S_SUBSCRIPT;
			break;
		case 'T':
			subscript = 0;
			break;
		case 'U':	/* unidirectional printint */
			state = S_IGNORE1;
			ignore("<ESC> U n");
			break;
		case 'W':
			state = S_DOUBLE;
			break;
		case 'Y':
			state = S_GRAPHIC1;
			g_mode = g_mode_y;
			break;
		case 'Z':
			state = S_GRAPHIC1;
			g_mode = g_mode_z;
			break;
		case '\\':
			if (lq_mode) 
				/* relative dot position - LQ-800 */
				state = S_RELATIVE1;
			else
				if (ibm_graphic)
					state = S_LITERAL1;
				else
					state = S_RELATIVE1;
			break;
		case '^':
			if (ibm_graphic)
				state = S_LITERAL;    /* literal character */
			else
				state = S_GRAPHIC9_MODE; /* 9 pin graphics */
			break;
		case 'a':	/* justification */
			state = S_IGNORE1;
			ignore("<ESC> a n");
			break;
		case 'b':
			state = S_VSETTABCHANNEL;
			tabindex = 0;
			break;
		case 'c':	/* top margin - Star NX-1000 */
			state = S_TOP;
			break;
		case 'e':
			state = S_TABINC;
			break;
		case 'f':
			state = S_SKIP;
			break;
		case 'g':	/* 15-pitch characters - LQ-800 */
			elite = 0;
			fifteen = 1;
			set_font();
			break;
		case 'h':	/* enlarge characters - Star NL-10 */
			state = S_ENLARGE;
			break;
		case 'i':	/* immediate print mode - Star NL-10 */
			state = S_IGNORE1;
			ignore("<ESC> i n");
			break;
		case 'j':	/* reverse line feed - Star NL-10 */
			state = S_REV_LINEFEED;
			break;
		case 'k':	/* select font family */
			state = S_IGNORE1;
			ignore("<ESC> k n");
			break;
		case 'l':
			state = S_LEFT;
			break;
		case 'm':	/* select special graphics */
			state = S_IGNORE1;
			ignore("<ESC> m n");
			break;
		case 'p':	/* proportional print - Star NL-10 */
			state = S_IGNORE1;
			ignore("<ESC> p n");
			break;
		case 'r':	/* top margin - Star NL-10 */
			state = S_TOP;
			break;
		case 's':	/* half speed */
			state = S_IGNORE1;
			ignore("<ESC> s n");
			break;
		case 't':	/* italic / graphics & international */
			state = S_IBM_MODE;
			break;
		case 'w':	/* double height characters - Star NX-1000 */
			state = S_DOUBLEHEIGHT;
			break;
		case 'x':	/* NLQ mode */
			state = S_NLQMODE;
			break;
		case '~':	/* slash zero - Star NL-10 */
			state = S_IGNORE1;
			ignore("<ESC> ~ n");
			break;
		default:
			if (debug)
			    fprintf(stderr,"Unknown sequence: <ESC> 0x%x\n",c);
			break;
		}
		break;
	case S_BASE:
		if (debug>=3)
			debug_state("S_BASE",c);
		if (!expand_print && c>=0x80 && c<=0x9f)
			c &= 0x7f;
		switch(c)
		{
		case 7:		/* bell (on stderr) */
			if (debug)
				(void)fputc((char)c,stderr);
			break;
		case '\b':	/* back space */
			putline();
			cx -= point_sizex;
			break;
		case '\t':
			{
				int i,l;

				l = cx / point_sizex + ptr;
				for (i = 0; i < NUM_TABS && tabs[i] <= l; i++)
					;
				if (tabs[i] == -1) break;
				i = tabs[i] - l;
				while (i-- > 0) dochar(' ')
					;
			}
			break;
		case '\n': 
			putline();
			newline();
			if (auto_cr) {
				cx = xstart;
				if (oneline_doublewidth) {
					oneline_doublewidth = 0;
					set_font();
				}
			}
			break;
		case 11:	/* vertical tab */
			putline();
			if (vtabs[vtabchannel][0] == 0)
				newline();
			else {
				int i,l;

				l = cy / line_space;
				for (i = 0; i < NUM_VTABS &&
					vtabs[vtabchannel][i] <= l; i++) ;
				if (vtabs[vtabchannel][i] == -1) {
				    eject_page();
				}
				else {
				    cy = vtabs[vtabchannel][i] * line_space
						+ YSTART;
				}
			}
			break;
		case '\f':
			putline();
			eject_page();
			break;
		case '\r':
			putline();
			cx = xstart;
			if (oneline_doublewidth) {
				oneline_doublewidth = 0;
				set_font();
			}
			if (auto_lf)
				newline();
			break;
		case 14:	/* 0x0e - select one line expanded */
			putline();
			oneline_doublewidth = 1;
			set_font();
			break;
		case 15:	/* 0x0f - select condensed */
			putline();
			condensed = 1;
			set_font();
			break;
		case 17:	/* 0x11 - set printer off line */
			ignore("<DC1>");
			break;
		case 18:	/* 0x12 - cancel condensed */
			putline();
			condensed = 0;
			set_font();
			break;
		case 19:	/* 0x11 - set printer on line */
			ignore("<DC3>");
			break;
		case 20:	/* 0x14 - cancel one line expanded */
			putline();
			oneline_doublewidth = 0;
			set_font();
			break;
		case 24:	/* 0x18 - cancel line */
			ignore("<CAN>");
			break;
		case 26:	/* CTRL-Z */
			break;
		case '\033':
			putline();
			state = S_ESC;
			break;
		case 127:	/* 0x7f - delete char */
			ignore("<DEL>");
			break;
		default:	/* print character */
			pchar(c);
			break;
		}
		break;
	}
}

static void
init_printer(void)
{
	maxx = page_formats[paper_type].nchars * get_pointx(PICA);
	maxy = page_formats[paper_type].nlines * get_pointy(FONTHEIGHT);
	line_space = get_pointy(FONTHEIGHT);

	if (left > page_formats[paper_type].nchars) {
		fprintf(stderr,"Left margin greater than right margin\n");
		return;
	}
	xstart = left * get_pointx(PICA);
	xstop = maxx;
	ystart = YSTART;
	ystop = maxy;
	page_ystop = ystop;

	fputs("%!PS-Adobe-2.0\n",outfile);
	fprintf(outfile,"%%%%BoundingBox: 0 0 %d %d\n",
		page_formats[paper_type].urx, page_formats[paper_type].ury);
	fputs("%%Creator: epsonps.c\n",outfile);
	fputs("%%Pages: (atend)\n",outfile);
	fputs("%%EndComments\n",outfile);
	fputs("/saveobj1 save def\n",outfile);
	if (page_formats[paper_type].rotate) {
	    fputs("90 rotate\n",outfile);
	    fprintf(outfile,"%d %d translate\n",page_formats[paper_type].lly,
	      -page_formats[paper_type].llx);
	    fprintf(outfile,"%d %d div %d %d div scale\n",
	      page_formats[paper_type].ury-page_formats[paper_type].lly, maxx,
	      page_formats[paper_type].llx-page_formats[paper_type].urx, maxy);
	}
	else {
	    fprintf(outfile,"%d %d translate\n",page_formats[paper_type].llx,
	      page_formats[paper_type].ury);
	    fprintf(outfile,"%d %d div %d %d div scale\n",
	      page_formats[paper_type].urx-page_formats[paper_type].llx, maxx,
	      page_formats[paper_type].lly-page_formats[paper_type].ury, maxy);
	}

	fprintf(outfile,"/xdpi %d def\n", get_pointx(1));
	fprintf(outfile,"/ydpi %d def\n", get_pointy(1));
	fputs("1.5 setlinewidth\nnewpath\n",outfile);

	/* send the header file */
	fputs("%%BeginProcSet: epsonps.pro\n",outfile);
	fputs("% Prolog for epsonps.c, by Russell Lang 19910507\n",outfile);
	fputs("/M {moveto} def\n",outfile);
	fputs("/L {lineto} def\n",outfile);
	fputs("/S {show} def\n",outfile);
	fputs("/pica    { /fwidth xdpi 10 idiv def /fheight ydpi 6 idiv def } def\n",outfile);
	fputs("/elite   { /fwidth xdpi 12 idiv def /fheight ydpi 6 idiv def } def\n",outfile);
	fputs("/fifteen { /fwidth xdpi 15 idiv def /fheight ydpi 6 idiv def } def\n",outfile);
	fputs("/condensed  { /fwidth fwidth 3 mul 5 idiv def } def\n",outfile);
	fputs("/doublewidth { /fwidth fwidth 2 mul def } def\n",outfile);
	fputs("/doubleheight { /fwidth fwidth 2 mul def /fheight fheight 2 mul def } def\n",outfile);
	fputs("/quadheight { /fwidth fwidth 4 mul def /fheight fheight 4 mul def } def\n",outfile);
	fputs("/normalfont {/Epson findfont\n",outfile);
	fputs("  [fwidth 10 6 div mul 0 0 fheight neg 0 0] makefont setfont} def\n",outfile);
	fputs("/boldfont {/Epson-Bold findfont\n",outfile);
	fputs("  [fwidth 10 6 div mul 0 0 fheight neg 0 0] makefont setfont} def\n",outfile);
	fputs("/italicfont {/Epson-Oblique findfont\n",outfile);
	fputs("  [fwidth 10 6 div mul 0 0 fheight neg 0 0] makefont setfont} def\n",outfile);
	fputs("/bolditalicfont {/Epson-BoldOblique findfont\n",outfile);
	fputs("  [fwidth 10 6 div mul 0 0 fheight neg 0 0] makefont setfont} def\n",outfile);
	fputs("/ibmgraphfont {/IBMgraphic findfont\n",outfile);
	fputs("  [fwidth 10 6 div mul 0 0 fheight neg 0 0] makefont setfont} def\n",outfile);
	fputs("/graphicbuf 2 string def\n",outfile);
	fputs("% copy StandardEncoding and call the new version IBMEncoding\n",outfile);
	fputs("StandardEncoding dup length array copy /IBMEncoding exch def\n",outfile);
	fputs("% Re Encode a font with IBMEncoding vector\n",outfile);
	fputs("/IBMEncode {\n",outfile);
	fputs("  /BasefontDict BasefontName findfont def\n",outfile);
	fputs("  /NewfontDict BasefontDict maxlength dict def\n",outfile);
	fputs("  BasefontDict\n",outfile);
	fputs("  { exch dup /FID ne\n",outfile);
	fputs("    { dup /Encoding eq\n",outfile);
	fputs("      { exch pop IBMEncoding NewfontDict 3 1 roll put }\n",outfile);
	fputs("      { exch NewfontDict 3 1 roll put }\n",outfile);
	fputs("      ifelse\n",outfile);
	fputs("    }\n",outfile);
	fputs("    { pop pop }\n",outfile);
	fputs("    ifelse\n",outfile);
	fputs("  } forall\n",outfile);
	fputs("  NewfontDict /FontName NewfontName put\n",outfile);
	fputs("  NewfontName NewfontDict definefont pop\n",outfile);
	fputs("} def\n",outfile);
	fputs("% Load a vector into the current Encoding Vector\n",outfile);
	fputs("/EncodingLoad { aload length 2 idiv\n",outfile);
	fputs("  {Encoding 3 1 roll put} repeat} def\n",outfile);
	fputs("% Create new Epson fonts from Courier fonts, with IBMEncoding\n",outfile);
	fputs("/BasefontName /Courier def /NewfontName /Epson def\n",outfile);
	fputs("IBMEncode\n",outfile);
	fputs("/BasefontName /Courier-Bold def /NewfontName /Epson-Bold def\n",outfile);
	fputs("IBMEncode\n",outfile);
	fputs("/BasefontName /Courier-BoldOblique def /NewfontName /Epson-BoldOblique def\n",outfile);
	fputs("IBMEncode\n",outfile);
	fputs("/BasefontName /Courier-Oblique def /NewfontName /Epson-Oblique def\n",outfile);
	fputs("IBMEncode\n",outfile);
	fputs("% add the extra character encodings for IBMEncoding\n",outfile);
	fputs("[ 128 /Ccedilla 129 /udieresis 130 /eacute 131 /acircumflex\n",outfile);
	fputs("  132 /adieresis 133 /agrave 134 /aring 135 /ccedilla 136 /ecircumflex\n",outfile);
	fputs("  137 /edieresis 138 /egrave 139 /idieresis 140 /icircumflex 141 /igrave\n",outfile);
	fputs("  142 /Adieresis 143 /Aring 144 /Eacute\n",outfile);
	fputs("  145 /ae % not in Courier.afm, but seems to be present\n",outfile);
	fputs("  146 /AE % not in Courier.afm, but seems to be present\n",outfile);
	fputs("  147 /ocircumflex 148 /odieresis 149 /ograve 150 /ucircumflex 151 /ugrave\n",outfile);
	fputs("  152 /ydieresis 153 /Odieresis 154 /Udieresis 155 /cent 156 /sterling 157 /yen\n",outfile);
	fputs("  158 /.notdef % Peseta\n",outfile);
	fputs("  159 /florin 160 /aacute 161 /iacute 162 /oacute 163 /uacute 164 /ntilde\n",outfile);
	fputs("  165 /Ntilde 166 /ordfeminine 167 /ordmasculine 168 /questiondown\n",outfile);
	fputs("  169 /.notdef % logical ??\n",outfile);
	fputs("  170 /.notdef % logical not\n",outfile);
	fputs("  171 /.notdef % half\n",outfile);
	fputs("  172 /.notdef % quarter\n",outfile);
	fputs("  173 /exclamdown 174 /guillemotleft 175 /guillemotright\n",outfile);
	fputs("]\n",outfile);
	fputs("/Epson findfont begin EncodingLoad end\n",outfile);
	fputs("% International character sets\n",outfile);
	fputs("/USA [ 35 /numbersign 36 /dollar 64 /at\n",outfile);
	fputs("  91 /bracketleft 92 /backslash 93 /bracketright 94 /asciicircum 96 /quoteleft\n",outfile);
	fputs("  123 /braceleft 124 /bar 125 /braceright 126 /asciitilde ] def\n",outfile);
	fputs("/France [ 35 /numbersign 36 /dollar 64 /agrave\n",outfile);
	fputs("  91 /ring 92 /ccedilla 93 /section 94 /asciicircum 96 /quoteleft\n",outfile);
	fputs("  123 /eacute 124 /ugrave 125 /egrave 126 /dieresis ] def\n",outfile);
	fputs("/Germany [ 35 /numbersign 36 /dollar 64 /section\n",outfile);
	fputs("  91 /Adieresis 92 /Odieresis 93 /Udieresis 94 /asciicircum 96 /quoteleft\n",outfile);
	fputs("  123 /adieresis 124 /odieresis 125 /udieresis 126 /germandbls ] def\n",outfile);
	fputs("/UnitedKingdom [ 35 /sterling 36 /dollar 64 /at\n",outfile);
	fputs("  91 /bracketleft 92 /backslash 93 /bracketright 94 /asciicircum 96 /quoteleft\n",outfile);
	fputs("  123 /braceleft 124 /bar 125 /braceright 126 /asciitilde ] def\n",outfile);
	fputs("/DenmarkI [ 35 /numbersign 36 /dollar 64 /at\n",outfile);
	fputs("  91 /AE 92 /Oslash 93 /Aring 94 /asciicircum 96 /quoteleft\n",outfile);
	fputs("  123 /ae 124 /oslash 125 /aring 126 /asciitilde ] def\n",outfile);
	fputs("/Sweden [ 35 /numbersign 36 /currency 64 /Eacute\n",outfile);
	fputs("  91 /Adieresis 92 /Odieresis 93 /Aring 94 /Udieresis 96 /eacute\n",outfile);
	fputs("  123 /adieresis 124 /odieresis 125 /aring 126 /udieresis ] def\n",outfile);
	fputs("/Italy [ 35 /numbersign 36 /dollar 64 /at\n",outfile);
	fputs("  91 /ring 92 /backslash 93 /eacute 94 /asciicircum 96 /ugrave\n",outfile);
	fputs("  123 /agrave 124 /ograve 125 /egrave 126 /igrave ] def\n",outfile);
	fputs("/SpainI [ 35 /currency 36 /dollar 64 /at  % 35 should be peseta\n",outfile);
	fputs("  91 /exclamdown 92 /Ntilde 93 /questiondown 94 /asciicircum 96 /quoteleft\n",outfile);
	fputs("  123 /dieresis 124 /ntilde 125 /braceright 126 /asciitilde ] def\n",outfile);
	fputs("/Japan [ 35 /numbersign 36 /dollar 64 /at\n",outfile);
	fputs("  91 /bracketleft 92 /yen 93 /bracketright 94 /asciicircum 96 /quoteleft\n",outfile);
	fputs("  123 /braceleft 124 /bar 125 /braceright 126 /asciitilde ] def\n",outfile);
	fputs("/Norway [ 35 /numbersign 36 /currency 64 /Eacute\n",outfile);
	fputs("  91 /AE 92 /Oslash 93 /Aring 94 /Udieresis 96 /eacute\n",outfile);
	fputs("  123 /ae 124 /oslash 125 /aring 126 /udieresis ] def\n",outfile);
	fputs("/DenmarkII [ 35 /numbersign 36 /dollar 64 /Eacute\n",outfile);
	fputs("  91 /AE 92 /Oslash 93 /Aring 94 /Udieresis 96 /eacute\n",outfile);
	fputs("  123 /ae 124 /oslash 125 /aring 126 /udieresis ] def\n",outfile);
	fputs("/SpainII [ 35 /numbersign 36 /dollar 64 /aacute\n",outfile);
	fputs("  91 /exclamdown 92 /Ntilde 93 /questiondown 94 /eacute 96 /quoteleft\n",outfile);
	fputs("  123 /iacute 124 /ntilde 125 /oacute 126 /uacute ] def\n",outfile);
	fputs("/LatinAmerica [ 35 /numbersign 36 /dollar 64 /aacute\n",outfile);
	fputs("  91 /exclamdown 92 /Ntilde 93 /questiondown 94 /eacute 96 /udieresis\n",outfile);
	fputs("  123 /iacute 124 /ntilde 125 /oacute 126 /uacute ] def\n",outfile);
	fputs("% set the international character set\n",outfile);
	fputs("% this relies on all four Epson fonts using the same copy of IBMEncoding\n",outfile);
	fputs("/InternationalSet {/Epson findfont begin EncodingLoad end} def\n",outfile);
	fputs("%%BeginFont: IBMgraphic\n",outfile);
	fputs("12 dict begin % start dictionary for font\n",outfile);
	fputs("% add FontInfo dictionary\n",outfile);
	fputs("/FontInfo 2 dict dup begin\n",outfile);
	fputs("/Notice (A partial IBM PC graphic characters font by Russell Lang 19910507) readonly def\n",outfile);
	fputs("/FullName (IBMgraphic) readonly def\n",outfile);
	fputs("end readonly def\n",outfile);
	fputs("% fill in rest of IBMgraphic dictionary\n",outfile);
	fputs("/FontName /IBMgraphic def\n",outfile);
	fputs("/PaintType 3 def   % fill or stroke\n",outfile);
	fputs("/FontType 3 def    % user defined\n",outfile);
	fputs("/FontMatrix [ 0.1 0 0 0.1 0 0 ] readonly def\n",outfile);
	fputs("/FontBBox [ 0 -2 6 8 ] readonly def\n",outfile);
	fputs("/Encoding 256 array def   % allocate the Encoding array\n",outfile);
	fputs("  0 1 255 { Encoding exch /.notdef put} for\n",outfile);
	fputs("% look up an IBM or Epson manual for explanation of these codes\n",outfile);
	fputs("[\n",outfile);
	fputs("  000 /blank 001 /face 002 /facereverse 003 /heart 004 /diamond 005 /club\n",outfile);
	fputs("  006 /spade 007 /disc 008 /discreverse 009 /circle 010 /circlereverse\n",outfile);
	fputs("  011 /male 012 /female 013 /note 014 /notes 015 /lamp\n",outfile);
	fputs("  016 /triangleright 017 /triangleleft 018 /arrowupdown 019 /exclamdouble\n",outfile);
	fputs("  020 /paragraph 021 /section 022 /I022 023 /arrowupdownunder 024 /arrowup\n",outfile);
	fputs("  025 /arrowdown 026 /arrowright 027 /arrowleft 028 /I028 029 /arrowboth\n",outfile);
	fputs("  030 /triangleup 031 /triangledown\n",outfile);
	fputs("  127 /delete 158 /peseta 169 /I169\n",outfile);
	fputs("  170 /logicalnot 171 /half 172 /quarter\n",outfile);
	fputs("  176 /lightgrey 177 /mediumgrey 178 /darkgrey\n",outfile);
	fputs("  179 /I179 180 /I180 181 /I181 182 /I182 183 /I183 184 /I184 185 /I185\n",outfile);
	fputs("  186 /I186 187 /I187 188 /I188 189 /I189 190 /I190 191 /I191 192 /I192\n",outfile);
	fputs("  193 /I193 194 /I194 195 /I195 196 /I196 197 /I197 198 /I198 199 /I199\n",outfile);
	fputs("  200 /I200 201 /I201 202 /I202 203 /I203 204 /I204 205 /I205 206 /I206\n",outfile);
	fputs("  207 /I207 208 /I208 209 /I209 210 /I210 211 /I211 212 /I212 213 /I213\n",outfile);
	fputs("  214 /I214 215 /I215 216 /I216 217 /I217 218 /I218\n",outfile);
	fputs("  219 /blockfull 220 /blockbot 221 /blockleft 222 /blockright 223 /blocktop\n",outfile);
	fputs("  224 /alpha 225 /beta 226 /Gamma 227 /pi 228 /Sigma 229 /sigma 230 /mu\n",outfile);
	fputs("  231 /tau 232 /Phi 233 /Theta 234 /Omega 235 /delta 236 /infinity\n",outfile);
	fputs("  237 /emptyset 238 /element 239 /intersection 240 /equivalence 241 /plusminus\n",outfile);
	fputs("  242 /greaterequal 243 /lessequal 244 /integraltop 245 /integralbot\n",outfile);
	fputs("  246 /divide 247 /approxequal 248 /degree 249 /bullet 250 /dotmath\n",outfile);
	fputs("  251 /radical 252 /supern 253 /super2 254 /slug\n",outfile);
	fputs("]\n",outfile);
	fputs("EncodingLoad\n",outfile);
	fputs("% show string from the Symbol font\n",outfile);
	fputs("/SymbolShow { % char_code matrix --\n",outfile);
	fputs("  /Symbol findfont exch makefont setfont\n",outfile);
	fputs("  0 0 moveto show\n",outfile);
	fputs("} def\n",outfile);
	fputs("% show string from the Courier font\n",outfile);
	fputs("/CourierShow { % char_code matrix --\n",outfile);
	fputs("  /Courier findfont exch makefont setfont\n",outfile);
	fputs("  0 0 moveto show\n",outfile);
	fputs("} def\n",outfile);
	fputs("% Build Character into cache\n",outfile);
	fputs("/BuildChar {  %  font_dictionary char_code --\n",outfile);
	fputs("  exch begin                      % this font dictionary\n",outfile);
	fputs("  Encoding exch get               % get character name\n",outfile);
	fputs("  gsave                           % save graphics state\n",outfile);
	fputs("  0.5 setlinewidth                % width of stroked lines\n",outfile);
	fputs("  newpath\n",outfile);
	fputs("  6 0                             % width vector (mono spaced)\n",outfile);
	fputs("  0 -2 6 8 setcachedevice         % cache this bounding box\n",outfile);
	fputs("  CharProcs exch get              % get procedure\n",outfile);
	fputs("  exec                            % draw it\n",outfile);
	fputs("  grestore                        % restore graphics state\n",outfile);
	fputs("  end                             % end of font dictionary\n",outfile);
	fputs("} def\n",outfile);
	fputs("% add CharProcs dictionary which describes each character\n",outfile);
	fputs("/CharProcs 118 dict dup begin\n",outfile);
	fputs("/.notdef {} def\n",outfile);
	fputs("/blank {} def\n",outfile);
	fputs("/face {5.75 4.5 M 4 4.5 1.75 0 90 arc 2 4.5 1.75 90 180 arc\n",outfile);
	fputs("  2 1.5 1.75 180 270 arc 4 1.5 1.75 270 360 arc 5.75 4.5 lineto\n",outfile);
	fputs("  5.25 4.5 M  4 1.5 1.25 360 270 arcn 2 1.5 1.25 270 180 arcn\n",outfile);
	fputs("  2 4.5 1.25 180 90 arcn 4 4.5 1.25 90 0 arcn 5.25 4.5 lineto\n",outfile);
	fputs("  2.5 4 M 2 4 0.5 360 0 arcn\n",outfile);
	fputs("  4.5 4 M 4 4 0.5 360 0 arcn\n",outfile);
	fputs("  1.375 2.185 M 3 5 3.25 240 300 arc 3 5 3.75 300 240 arcn closepath\n",outfile);
	fputs("  fill} def\n",outfile);
	fputs("/facereverse {5.75 4.5 M 4 4.5 1.75 0 90 arc 2 4.5 1.75 90 180 arc\n",outfile);
	fputs("  2 1.5 1.75 180 270 arc 4 1.5 1.75 270 360 arc 5.75 4.5 lineto\n",outfile);
	fputs("  2.5 4 M 2 4 0.5 360 0 arcn\n",outfile);
	fputs("  4.5 4 M 4 4 0.5 360 0 arcn\n",outfile);
	fputs("  1.375 2.185 M 3 5 3.25 240 300 arc 3 5 3.75 300 240 arcn closepath\n",outfile);
	fputs("  fill} def\n",outfile);
	fputs("/heart {(\\251) [8 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/diamond {(\\250) [8 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/club {(\\247) [8 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/spade {(\\252) [8 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/disc {3 3 1.5 0 360 arc closepath fill} def\n",outfile);
	fputs("/discreverse {0 -2 M 6 -2 L 6 8 L 0 8 L closepath\n",outfile);
	fputs("  4.5 3 M 3 3 1.5 360 0 arcn closepath fill} def\n",outfile);
	fputs("/circle {3 3 2.5 0 360 arc 3 3 2 360 0 arcn fill} def\n",outfile);
	fputs("/circlereverse {0 -2 M 6 -2 L 6 8 L 0 8 L closepath\n",outfile);
	fputs("  5.5 3 M 3 3 2.5 360 0 arcn 5 3 M 3 3 2 0 360 arc fill} def\n",outfile);
	fputs("/male {2.5 1.5 1.5 0 360 arc\n",outfile);
	fputs("  2.5 3 M 5 5.5 L 3 5.5 M 5 5.5 L 5 3.5 L stroke } def\n",outfile);
	fputs("/female {3 4.5 1.5 0 360 arc 3 3 M 3 0 L 1.5 1.5 M 4.5 1.5 L stroke} def\n",outfile);
	fputs("/note {1.5 1.5 1 0 360 arc fill\n",outfile);
	fputs("  2.25 1.5 M 2.25 6 L 5 6 L 5 5 L 2.25 5 L stroke} def\n",outfile);
	fputs("/notes {1.5 1.5 1 0 360 arc fill 4.25 1.5 1 0 360 arc fill\n",outfile);
	fputs("  2.25 1.5 M 2.25 6 L 5 6 L 5 5 L 2.25 5 L 5 6 M 5 1.5 L stroke} def\n",outfile);
	fputs("/lamp {3 3 1 0 360 arc 3 6 M 3 4 L 3 0 M 3 2 L\n",outfile);
	fputs("  4 4 M 5 5 L 2 4 M 1 5 L 2 2 M 1 1 L 4 2 M 5 1 L stroke} def\n",outfile);
	fputs("/triangleleft  {5 0 M 1 3 L 5 6 L 5 0 L closepath fill} def\n",outfile);
	fputs("/triangleright {1 0 M 1 6 L 5 3 L 1 0 L closepath fill} def\n",outfile);
	fputs("/arrowupdown {3 0.5 M 3 5.5 L stroke\n",outfile);
	fputs("  1.5 4.5 M 3 6 L 4.5 4.5 L closepath fill\n",outfile);
	fputs("  1.5 1.5 M 3 0 L 4.5 1.5 L closepath fill} def\n",outfile);
	fputs("/exclamdouble {(!) [10 0 0 10 -1.5 0] CourierShow\n",outfile);
	fputs("  (!) [10 0 0 10  1.5 0] CourierShow} def\n",outfile);
	fputs("/paragraph {(\\266) [10 0 0 10 0 0] CourierShow} def\n",outfile);
	fputs("/section {(\\247) [10 0 0 10 0 0] CourierShow} def\n",outfile);
	fputs("/I022 {0 0 M 0 1.5 L 6 1.5 L 6 0 L 0 0 L closepath fill} def % cursor ??\n",outfile);
	fputs("/arrowupdownunder {CharProcs /arrowupdown get exec\n",outfile);
	fputs("  1.5 -0.25 M 4.5 -0.25 L stroke} def\n",outfile);
	fputs("/arrowup    {1.5 4.5 M 3 6 L 4.5 4.5 L closepath fill\n",outfile);
	fputs("  3 0 M 3 4.5 L stroke} def\n",outfile);
	fputs("/arrowdown  {1.5 1.5 M 3 0 L 4.5 1.5 L closepath fill\n",outfile);
	fputs("  3 1.5 M 3 6 L stroke} def\n",outfile);
	fputs("/arrowleft  {2 4.5 M 0.5 3 L 2 1.5 L closepath fill\n",outfile);
	fputs("  2 3 M 5.25 3 L stroke} def\n",outfile);
	fputs("/arrowright {4 4.5 M 5.5 3 L 4 1.5 L closepath fill\n",outfile);
	fputs("  0.25 3 M 4.5 3 L stroke} def\n",outfile);
	fputs("/I028 {0.4 setlinewidth 1 6 M 1 4 L 5 4 L stroke} def % logical ??\n",outfile);
	fputs("/arrowboth {2 3 M 4.5 3 L stroke\n",outfile);
	fputs("  2 4.5 M 0.5 3 L 2 1.5 L closepath fill\n",outfile);
	fputs("  4 4.5 M 5.5 3 L 4 1.5 L closepath fill} def\n",outfile);
	fputs("/triangleup   {1 1 M 3 5 L 5 1 L 1 1 L closepath fill} def\n",outfile);
	fputs("/triangledown {1 5 M 3 1 L 5 5 L 1 5 L closepath fill} def\n",outfile);
	fputs("% Strays\n",outfile);
	fputs("/delete {1 1 M 5 1 L 5 3 L 3 5 L 1 3 L 1 1 L closepath stroke} def\n",outfile);
	fputs("/peseta { /Courier findfont [6 0 0 10 0 0] makefont setfont\n",outfile);
	fputs("  0 0 M (P) show 2.5 0 M (t) show} def\n",outfile);
	fputs("/I169 {0.4 setlinewidth 1 2 M 1 4 L 5 4 L stroke} def % logical ??\n",outfile);
	fputs("/logicalnot {0.4 setlinewidth 1 4 M 5 4 L 5 2 L stroke} def\n",outfile);
	fputs("/half { /Courier findfont [5 0 0 5 0 0] makefont setfont 0 4 M (1) show\n",outfile);
	fputs("  2.5 0 M (2) show 0.2 setlinewidth 0.5 2 M 5.5 5 L stroke} def\n",outfile);
	fputs("/quarter { /Courier findfont [5 0 0 5 0 0] makefont setfont 0 4 M (1) show\n",outfile);
	fputs("  2.5 0 M (4) show 0.2 setlinewidth 0.5 2 M 5.5 5 L stroke} def\n",outfile);
	fputs("% Grey Blobs\n",outfile);
	fputs("/lightgrey {1 setlinewidth\n",outfile);
	fputs("  -0.5 2 8 {0 3 6 {1 index moveto currentlinewidth 0 rlineto} for pop} for\n",outfile);
	fputs("  -1.5 2 8 {1.5 3 6 {1 index moveto currentlinewidth 0 rlineto} for pop} for\n",outfile);
	fputs("  stroke} def\n",outfile);
	fputs("/mediumgrey {1 setlinewidth\n",outfile);
	fputs("  -0.5 2 8 {0 2 6 {1 index moveto currentlinewidth 0 rlineto} for pop} for\n",outfile);
	fputs("  -1.5 2 8 {1 2 6 {1 index moveto currentlinewidth 0 rlineto} for pop} for\n",outfile);
	fputs("  stroke} def\n",outfile);
	fputs("/darkgrey {1 setlinewidth\n",outfile);
	fputs("  -0.5 2 8 {0 exch M 2   0 rlineto 1 0 rmoveto 2 0 rlineto} for\n",outfile);
	fputs("  -1.5 2 8 {0 exch M 0.5 0 rlineto 1 0 rmoveto 2 0 rlineto\n",outfile);
	fputs("   1 0 rmoveto 1.5 0 rlineto} for stroke} def\n",outfile);
	fputs("% Box line line segments\n",outfile);
	fputs("/I179 {3 -2 M 3 8 L stroke} def\n",outfile);
	fputs("/I180 {3 -2 M 3 8 L 0 4 M 3 4 L stroke} def\n",outfile);
	fputs("/I181 {3 -2 M 3 8 L 0 4 M 3 4 L 0 2 M 3 2 L stroke} def\n",outfile);
	fputs("/I182 {2 -2 M 2 8 L 4 -2 M 4 8 L 0 4 M 2 4 L stroke} def\n",outfile);
	fputs("/I183 {0 4 M 4 4 L 4 -2 L 2 -2 M 2 4 L stroke} def\n",outfile);
	fputs("/I184 {0 4 M 3 4 L 3 -2 L 0 2 M 3 2 L stroke} def\n",outfile);
	fputs("/I185 {4 -2 M 4 8 L 0 4 M 2 4 L 2 8 L 0 2 M 2 2 L 2 -2 L stroke} def\n",outfile);
	fputs("/I186 {2 -2 M 2 8 L 4 -2 M 4 8 L stroke} def\n",outfile);
	fputs("/I187 {0 4 M 4 4 L 4 -2 L 0 2 M 2 2 L 2 -2 L stroke} def\n",outfile);
	fputs("/I188 {0 2 M 4 2 L 4 8 L 0 4 M 2 4 L 2 8 L stroke} def\n",outfile);
	fputs("/I189 {0 4 M 4 4 L 4 8 L 2 4 M 2 8 L stroke} def\n",outfile);
	fputs("/I190 {0 2 M 3 2 L 3 8 L 0 4 M 3 4 L stroke} def\n",outfile);
	fputs("/I191 {0 4 M 3 4 L 3 -2 L stroke} def\n",outfile);
	fputs("/I192 {3 8 M 3 4 L 6 4 L stroke} def\n",outfile);
	fputs("/I193 {0 4 M 6 4 L 3 4 M 3 8 L stroke} def\n",outfile);
	fputs("/I194 {0 4 M 6 4 L 3 4 M 3 -2 L stroke} def\n",outfile);
	fputs("/I195 {3 -2 M 3 8 L 3 4 M 6 4 L stroke} def\n",outfile);
	fputs("/I196 {0 4 M 6 4 L stroke} def\n",outfile);
	fputs("/I197 {0 4 M 6 4 L 3 -2 M 3 8 L stroke} def\n",outfile);
	fputs("/I198 {3 -2 M 3 8 L 3 4 M 6 4 L 3 2 M 6 2 L stroke} def\n",outfile);
	fputs("/I199 {2 -2 M 2 8 L 4 -2 M 4 8 L 4 4 M 6 4 L stroke} def\n",outfile);
	fputs("/I200 {2 8 M 2 2 L 6 2 L 4 8 M 4 4 L 6 4 L stroke} def\n",outfile);
	fputs("/I201 {2 -2 M 2 4 L 6 4 L 4 -2 M 4 2 L 6 2 L stroke} def\n",outfile);
	fputs("/I202 {0 2 M 6 2 L 0 4 M 2 4 L 2 8 L 4 8 M 4 4 L 6 4 L stroke} def\n",outfile);
	fputs("/I203 {0 4 M 6 4 L 0 2 M 2 2 L 2 -2 L 4 -2 M 4 2 L 6 2 L stroke} def\n",outfile);
	fputs("/I204 {2 -2 M 2 8 L 4 8 M 4 4 L 6 4 L 4 -2 M 4 2 L 6 2 L stroke} def\n",outfile);
	fputs("/I205 {0 4 M 6 4 L 0 2 M 6 2 L stroke} def\n",outfile);
	fputs("/I206 {0 4 M 2 4 L 2 8 L 4 8 M 4 4 L 6 4 L 6 2 M 4 2 L 4 -2 L\n",outfile);
	fputs("  2 -2 M 2 2 L 0 2 L stroke} def\n",outfile);
	fputs("/I207 {0 2 M 6 2 L 0 4 M 6 4 L 3 4 M 3 8 L stroke} def\n",outfile);
	fputs("/I208 {0 4 M 6 4 L 2 4 M 2 8 L 4 4 M 4 8 L stroke} def\n",outfile);
	fputs("/I209 {0 4 M 6 4 L 0 2 M 6 2 L 3 2 M 3 -2 L stroke} def\n",outfile);
	fputs("/I210 {0 4 M 6 4 L 2 4 M 2 -2 L 4 4 M 4 -2 L stroke} def\n",outfile);
	fputs("/I211 {2 8 M 2 4 L 6 4 L 4 8 M 4 4 L stroke} def\n",outfile);
	fputs("/I212 {3 8 M 3 2 L 6 2 L 3 4 M 6 4 L stroke} def\n",outfile);
	fputs("/I213 {3 -2 M 3 4 L 6 4 L 3 2 M 6 2 L stroke} def\n",outfile);
	fputs("/I214 {2 -2 M 2 4 L 6 4 L 4 -2 M 4 4 L stroke} def\n",outfile);
	fputs("/I215 {2 -2 M 2 8 L 4 -2 M 4 8 L 0 4 M 6 4 L stroke} def\n",outfile);
	fputs("/I216 {3 -2 M 3 8 L 0 4 M 6 4 L 0 2 M 6 2 L stroke} def\n",outfile);
	fputs("/I217 {0 4 M 3 4 L 3 8 L stroke} def\n",outfile);
	fputs("/I218 {3 -2 M 3 4 L 6 4 L stroke} def\n",outfile);
	fputs("% Block segments\n",outfile);
	fputs("/blockfull  {0 -2 M 6 -2 L 6 8 L 0 8 L 0 -2 L closepath fill} def\n",outfile);
	fputs("/blockbot   {0 -2 M 6 -2 L 6 3 L 0 3 L 0 -2 L closepath fill} def\n",outfile);
	fputs("/blockleft  {0 -2 M 3 -2 L 3 8 L 0 8 L 0 -2 L closepath fill} def\n",outfile);
	fputs("/blockright {3 -2 M 6 -2 L 6 8 L 3 8 L 3 -2 L closepath fill} def\n",outfile);
	fputs("/blocktop   {0 3 M 6 3 L 6 8 L 0 8 L 0 3 L closepath fill} def\n",outfile);
	fputs("% from symbol font\n",outfile);
	fputs("/alpha {(a) [9.5 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/beta {(b) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/Gamma {(G) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/pi {(p) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/Sigma {(S) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/sigma {(s) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/mu {(m) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/tau {(t) [10 0 0 10 0.5 0] SymbolShow} def\n",outfile);
	fputs("/Phi {(F) [8 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/Theta {(Q) [8 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/Omega {(W) [8 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/delta {(d) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/infinity {(\\245) [8.5 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/emptyset {(\\306) [7.5 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/element {(\\316) [8.5 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/intersection {(\\307) [8 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/equivalence {(\\272) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/plusminus {(\\261) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/greaterequal {(\\263) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/lessequal {(\\243) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("% integral pieces\n",outfile);
	fputs("/integraltop {3 -2 M 3 6 L 4 6 1 180 0 arcn stroke} def\n",outfile);
	fputs("/integralbot {3 8 M 3 0 L 2 0 1 0 180 arcn stroke} def\n",outfile);
	fputs("% from symbol font\n",outfile);
	fputs("/divide {(\\270) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/approxequal {(\\273) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/degree {(\\260) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/bullet {(\\267) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/dotmath {(\\327) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("/radical {(\\326) [10 0 0 10 0 0] SymbolShow} def\n",outfile);
	fputs("% from courier font\n",outfile);
	fputs("/supern { /Courier findfont [7 0 0 5 0 0] makefont setfont\n",outfile);
	fputs("  0 4 moveto (n) show } def\n",outfile);
	fputs("/super2 { /Courier findfont [7 0 0 5 0 0] makefont setfont\n",outfile);
	fputs("  0 4 moveto (2) show } def\n",outfile);
	fputs("% slug\n",outfile);
	fputs("/slug {1.5 0 M 4.5 0 L 4.5 5 L 1.5 5 L 1.5 0 L closepath fill} def\n",outfile);
	fputs("end readonly def % end CharProcs\n",outfile);
	fputs("currentdict end  % end the font dictionary, and leave it on the stack\n",outfile);
	fputs("dup /FontName get exch definefont pop % define the new font\n",outfile);
	fputs("%%EndFont\n",outfile);
	fputs("% end of epsonps.pro\n",outfile);
	fputs("%%EndProcSet\n",outfile);

	fprintf(outfile,"%s InternationalSet\n",international[country_num]);

	fputs("%%%%EndProlog\n",outfile);

        if (screendump_mode) state = S_SCREENDUMP;
	init_tabs();	/* initialise tabs */
	macro[0] = 30;	/* initialise macro */

	page = 0;
	new_page();
}

int
epsonps(const char *inputfile, const char *outputfile)
{
	int ch;

	if ((infile = fopen(inputfile,READBIN)) == NULL) {
		fprintf(stderr,"Unable to open %s\n",inputfile);
		return(-1);
	}

	if ((outfile = fopen(outputfile,"w")) == NULL) {
		fprintf(stderr,"Unable to open %s\n",outputfile);
		return(-1);
	}

	init_sets();
	init_printer();

	while ( (ch=fgetc(infile)) != EOF)
		dochar((unsigned char)ch);
	if (ptr>0)
		putline();
	if (cx > xstart || cy > ystart)
		end_page();
	else {
		fputs("saveobj2 restore\n",outfile);
		page--;
	}
	fputs("%%Trailer\n",outfile);
	fprintf(outfile,"%%%%Pages: %d\n",page);
	fputs("saveobj1 restore\n",outfile);

	fclose(infile);
	fclose(outfile);

	return(0);
}
/* end of epsonps.c */
