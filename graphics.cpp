// graphics.cpp
// Revision 24-apr-2009

#ifdef __BORLANDC__
#pragma warn -8027
#endif

#include "graphics.h"
#include "sysvar.h"
#include "error.h"
#include "var.h"
#include "key.h"
#include "charset.h"

#include "util.h"
using util::touch;
using util::to_string;

#include "trace.h"

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include <memory>
using std::auto_ptr;

#include <map>
#include <queue>

#include <string.h>
#include <limits.h>
#include <math.h>

// Para depuracion
#include <iostream>
using std::cerr;
using std::endl;
#if defined __unix__ || defined __linux__
#include <unistd.h>
#endif
#include <cassert>
#define ASSERT assert


// This is controlled with the configure option --disable-graphics
#ifndef BLASSIC_CONFIG_NO_GRAPHICS

#if defined BLASSIC_USE_WINDOWS || defined BLASSIC_USE_X || \
	defined BLASSIC_USE_SVGALIB

// If configure is not used you can comment the following line
// to disable graphics.

#define BLASSIC_HAS_GRAPHICS

#endif

#endif


#ifdef BLASSIC_HAS_GRAPHICS

#ifdef BLASSIC_USE_SVGALIB

#include <unistd.h>
#include <sys/types.h>
#include <vga.h>
#include <vgagl.h>

#endif
// BLASSIC_USE_SVGALIB

#ifdef BLASSIC_USE_X

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#endif
// BLASSIC_USE_X

#ifdef BLASSIC_USE_WINDOWS

#include <process.h>
#include <windows.h>
#undef min
#undef max

#if defined __CYGWIN32__ || defined __CYGWIN__
// This macros are from Anders Norlander, modified to add
// the cast to start_proc.
/* Macro uses args se we can cast proc to LPTHREAD_START_ROUTINE
   in order to avoid warings because of return type */
#define _beginthreadex(security, stack_size, start_proc, arg, flags, pid) \
CreateThread (security, stack_size, (LPTHREAD_START_ROUTINE) start_proc, \
	arg, flags, (LPDWORD) pid)
#define _endthreadex ExitThread
#endif

// Use Polyline to draw a point.
#define USE_POLY

#endif
// BLASSIC_USE_WINDOWS

#endif
// BLASSIC_HAS_GRAPHICS

namespace sysvar= blassic::sysvar;


// Character set

charset::chardataset charset::data;

const charset::chardataset * charset::default_charset=
	& charset::default_data;

namespace {

#ifndef BLASSIC_HAS_GRAPHICS

void no_graphics_support ()
{
	if (showdebuginfo () )
		cerr << "This version of Blassic was compiled "
			"without graphics support" << endl;
	throw ErrFunctionCall;
}

#endif
// BLASSIC_HAS_GRAPHICS

#ifdef BLASSIC_HAS_GRAPHICS

#ifdef BLASSIC_USE_SVGALIB

char * font= NULL;

#endif
// BLASSIC_USE_SVGALIB

#ifdef BLASSIC_USE_X

Display * display= 0;
XIM xim= 0;
XIC xic= 0;
int screen;
Window window;
Pixmap pixmap;
bool pixmap_created= false;
GC gc, gcp;
XGCValues gcvalues, gcvaluesp;

//XEvent x_event;

long eventusedmask= StructureNotifyMask | ExposureMask |
	KeyPressMask | KeyReleaseMask |
	ButtonPressMask | ButtonReleaseMask |
	PointerMotionMask | EnterWindowMask;
long eventusedmaskactual;

typedef XColor color_t;

typedef unsigned long ColorValue;

#endif
// BLASSIC_USE_X

#ifdef BLASSIC_USE_WINDOWS

ATOM atomClass;
HANDLE hEvent;
HWND window;
HDC hdc= 0;
HBITMAP pixmap;
HDC hdcPixmap= 0;
typedef HPEN color_t;

typedef COLORREF ColorValue;

#endif
// BLASSIC_USE_WINDOWS

#if defined (BLASSIC_USE_WINDOWS) || defined (BLASSIC_USE_X)

color_t xcBlack, xcBlue, xcGreen, xcCyan,
	xcRed, xcMagenta, xcBrown, xcLightGrey,
	xcDarkGrey, xcLightBlue, xcLightGreen, xcLightCyan,
	xcLightRed, xcLightMagenta, xcYellow, xcWhite;

typedef color_t * pcolor;

pcolor pforeground, pbackground,
	//default_foreground= & xcBlack, default_background= & xcWhite,
	activecolor= NULL;

int default_pen, default_paper;

int graphics_pen, graphics_paper;

#endif
// defined BLASSIC_USE_WINDOWS || defined BLASSIC_USE_X

std::string default_title ("blassic");

bool fSynchro= false;

#ifdef BLASSIC_USE_WINDOWS

class CriticalSection {
public:
	CriticalSection ()
	{
		InitializeCriticalSection (& cs);
	}
	~CriticalSection ()
	{
		DeleteCriticalSection (& cs);
	}
	void enter ()
	{
		EnterCriticalSection (& cs);
	}
	void leave ()
	{
		LeaveCriticalSection (& cs);
	}
private:
	CRITICAL_SECTION cs;
};

class CriticalLock {
public:
	CriticalLock (CriticalSection & cs) :
		cs (cs)
	{
		cs.enter ();
	}
	~CriticalLock ()
	{
		cs.leave ();
	}
private:
	CriticalSection & cs;
};

#else

// Empty implementation, not using threads.

class CriticalSection { };

class CriticalLock {
public:
	CriticalLock (CriticalSection &)
	{ }
};

#endif

class QueueKey {
public:
	QueueKey ()
	{
	}
	void push (const std::string & str)
	{
		CriticalLock lock (cs);
		touch (lock);
		q.push (str);
	}
	std::string pop ()
	{
		CriticalLock lock (cs);
		touch (lock);
		std::string str= q.front ();
		q.pop ();
		return str;
	}
	bool empty ()
	{
		return q.empty ();
	}
	void erase ()
	{
		CriticalLock lock (cs);
		touch (lock);
		while (! q.empty () )
			q.pop ();
	}
private:
	std::queue <std::string> q;
	CriticalSection cs;
};

#if 0
const size_t MAXKEYSYM= 65535;
std::vector <bool> keypressedmap (MAXKEYSYM);
#endif

const size_t MAXINKEYCODE= 79;

#ifdef BLASSIC_USE_WINDOWS

#if 0
unsigned int presscode [MAXINKEYCODE + 1]= {
};
#endif

#elif defined BLASSIC_USE_X

#if 0
const unsigned int KEYSYMUNUSED= 1;
unsigned int presscode [MAXINKEYCODE + 1]= {
	XK_Up,          // 0
	XK_Right,       // 1
	XK_Down,        // 2
	XK_KP_9,        // 3
	XK_KP_6,        // 4
	XK_KP_3,        // 5
	XK_Execute,     // 6
	XK_KP_Decimal,  // 7
	XK_Left,        // 8
	KEYSYMUNUSED,   // 9 "Copy" key
	XK_KP_7,        // 10
	XK_KP_8,        // 11
	XK_KP_5,        // 12
	XK_KP_1,        // 13
	XK_KP_2,        // 14
	XK_KP_0,        // 15
	XK_Delete,      // 16
	KEYSYMUNUSED,   // 17
	XK_Return,      // 18
	KEYSYMUNUSED,   // 19
	XK_KP_4,        // 20
	XK_Shift_L,     // 21
	KEYSYMUNUSED,   // 22
	XK_Control_L,   // 23
	KEYSYMUNUSED,   // 24
	KEYSYMUNUSED,   // 25
	KEYSYMUNUSED,   // 26
	XK_P,           // 27
	KEYSYMUNUSED,   // 28
	KEYSYMUNUSED,   // 29
	KEYSYMUNUSED,   // 30
	KEYSYMUNUSED,   // 31
	XK_0,           // 32
	XK_9,           // 33
	XK_O,           // 34
	XK_I,           // 35
	XK_L,           // 36
	XK_K,           // 37
	XK_M,           // 38
	KEYSYMUNUSED,   // 39
	XK_8,           // 40
	XK_7,           // 41
	XK_U,           // 42
	XK_Y,           // 43
	XK_H,           // 44
	XK_J,           // 45
	XK_N,           // 46
	KEYSYMUNUSED,   // 47
	XK_6,           // 48
	XK_5,           // 49
	XK_R,           // 50
	XK_T,           // 51
	XK_G,           // 52
	XK_F,           // 53
	XK_B,           // 54
	XK_V,           // 55
	XK_4,           // 56
	XK_3,           // 57
	XK_E,           // 58
	XK_W,           // 59
	XK_S,           // 60
	XK_D,           // 61
	XK_C,           // 62
	XK_X,           // 63
	XK_1,           // 64
	XK_2,           // 65
	XK_Escape,      // 66
	XK_Q,           // 67
	XK_Tab,         // 68
	XK_A,           // 69
	XK_Caps_Lock,   // 70
	XK_Z,           // 71
	KEYSYMUNUSED,   // 72
	KEYSYMUNUSED,   // 73
	KEYSYMUNUSED,   // 74
	KEYSYMUNUSED,   // 75
	KEYSYMUNUSED,   // 76
	KEYSYMUNUSED,   // 77
	KEYSYMUNUSED,   // 78
	XK_BackSpace,   // 79
};

inline void keysymtoupper (KeySym & ks)
{
	KeySym discard;
	// Convert to upper case.
	XConvertCase (ks, & discard, & ks);
}

void set_pressed (KeySym ks)
{
	keysymtoupper (ks);
	if (ks >= 0 && ks <= MAXKEYSYM)
		keypressedmap [ks]= 1;
}

void reset_pressed (KeySym ks)
{
	keysymtoupper (ks);
	if (ks >= 0 && ks <= MAXKEYSYM)
		keypressedmap [ks]= 0;
}

#endif

#endif

#ifdef BLASSIC_USE_X
const unsigned int MAXKEYCODE= 255;
#elif defined BLASSIC_USE_WINDOWS
const unsigned int MAXKEYCODE= 511;
#endif

std::vector <bool> keycode_pressed (MAXKEYCODE + 1);

#ifdef BLASSIC_USE_X

const unsigned int NOUS= 0xFF;

const unsigned int inkeytocode [MAXINKEYCODE + 1]={
	// The key symbols indicated are the corresponding
	// to a spanish keyboard.
	0x62, //  0 Up
	0x66, //  1 Right
	0x68, //  2 Down
	0x51, //  3 Numeric 9
	0x55, //  4 Numeric 6
	0x59, //  5 Numeric 3
	0x6C, //  6 Intro
	0x5B, //  7 Numeric .
	0x64, //  8 Left
	0x00, //  9 Copy
	0x4F, // 10 Numeric 7
	0x50, // 11 Numeric 8
	0x54, // 12 Numeric 5
	0x57, // 13 Numeric 1
	0x58, // 14 Numeric 2
	0x5A, // 15 Numeric 0
	0x6B, // 16 Delete
	0x23, // 17 +
	0x24, // 18 Return
	0x33, // 19 	0x53, // 20 Numeric 4
	0x3E, // 21 Shift
	0x5E, // 22 \ in the CPC, <>
	0x6D, // 23 Control
	0x15, // 24 	0x14, // 25 '
	0x22, // 26 `[
	0x21, // 27 P
	0x30, // 28 '{
	0x2F, // 29 	0x3D, // 30 -
	0x3C, // 31 .
	0x13, // 32 0
	0x12, // 33 9
	0x20, // 34 O
	0x1F, // 35 I
	0x2E, // 36 L
	0x2D, // 37 K
	0x3A, // 38 M
	0x3B, // 39 ,
	0x11, // 40 8
	0x10, // 41 7
	0x1E, // 42 U
	0x1D, // 43 Y
	0x2B, // 44 H
	0x2C, // 45 J
	0x39, // 46 N
	0x41, // 47 space
	0x0F, // 48 6
	0x0E, // 49 5
	0x1B, // 50 R
	0x1C, // 51 T
	0x2A, // 52 G
	0x29, // 53 F
	0x38, // 54 B
	0x37, // 55 V
	0x0D, // 56 4
	0x0C, // 57 3
	0x1A, // 58 E
	0x19, // 59 W
	0x27, // 60 S
	0x28, // 61 D
	0x36, // 62 C
	0x35, // 63 X
	0x0A, // 64 1
	0x0B, // 65 2
	0x09, // 66 Escape
	0x18, // 67 Q
	0x17, // 68 Tab
	0x26, // 69 A
	0x42, // 70 Caps lock
	0x34, // 71 Z
	NOUS, // 72 joystick on cpc, unasigned
	NOUS, // 73 joystick on cpc, unasigned
	NOUS, // 74 joystick on cpc, unasigned
	NOUS, // 75 joystick on cpc, unasigned
	NOUS, // 76 joystick on cpc, unasigned
	NOUS, // 77 joystick on cpc, unasigned
	NOUS, // 78 Inexistent
	0x16, // 79 Backspace
};

#elif defined BLASSIC_USE_WINDOWS

const unsigned int NOUS= 0x1FF;

const unsigned int inkeytocode [MAXINKEYCODE + 1]={
	// The key symbols indicated are the corresponding
	// to a spanish keyboard.
	0x008, //  0 Up
	0x00D, //  1 Right
	0x068, //  2 Down
	0x049, //  3 Numeric 9
	0x04d, //  4 Numeric 6
	0x051, //  5 Numeric 3
	0x11C, //  6 Intro
	0x05B, //  7 Numeric .
	0x064, //  8 Left
	0x000, //  9 Copy
	0x047, // 10 Numeric 7
	0x048, // 11 Numeric 8
	0x04C, // 12 Numeric 5
	0x04F, // 13 Numeric 1
	0x050, // 14 Numeric 2
	0x052, // 15 Numeric 0
	0x153, // 16 Delete
	0x01B, // 17 +
	0x01C, // 18 Return
	0x02B, // 19 	0x04B, // 20 Numeric 4
	0x02A, // 21 Shift
	0x056, // 22 \ on the CPC, <>
	0x01D, // 23 Control
	0x00D, // 24 	0x00C, // 25 '
	0x01A, // 26 `[
	0x019, // 27 P
	0x028, // 28 '{
	0x027, // 29 	0x035, // 30 -
	0x034, // 31 .
	0x00B, // 32 0
	0x00A, // 33 9
	0x018, // 34 O
	0x017, // 35 I
	0x026, // 36 L
	0x025, // 37 K
	0x032, // 38 M
	0x033, // 39 ,
	0x009, // 40 8
	0x008, // 41 7
	0x016, // 42 U
	0x015, // 43 Y
	0x023, // 44 H
	0x024, // 45 J
	0x031, // 46 N
	0x039, // 47 space
	0x007, // 48 6
	0x006, // 49 5
	0x013, // 50 R
	0x014, // 51 T
	0x022, // 52 G
	0x021, // 53 F
	0x030, // 54 B
	0x02F, // 55 V
	0x005, // 56 4
	0x004, // 57 3
	0x012, // 58 E
	0x011, // 59 W
	0x01F, // 60 S
	0x020, // 61 D
	0x02E, // 62 C
	0x02D, // 63 X
	0x002, // 64 1
	0x003, // 65 2
	0x001, // 66 Escape
	0x010, // 67 Q
	0x00F, // 68 Tab
	0x01E, // 69 A
	0x03A, // 70 Caps lock
	0x02C, // 71 Z
	NOUS,  // 72 joystick on cpc, unasigned
	NOUS,  // 73 joystick on cpc, unasigned
	NOUS,  // 74 joystick on cpc, unasigned
	NOUS,  // 75 joystick on cpc, unasigned
	NOUS,  // 76 joystick on cpc, unasigned
	NOUS,  // 77 joystick on cpc, unasigned
	NOUS,  // 78 Inexistent
	0x00E, // 79 Backspace
};

#endif

void keycode_press (unsigned int keycode)
{
	//std::cerr << "Pressed " << std::hex << std::setfill ('0') <<
	//	std::setw (3) << keycode << std::endl;
	if (keycode <= MAXKEYCODE)
		keycode_pressed [keycode]= true;
}

void keycode_release (unsigned int keycode)
{
	//std::cerr << "Released " << std::hex << std::setfill ('0') <<
	//	std::setw (3) << keycode << std::endl;
	if (keycode <= MAXKEYCODE)
		keycode_pressed [keycode]= false;
}

int keypressed (int keynum)
{
	if (keynum < 0 || keynum > static_cast <int> (MAXINKEYCODE) )
		return -1;
	graphics::idle ();
	//if (! keypressedmap [presscode [keynum] ] )
	//	return -1;

	const int shiftpressed= 32, ctrlpressed= 128;

	#ifdef BLASSIC_USE_X
	const unsigned int
		shift_left= 0x32, shift_right= 0x3E,
		control_left= 0x25, control_right= 0x6D;
	#else
	const unsigned int
		shift_left= 0x2A, shift_right= 0x36,
		control_left= 0x1D, control_right= 0x11D;
	#endif

	bool shifted= keycode_pressed [shift_left] ||
		keycode_pressed [shift_right];
	bool controled= keycode_pressed [control_left] ||
		keycode_pressed [control_right];

	bool pressed;
	switch (keynum) {
	case 21:
		pressed= shifted;
		break;
	case 23:
		pressed= controled;
		break;
	default:
		pressed= keycode_pressed [inkeytocode [keynum] ];
	}
	if (! pressed)
		return -1;
	int r= 0;
	//if (keypressedmap [XK_Shift_L] || keypressedmap [XK_Shift_R] )
	if (shifted)
		r|= shiftpressed;
	//if (keypressedmap [XK_Control_L] || keypressedmap [XK_Control_R] )
	if (controled)
		r|= ctrlpressed;

	return r;
}

QueueKey queuekey;

//#endif
//// BLASSIC_HAS_GRAPHICS

bool inited= false;
bool window_created= false;
bool opaquemode= true;

int xmousepos, ymousepos;

const int text_mode= 0, user_mode= -1;

bool graphics_mode_active= false;
int actualmode= text_mode;

int screenwidth, screenheight;
int realwidth, realheight;
int originx= 0, originy= 0;

bool limited= false;
int limit_minx, limit_miny, limit_maxx, limit_maxy;

enum RotateMode { RotateNone, Rotate90 };
RotateMode rotate= RotateNone;

template <class C>
inline void do_rotate (C & x, C & y)
{
	switch (rotate)
	{
	case RotateNone:
		// Nothing to do.
		break;
	case Rotate90:
		{
			//int newx= y;
			C newx= y;
			//y= screenheight - x;
			y= static_cast <C> (screenwidth - x - 1);
			x= newx;
		}
		break;
	}
}

template <class C>
inline void do_unrotate (C & x, C & y)
{
	switch (rotate)
	{
	case RotateNone:
		// Nothing to do.
		break;
	case Rotate90:
		{
			//int newy= x;
			C newy= x;
			x= static_cast <C> (screenwidth - y - 1);
			y= newy;
		}
		break;
	}
}

template <class C>
inline void do_rotate_rel (C & x, C & y)
{
	switch (rotate)
	{
	case RotateNone:
		// Nothing to do.
		break;
	case Rotate90:
		{
			std::swap (x, y);
		}
		break;
	}
}

enum TransformType { TransformIdentity, TransformInvertY };

TransformType activetransform= TransformIdentity;

inline void transform_x (int & x)
{
	x+= originx;
}

inline int transform_inverse_x (int x)
{
	return x - originx;
}

inline void adjust_y (int & y)
{
	switch (activetransform)
	{
	case TransformIdentity:
		break; // Nothing to do
	case TransformInvertY:
		y= screenheight - 1 - y;
		break;
	}
}

inline void transform_y (int & y)
{
	y+= originy;
	adjust_y (y);
}

inline int transform_inverse_y (int y)
{
	switch (activetransform)
	{
	case TransformIdentity:
		break; // Nothing to do
	case TransformInvertY:
		y= screenheight - 1 - y;
		break;
	}
	return y - originy;
}

inline void set_origin (int x, int y)
{
	originx= x;
	//adjust_y (y);
	originy= y;
}

void clear_limits ()
{
	limited= false;
}

void set_limits (int minx, int maxx, int miny, int maxy)
{
	limited= true;
	if (minx > maxx)
		std::swap (minx, maxx);
	limit_minx= minx;
	limit_maxx= maxx;
	adjust_y (miny);
	adjust_y (maxy);
	if (miny > maxy)
		std::swap (miny, maxy);
	limit_miny= miny;
	limit_maxy= maxy;
	if (limit_minx <= 0 && limit_maxx >= screenwidth - 1 &&
		limit_miny <= 0 && limit_maxy >= screenheight - 1)
	{
		limited= false;
	}
}

inline bool check_limit (int x, int y)
{
	if (x < 0 || y < 0 || x >= screenwidth || y >= screenheight)
		return false;
	return (! limited) || (x >= limit_minx && x <= limit_maxx &&
		y >= limit_miny && y <= limit_maxy);
}

#ifdef BLASSIC_USE_SVGALIB

bool svgalib= false;

#else

//const bool svgalib= false;

#endif

int lastx, lasty;

#if defined BLASSIC_USE_X

static const int
	drawmode_copy= GXcopy,
	drawmode_xor= GXxor,
	drawmode_and= GXand,
	drawmode_or= GXor,
	drawmode_invert= GXinvert;

#elif defined BLASSIC_USE_WINDOWS

static const int
	drawmode_copy= R2_COPYPEN,
	drawmode_xor= R2_XORPEN,
	// Revisar los valores para and y or.
	drawmode_and= R2_MASKPEN,
	drawmode_or= R2_MERGEPEN,
	drawmode_invert= R2_NOT;

#endif

//#ifdef BLASSIC_HAS_GRAPHICS

int drawmode= drawmode_copy;

// Numeric draw modes:
// 0: normal copy mode.
// 1: XOR
// 2: AND
// 3: OR
// 0 to 3 are Amstrad CPC modes.
// 4: INVERT, NOT.

static int drawmodesbynumber []= { drawmode_copy, drawmode_xor,
	drawmode_and, drawmode_or, drawmode_invert };

int getdrawmode (int mode)
{
	if (mode < 0 || size_t (mode) >= util::dim_array (drawmodesbynumber) )
		throw ErrFunctionCall;
	return drawmodesbynumber [mode];
}

#ifdef BLASSIC_USE_WINDOWS

static int bitbltmodesbynumber []= { SRCCOPY, SRCINVERT,
	SRCAND, SRCPAINT, DSTINVERT };

int getbitbltmode (int mode)
{
	if (mode < 0 ||
		size_t (mode) >= util::dim_array (bitbltmodesbynumber) )
	{
		throw ErrFunctionCall;
	}
	return bitbltmodesbynumber [mode];
}


#endif
// BLASSIC_USE_WINDOWS

//#endif
//// BLASSIC_HAS_GRAPHICS

const int BASIC_COLORS= 16;

const int LancelotsFavouriteColour= 0x0204FB;
// http://mindprod.com/unmainnaming.html

bool colors_inited= false;

struct ColorRGB {
	int r;
	int g;
	int b;
};

const ColorRGB assignRGB []= {
	{    0,    0,    0 },
	{    0,    0, 0xA8 },
	{    0, 0xA8,    0 },
	{    0, 0xA8, 0xA8 },
	{ 0xA8,    0,    0 },
	{ 0xA8,    0, 0xA8 },
	{ 0xA8, 0x54,    0 },
	{ 0xA8, 0xA8, 0xA8 },

	{ 0x54, 0x54, 0x54 },
	{ 0x54, 0x54, 0xFF },
	{ 0x54, 0xFF, 0x54 },
	{ 0x54, 0xFF, 0xFF },
	{ 0xFF, 0x54, 0x54 },
	{ 0xFF, 0x54, 0xFF },
	{ 0xFF, 0xFF, 0x54 },
	{ 0xFF, 0xFF, 0xFF }
};

//#ifdef BLASSIC_HAS_GRAPHICS

struct ColorInUse {
	pcolor pc;
	ColorRGB rgb;
};

#ifdef BLASSIC_USE_X

ColorValue getColorValue (const ColorInUse & c)
{
	return c.pc->pixel;
}

#elif defined BLASSIC_USE_WINDOWS

ColorValue getColorValue (const ColorInUse & c)
{
	return RGB (c.rgb.r, c.rgb.g, c.rgb.b);
}

#endif

typedef std::map <int, ColorInUse> definedcolor_t;

definedcolor_t definedcolor;

ColorInUse tablecolors []=
{
	{ &xcBlack,        { 0, 0, 0} },
	{ &xcBlue,         { 0, 0, 0} },
	{ &xcGreen,        { 0, 0, 0} },
	{ &xcCyan,         { 0, 0, 0} },
	{ &xcRed,          { 0, 0, 0} },
	{ &xcMagenta,      { 0, 0, 0} },
	{ &xcBrown,        { 0, 0, 0} },
	{ &xcLightGrey,    { 0, 0, 0} },

	{ &xcDarkGrey,     { 0, 0, 0} },
	{ &xcLightBlue,    { 0, 0, 0} },
	{ &xcLightGreen,   { 0, 0, 0} },
	{ &xcLightCyan,    { 0, 0, 0} },
	{ &xcLightRed,     { 0, 0, 0} },
	{ &xcLightMagenta, { 0, 0, 0} },
	{ &xcYellow,       { 0, 0, 0} },
	{ &xcWhite,        { 0, 0, 0} }
};

inline ColorInUse & mapcolor (int color)
{
	if (color >= 0 && color < BASIC_COLORS)
		return tablecolors [color];
	definedcolor_t::iterator it= definedcolor.find (color);
	if (it != definedcolor.end () )
		return it->second;
	return tablecolors [0];
}

inline ColorInUse & mapnewcolor (int color)
{
	if (color >= 0 && color < BASIC_COLORS)
		return tablecolors [color];
	definedcolor_t::iterator it= definedcolor.find (color);
	if (it != definedcolor.end () )
		return it->second;
	ColorInUse n= { new color_t, { 0, 0, 0} };
	return definedcolor.insert (std::make_pair (color, n) ).first->second;
}

void setink (int inknum, const ColorRGB & rgb)
{
	ColorInUse & ciu= mapnewcolor (inknum);

	#ifdef BLASSIC_USE_WINDOWS

	ASSERT (hdcPixmap);
	COLORREF newcolor=
		GetNearestColor (hdcPixmap, RGB (rgb.r, rgb.g, rgb.b) );
	ciu.rgb.r= GetRValue (newcolor);
	ciu.rgb.g= GetGValue (newcolor);
	ciu.rgb.b= GetBValue (newcolor);
	HPEN newpen= CreatePen (PS_SOLID, 1, newcolor);
	if (ciu.pc == pforeground)
	{
		//SelectObject (hdc, * ciu.pc);
		//SelectObject (hdcPixmap, * ciu.pc);
		SelectObject (hdc, newpen);
		SelectObject (hdcPixmap, newpen);
	}
	if (* ciu.pc != NULL)
		DeleteObject (* ciu.pc);
	* ciu.pc= newpen;

	#elif defined BLASSIC_USE_X

	ciu.rgb= rgb;
	ASSERT (display);
	Colormap cm= DefaultColormap (display, screen);
	XColor xc;
	std::ostringstream namecolor;
	namecolor << "rgb:" << std::hex << std::setfill ('0') <<
		std::setw (2) << rgb.r << '/' <<
		std::setw (2) << rgb.g << '/' <<
		std::setw (2) << rgb.b;
	XColor newpen;
	XAllocNamedColor (display, cm,
		namecolor.str ().c_str (), & newpen, & xc);
	if (ciu.pc == pforeground)
	{
		//XSetForeground (display, gcp, ciu.pc->pixel);
		//XSetForeground (display, gc, ciu.pc->pixel);
		XSetForeground (display, gcp, newpen.pixel);
		XSetForeground (display, gc, newpen.pixel);
	}
	// Not sure if previous color needs to be freed and why.
	* ciu.pc= newpen;

	#endif
}

void setcpcink (int inknum, int cpccolor)
{
	// These rgb values are taken from screen captures
	// of the WinAPE2 Amstrad CPC emulator.
	static const ColorRGB cpctable []= {
		{   0,   0,   0 }, // Black
		{   0,   0,  96 }, // Blue
		{   0,   0, 255 }, // Bright blue
		{  96,   0,   0 }, // Red
		{  96,   0,  96 }, // Magenta
		{  96,   0, 255 }, // Mauve
		{ 255,   0,   0 }, // Bright red
		{ 255,   0,  96 }, // Purple
		{ 255,   0, 255 }, // Bright magenta
		{   0, 103,   0 }, // Green
		{   0, 103,  96 }, // Cyan
		{   0, 103, 255 }, // Sky blue
		{  96, 103,   0 }, // Yellow
		{  96, 103,  96 }, // White
		{  96, 103, 255 }, // Pastel blue
		{ 255, 103,   0 }, // Orange
		{ 255, 103,  96 }, // Pink
		{ 255, 103, 255 }, // Pastel magenta
		{   0, 255,   0 }, // Bright green
		{   0, 255,  96 }, // Sea green
		{   0, 255, 255 }, // Bright cyan
		{  96, 255,   0 }, // Lime green
		{  96, 255,  96 }, // Pastel green
		{  96, 255, 255 }, // Pastel cyan
		{ 255, 255,   0 }, // Bright yellow
		{ 255, 255,  96 }, // Pastel yellow
		{ 255, 255, 255 }, // Brigth white
	};
	ASSERT (cpccolor >= 0 &&
		cpccolor < static_cast <int> ( util::dim_array (cpctable) ) );
	const ColorRGB & rgb= cpctable [cpccolor];

	setink (inknum, rgb);
}

void cpc_default_inks ()
{
	static const int default_ink []=
		{ 1, 24, 20, 6, 26, 0, 2, 8, 10, 12, 14, 16, 18, 22, 1, 16 };
	// The last two are blinking on the CPC, we use the first color.
	for (int i= 0; i < int (util::dim_array (default_ink) ); ++i)
	//for (int i= 0; i < 16; ++i)
		setcpcink (i, default_ink [i] );
}

void spectrum_inks ()
{
	// Taken from a screen capture of the Spectaculator Spectrum Emulator.
	static const ColorRGB spectrumtable []=
	{
		{   0,   0,   0 }, // Black
		{   0,   0, 207 }, // Blue
		{ 207,   0,   0 }, // Red
		{ 207,   0, 207 }, // Magenta
		{   0, 200,   0 }, // Green
		{   0, 200, 207 }, // Cyan
		{ 207, 200,   0 }, // Yellow
		{ 207, 200, 207 }, // White
		{   0,   0,   0 }, // Black bright
		{   0,   0, 255 }, // Blue bright
		{ 255,   0,   0 }, // Red bright
		{ 255,   0, 255 }, // Magenta bright
		{   0, 248,   0 }, // Green bright
		{   0, 248, 255 }, // Cyan bright
		{ 255, 248,   0 }, // Yellow bright
		{ 255, 248, 255 }, // White bright
	};
	for (int i= 0; i < 16; ++i)
		setink (i, spectrumtable [i] );
}

enum Inkset { InkStandard, InkCpc, InkSpectrum } inkset= InkStandard;

void init_colors ()
{
	TRACEFUNC (tr, "init_colors");

	ASSERT (sizeof (assignRGB) / sizeof (ColorRGB) == BASIC_COLORS);
	ASSERT (sizeof (tablecolors) / sizeof (ColorInUse) == BASIC_COLORS);

	switch (inkset)
	{
	case InkStandard:
		for (int i= 0; i < BASIC_COLORS; ++i)
		{
			const ColorRGB & rgb= assignRGB [i];
			setink (i, rgb);
		}
		break;
	case InkCpc:
		cpc_default_inks ();
		break;
	case InkSpectrum:
		spectrum_inks ();
		break;
	}
	colors_inited= true;
	pforeground= mapcolor (default_pen).pc;
	pbackground= mapcolor (default_paper).pc;
}

void reinit_pixmap ()
{
	#ifdef BLASSIC_USE_WINDOWS

	//RECT r = { 0, 0, screenwidth, screenheight };
	RECT r = { 0, 0, realwidth, realheight };
	FillRect (hdcPixmap, & r, (HBRUSH) GetStockObject (WHITE_BRUSH) );

	#elif defined BLASSIC_USE_X

	XSetForeground (display, gcp, WhitePixel (display, screen) );
	XFillRectangle (display, pixmap, gcp, 0, 0, realwidth, realheight);
	XSetForeground (display, gcp, BlackPixel (display, screen) );

	#endif
}

void reinit_window ()
{
	#ifdef BLASSIC_USE_WINDOWS

	//BitBlt (hdc, 0, 0, screenwidth, screenheight, hdcPixmap,
	BitBlt (hdc, 0, 0, realwidth, realheight, hdcPixmap,
		0, 0, SRCCOPY);

	#elif defined BLASSIC_USE_X

	XSetFunction (display, gc, drawmode_copy);
	XCopyArea (display, pixmap, window, gc,
		//0, 0, screenwidth, screenheight, 0, 0);
		0, 0, realwidth, realheight, 0, 0);
	//XSetForeground (display, gc, BlackPixel (display, screen) );
	//XSetForeground (display, gc, pforeground->pixel);
	XFlush (display);
	XSetFunction (display, gc, drawmode);

	#endif
}

void reinit_window (int x, int y, int width, int height)
{
	#ifdef BLASSIC_USE_WINDOWS

	BitBlt (hdc, x, y, width, height, hdcPixmap,
		x, y, SRCCOPY);

	#elif defined BLASSIC_USE_X

	XSetFunction (display, gc, drawmode_copy);
	XCopyArea (display, pixmap, window, gc,
		x, y, width, height, x, y);
	//XSetForeground (display, gc, BlackPixel (display, screen) );
	//XSetForeground (display, gc, pforeground->pixel);
	XFlush (display);
	XSetFunction (display, gc, drawmode);

	#endif
}

#ifdef BLASSIC_USE_WINDOWS

const UINT
	WM_USER_CREATE_WINDOW= WM_USER + 3,
	WM_USER_DESTROY_WINDOW= WM_USER + 4;

HANDLE hthread= NULL;
DWORD idthread= 0;

inline unsigned int getkeycode (LPARAM lParam)
{
	return (lParam & 0x001FF0000) >> 16;
}

LRESULT APIENTRY windowproc
	(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int width, height;
	switch (uMsg) {
	case WM_SIZE:
		width= LOWORD (lParam);
		height= HIWORD (lParam);
		return TRUE;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
		{
		//err << "WM_PAINT " << width << ", " << height << endl;
		PAINTSTRUCT paintstruct;
		HDC hdc= BeginPaint (hwnd, & paintstruct);
		BitBlt (hdc, 0, 0, width, height, hdcPixmap, 0, 0, SRCCOPY);
		EndPaint (hwnd, & paintstruct);
		}
		return FALSE;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		keycode_press (getkeycode (lParam) );
		{
			WORD k= (WORD) wParam;
			//std::string str= string_from_virtual_key (k);
			switch (rotate)
			{
			case RotateNone:
				// Nothing to do.
				break;
			case Rotate90:
				switch (k)
				{
				case VK_LEFT:
					k= VK_UP; break;
				case VK_UP:
					k= VK_RIGHT; break;
				case VK_RIGHT:
					k= VK_DOWN; break;
				case VK_DOWN:
					k= VK_LEFT; break;
				}
				break;
			}
			std::string str= string_from_key (k);
			if (! str.empty () )
			{
				queuekey.push (str);
				return TRUE;
			}
		}
		return FALSE;
	case WM_KEYUP:
		keycode_release (getkeycode (lParam) );
		return FALSE;
//	case WM_SYSKEYDOWN:
//		keycode_press (getkeycode (lParam) );
//		return FALSE;
	case WM_SYSKEYUP:
		keycode_release (getkeycode (lParam) );
		return FALSE;
	case WM_CHAR:
		{
			char c= (char) wParam;
			queuekey.push (std::string (1, c) );
		}
		return TRUE;
	case WM_MOUSEMOVE:
		xmousepos= LOWORD (lParam);
		ymousepos= HIWORD (lParam);
		do_unrotate (xmousepos, ymousepos);
		return TRUE;
	case WM_LBUTTONDOWN:
		queuekey.push (strCLICK);
		return TRUE;
	case WM_RBUTTONDOWN:
		queuekey.push (strSCLICK);
		return TRUE;
	case WM_LBUTTONUP:
		queuekey.push (strRELEASE);
		return TRUE;
	case WM_RBUTTONUP:
		queuekey.push (strSRELEASE);
		return TRUE;
	case WM_DESTROY:
		SetEvent (hEvent);
		return TRUE;
	default:
		return DefWindowProc (hwnd, uMsg, wParam, lParam);
	}
}

class ProtectWindow {
	HWND hwnd;
public:
	ProtectWindow (HWND hwnd) : hwnd (hwnd) { }
	~ProtectWindow ()
	{
		if (hwnd)
			DestroyWindow (hwnd);
	}
	void release ()
	{
		hwnd= 0;
	}
};

class ProtectPixmap {
	HBITMAP & pixmap;
	bool active;
public:
	ProtectPixmap (HBITMAP & pixmap) : pixmap (pixmap), active (true) { }
	~ProtectPixmap ()
	{
		if (active)
		{
			DeleteObject (pixmap);
			pixmap= NULL;
		}
	}
	void release ()
	{
		active= false;
	}
};

void thread_create_window (int width, int height)
{
	window= CreateWindow (
		LPCTSTR (atomClass),
		default_title.c_str (),
		/*WS_VISIBLE | */ WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0,
		width + GetSystemMetrics (SM_CXDLGFRAME) * 2,
		height + GetSystemMetrics (SM_CYDLGFRAME) * 2 +
			GetSystemMetrics (SM_CYCAPTION),
		NULL,
		NULL,
		GetModuleHandle (0),
		0);
	if (window)
	{
		ProtectWindow pw (window);
		hdc= GetDC (window);
		if (hdc == NULL)
			return;
		pixmap= CreateCompatibleBitmap (hdc, width, height);
		if (pixmap == NULL)
			return;
		ProtectPixmap pp (pixmap);
		hdcPixmap= CreateCompatibleDC (hdc);
		if (hdcPixmap == NULL)
			return;
		SelectObject (hdcPixmap, pixmap);
		init_colors ();
		reinit_pixmap ();
		window_created= true;
		ShowWindow (window, SW_SHOWNORMAL);
		//SetActiveWindow (window);
		SetForegroundWindow (window);
		pw.release ();
		pp.release ();
	}
}

// Testing new method of create and destroy windows.

struct ThreadParams {
	int width, height;
};

unsigned WINAPI threadproc (void * arg)
{
	MSG msg;
	ThreadParams * tp= reinterpret_cast <ThreadParams *> (arg);
	thread_create_window (tp->width, tp->height);
	// Ensure the message queue exist before the main thread continues.
	// Perhpas unnecesary with the new method, but...
	PeekMessage (& msg, NULL, 0, UINT (-1), PM_NOREMOVE);
	SetEvent (hEvent);
	if (! window_created)
		return 0;
	while (GetMessage (& msg, NULL, 0, 0) )
	{
		switch (msg.message)
		{
		case WM_USER_CREATE_WINDOW:
			//thread_create_window (msg.wParam, msg.lParam);
			SetEvent (hEvent);
			break;
		case WM_USER_DESTROY_WINDOW:
			if (DestroyWindow (window) == 0)
				cerr << "Error destroying: " <<
					GetLastError () << endl;
			break;
		default:
			TranslateMessage (& msg);
			DispatchMessage (& msg);
		}
	}
	return 0;
}

void create_thread (int width, int height)
{
	ThreadParams tp= { width, height };
	hthread= HANDLE (_beginthreadex (NULL, 0, threadproc,
		& tp, 0, (unsigned int *) (& idthread) ) );
	if (hthread == NULL)
	{
		if (showdebuginfo () )
			cerr << "Error creating graphics thread" << endl;
		throw ErrBlassicInternal;
	}
	WaitForSingleObject (hEvent, INFINITE);
	if (! window_created)
	{
		WaitForSingleObject (hthread, INFINITE);
		CloseHandle (hthread);
		idthread= 0;
		hthread= NULL;
	}
}

void destroy_thread ()
{
	if (idthread)
	{
		BOOL r= PostThreadMessage (idthread, WM_QUIT, 0, 0);
		if (r == 0)
		{
			TerminateThread (hthread, 0);
		}
		else
		{
			WaitForSingleObject (hthread, INFINITE);
		}
		CloseHandle (hthread);
		idthread= 0;
		hthread= NULL;
	}
}

#endif // WINDOWS

void create_window (int width, int height)
{
	TRACEFUNC (tr, "create_window");

	#ifdef BLASSIC_USE_WINDOWS

	if (hthread == NULL)
	{
		create_thread (width, height);
		if (hthread == NULL)
			throw ErrBlassicInternal;
	}
	BOOL r= PostThreadMessage (idthread, WM_USER_CREATE_WINDOW,
		WPARAM (width), LPARAM (height) );
	if (r == 0)
	{
		cerr << "Error communicating with graphics thread"
			"GetLastError =" << GetLastError () <<
			endl;
		destroy_thread ();
		throw ErrBlassicInternal;
	}
	WaitForSingleObject (hEvent, INFINITE);
	if (! window_created)
	{
		if (showdebuginfo () )
			cerr << "Error creating window" << endl;
		throw ErrBlassicInternal;
	}

	#elif defined BLASSIC_USE_X

	#if 1
	window= XCreateSimpleWindow (display,
		RootWindow (display, screen),
		0, 0, width, height,
		5, BlackPixel (display, screen),
		WhitePixel (display, screen) );
	#else
	int depth= 8;
	window= XCreateWindow (display,
		RootWindow (display, screen),
		0, 0, width, height,
		5,
		depth,
		InputOutput,
		CopyFromParent,
		0,
		NULL);
	#endif
	window_created= true;

	#if 0
	int depth= DefaultDepth (display, DefaultScreen (display) );
	#else
	unsigned int depth;
	{
		XWindowAttributes attr;
		XGetWindowAttributes (display, window, & attr);
		depth= attr.depth;
	}
	#endif
	pixmap= XCreatePixmap (display, window,
		width, height, depth);
	pixmap_created= true;

	gc= XCreateGC (display, window, 0, & gcvalues);
	gcp= XCreateGC (display, pixmap, 0, & gcvaluesp);
	init_colors ();
	reinit_pixmap ();
	XSetStandardProperties (display, window,
			default_title.c_str (),
			default_title.c_str (),
			None,
			0, 0, NULL);

	eventusedmaskactual= eventusedmask;

	if (xim)
	{
		XIMStyle input_style= XIMPreeditNothing | XIMStatusNothing;
		xic= XCreateIC (xim,
			XNInputStyle, input_style,
			XNClientWindow, window,
			XNFocusWindow, window,
			NULL);
	}
	if (xic != NULL)
	{
		TRMESSAGE (tr, "XIC created");

		long filterevents;
		if (XGetICValues (xic,
				XNFilterEvents, & filterevents,
				NULL)
			== NULL);
		{
			eventusedmaskactual |= filterevents;
		}
	}
	else
	{
		TRMESSAGE (tr, "XIC not created");
	}

	XSelectInput (display, window, eventusedmaskactual);
	XMapWindow (display, window);

	// Wait for window mapping.
	{
		XEvent e;
		do
		{
			XNextEvent (display, & e);
		} while (e.type != MapNotify);
	}

	graphics::idle ();

	#endif
}

void destroy_window ()
{
	#ifdef BLASSIC_USE_WINDOWS

	PostThreadMessage (idthread, WM_USER_DESTROY_WINDOW, 0, 0);
	WaitForSingleObject (hEvent, INFINITE);
	DeleteDC (hdcPixmap);
	DeleteObject (pixmap);
	window= 0;
	window_created= false;
	destroy_thread ();

	#elif defined BLASSIC_USE_X

	XDestroyWindow (display, window);
	window_created= false;
	XFreePixmap (display, pixmap);
	pixmap_created= false;
	XFlush (display);
	graphics::idle ();

	#endif
}

#endif
// BLASSIC_HAS_GRAPHICS

inline void requiregraphics ()
{
	#ifndef BLASSIC_HAS_GRAPHICS

	no_graphics_support ();

	#else

	//if (actualmode == text_mode)
	if (! graphics_mode_active)
		throw ErrNoGraphics;

	#endif
}

std::string program_name;

#ifdef BLASSIC_USE_X

std::string getDISPLAY ()
{
	const char * strdisplay= getenv ("DISPLAY");
	if (strdisplay == NULL)
		return std::string ();
	else
		return strdisplay;
}

std::string last_display;

#endif

void initialize_graphics ()
{
	TRACEFUNC (tr, "initialize_graphics");
	// Does the real initialization of the graphics system.
	// It will not be called until a graphics mode ise
	// established.

	#ifdef BLASSIC_HAS_GRAPHICS

	ASSERT (! inited);

	const bool showfailinfo= showdebuginfo ();

	#ifdef BLASSIC_USE_SVGALIB

	if (geteuid () == 0) {
		std::string prog (program_name);
		std::string::size_type l= prog.size ();
		if (l > 3 && prog.substr (l - 3) == "vga") {
			vga_init ();
			inited= true;
			svgalib= true;
			return;
		}
		else
			if (getuid () != 0)
				seteuid (getuid () );
	}
	#endif

	#ifdef BLASSIC_USE_X

	const char * strDisplay;
	static const char WITHOUT_GRAPHICS []=
		", running without graphics support.";

	if ( (strDisplay= getenv ("DISPLAY") ) != NULL &&
		strDisplay [0] != '\0')
	{
		TRMESSAGE (tr, std::string ("Opening ") + strDisplay);
		display= XOpenDisplay (0);
		if (display)
		{
			last_display= strDisplay;
			TRMESSAGE (tr, "Display opened");
			inited= true;
			XSetLocaleModifiers (""); // Testing.
			XSetLocaleModifiers ("@im=none"); // Testing.
			xim= XOpenIM (display, NULL, NULL, NULL);
			if (xim != NULL)
			{
				TRMESSAGE (tr, "XIM opened");
			}
			screen= DefaultScreen (display);
			//init_xcolors ();
			//init_colors ();
		}
		else
		{
			static const char ERROR_OPEN []=
				"Error opening DISPLAY '";
			TRMESSAGE (tr, std::string (ERROR_OPEN) +
				strDisplay+ '\'');
			if (showfailinfo)
				cerr << ERROR_OPEN <<
					strDisplay << '\'' <<
					WITHOUT_GRAPHICS <<
					endl;
		}
	}
	else
	{
		const char * const message= strDisplay ? "Empty" : "No";
		static const char DISPLAY []= " DISPLAY value";
		TRMESSAGE (tr, std::string (message) + DISPLAY);
		if (showfailinfo)
			cerr << message << DISPLAY << WITHOUT_GRAPHICS <<
				endl;
	}

	#elif defined  BLASSIC_USE_WINDOWS

	WNDCLASS wndclass;
	wndclass.style= CS_NOCLOSE | CS_OWNDC;
	wndclass.lpfnWndProc= windowproc;
	wndclass.cbClsExtra= 0;
	wndclass.cbWndExtra= 0;
	wndclass.hInstance= GetModuleHandle (0);
	wndclass.hIcon= 0;
	wndclass.hCursor= LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground= HBRUSH (GetStockObject (WHITE_BRUSH) );
	wndclass.lpszMenuName= 0;
	wndclass.lpszClassName= program_name.c_str ();
	atomClass= RegisterClass (& wndclass);
	if (atomClass == 0)
	{
		if (showfailinfo)
			cerr << "Error registering class" << endl;
	}
	else
	{
		inited= true;
		//init_wincolors ();
		//init_colors ();
	}
	hEvent= CreateEvent (NULL, FALSE, FALSE, NULL);
	// Event automatic, initial nonsignaled
	//create_thread ();

	#endif

	#else
	// No BLASSIC_HAS_GRAPHICS

	#endif
}

void destroy_text_windows ();

void check_initialized ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	if (! inited)
		initialize_graphics ();
	else
	{
		#ifdef BLASSIC_USE_X
		if (last_display != getDISPLAY () )
		{
			destroy_text_windows ();
			graphics::setmode (text_mode);
			graphics::uninitialize ();
			initialize_graphics ();
		}
		#endif
	}

	#endif
}

} // namespace

void graphics::initialize (const char * progname)
{
	TRACEFUNC (tr, "graphics::initialize");

	// Default symbol after and charset initialization:
	symbolafter (0);

	// Now complete initialization is not done here.
	#if 1

	program_name= progname;

	#else

	#ifdef BLASSIC_HAS_GRAPHICS

	ASSERT (! inited);

	const bool showfailinfo= showdebuginfo ();

	#ifdef BLASSIC_USE_SVGALIB

	if (geteuid () == 0) {
		std::string prog (progname);
		std::string::size_type l= prog.size ();
		if (l > 3 && prog.substr (l - 3) == "vga") {
			vga_init ();
			inited= true;
			svgalib= true;
			return;
		}
		else
			if (getuid () != 0)
				seteuid (getuid () );
	}
	#endif

	#ifdef BLASSIC_USE_X

	touch (progname);
	const char * strDisplay;
	static const char WITHOUT_GRAPHICS []=
		", running without graphics support.";

	if ( (strDisplay= getenv ("DISPLAY") ) != NULL &&
		strDisplay [0] != '\0')
	{
		TRMESSAGE (tr, std::string ("Opening ") + strDisplay);
		display= XOpenDisplay (0);
		if (display)
		{
			TRMESSAGE (tr, "Display opened");
			inited= true;
			XSetLocaleModifiers (""); // Testing.
			XSetLocaleModifiers ("@im=none"); // Testing.
			xim= XOpenIM (display, NULL, NULL, NULL);
			if (xim != NULL)
			{
				TRMESSAGE (tr, "XIM opened");
			}
			screen= DefaultScreen (display);
			//init_xcolors ();
			//init_colors ();
		}
		else
		{
			static const char ERROR_OPEN []=
				"Error opening DISPLAY '";
			TRMESSAGE (tr, std::string (ERROR_OPEN) +
				strDisplay+ '\'');
			if (showfailinfo)
				cerr << ERROR_OPEN <<
					strDisplay << '\'' <<
					WITHOUT_GRAPHICS <<
					endl;
		}
	}
	else
	{
		const char * const message= strDisplay ? "Empty" : "No";
		static const char DISPLAY []= " DISPLAY value";
		TRMESSAGE (tr, std::string (message) + DISPLAY);
		if (showfailinfo)
			cerr << message << DISPLAY << WITHOUT_GRAPHICS <<
				endl;
	}

	#elif defined  BLASSIC_USE_WINDOWS

	WNDCLASS wndclass;
	wndclass.style= CS_NOCLOSE | CS_OWNDC;
	wndclass.lpfnWndProc= windowproc;
	wndclass.cbClsExtra= 0;
	wndclass.cbWndExtra= 0;
	wndclass.hInstance= GetModuleHandle (0);
	wndclass.hIcon= 0;
	wndclass.hCursor= LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground= HBRUSH (GetStockObject (WHITE_BRUSH) );
	wndclass.lpszMenuName= 0;
	wndclass.lpszClassName= progname;
	atomClass= RegisterClass (& wndclass);
	if (atomClass == 0)
	{
		if (showfailinfo)
			cerr << "Error registering class" << endl;
	}
	else
	{
		inited= true;
		//init_wincolors ();
		//init_colors ();
	}
	hEvent= CreateEvent (NULL, FALSE, FALSE, NULL);
	// Event automatic, initial nonsignaled
	//create_thread ();

	#endif

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (progname);

	#endif

	#endif
}

void graphics::uninitialize ()
{
	TRACEFUNC (tr, "graphics::uninitialize");

	#ifdef BLASSIC_HAS_GRAPHICS

	if (! inited) return;

	//if (actualmode != 0)
	if (graphics_mode_active)
		setmode (0);

	#ifdef BLASSIC_USE_SVGA
	#if 0
	if (svgalib)
		//vga_setmode (TEXT);
		setmode (0);
	#endif
	#endif

	#ifdef BLASSIC_USE_X

	if (display)
	{
		clear_images ();
		TRMESSAGE (tr, "closing display");
		//if (window_created)
		//	destroy_window ();
		if (xic)
			XDestroyIC (xic);
		if (xim)
			XCloseIM (xim);
		XCloseDisplay (display);
		TRMESSAGE (tr, "display is closed");
		display= 0;
	}

	#elif defined BLASSIC_USE_WINDOWS

	//destroy_thread ();
	//if (window_created)
	//	destroy_window ();

	if (atomClass)
		UnregisterClass (LPCTSTR (atomClass),
			GetModuleHandle (0) );
	#endif

	inited= false;

	#endif
	// BLASSIC_HAS_GRAPHICS
}

void graphics::origin (int x, int y)
{
	TRACEFUNC (tr, "graphics::origin");

	#ifdef BLASSIC_HAS_GRAPHICS

	set_origin (x, y);

	#else

	touch (x, y);
	no_graphics_support ();

	#endif
}

void graphics::limits (int minx, int maxx, int miny, int maxy)
{
	TRACEFUNC (tr, "graphics::limits");

	#ifdef BLASSIC_HAS_GRAPHICS

	set_limits (minx, maxx, miny, maxy);

	#else

	touch (minx, maxx, miny, maxy);
	no_graphics_support ();

	#endif
}

void graphics::ink (int inknum, int cpccolor)
{
	requiregraphics ();

	// Check not needed, is done in setcpcink.
	//if (cpccolor < 0 || cpccolor > 26)
	//	throw ErrFunctionCall;

	#ifdef BLASSIC_HAS_GRAPHICS

	setcpcink (inknum, cpccolor);

	#else

	touch (inknum, cpccolor);

	#endif
}

void graphics::ink (int inknum, int r, int g, int b)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	ColorRGB rgb= { r, g, b };
	setink (inknum, rgb);

	#else

	touch (inknum, r, g, b);

	#endif
}

void graphics::clearink ()
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	init_colors ();

	#endif
}

namespace {

#ifdef BLASSIC_HAS_GRAPHICS

#ifdef BLASSIC_USE_X

void keypress (XKeyPressedEvent & xk)
{
	TRACEFUNC (tr, "keypress");

	KeySym ks= 0;
	const int STRBUFSIZE= 500; // Value used in xterm.
	char buffer [STRBUFSIZE];
	int r;
	if (xic != NULL)
	{
		Status status;
		r= XmbLookupString (xic, & xk, buffer, STRBUFSIZE,
			& ks, & status);
	}
	else
	{
		r= XLookupString (& xk, buffer, STRBUFSIZE - 1,
			& ks, NULL);
	}

	// Change cursor keys if rotated.
	switch (rotate)
	{
	case RotateNone:
		// Nothing to do.
		break;
	case Rotate90:
		switch (ks)
		{
		case XK_Left:
			ks= XK_Up; break;
		case XK_Up:
			ks= XK_Right; break;
		case XK_Right:
			ks= XK_Down; break;
		case XK_Down:
			ks= XK_Left; break;

		case XK_KP_Left:
			ks= XK_KP_Up; break;
		case XK_KP_Up:
			ks= XK_KP_Right; break;
		case XK_KP_Right:
			ks= XK_KP_Down; break;
		case XK_KP_Down:
			ks= XK_KP_Left; break;
		}
		break;
	}
	#ifndef NDEBUG
	{
		std::ostringstream oss;
		//oss << std::hex << std::setw (4) << ks;
		oss << std::hex << std::setw (4) << xk.keycode;
		//if (r > 0)
			oss << " CHAR: " << buffer [0];
		TRMESSAGE (tr, oss.str () );
	}
	#endif
	//set_pressed (ks);

	#if 0
	std::string str= string_from_key (ks);
	#else

	// I don't know why XmbLookupString does not return
	// the Euro sign as string, this is a dirty solution.
	std::string str;
	#ifdef XK_EuroSign
	if (ks == XK_EuroSign)
	{
		const char eurochar (164);
		str= std::string (1, eurochar);
	}
	else

	#endif
		str= string_from_key (ks);
	#endif

	if (! str.empty () )
	{
		TRMESSAGE (tr, std::string ("key") + str);
		queuekey.push (str);
	}
	else
		if (r > 0)
		{
			TRMESSAGE (tr, std::string ("key: ") +
				std::string (buffer, r) );
			queuekey.push (std::string (buffer, r) );
		}
}

void keyrelease (XKeyReleasedEvent & xk)
{
	KeySym ks= 0;
	const int STRBUFSIZE= 500; // Value used in xterm.
	char buffer [STRBUFSIZE];
	int r;
	if (xic == NULL)
	{
		r= XLookupString (& xk, buffer, STRBUFSIZE - 1,
			& ks, NULL);
	}

	// Change cursor keys if rotated.
	switch (rotate)
	{
	case RotateNone:
		// Nothing to do.
		break;
	case Rotate90:
		switch (ks)
		{
		case XK_Left:
			ks= XK_Up; break;
		case XK_Up:
			ks= XK_Right; break;
		case XK_Right:
			ks= XK_Down; break;
		case XK_Down:
			ks= XK_Left; break;

		case XK_KP_Left:
			ks= XK_KP_Up; break;
		case XK_KP_Up:
			ks= XK_KP_Right; break;
		case XK_KP_Right:
			ks= XK_KP_Down; break;
		case XK_KP_Down:
			ks= XK_KP_Left; break;
		}
		break;
	}
	//reset_pressed (ks);
}

void process_event (XEvent & x_event, bool & do_copy)
{
	// Set the state of pressed keys before checking XFilterEvent.
	switch (x_event.type)
	{
	case KeyPress:
		keycode_press (x_event.xkey.keycode);
		break;
	case KeyRelease:
		keycode_release (x_event.xkey.keycode);
		break;
	}

	if (XFilterEvent (& x_event, window) )
		return;

	switch (x_event.type)
	{
	case Expose:
		do_copy= true;
		break;
	case KeyPress:
		keypress (x_event.xkey);
		break;
	case KeyRelease:
		keyrelease (x_event.xkey);
		break;
	case ButtonPress:
		{
			XButtonEvent & xbpe= x_event.xbutton;
			//cerr << "ButtonPress event, button=" <<
			//	xbpe.button <<
			//	endl;
			switch (xbpe.button)
			{
			case 1:
				queuekey.push (strCLICK);
				break;
			case 3:
				queuekey.push (strSCLICK);
				break;
			default:
				;
			}
		}
		break;
	case ButtonRelease:
		{
			XButtonEvent & xbpe= x_event.xbutton;
			//cerr << "ButtonRelease event, button=" <<
			//	xbpe.button <<
			//	endl;
			switch (xbpe.button)
			{
			case 1:
				queuekey.push (strRELEASE);
				break;
			case 3:
				queuekey.push (strSRELEASE);
				break;
			default:
				;
			}
		}
		break;
	case MotionNotify:
		{
			XMotionEvent & xme= x_event.xmotion;
			//cerr << "MotionNotify event " <<
			//	xme.x << ", " << xme.y <<
			//	endl;
			xmousepos= xme.x;
			ymousepos= xme.y;
			do_unrotate (xmousepos, ymousepos);
		}
		break;
	case EnterNotify:
		{
			XCrossingEvent  & xce= x_event.xcrossing;
			//cerr << "EnterNotify event" << endl;
			xmousepos= xce.x;
			ymousepos= xce.y;
			do_unrotate (xmousepos, ymousepos);
		}
		break;
	default:
		//cerr << "Another event." << endl;
		;
	} // switch type of event
}

void wait_X_event ()
{
	ASSERT (window_created);

	XEvent x_event;
	bool do_copy= false;
	XWindowEvent (display, window, eventusedmaskactual, & x_event);
	process_event (x_event, do_copy);
	if (do_copy && pixmap_created)
		reinit_window ();
}

#endif
// BLASSIC_USE_X

#endif
// BLASSIC_HAS_GRAPHICS

} // namespace

//void graphics::idle ()
void blassic::idle ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	if (! window_created)
		return;

	#ifdef BLASSIC_USE_X

	XEvent x_event;
	bool do_copy= false;
	while (XCheckWindowEvent (display, window,
		eventusedmaskactual, & x_event) )
	{
		process_event (x_event, do_copy);
	} // while
	if (do_copy && pixmap_created)
	{
		#if 0
		XCopyArea (display, pixmap, window, gc,
			0, 0, screenwidth, screenheight, 0, 0);
		XFlush (display);
		#else
		//XSetFunction (display, gc, drawmode_copy);
		reinit_window ();
		//XSetFunction (display, gc, drawmode);
		#endif
		//cerr << "Copied." << endl;
	}

	#endif

	#ifdef BLASSIC_USE_WINDOWS
	//UpdateWindow (window);
	//Sleep (0);
	#endif

	#endif
	// BLASSIC_HAS_GRAPHICS
}

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

void setactivecolor (pcolor pxc)
{
	activecolor= pxc;

	#ifdef BLASSIC_USE_X

	XSetForeground (display, gcp, pxc->pixel);
	XSetForeground (display, gc, pxc->pixel);

	#elif defined BLASSIC_USE_WINDOWS

	SelectObject (hdc, * pxc);
	SelectObject (hdcPixmap, *pxc);

	#endif
}

void textscroll ()
{
	#ifdef BLASSIC_USE_SVGALIB
	if (svgalib)
	{
		// PENDIENTE
		return;
	}
	#endif

	#ifdef BLASSIC_USE_X

	int h= screenheight - 8;
	//unsigned long white= WhitePixel (display, screen),
	//	black= BlackPixel (display, screen);

	XSetFunction (display, gcp, drawmode_copy);
	XCopyArea (display, pixmap, pixmap, gcp,
		0, 8, screenwidth, h, 0, 0);
	setactivecolor (pbackground);
	XFillRectangle (display, pixmap, gcp,
		0, h, screenwidth, 8);
	setactivecolor (pforeground);
	XSetFunction (display, gcp, drawmode);

	if (! fSynchro)
		reinit_window ();

	#elif defined BLASSIC_USE_WINDOWS

	RECT r = { 0, screenheight - 8, screenwidth, screenheight };

	int h= screenheight - 8;
	BitBlt (hdcPixmap, 0, 0, screenwidth, h,
		hdcPixmap, 0, 8, SRCCOPY);
	//HBRUSH hbrush= (HBRUSH) GetStockObject (WHITE_BRUSH);
	LOGPEN logpen;
	GetObject (* pbackground, sizeof (LOGPEN), & logpen);
	HBRUSH hbrush= CreateSolidBrush (logpen.lopnColor);
	FillRect (hdcPixmap, & r, hbrush);
	DeleteObject (hbrush);
	if (! fSynchro)
		reinit_window ();

	#endif

}

void do_fill_rectangle (int x1, int y1, int x2, int y2, bool limitable)
{
	using std::min;
	using std::max;
	if (limitable && limited)
	{
		x1= max (x1, limit_minx);
		y1= max (y1, limit_miny);
		x2= min (x2, limit_maxx);
		y2= min (y2, limit_maxy);
		if (x1 > limit_maxx || x2 < limit_minx ||
				y1 > limit_maxy || y2 < limit_miny)
			return;
	}

	do_rotate (x1, y1);
	do_rotate (x2, y2);
	if (x1 > x2)
		std::swap (x1, x2);
	if (y1 > y2)
		std::swap (y1, y2);

	#ifdef BLASSIC_USE_WINDOWS

	RECT r = { x1, y1, x2 + 1, y2 + 1 };
	LOGPEN logpen;
	//GetObject (* pforeground, sizeof (LOGPEN), & logpen);
	GetObject (* activecolor, sizeof (LOGPEN), & logpen);
	HBRUSH hbrush= CreateSolidBrush (logpen.lopnColor);
	FillRect (hdcPixmap, & r, hbrush);
	if (! fSynchro)
		FillRect (hdc, & r, hbrush);
	DeleteObject (hbrush);

	#elif defined BLASSIC_USE_X

	//int w= std::abs (x2 - x1) + 1;
	//int h= std::abs (y2 - y1) + 1;
	int w= x2 - x1 + 1;
	int h= y2 - y1 + 1;
	XFillRectangle (display, pixmap, gcp,
		x1, y1, w, h);
	if (! fSynchro)
		XFillRectangle (display, window, gc,
			x1, y1, w, h);

	#endif
}

#ifdef BLASSIC_USE_WINDOWS

// Define in windows the struct used in X for XDrawPoints.
struct XPoint {
	short x, y;
};

#endif

inline void do_plot (int x, int y)
{
	if (! check_limit (x, y) )
		return;

	do_rotate (x, y);

	#ifdef BLASSIC_USE_SVGALIB

	if (svgalib)
	{
		vga_drawpixel (x, y);
		return;
	}

	#endif

	#ifdef BLASSIC_USE_X

	if (! fSynchro)
		XDrawPoint (display, window, gc, x, y);
	XDrawPoint (display, pixmap, gcp, x, y);

	#elif defined BLASSIC_USE_WINDOWS

	#ifdef USE_POLY
	POINT p [2]= { {x, y}, {x + 1, y} };
	#endif

	if (! fSynchro)
	{
		#ifdef USE_POLY
		Polyline (hdc, p, 2);
		#else
		MoveToEx (hdc, x, y, 0);
		LineTo (hdc, x + 1, y);
		#endif
	}
	#ifdef USE_POLY
	Polyline (hdcPixmap, p, 2);
	#else
	MoveToEx (hdcPixmap, x, y, 0);
	LineTo (hdcPixmap, x + 1, y);
	#endif

	#endif
}

// This are now not used, every text window has his own
//int tcol, trow;

int maxtcol= 40, maxtrow= 25;

const int MAXZOOMTEXTY= 4;
const int MAXZOOMTEXTX= 4;
//const int MAXZOOMTEXT= std::max (MAXZOOMTEXTX, MAXZOOMTEXTY);
// Borland C++ can't evaluate this at compile time.
const int MAXZOOMTEXT= 4;

// Default values are needed by the initialization of windowzero.
int zoomtextx= 1, zoomtexty= 1;
//int zoomtextxrot= 1, zoomtextyrot= 1;
int charwidth= 8, charheight= 8;
//int charwidthrot= 9, charheightrot= 8;

#ifdef BLASSIC_USE_WINDOWS

// Length of each segment: all are 2 points.
DWORD poly_points [64 * MAXZOOMTEXT];
bool init_poly_points ()
{
	std::fill_n (poly_points, util::dim_array (poly_points), 2);
	return true;
}
bool poly_points_inited= init_poly_points ();

#endif

inline void do_plot_points (XPoint point [], int npoints, bool limitable)
{
	ASSERT (npoints < 64 * zoomtexty);

	if (rotate != RotateNone)
		for (int i= 0; i < npoints; ++i)
			do_rotate (point [i].x, point [i].y);

	#ifdef BLASSIC_USE_X

	if (zoomtextx == 1)
	{
		XDrawPoints (display, pixmap, gcp,
			point, npoints, CoordModeOrigin);
		if (! fSynchro)
			XDrawPoints (display, window, gc,
				point, npoints, CoordModeOrigin);
	}
	else
	{
		// 64 points each char * max zoomtexty
		XSegment seg [64 * MAXZOOMTEXT];
		int inc= zoomtextx - 1;
		int xinc= 0, yinc= 0;
		switch (rotate)
		{
		case RotateNone:
			xinc= inc; break;
		case Rotate90:
			yinc= -inc; break;
		}
		for (int i= 0; i < npoints; ++i)
		{
			int xpos= point [i].x;
			int ypos= point [i].y;
			seg [i].x1= xpos;
			seg [i].y1= ypos;
			//xpos+= zoomtextx - 1;
			xpos+= xinc;
			ypos+= yinc;
			if (limitable && limited)
			{
				xpos= std::min (xpos, limit_maxx);
				ypos= std::max (ypos, limit_miny);
			}
			seg [i].x2= xpos;
			seg [i].y2= ypos;
		}
		XDrawSegments (display, pixmap, gcp, seg, npoints);
		if (! fSynchro)
			XDrawSegments (display, window, gc, seg, npoints);
	}

	#endif

	#ifdef BLASSIC_USE_WINDOWS

	// The PolyPolyline is a bit faster than doing each
	// point separately.
	// 64 * 2 points by segment * max zoomtexty
	static POINT p [64 * 2 * MAXZOOMTEXT];
	// 64 points each char * max zoomtexty

	int inc= zoomtextx;
	//if (zoomtextx > 1)
	//	++inc;
	int xinc= 0, yinc= 0;
	switch (rotate)
	{
	case RotateNone:
		xinc= inc; break;
	case Rotate90:
		yinc= -inc; break;
	}
	for (int i= 0; i < npoints; ++i)
	{
		int xpos= point [i].x;
		int ypos= point [i].y;
		p [2 * i].x= xpos;
		p [2 * i].y= ypos;
		//xpos+= zoomtextxrot;
		//if (zoomtextxrot > 1) ++xpos;
		xpos+= xinc;
		ypos+= yinc;
		if (limitable && limited)
		{
			xpos= std::min (xpos, limit_maxx);
			ypos= std::max (ypos, limit_miny);
		}
		p [2 * i + 1].x= xpos;
		p [2 * i + 1].y= ypos;
	}
	if (! fSynchro)
		PolyPolyline (hdc, p, poly_points, npoints);
	PolyPolyline (hdcPixmap, p, poly_points, npoints);

	#endif
}

void printxy (int x, int y, unsigned char ch,
	bool limitable, bool inverse= false, bool underline= false)
{
	TRACEFUNC (tr, "printxy");

	static unsigned char mask [8]= { 128, 64, 32, 16, 8, 4, 2, 1 };

	//charset::chardata & data= charset::data [ch];

	charset::chardata data;
	memcpy (data, charset::data [ch], sizeof (charset::chardata) );
	if (underline)
		data [7]= 255;

	XPoint point [64 * MAXZOOMTEXT]; // 64 pixels * max zoom height
	int n= 0, npoints= 0;
	for (int i= 0, yi= y; i < 8; ++i, yi+= zoomtexty)
	{
		unsigned char c= data [i];
		if (inverse)
			c= static_cast <unsigned char> (~ c);

		// Little optimization:
		if (c == 0)
			continue;

		for (int j= 0, xj= x; j < 8; ++j, xj+= zoomtextx)
		{
			if (c & mask [j] )
			{
				for (int z= 0; z < zoomtexty; ++z)
				{
					int y= yi + z;
					if (! limitable ||
						check_limit (xj, y) )
					{
						point [n].x= static_cast
							<short> (xj);
						point [n].y= static_cast
							<short> (y);
						++n;
					}
				}
				++npoints;
			}
		}
	}
	if (npoints < 64)
	{
		if (opaquemode)
		{
			setactivecolor (pbackground);
			do_fill_rectangle (x, y,
				x + charwidth - 1, y + charheight - 1,
				limitable);
		}
		if (n > 0)
		{
			setactivecolor (pforeground);
			do_plot_points (point, n, limitable);
		}
		else
			setactivecolor (pforeground);
	}
	else
	{
		setactivecolor (pforeground);
		do_fill_rectangle (x, y,
			x + charwidth - 1, y + charheight - 1,
			limitable);
	}
}

inline void print (int col, int row, unsigned char ch,
	bool inverse, bool underline= false)
{
	printxy (col * charwidth, row * charheight, ch, false,
		inverse, underline);
}

void setmaxtext ()
{
	charwidth= 8 * zoomtextx;
	charheight= 8 * zoomtexty;
	//charwidthrot= charwidth;
	//charheightrot= charheight;
	//do_rotate_rel (charwidthrot, charheightrot);

	//maxtcol= screenwidth / charwidthrot;
	//maxtrow= screenheight / charheightrot;
	maxtcol= screenwidth / charwidth;
	maxtrow= screenheight / charheight;

	#if 0
	switch (rotate)
	{
	case RotateNone:
		break;
	case Rotate90:
		std::swap (maxtcol, maxtrow);
		break;
	}
	#endif
}

void recreate_windows ();

void set_mode (int width, int height, int mode, int charx= 1, int chary= 1)
{
	TRACEFUNC (tr, "set_mode");

	ASSERT (charx >= 1 && charx <= MAXZOOMTEXTX);
	ASSERT (chary >= 1 && chary <= MAXZOOMTEXTY);

	std::ostringstream oss;
	oss << "Width " << short (width) << ", height " << short (height);
	TRMESSAGE (tr, oss.str () );

	if (mode != 0 && (width <= 0 || height <= 0) )
		throw ErrImproperArgument;

	if (mode != text_mode)
	{
		check_initialized ();
		if (! inited)
		{
			if (showdebuginfo () )
				cerr << "Graphics system not initialized" <<
					endl;
			throw ErrFunctionCall;
		}
	}

	sysvar::set16 (sysvar::GraphicsWidth, short (width) );
	sysvar::set16 (sysvar::GraphicsHeight, short (height) );
	screenwidth= width;
	screenheight= height;
	switch (sysvar::get (sysvar::GraphRotate) )
	{
	case 0:
		rotate= RotateNone;
		// Nothing to do.
		break;
	case 1:
		rotate= Rotate90;
		// Rotate 90 degrees.
		std::swap (width, height);
		break;
	default:
		throw ErrFunctionCall;
	}
	realwidth= width;
	realheight= height;
	activetransform= TransformIdentity;
	set_origin (0, 0);
	fSynchro= false;
	zoomtextx= charx;
	zoomtexty= chary;
	//zoomtextxrot= zoomtextx;
	//zoomtextyrot= zoomtextyrot;
	//do_rotate_rel (zoomtextxrot, zoomtextyrot);
	clear_limits ();

	#ifdef BLASSIC_USE_SVGALIB

	if (svgalib) {
		if (mode == user_mode)
			throw ErrFunctionCall;
		if (actualmode != text_mode)
		{
			free (font);
		}
		vga_setmode (mode);
		if (mode != text_mode) {
			setmaxtext ();
			gl_setcontextvga (mode);
			//font= new char [256 * 8 * 8 * BYTESPERPIXEL];
			font= (char *)
				malloc (2 * 256 * 8 * 8 * BYTESPERPIXEL);
			gl_expandfont (8, 8, 15, gl_font8x8, font);
			gl_setfont (8, 8, font);
			//cout << "Listo" << endl;
			graphics_mode_active= true;
		}
		else
			graphics_mode_active= false;
		actualmode= mode;
		return;
	}

	#endif

	if (mode != actualmode || mode == user_mode)
	{
		{
			std::ostringstream oss;
			oss << "Changing mode from " << actualmode <<
				" to " << mode;
			TRMESSAGE (tr, oss.str () );
		}
		if (window_created)
			destroy_window ();
		if (mode != text_mode)
		{
			create_window (width, height);
			graphics_mode_active= true;
		}
		else
			graphics_mode_active= false;
	}
	else
	{
		if (mode != text_mode)
		{
			//graphics::setcolor (0);
			graphics::setdrawmode (0);
			reinit_pixmap ();
			reinit_window ();
		}

		#ifdef _Windows
		else
			if (actualmode != text_mode)
			{
				// REVISAR: ESTO NO PARECE VALIDO.
				destroy_thread ();
				graphics_mode_active= false;
			}
		#endif
	}

	actualmode= mode;

	if (mode != text_mode)
	{
		//if (! colors_inited)
		//	init_colors ();
		setmaxtext ();
		//tcol= trow= 0;
		lastx= lasty= 0;

		#if defined (BLASSIC_USE_WINDOWS) || defined (BLASSIC_USE_X)
		//pforeground= default_foreground;
		//pbackground= default_background;
		#endif

		recreate_windows ();
	}
}

} // namespace

#endif

void graphics::cls ()
{
	TRACEFUNC (tr, "graphics::cls");

	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	#ifdef BLASSIC_USE_SVGALIB

	if (svgalib)
	{
		// PENDIENTE
		return;
	}

	#endif

	//tcol= trow= 0;
	//reinit_pixmap ();

	#if 0

	int x1, y1, width, height;
	if (limited)
	{
		TRMESSAGE (tr, "lmited");
		x1= limit_minx; width= limit_maxx - limit_minx + 1;
		y1= limit_miny; height= limit_maxy - limit_miny + 1;
	}
	else
	{
		TRMESSAGE (tr, "unlimited");
		x1= 0; width= screenwidth;
		y1= 0; height= screenheight;
	}

	#ifdef BLASSIC_USE_WINDOWS

	//RECT r= { 0, 0, screenwidth, screenheight };
	RECT r= { x1, y1, x1 + width + 1, y1 + height + 1 };
	LOGPEN logpen;
	GetObject (* pbackground, sizeof (LOGPEN), & logpen);
	HBRUSH hbrush= CreateSolidBrush (logpen.lopnColor);
	if (! fSynchro)
		FillRect (hdc, & r, hbrush);
	FillRect (hdcPixmap, & r, hbrush);
	DeleteObject (hbrush);

	#elif defined BLASSIC_USE_X

	setactivecolor (pbackground);
	XSetFunction (display, gcp, drawmode_copy);
	XFillRectangle (display, pixmap, gcp,
		//0, 0, screenwidth, screenheight);
		x1, y1, width, height);
	XSetFunction (display, gcp, drawmode);
	if (! fSynchro)
	{
		XSetFunction (display, gc, drawmode_copy);
		XFillRectangle (display, window, gc,
			//0, 0, screenwidth, screenheight);
			x1, y1, width, height);
		XSetFunction (display, gc, drawmode);
		// Inserted an idle call because without it
		// the window sometimes is not updated.
		graphics::idle ();
	}
	setactivecolor (pforeground);

	#endif

	#else

	// Limit control is done by do_fill_rectangle.
	setactivecolor (pbackground);
	do_fill_rectangle (0, 0, screenwidth - 1, screenheight - 1, true);
	setactivecolor (pforeground);

	#endif

	#endif
	// BLASSIC_HAS_GRAPHICS
}

void graphics::setmode (int width, int height, bool inverty,
	int zoomx, int zoomy)
{
	TRACEFUNC (tr, "graphics::setmode");

	#ifndef BLASSIC_HAS_GRAPHICS

	touch (width, height, inverty, zoomx, zoomy);
	no_graphics_support ();

	#else

	#if 0
	if (! inited)
	{
		if (showdebuginfo () )
			cerr << "Graphics system not initialized" << endl;
		throw ErrFunctionCall;
	}
	#endif

	if (zoomx < 1 || zoomx > MAXZOOMTEXTX)
		throw ErrImproperArgument;
	if (zoomy < 1 || zoomy > MAXZOOMTEXTY)
		throw ErrImproperArgument;

	inkset= InkStandard;
	//default_foreground= & xcBlack;
	//default_background= & xcWhite;
	default_pen= 0;
	default_paper= 15;
	::set_mode (width, height, user_mode, zoomx, zoomy);
	if (inverty)
		activetransform= TransformInvertY;

	#endif
}

void graphics::setmode (int mode)
{
	TRACEFUNC (tr, "graphics::setmode");

	#ifndef BLASSIC_HAS_GRAPHICS

	no_graphics_support ();
	touch (mode);

	#else

	#if 0
	if (! inited && mode != text_mode)
	{
		if (showdebuginfo () )
			cerr << "Graphics system not initialized" << endl;
		throw ErrFunctionCall;
	}
	#endif

	int width, height;
	switch (mode)
	{
	case 1:
		width= 320; height= 200; break;
	case 2:
		width= 640; height= 200; break;
	case 5:
		width= 320; height= 200; break;
	case 10:
		width= 640; height= 480; break;
	case 11:
		width= 800; height= 600; break;
	default:
		width= 0; height= 0;
	}
	if (mode != 0 && width == 0)
	{
		if (showdebuginfo () )
			cerr << "Invalid mode number " << mode << endl;
		throw ErrFunctionCall;
	}

	//default_foreground= & xcBlack;
	//default_background= & xcWhite;
	if (mode != text_mode)
	{
		inkset= InkStandard;
		default_pen= 0;
		default_paper= 15;
	}
	::set_mode (width, height, mode);

	#endif
}

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

struct SpecialMode {
	std::string name;
	Inkset inkset;
	int pen;
	int paper;
	int width;
	int height;
	TransformType transform;
	int zoomx;
	int zoomy;
	SpecialMode (const std::string & name, Inkset inkset,
			int pen, int paper, int width, int height,
			TransformType transform, int zoomx, int zoomy) :
		name (name), inkset (inkset),
		pen (pen), paper (paper), width (width), height (height),
		transform (transform), zoomx (zoomx), zoomy (zoomy)
	{ }
};

const SpecialMode specialmodes []= {
	SpecialMode ("spectrum", InkSpectrum, 0,  7, 256, 192,
		TransformInvertY,  1, 1),
	SpecialMode ("cpc0",     InkCpc,      1,  0, 640, 400,
		TransformInvertY,  4, 2),
	SpecialMode ("cpc1",     InkCpc,      1,  0, 640, 400,
		TransformInvertY,  2, 2),
	SpecialMode ("cpc2",     InkCpc,      1,  0, 640, 400,
		TransformInvertY,  1, 2),
	SpecialMode ("pcw",      InkStandard, 0, 15, 720, 248,
		TransformIdentity, 1, 1),
	SpecialMode ("pcw2",     InkStandard, 0, 15, 720, 496,
		TransformIdentity, 1, 2),
};

} // namespace

#endif
// BLASSIC_HAS_GRAPHICS

void graphics::setmode (const std::string & mode)
{
	TRACEFUNC (tr, "graphics::setmode (string)");
	TRMESSAGE (tr, "mode: " + mode);

	#ifndef BLASSIC_HAS_GRAPHICS

	no_graphics_support ();
	touch (mode);

	#else

	#if 0
	if (! inited)
		throw ErrFunctionCall;
	#endif

	for (size_t i= 0; i < util::dim_array (specialmodes); ++i)
	{
		const SpecialMode & m= specialmodes [i];
		if (m.name == mode)
		{
			inkset= m.inkset;
			default_pen= m.pen;
			default_paper= m.paper;
			::set_mode (m.width, m.height, user_mode,
				m.zoomx, m.zoomy);
			activetransform= m.transform;
			// Spectrum is an special case.
			if (mode == "spectrum")
			{
				set_origin (0, 16);
				set_limits (0, 256, 16, 192);
			}
			return;
		}
	}

	TRMESSAGE (tr, "Invalid mode");
	if (showdebuginfo () )
		cerr << '\'' << mode << "' is not a valid graphics mode" <<
			endl;
	throw ErrFunctionCall;

	#endif
}

bool graphics::ingraphicsmode ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	//return actualmode != text_mode;
	return graphics_mode_active;

	#else

	return false;

	#endif
}

void graphics::setcolor (int color)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	#ifdef BLASSIC_USE_SVGALIB

	if (svgalib) {
		vga_setcolor (color);
		return;
	}

	#endif

	graphics_pen= color;
	pcolor pxc= mapcolor (color).pc;

	setactivecolor (pxc);

	#if defined (BLASSIC_USE_WINDOWS) || defined (BLASSIC_USE_X)
	pforeground= pxc;
	#endif

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (color);

	#endif
}

int graphics::getcolor ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return graphics_pen;

	#else

	throw ErrFunctionCall;

	#endif
}

void graphics::setbackground (int color)
{
	//if (! inited) return;

	#ifdef BLASSIC_HAS_GRAPHICS

	graphics_paper= color;
	pcolor pxc= mapcolor (color).pc;
	pbackground= pxc;

	#else

	touch (color);

	#endif
}

int graphics::getbackground ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return graphics_paper;

	#else

	return 0;

	#endif
}

void graphics::settransparent (int transpmode)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	if (! inited)
		return;

	opaquemode= ! (transpmode & 1);

	#else

	touch (transpmode);

	#endif
}

void graphics::setdrawmode (int mode)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	if (! inited)
		return;

	#if 0
	// Draw modes:
	// 0: normal copy mode.
	// 1: XOR
	// 2: AND
	// 3: OR
	// 0 to 3 are Amstrad CPC modes.
	// 4: INVERT, NOT.
	static int modes []= { drawmode_copy, drawmode_xor,
		drawmode_and, drawmode_or, drawmode_invert };

	if (mode < 0 || size_t (mode) >= util::dim_array (modes) )
		return;
	drawmode= modes [mode];
	#else

	drawmode= getdrawmode (mode);

	#endif

	#ifdef BLASSIC_USE_X

	XSetFunction (display, gc, drawmode);
	XSetFunction (display, gcp, drawmode);

	#elif defined BLASSIC_USE_WINDOWS

	//HDC hdc= GetDC (window);
	SetROP2 (hdc, drawmode);
	//ReleaseDC (window, hdc);
	SetROP2 (hdcPixmap, drawmode);

	#endif

	#else
	// No graphics.

	touch (mode);

	#endif
}

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

void do_line_unmasked (int x, int y)
{
	int prevx= lastx, prevy= lasty;
	lastx= x; lasty= y;

	transform_x (x); transform_x (prevx);
	transform_y (y); transform_y (prevy);

	do_rotate (prevx, prevy);
	do_rotate (x, y);

	setactivecolor (pforeground);

	#ifdef BLASSIC_USE_SVGALIB
	if (svgalib)
	{
		vga_drawline (prevx, prevy, x, y);
		return;
	}
	#endif

	#ifdef BLASSIC_USE_X

	if (! fSynchro)
		XDrawLine (display, window, gc, prevx, prevy, x, y);
	XDrawLine (display, pixmap, gcp, prevx, prevy, x, y);
	XFlush (display);

	#elif defined BLASSIC_USE_WINDOWS

	if (! fSynchro)
	{
		MoveToEx (hdc, prevx, prevy, 0);
		LineTo (hdc, x, y);
		LineTo (hdc, x + 1, y); // Last point
	}

	MoveToEx (hdcPixmap, prevx, prevy, 0);
	LineTo (hdcPixmap, x, y);
	LineTo (hdcPixmap, x + 1, y); // Last point

	#endif
}

const unsigned char maskvaluedefault= '\xFF';
unsigned char maskvalue= maskvaluedefault;
bool maskdrawfirst= true;
unsigned maskpos= 0;
unsigned char auxmask []= { 1, 2, 4, 8, 16, 32, 64, 128 };

inline void impl_plot_mask (int x, int y)
{
	if (maskvalue == maskvaluedefault)
		do_plot (x, y);
	else
	{
		if (maskvalue & auxmask [maskpos] )
			do_plot (x, y);
		if (++maskpos == 8) maskpos= 0;
	}
}

void do_line_mask (int x, int y)
{
	int prevx= lastx, prevy= lasty;
	lastx= x; lasty= y;

	transform_x (prevx); transform_x (x);
	transform_y (prevy); transform_y (y);

	setactivecolor (pforeground);

	int px= x - prevx;
	int py= y - prevy;
	int d1x= px < 0 ? -1 : px > 0 ? 1 : 0;
	int d1y= py < 0 ? -1 : py > 0 ? 1 : 0;
	int d2x= d1x;
	int d2y= 0;
	int m= abs (px);
	int n= abs (py);
	if (m <= n)
	{
		d2x= 0;
		d2y= d1y;
		m= abs (py);
		n= abs (px);
	}
	int s= m / 2;
	for (int i= 0; i <= m; ++i)
	{
		if (i != 0 || maskdrawfirst)
		{
			//if (maskvalue & auxmask [maskpos] )
			//	do_plot (prevx, prevy);
			//if (++maskpos == 8) maskpos= 0;
			impl_plot_mask (prevx, prevy);
		}
		s+= n;
		if (s >= m)
		{
			s-= m;
			prevx+= d1x;
			prevy+= d1y;
		}
		else
		{
			prevx+= d2x;
			prevy+= d2y;
		}
	}
}

inline void do_line (int x, int y)
{
	// When limited we use line masked to simplify line unmasked.
	if (! limited && (maskvalue == maskvaluedefault && maskdrawfirst) )
		do_line_unmasked (x, y);
	else
		do_line_mask (x, y);
}

	// --------------------------------
	// drawarc and auxiliary functions.
	// --------------------------------

// Mimic the Spectrum DRAW instruction, even with values
// of angle greater than 2 * pi.

// Workaround for a problem on gcc.
double zero= 0.0; // Don't make this var const!

inline int signum (double d)
{
	return d < zero ? -1 : d > zero ? 1 : 0;
}

inline int roundint (double d)
{
	double frac= modf (d, & d);
	if (frac > 0.5)
		++d;
	else
		if (frac < -0.5)
			--d;
	return int (d);
}

inline void do_drawarcsegment (int xd, int yd,
	int & coords_x, int & coords_y, bool & firstpoint)
{
	// This function is used instead of the other line
	// draw functions to ensure exact compatibility with
	// the Spectrum and because of first point of line
	// treatement.

	int b= abs (yd);
	int c= abs (xd);
	int d= signum (yd);
	int e= signum (xd);
	//std::cerr << "do_drawarcsegment " << c << ' ' << b << ' ' <<
	//	e << ' ' << d << std::endl;
	int h, l, hvx, hvy;
	if (c >= b)
	{
		h= c; l= b; hvx= e; hvy= 0;
	}
	else
	{
		if (b == 0)
			return;
		h= b; l= c; hvx= 0; hvy= d;
	}
	b= h;
	int a= h / 2;
	for (int i= 0; i < b; ++i)
	{
		a+= l;
		if (a < h)
		{
			// Horizontal or vertical step.
			coords_x+= hvx;
			coords_y+= hvy;
		}
		else
		{
			// Diagonal step.
			a-= h;
			coords_x+= e;
			coords_y+= d;
		}
		if (firstpoint)
			firstpoint= false;
		else
		{
			int x= coords_x; int y= coords_y;
			transform_x (x);
			transform_y (y);
			impl_plot_mask (x, y);
		}
	}
}

void do_drawarc (int x, int y, double g)
{
	// The original algorithm uses the Spectrum cooordinates,
	// then the angle is inverted when using non inverted
	// coordinates.
	// To an explanation of the algorithm used, see "The Complete
	// Spectrum ROM Disassembly" or some other disassembly of the
	// Spectrum ROM (the book has more complete comments).

	switch (activetransform)
	{
	case TransformIdentity:
		g= -g;
		break;
	case TransformInvertY:
		break; // Nothing to do
	}

	setactivecolor (pforeground);

	// If drawing of first point is active, no special treatement
	// for the first point by marking as if not were the first.
	bool firstpoint= ! maskdrawfirst;

	int xend= lastx + x, yend= lasty + y;

	double sing2= sin (g / 2);
	double z;
	if (sing2 != 0 && (z= fabs ( (abs (x) + abs (y) ) / sing2) ) >= 1)
	{
		int a= roundint (fabs (g * sqrt (z) / 2) );
		if (a <= 255)
		{
			a= (a & 0xFC) + 4;
			if (a >= 256)
				a= 252;
		}
		else
			a= 252;

		double singa= sin (g / a);
		double cosga= cos (g / a);
		double w= sin (g / (2.0 * a) ) / sing2;
		double f= g / 2 - g / (2.0 * a);
		double sinf= sin (f);
		double cosf= cos (f);
		double un= y * w * sinf + x * w * cosf;
		double vn= y * w * cosf - x * w * sinf;

		double uv= fabs (un) + fabs (vn);
		if (uv >= 1)
		{
			double xn= lastx;
			double yn= lasty;
			a= a - 1;
			while (a > 0)
			{
				xn= xn + un;
				yn= yn + vn;
				do_drawarcsegment (
					roundint (xn - lastx),
					roundint (yn - lasty),
					lastx, lasty, firstpoint);
				a= a - 1;
				if (a > 0)
				{
					double un1= un;
					un= un1 * cosga - vn * singa;
					vn= un1 * singa + vn * cosga;
				}
			}
		}
	}
	// Draw the last segment (can be the entire arc).
	do_drawarcsegment (xend - lastx, yend - lasty,
		lastx, lasty, firstpoint);
}

#ifdef BLASSIC_USE_X

inline void do_rotate_in_char (int & x, int & y)
{
	switch (rotate)
	{
	case RotateNone:
		break;
	case Rotate90:
		#if 1
		int newx= y;
		y= charwidth - x - 1;
		x= newx;
		#else
		int newy= x;
		x= charwidth - y - 1;
		y= newy;
		#endif
		break;
	}
}

inline bool load_char_image (int x, int y, unsigned char (& ch) [8] )
{
	TRACEFUNC (tr, "load_char_image");

	class ImageGuard {
	public:
		ImageGuard (XImage * img) :
			img (img)
		{ }
		~ImageGuard ()
		{
			XDestroyImage (img);
		}
	private:
		XImage * img;
	};

	int width= charwidth, height= charheight;
	do_rotate_rel (width, height);
	do_rotate (x, y);
	switch (rotate)
	{
	case RotateNone:
		break;
	case Rotate90:
		y-= height - 1;
		break;
	}
	TRMESSAGE (tr, "at " + to_string (x) + ", " + to_string (y) );

	XImage * img= XGetImage (display, pixmap, x, y,
		width, height, AllPlanes, XYPixmap);
	if (img == NULL)
		throw ErrNoGraphics; // Not a good idea, provisional.
	ImageGuard guard (img);
	unsigned long back= XGetPixel (img, 0, 0);
	bool fFore= false;
	unsigned long fore= 0;
	for (int i= 0, ipos= 0; i < 8; ++i, ipos+= zoomtexty)
		for (int j= 0, jpos= 0; j < 8; ++j, jpos+= zoomtextx)
		{
			int rjpos= jpos, ripos= ipos;
			do_rotate_in_char (rjpos, ripos);
			unsigned long c= XGetPixel (img, rjpos, ripos);
			bool bit;
			if (c == back)
			{
				bit= false;
			}
			else
			{
				if (! fFore)
				{
					fFore= true;
					fore= c;
				}
				if (c != fore)
					return false;
				bit= true;
			}
			#if 1
			// Quitado provisionalmente
			if (zoomtextx > 1 || zoomtexty > 1)
			{
				int zx= zoomtextx, zy= zoomtexty;
				//do_rotate_rel (zx, zy);
				const int iend= ipos + zy;
				const int jend= jpos + zx;
				for (int ii= ipos; ii < iend; ++ii)
					for (int jj= jpos; jj < jend; ++jj)
					{
						int rj= jj, ri= ii;
						do_rotate_in_char (rj, ri);
						if (XGetPixel (img, rj, ri)
								!= c)
							return false;
					}
			}
			#endif
			ch [i]<<= 1;
			ch [i]|= bit;
		}
	//XDestroyImage (img);
	return true;
}

#elif defined BLASSIC_USE_WINDOWS

inline bool load_char_image (int x, int y, unsigned char (& ch) [8] )
{
	//COLORREF back= GetPixel (hdcPixmap, x, y);
	int rx= x, ry= y;
	do_rotate (rx, ry);
	COLORREF back= GetPixel (hdcPixmap, rx, ry);
	bool fFore= false;
	COLORREF fore= 0;
	for (int i= 0, ipos= y; i < 8; ++i, ipos+= zoomtexty)
		for (int j= 0, jpos= x; j < 8; ++j, jpos+= zoomtextx)
		{
			//COLORREF c= GetPixel (hdcPixmap, jpos, ipos);
			int rjpos= jpos, ripos= ipos;
			do_rotate (rjpos, ripos);
			COLORREF c= GetPixel (hdcPixmap, rjpos, ripos);
			bool bit;
			if (c == back)
				bit= false;
			else
			{
				if (! fFore)
				{
					fFore= true;
					fore= c;
				}
				if (c != fore)
					//return  std::string ();
					return false;
				bit= true;
			}
			if (zoomtextx > 1 || zoomtexty > 1)
			{
				const int iend= ipos + zoomtexty;
				const int jend= jpos + zoomtextx;
				for (int ii= ipos; ii < iend; ++ii)
					for (int jj= jpos; jj < jend; ++jj)
					{
						int rj= jj, ri= ii;
						do_rotate (rj, ri);
						if (GetPixel (hdcPixmap,
								rj, ri) != c)
							return false;
					}
			}
			ch [i]<<= 1;
			ch [i]|= bit;
		}
	return true;
}

#else

inline bool load_char_image (int, int, unsigned char (&) [8] )
{
	return false;
}

#endif

std::string copychrat (int x, int y, BlChar from, BlChar to)
{
	TRACEFUNC (tr, "copychrat");
	{
		std::ostringstream oss;
		oss << "at " << x << ", " << y;
		TRMESSAGE (tr, oss.str () );
	}

	unsigned char ch [8]= {0};

	//if (x < 0 || x > screenwidth - 8 || y < 0 || y > screenheight - 8)
	if (x < 0 || x > screenwidth - charwidth ||
		y < 0 || y > screenheight - charheight)
		return std::string ();

	if (! load_char_image (x, y, ch) )
		return std::string ();

	#ifndef NDEBUG
	{
		std::ostringstream oss;
		oss << "Char: " << std::hex;
		for (int i= 0; i < 8; ++i)
		{
			oss << std::setw (2) << std::setfill ('0') <<
				static_cast <unsigned int> (ch [i]) << ", ";
		}
		TRMESSAGE (tr, oss.str () );
	}
	#endif

	char chinv [8];
	for (int i= 0; i < 8; ++i)
		chinv [i]= static_cast <unsigned char> (~ ch [i]);

	//for (int i= 0; i < 256; ++i)
	int iFrom= static_cast <int> (from);
	int iTo= static_cast <int> (to) + 1;
	for (int i= iFrom; i < iTo; ++i)
	{
		if (memcmp (charset::data [i], ch, 8) == 0 ||
			memcmp (charset::data [i], chinv, 8) == 0)
		{
			return std::string (1, static_cast <char> (i) );
		}
	}

	return std::string ();
}

class ColorTester {
	#ifdef BLASSIC_USE_X
	XImage * img;
	#define TYPETESTER 2
	#endif
public:
	ColorTester ()
	{
		#ifdef BLASSIC_USE_X
		#if TYPETESTER == 1
		img= XGetImage (display, pixmap,
			0, 0, 1, 1,
			AllPlanes, XYPixmap);
		#else
		int width= screenwidth, height= screenheight;
		do_rotate_rel (width, height);
		img= XGetImage (display, pixmap,
			0, 0, width, height,
			AllPlanes, XYPixmap);
		#endif
		if (img == NULL)
			throw ErrNoGraphics;
		#endif
	}
	~ColorTester ()
	{
		#ifdef BLASSIC_USE_X
		XDestroyImage (img);
		#endif
	}
	ColorValue operator () (int x, int y)
	{
		do_rotate (x, y);
		#ifdef BLASSIC_USE_X
		#if TYPETESTER == 1
		XGetSubImage (display, pixmap, x, y, 1, 1,
			AllPlanes,XYPixmap,
			img, 0, 0);
		return XGetPixel (img, 0, 0);
		#else
		return XGetPixel (img, x, y);
		#endif
		#elif defined BLASSIC_USE_WINDOWS
		return GetPixel (hdcPixmap, x, y);
		#endif
	}
};

ColorValue do_test_color (int x, int y)
{
	do_rotate (x, y);

	#ifdef BLASSIC_USE_X

	XImage * img= XGetImage (display, pixmap, x, y, 1, 1,
		AllPlanes, XYPixmap);
	if (img == NULL)
		throw ErrNoGraphics; // Not a good idea, provisional.
	unsigned long color= XGetPixel (img, 0, 0);
	XDestroyImage (img);
	return color;

	#else

	return GetPixel (hdcPixmap, x, y);

	#endif
}

int do_test (int x, int y)
{
	//int x= lastx;
	//int y= lasty;
	//transform_x (x);
	//transform_y (y);

	if (x < 0 || x >= screenwidth || y < 0 || y >= screenheight)
		return 0;

	do_rotate (x, y);

	#ifdef BLASSIC_USE_X

	XImage * img= XGetImage (display, pixmap, x, y, 1, 1,
		AllPlanes, XYPixmap);
	if (img == NULL)
		throw ErrNoGraphics; // Not a good idea, provisional.
	unsigned long color= XGetPixel (img, 0, 0);
	XDestroyImage (img);
	for (int i= 0; i < 16; ++i)
		if (mapcolor (i).pc->pixel == color)
			return i;
	for (definedcolor_t::iterator it= definedcolor.begin ();
		it != definedcolor.end (); ++it)
	{
		if (it->second.pc->pixel == color)
			return it->first;
	}
	return -1;

	#elif defined BLASSIC_USE_WINDOWS

	COLORREF color= GetPixel (hdcPixmap, x, y);
	for (int i= 0; i < 16; ++i)
	{
		//const ColorRGB & c= assignRGB [i];
		const ColorRGB & c= mapcolor (i).rgb;
		if (RGB (c.r, c.g, c.b) == color)
		//if (GetNearestColor (hdcPixmap, RGB (c.r, c.g, c.b) ) ==
		//	color)
			return i;
	}
	for (definedcolor_t::iterator it= definedcolor.begin ();
		it != definedcolor.end (); ++it)
	{
		const ColorRGB & c= it->second.rgb;
		if (RGB (c.r, c.g, c.b) == color)
			return it->first;
	}
	return -1;

	#else

	return -1;

	#endif
}

int test ()
{
	int x= lastx;
	int y= lasty;
	transform_x (x);
	transform_y (y);
	return do_test (x, y);
}

} // namespace

#endif
//BLASSIC_HAS_GRAPHICS

void graphics::line (int x, int y)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	do_line (x, y);

	#else

	touch (x, y);

	#endif
}

void graphics::liner (int x, int y)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	line (lastx + x, lasty + y);

	#else

	touch (x, y);

	#endif
}

void graphics::drawarc (int x, int y, double g)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	do_drawarc (x, y, g);

	#else

	touch (x, y, g);

	#endif
}

void graphics::rectangle (Point org, Point dest)
{
	TRACEFUNC (tr, "graphics::rectangle");

	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	int x1= org.x;
	int y1= org.y;
	int x2= dest.x;
	int y2= dest.y;
	lastx= x2; lasty= y2;

	transform_x (x1); transform_x (x2);
	transform_y (y1); transform_y (y2);

	do_rotate (x1, y1);
	do_rotate (x2, y2);

	if (x1 > x2) std::swap (x1, x2);
	if (y1 > y2) std::swap (y1, y2);

	#ifdef BLASSIC_USE_WINDOWS

	HBRUSH hold= (HBRUSH) SelectObject (hdcPixmap, GetStockObject (NULL_BRUSH) );
	Rectangle (hdcPixmap, x1, y1, x2 + 1, y2 + 1);
	SelectObject (hdc, hold);
	if (! fSynchro)
	{

		hold= (HBRUSH) SelectObject (hdc, GetStockObject (NULL_BRUSH) );
		Rectangle (hdc, x1, y1, x2 + 1, y2 + 1);
		SelectObject (hdc, hold);
	}

	#elif defined BLASSIC_USE_X

	XDrawRectangle (display, pixmap, gcp,
		x1, y1, x2 - x1, y2 - y1);
	if (! fSynchro)
		XDrawRectangle (display, window, gc,
			x1, y1, x2 - x1, y2 - y1);

	#endif

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (org, dest);

	#endif
}

void graphics::rectanglefilled (Point org, Point dest)
{
	TRACEFUNC (tr, "graphics::rectanglefilled");

	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	int x1= org.x;
	int y1= org.y;
	int x2= dest.x;
	int y2= dest.y;
	lastx= x2; lasty= y2;

	transform_x (x1); transform_x (x2);
	transform_y (y1); transform_y (y2);

	if (x1 > x2) std::swap (x1, x2);
	if (y1 > y2) std::swap (y1, y2);
	do_fill_rectangle (x1, y1, x2, y2, true);

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (org, dest);

	#endif
}

void graphics::zxplot (Point p)
{
	TRACEFUNC (tr, "graphics::zxplot");

	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	int x= p.x / 2 + 1;
	int y= 21 - p.y / 2;
	p.x*= 4;
	p.y*= 4;
	Point p2 (p);
	p2.x+= 3;
	p2.y+= 3;
	rectanglefilled (p, p2);
	gotoxy (BlChannel (0), x, y);

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (p);

	#endif
}

void graphics::zxunplot (Point p)
{
	TRACEFUNC (tr, "graphics::zxunplot");

	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	setactivecolor (pbackground);
	zxplot (p);
	setactivecolor (pforeground);

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (p);

	#endif
}


void graphics::move (int x, int y)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	lastx= x; lasty= y;

	#else

	touch (x, y);

	#endif
}

void graphics::mover (int x, int y)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	lastx+= x; lasty+= y;

	#else

	touch (x, y);

	#endif
}

void graphics::plot (int x, int y)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	lastx= x; lasty= y;
	transform_x (x);
	transform_y (y);
	setactivecolor (pforeground);
	do_plot (x, y);

	#else

	touch (x, y);

	#endif
}

void graphics::plotr (int x, int y)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	// No need to check here, plot will do
	plot (lastx + x, lasty + y);

	#else

	touch (x, y);
	no_graphics_support ();

	#endif
}

int graphics::test (int x, int y, bool relative)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	// Check for graphics mode is done when calling move.
	if (relative)
	{
		x+= lastx;
		y+= lasty;
	}
	move (x, y);

	return ::test ();

	#else

	no_graphics_support ();
	touch (x, y, relative);
	throw ErrBlassicInternal; // Make the compiler happy.

	#endif
}

namespace {

void line_to_point (const graphics::Point & p)
{
	graphics::line (p.x, p.y);
}

} // namespace

void graphics::plot (std::vector <Point> & points)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	ASSERT (points.size () != 0);
	lastx= points [0].x;
	lasty= points [0].y;
	std::for_each (points.begin () + 1, points.end (), line_to_point);

	#else

	// Must not come here, requiregraphics must throw.
	ASSERT (false);
	touch (points);

	#endif
}

//#define DEBUG_CIRCLE

#ifdef DEBUG_CIRCLE

#include <unistd.h>

namespace { inline void pausec () { usleep (100000); } }

#else

namespace { inline void pausec () { } }

#endif

void graphics::circle (int x, int y, int radius)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	setactivecolor (pforeground);

	lastx= x + radius;
	lasty= y;

	transform_x (x);
	transform_y (y);

	if (radius == 0)
	{
		impl_plot_mask (x, y);
		return;
	}

	// sq2_2 is sin (pi/4)
	#ifdef M_SQRT2
	static const double sq2_2= M_SQRT2 / 2.0;
	#else
	static const double sq2_2= sqrt (2.0) / 2.0;
	#endif
	//int r= int (radius * sq2_2 + .5) + 1;
	int r= int (radius * sq2_2 + 1.0);
	int rr= int (sqrt (radius * radius - r * r) + .5);
	int dim= r;
	if (rr >= r) ++dim;

	#ifdef DEBUG_CIRCLE
	cerr << "Circle: " << radius << ", " << r << endl;
	#endif

	util::auto_buffer <int> p (dim);

	// Bresenham algorithm.
	for (int i= 0, j= radius, d= 1 - radius; i < dim; ++i)
	{
		p [i]= j;
		if (d < 0)
			d+= 2 * i + 3;
		else
		{
			d+= 2 * (i - j) + 5;
			--j;
		}
	}

	rr= p [r - 1] - 1;
	ASSERT (rr <= dim - 1);

	// The first point in each quadrant is plotted independently.
	// In the first quadrant is omitted, we plot it at the end.
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (4);
	#endif
	for (int j= 1; j < r; ++j)
	{
		//do_line (x + p [j], y - j);
		impl_plot_mask (x + p [j], y - j);
		pausec ();
	}
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (0);
	#endif
	for (int i= rr; i > 0; --i)
	{
		//do_line (x + i, y - p [i] );
		impl_plot_mask (x + i, y - p [i] );
		pausec ();
	}

	//do_line (x, y - radius);
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (4);
	#endif
	impl_plot_mask (x, y - radius);
	for (int i= 1; i < r; ++i)
	{
		//do_line (x - i, y - p [i] );
		impl_plot_mask (x - i, y - p [i] );
		pausec ();
	}
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (0);
	#endif
	for (int j= rr; j > 0; --j)
	{
		//do_line (x - p [j], y - j);
		impl_plot_mask (x - p [j], y - j);
		pausec ();
	}

	//do_line (x - radius, y);
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (4);
	#endif
	impl_plot_mask (x - radius, y);
	for (int j= 1; j < r; ++j)
	{
		//do_line (x - p [j], y + j);
		impl_plot_mask (x - p [j], y + j);
		pausec ();
	}
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (0);
	#endif
	for (int i= rr; i > 0; --i)
	{
		//do_line (x - i, y + p [i] );
		impl_plot_mask (x - i, y + p [i] );
		pausec ();
	}

	//do_line (x, y + radius);
	impl_plot_mask (x, y + radius);
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (4);
	#endif
	for (int i= 1; i < r; ++i)
	{
		//do_line (x + i, y + p [i] );
		impl_plot_mask (x + i, y + p [i] );
		pausec ();
	}
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (0);
	#endif
	for (int j= rr; j > 0; --j)
	{
		//do_line (x + p [j], y + j);
		impl_plot_mask (x + p [j], y + j);
		pausec ();
	}
	//do_line (x + radius, y);
	#ifdef DEBUG_CIRCLE
	graphics::setcolor (4);
	#endif
	impl_plot_mask (x + radius, y);

	#else
	// BLASSIC_HAS_GRAPHICS

	touch (x, y, radius);

	#endif
}

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

inline void get_point_on_arc (int r, BlNumber a, int & out_x, int & out_y)
{
	BlNumber s= sin (a) * r, c= cos (a) * r;
	out_x= static_cast <int> (c < 0 ? c - .5 : c + .5);
	out_y= static_cast <int> (s < 0 ? s - .5 : s + .5);
}

inline int get_quadrant (int x, int y)
{
	if (x >= 0)
	{
		if (y >= 0)
			return 0;
		else
			return 3;
	}
	else
	{
		if (y > 0)
			return 1;
		else
			return 2;
	}
}

} // namespace

#endif
// BLASSIC_HAS_GRAPHICS

void graphics::arccircle (int x, int y, int radius,
	BlNumber arcbeg, BlNumber arcend)
{
	/*
		The code for this function and his auxiliarys
		is taken from the Allegro library.
		Many thanks.
	*/

	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	setactivecolor (pforeground);

	//#define DEBUG_ARCCIRCLE

	#ifdef DEBUG_ARCCIRCLE
	cerr << "arccircle: " << x << ", " << y << ", " << radius << ", " <<
		arcbeg << ", " << arcend << endl;
	#endif

	lastx= x + radius;
	lasty= y;
	transform_x (x);
	transform_y (y);

	int px, py; // Current position and auxiliary.
	get_point_on_arc (radius, arcend, px, py);
	const int ex= px, ey= py; // End position.
	get_point_on_arc (radius, arcbeg, px, py);
	const int sx= px, sy= py; // Start position.
	// Current position start at start position.

	const int sq= get_quadrant (sx, sy); // Start quadrant.
	// Calculate end quadrant, considering that end point
	// must be after start point.
	int q= get_quadrant (ex, ey);
	if (sq > q)
		// Quadrant end must be greater or equal.
		q+= 4;
	else if (sq == q && arcbeg > arcend)
		// If equal, consider the angle.
		q+= 4;
	const int qe= q;
	q= sq; // Current cuadrant.

	#ifdef DEBUG_ARCCIRCLE
	cerr << "Quadrant from " << sq << " to " << qe << endl;
	#endif

	// Direction of movement.
	int dy = ( ( (q + 1) & 2) == 0) ? 1 : -1;
	int dx= ( (q & 2) == 0) ? -1 : 1;

	const int rr= radius * radius;
	int xx= px * px;
	int yy= py * py - rr;

	while (true)
	{
		// Change quadrant when needed, adjusting directions.
		if ( (q & 1) == 0)
		{
			if (px == 0)
			{
				if (qe == q)
					break;
				++q;
				dy= -dy;
			}
		}
		else
		{
			if (py == 0)
			{
				if (qe == q)
					break;
				++q;
				dx= -dx;
			}
		}
		// If we are in the end quadrant, check if at the end position.
		if (qe == q)
		{
			int det= 0;
			if (dy > 0)
			{
				if (py >= ey)
					++det;
			}
			else
			{
				if (py <= ey)
					++det;
			}
			if (dx > 0)
			{
				if (px >= ex)
					++det;
			}
			else
			{
				if (px <= ex)
					++det;
			}
			if (det == 2)
				break;
		}

		impl_plot_mask (x + px, y - py);

		int xx_new= (px + dx) * (px + dx);
		int yy_new= (py + dy) * (py + dy) - rr;
		int rr1= xx_new + yy;
		int rr2= xx_new + yy_new;
		int rr3= xx + yy_new;
		if (rr1 < 0) rr1= -rr1;
		if (rr2 < 0) rr2= -rr2;
		if (rr3 < 0) rr3= -rr3;
		if (rr3 >= std::min (rr1, rr2) )
		{
			px+= dx;
			xx= xx_new;
		}
		if (rr1 > std::min (rr2, rr3) )
		{
			py+= dy;
			yy= yy_new;
		}
	}
	if (px != sx || py != sy || sq == qe)
		impl_plot_mask (x + px, y - py);

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (x, y, radius, arcbeg, arcend);

	#endif
}

void graphics::ellipse (int ox, int oy, int rx, int ry)
{
	// Based on "A fast Bresenham type algorithm
	// for drawing ellipses", by John Kennedy.

	//#define DEBUG_ELLIPSE

	#ifdef DEBUG_ELLIPSE
	cerr << "Ellipse: " << rx << ", " << ry << endl;
	#endif

	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	setactivecolor (pforeground);
	lastx= ox+ rx;
	lasty= oy;

	transform_x (ox);
	transform_y (oy);

	const int ry2= ry * ry;
	const int rx2= rx * rx;
	//const int dimy= int (ry2 / sqrt (ry2 + rx2) + 0.5);
	const int dimy= int (ry2 / sqrt (ry2 + rx2) ) + 1;

	#ifdef DEBUG_ELLIPSE
	cerr << "Dimy: " << dimy << endl;
	#endif

	util::auto_buffer <int> py (dimy);
	//#define SIMPLER
	#ifndef SIMPLER
	const int ry2_2= ry2 * 2;
	const int rx2_2= rx2 * 2;
	int xchange= ry2 * (1 - 2 * rx);
	int ychange= rx2;
	int ellipseerror= 0;
	for (int y= 0, x= rx; y < dimy; ++y)
	{
		py [y]= x;
		if (x == 0) // Needed for little rx
			continue;
		ellipseerror+= ychange;
		ychange+= rx2_2;
		if (2 * ellipseerror + xchange > 0)
		{
			--x;
			ellipseerror+= xchange;
			xchange+= ry2_2;
		}
	}
	#else
	for (int y= 0; y < dimy; ++y)
		py [y]= int (rx * sqrt (ry * ry - y * y) / ry + 0.5);
	#endif

	int aux= dimy > 0 ? py [dimy - 1] : rx;
	const int dimx= aux < 0 ? 0 : aux;

	#ifdef DEBUG_ELLIPSE
	cerr << "Dimx: " << dimx << endl;
	#endif

	util::auto_buffer <int> px (dimx);
	#ifndef SIMPLER
	xchange= ry2;
	ychange= rx2 * (1 - 2 * ry);
	ellipseerror= 0;
	for (int x= 0, y= ry; x < dimx; ++x)
	{
		px [x]= y;
		if (y == 0) // Needed for little ry
			continue;
		ellipseerror+= xchange;
		xchange+= ry2_2;
		if (2 * ellipseerror + ychange > 0)
		{
			--y;
			ellipseerror+= ychange;
			ychange+= rx2_2;
		}
	}
	#else
	for (int x= 0; x < dimx; ++x)
		px [x]= int (ry * sqrt (rx * rx - x * x) / rx + 0.5);
	#endif

	for (int y= 1; y < dimy; ++y)
		impl_plot_mask (ox + py [y], oy - y);
	for (int x= dimx - 1; x > 0; --x)
		impl_plot_mask (ox + x, oy - px [x] );
	impl_plot_mask (ox, oy - ry);
	for (int x= 1; x < dimx; ++x)
		impl_plot_mask (ox - x, oy - px [x] );
	for (int y= dimy - 1; y > 0; --y)
		impl_plot_mask (ox - py [y], oy - y);
	impl_plot_mask (ox - rx, oy);
	for (int y= 1; y < dimy; ++y)
		impl_plot_mask (ox - py [y], oy + y);
	for (int x= dimx - 1; x > 0; --x)
		impl_plot_mask (ox - x, oy + px [x] );
	impl_plot_mask (ox, oy + ry);
	for (int x= 1; x < dimx; ++x)
		impl_plot_mask (ox + x, oy + px [x] );
	for (int y= dimy - 1; y > 0; --y)
		impl_plot_mask (ox + py [y], oy + y);
	impl_plot_mask (ox + rx, oy);

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (ox, oy, rx, ry);

	#endif
}

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

inline void get_point_on_arc (int rx, int ry, BlNumber a,
	int & out_x, int & out_y)
{
	BlNumber s= sin (a) * ry, c= cos (a) * rx;
	out_x= static_cast <int> (c < 0 ? c - .5 : c + .5);
	out_y= static_cast <int> (s < 0 ? s - .5 : s + .5);
}

} // namespace

#endif
// BLASSIC_HAS_GRAPHICS

void graphics::arcellipse (int ox, int oy, int rx, int ry,
	BlNumber arcbeg, BlNumber arcend)
{
	/*
		Adapted from the arccircle algorithm,
		not optimal but works.
	*/
	//#define DEBUG_ARCELLIPSE

	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	setactivecolor (pforeground);
	lastx= ox+ rx;
	lasty= oy;

	transform_x (ox);
	transform_y (oy);

	int px, py; // Current position and auxiliary
	get_point_on_arc (rx, ry, arcend, px, py);
	const int ex= px, ey= py; // End position
	get_point_on_arc (rx, ry, arcbeg, px, py);
	const int sx= px, sy= py; // Start position

	const int sq= get_quadrant (sx, sy); // Start quadrant.
	// Calculate end quadrant, considering that end point
	// must be after start point.
	int q= get_quadrant (ex, ey);
	if (sq > q)
		// Quadrant end must be greater or equal.
		q+= 4;
	else if (sq == q && arcbeg > arcend)
		// If equal, consider the angle.
		q+= 4;
	const int qe= q;
	q= sq; // Current cuadrant.

	#ifdef DEBUG_ARCELLIPSE
	cerr << "Arc llipse from: " << arcbeg << " to " << arcend << endl;
	cerr << "Quadrant from " << sq << " to " << qe << endl;
	#endif

	// Direction of movement.
	int dy = ( ( (q + 1) & 2) == 0) ? 1 : -1;
	int dx= ( (q & 2) == 0) ? -1 : 1;

	const int rx2= rx * rx;
	const int ry2= ry * ry;
	const int rxy2= rx2 * ry2;

	while (true)
	{
		// Change quadrant when needed, adjusting directions.
		if ( (q & 1) == 0)
		{
			// Take care that in very eccentric ellipses
			// can be in 0 before the extreme point.
			if (px == 0 && abs (py) == ry)
			{
				if (qe == q)
					break;
				++q;
				dy= -dy;
			}
		}
		else
		{
			if (py == 0 && abs (px) == rx)
			{
				if (qe == q)
					break;
				++q;
				dx= -dx;
			}
		}
		// If we are in the end quadrant, check if at the end position.
		if (qe == q)
		{
			int det= 0;
			if (dy > 0)
			{
				if (py >= ey)
					++det;
			}
			else
			{
				if (py <= ey)
					++det;
			}
			if (dx > 0)
			{
				if (px >= ex)
					++det;
			}
			else
			{
				if (px <= ex)
					++det;
			}
			if (det == 2)
				break;
		}

		impl_plot_mask (ox + px, oy - py);

		int rr1= ry2 * (px + dx) * (px + dx) +
			rx2 * py * py - rxy2;
		int rr2= ry2 * (px + dx) * (px + dx) +
			rx2 * (py + dy) * (py + dy) - rxy2;
		int rr3= ry2 * px * px +
			rx2 * (py + dy) * (py + dy) - rxy2;
		if (rr1 < 0) rr1= -rr1;
		if (rr2 < 0) rr2= -rr2;
		if (rr3 < 0) rr3= -rr3;
		if (rr3 >= std::min (rr1, rr2) )
		{
			px+= dx;
			//xx= xx_new;
		}
		if (rr1 > std::min (rr2, rr3) )
		{
			py+= dy;
			//yy= yy_new;
		}
	}
	if (px != sx || py != sy || sq == qe)
		impl_plot_mask (ox + px, oy - py);

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (ox, oy, rx, ry, arcbeg, arcend);

	#endif
}

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

#if 0

class Painter {
	ColorValue paint, border;
	std::vector <std::vector <bool> > visited;
	ColorTester test;
public:
	Painter (ColorValue paint, ColorValue border) :
		paint (paint),
		border (border),
		visited (screenwidth, std::vector <bool> (screenheight) )
	{
	}

	bool check (int x, int y)
	{
		if (! check_limit (x, y) )
			return false;
		std::vector <bool>::reference v= visited [x] [y];
		if (v)
			return false;
		v= true;
		ColorValue c= test (x, y);
		if (c == border || c == paint)
			return false;
		return true;
	}

void do_paint_it (int x, int y/*, int xd, int yd*/)
{
	//if (! check_limit (x, y) )
	//	return;
	//int c= do_test (x, y);
	//if (c == borderattr || c == paintattr)
	//	return;

	do_plot (x, y);
	//visited [x] [y]= true;

	//ColorValue c;
	//if (xd != -1)
		#if 0
		for (int i=1; /*check_limit (x - i, y)*/ i < 2; ++i)
		{
			if (visited [x - i] [y] )
				break;
			visited [x - i] [y]= true;
			//c= do_test_color (x - i, y);
			c= test (x - i, y);
			if (c != border && c != paint)
				do_paint_it (x - i, y, 1, 0);
			else
				break;
		}
		#else
		if (check (x - 1, y) )
			do_paint_it (x - 1, y/*, 1, 0*/);
		#endif
	//if (xd != 1)
		#if 0
		for (int i= 1; /*check_limit (x + i, y)*/ i < 2; ++i)
		{
			if (visited [x + i] [y] )
				break;
			visited [x + i] [y]= true;
			//c= do_test_color (x + i, y);
			c= test (x + i, y);
			if (c != border && c != paint)
				do_paint_it (x + i, y, -1, 0);
			else
				break;
		}
		#else
		if (check (x + 1, y) )
			do_paint_it (x + 1, y/*, 1, 0*/);
		#endif
	//if (yd != -1)
		#if 0
		for (int i= 1; /*check_limit (x, y - i)*/ i < 2; ++i)
		{
			if (visited [x] [y - i] )
				break;
			visited [x] [y - i]= true;
			//c= do_test_color (x, y - i);
			c= test (x, y - i);
			if (c != border && c != paint)
				do_paint_it (x, y - i, 0, 1);
			else
				break;
		}
		#else
		if (check (x, y - 1) )
			do_paint_it (x, y - 1/*, 0, 1*/);
		#endif
	//if (yd != 1)
		#if 0
		for (int i= 1; /*check_limit (x, y + i)*/ i < 2; ++i)
		{
			if (visited [x] [y + i] )
				break;
			visited [x] [y + i]= true;
			//c= do_test_color (x, y + i);
			c= test (x, y + i);
			if (c != border && c != paint)
				do_paint_it (x, y + i, 0, -1);
			else
				break;
		}
		#else
		if (check (x, y + 1) )
			do_paint_it (x, y + 1/*, 0, -1*/);
		#endif
}

	void do_paint (int x, int y)
	{
		if (check (x, y) )
			do_paint_it (x, y/*, 0, 0*/);
	}

};

#elif 0

class Painter {
	ColorValue paint, border;
	std::vector <std::vector <bool> > visited;
	ColorTester test;
public:
	Painter (ColorValue paint, ColorValue border) :
		paint (paint),
		border (border),
		visited (screenwidth, std::vector <bool> (screenheight) )
	{
	}

//void paintN (int x, int y);
//void paintS (int x, int y);
//void paintE (int x, int y);
//void paintW (int x, int y);

void do_paint (int x, int y)
{
	if (! check_limit (x, y) )
		return;
	//ColorValue c= do_test_color (x, y);
	ColorValue c= test (x, y);
	if (c == paint || c == border)
		return;
	do_plot (x, y);
	visited [x] [y]= true;
	paintN (x, y - 1);
	paintS (x, y + 1);
	paintE (x + 1, y);
	paintW (x - 1, y);
}

void paintN (int x, int y)
{
	if (! check_limit (x, y) )
		return;
	if (visited [x] [y] )
		return;
	visited [x] [y]= true;
	//ColorValue c= do_test_color (x, y);
	ColorValue c= test (x, y);
	if (c == paint || c == border)
		return;
	do_plot (x, y);
	paintN (x, y - 1);
	paintE (x + 1, y);
	paintW (x - 1, y);
}

void paintS (int x, int y)
{
	if (! check_limit (x, y) )
		return;
	if (visited [x] [y] )
		return;
	visited [x] [y]= true;
	//ColorValue c= do_test_color (x, y);
	ColorValue c= test (x, y);
	if (c == paint || c == border)
		return;
	do_plot (x, y);
	paintS (x, y + 1);
	paintE (x + 1, y);
	paintW (x - 1, y);
}

void paintE (int x, int y)
{
	if (! check_limit (x, y) )
		return;
	if (visited [x] [y] )
		return;
	visited [x] [y]= true;
	//ColorValue c= do_test_color (x, y);
	ColorValue c= test (x, y);
	if (c == paint || c == border)
		return;
	do_plot (x, y);
	paintN (x, y - 1);
	paintS (x, y + 1);
	paintE (x + 1, y);
}

void paintW (int x, int y)
{
	if (! check_limit (x, y) )
		return;
	if (visited [x] [y] )
		return;
	visited [x] [y]= true;
	//ColorValue c= do_test_color (x, y);
	ColorValue c= test (x, y);
	if (c == paint || c == border)
		return;
	do_plot (x, y);
	paintN (x, y - 1);
	paintS (x, y + 1);
	paintW (x - 1, y);
}

};

#else

struct PPoint {
	int x;
	int y;
	PPoint () { }
	PPoint (int x, int y) : x (x), y (y) { }
	//PPoint (const PPoint & p) : x (p.x), y (p.y) { }
	bool check (ColorValue paint, ColorValue border,
		std::vector <std::vector <bool> > & visit,
		ColorTester & test)
	{
		if (! check_limit (x, y) )
			return false;
		std::vector <bool>::reference v= visit [x] [y];
		if (v)
			return false;
		v= true;
		//ColorValue c= do_test_color (x, y);
		ColorValue c= test (x, y);
		if (c == paint || c == border)
			return false;
		return true;
	}
	void plot () { do_plot (x, y); }
};

#if 0

class Painter {
	ColorValue paint, border;
	ColorTester test;
	struct seg {
		int y, xl, xr, dy;
		seg () { }
		seg (int y, int xl, int xr, int dy) :
			y (y), xl (xl), xr (xr), dy (dy)
		{ }
	};
	std::deque <seg> stack;
	bool check (int x, int y)
	{
		if (! check_limit (x, y) )
			return false;
		ColorValue c= test (x, y);
		if (c == paint || c == border)
			return false;
		return true;
	}
public:
	Painter (ColorValue paint, ColorValue border) :
		paint (paint),
		border (border)
	{
	}

void do_paint (int x, int y)
{
	seg s;
	int l, x1, x2, dy;
	if (! check_limit (x, y) )
		return;
	ColorValue ov;
	ov= test (x, y);
	if (ov == paint || ov == border)
		return;
	stack.push_back (seg (y,     x, x,  1) );
	stack.push_back (seg (y + 1, x, x, -1) );
	while (! stack.empty () )
	{
		s= stack.back ();
		stack.pop_back ();
		y= s.y + s.dy; x1= s.xl; x2= s.xr; dy= s.dy;
		for (x= x1; check (x, y); --x)
			do_plot (x, y);
		if (x >= x1)
			goto skip;
		l= x + 1;
		if (l < x1)
			stack.push_back (seg (y, l, x1 - 1, -dy) );
		x= x1 + 1;
		do {
			for ( ; check (x, y); ++x)
				do_plot (x, y);
			stack.push_back (seg (y, l, x - 1, dy) );
			if (x > x2 + 1)
				stack.push_back (seg (y, x2 + 1, x - 1, -dy) );
		skip:
			for (x++; x <= x2 && ! test (x, y); ++x)
				continue;
			l= x;
		} while (x <= x2);
	}
}

};


#elif 0

class Painter {
	ColorValue paint, border;
	ColorTester test;
public:
	Painter (ColorValue paint, ColorValue border) :
		paint (paint),
		border (border)
	{
	}

void do_paint (int x, int y)
{
	using std::vector;
	vector <vector <bool> >
		visit (screenwidth, vector <bool> (screenheight) );
	std::deque <PPoint> stack;
	PPoint pt, pn;
	stack.push_back (PPoint (x, y) );
	while (! stack.empty () )
	{
		pt= stack.back ();
		stack.pop_back ();

		pt.plot ();
		pn= PPoint (pt.x, pt.y + 1);
		if (pn.check (paint, border, visit, test) )
			stack.push_back (pn);

		pn= PPoint (pt.x, pt.y - 1);
		if (pn.check (paint, border, visit, test) )
			stack.push_back (pn);

		pn= PPoint (pt.x + 1, pt.y);
		if (pn.check (paint, border, visit, test) )
			stack.push_back (pn);

		pn= PPoint (pt.x - 1, pt.y);
		if (pn.check (paint, border, visit, test) )
			stack.push_back (pn);
	}
}

};

#else

class Painter {
	ColorValue paint, border;
	std::vector <std::vector <bool> > visited;
	std::deque <PPoint> processlist, nextlist;
	ColorTester test;
public:
	Painter (ColorValue paint, ColorValue border) :
		paint (paint),
		border (border),
		visited (screenwidth, std::vector <bool> (screenheight) )
	{
	}
	bool check (int x, int y)
	{
		if (! check_limit (x, y) )
			return false;
		std::vector <bool>::reference v= visited [x] [y];
		if (v)
			return false;
		v= true;
		ColorValue c= test (x, y);
		if (c == paint || c == border)
			return false;
		return true;
	}
	void gf (PPoint p)
	{
		//if (! p.check (paint, border, visited, test) )
		if (! check (p.x, p.y) )
			return;
		p.plot ();
		nextlist.push_back ( PPoint (p.x, p.y - 1) );
		nextlist.push_back ( PPoint (p.x, p.y + 1) );
		nextlist.push_back ( PPoint (p.x - 1, p.y) );
		nextlist.push_back ( PPoint (p.x + 1, p.y) );

	}
	void do_paint (int x, int y)
	{
		processlist.push_back (PPoint (x, y) );
		while (! processlist.empty () )
		{
			nextlist.clear ();
			while (! processlist.empty () )
			{
				PPoint cp= processlist.back ();
				processlist.pop_back ();
				gf (cp);
			}
			processlist= nextlist;
		}
	}
};

#endif

#endif

} // namespace

#endif
// BLASSIC_HAS_GRAPHICS

void graphics::paint (int x, int y, int paintattr, int borderattr)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	transform_x (x);
	transform_y (y);

	ColorValue paint= getColorValue (mapcolor (paintattr) );
	ColorValue border= getColorValue (mapcolor (borderattr) );

	setactivecolor (mapcolor (paintattr).pc);
	//do_paint (x, y /*, 0, 0*/, paint, border);
	Painter p (paint, border);
	p.do_paint (x, y /*, 0, 0*/ );
	setactivecolor (pforeground);

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (x, y, paintattr, borderattr);

	#endif
}

void graphics::mask (int m)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	maskvalue= static_cast <unsigned char> (m);
	maskpos= 0;

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (m);

	#endif
}

void graphics::maskdrawfirstpoint (bool f)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	maskdrawfirst= f;

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (f);

	#endif
}

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

char skipblank (const char * & s)
{
	char c;
	while ( (c= * s) == ' ' || c == '\t')
		++s;
	return c;
}

int getnum (const char * & s)
{
	int r= 0;
	char c;
	if (! isdigit (* s) )
	{
		if (showdebuginfo () )
			cerr << "Inavlid number in DRAW string" << endl;
		throw ErrSyntax;
	}
	while ( isdigit (c= * s) )
	{
		r*= 10;
		r+= c - '0';
		++s;
	}
	//cerr << r;
	return r;
}

void drawsyntaxerror ()
{
	if (showdebuginfo () )
		cerr << "Invalid syntax in DRAW string" << endl;
	throw ErrSyntax;
}

enum TypeMove { MoveAbs, MovePos, MoveNeg };

TypeMove gettypemove (const char * & s)
{
	TypeMove typemove= MoveAbs;
	char c= skipblank (s);
	switch (c)
	{
	case '+':
		//cerr << '+';
		typemove= MovePos;
		++s;
		skipblank (s);
		break;
	case '-':
		//cerr << '-';
		typemove= MoveNeg;
		++s;
		skipblank (s);
		break;
	}
	return typemove;
}

inline void adjust (int & value, TypeMove t, int last)
{
	switch (t)
	{
	case MoveAbs:
		break;
	case MovePos:
		value= last + value;
		break;
	case MoveNeg:
		value= last - value;
		break;
	}
}

} // namespace

#endif
// BLASSIC_HAS_GRAPHICS

void graphics::draw (const std::string & str)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	const char * s= str.c_str ();
	char c;
	int i;
	while ( (c= skipblank (s) ) != '\0')
	{
		//cerr << c;
		++s;
		bool nopaint= false;
		if (c == 'B')
		{
			c= skipblank (s);
			//cerr << c;
			++s;
			nopaint= true;
		}
		switch (c)
		{
		case 'M':
			{
				TypeMove mx= gettypemove (s);
				int x= getnum (s);
				adjust (x, mx, lastx);
				c= skipblank (s);
				if (c != ',')
					//throw ErrSyntax;
					drawsyntaxerror ();
				//cerr << ',';
				++s;
				TypeMove my= gettypemove (s);
				int y= getnum (s);
				adjust (y, my, lasty);
				if (nopaint)
					graphics::move (x, y);
				else
					graphics::line (x, y);
			}
			break;
		case 'U':
			skipblank (s);
			i= getnum (s);
			if (nopaint)
				graphics::move (lastx, lasty - i);
			else
				graphics::line (lastx, lasty - i);
			break;
		case 'D':
			skipblank (s);
			i= getnum (s);
			if (nopaint)
				graphics::move (lastx, lasty + i);
			else
				graphics::line (lastx, lasty + i);
			break;
		case 'R':
			skipblank (s);
			i= getnum (s);
			if (nopaint)
				graphics::move (lastx + i, lasty);
			else
				graphics::line (lastx + i, lasty);
			break;
		case 'L':
			skipblank (s);
			i= getnum (s);
			if (nopaint)
				graphics::move (lastx - i, lasty);
			else
				graphics::line (lastx - i, lasty);
			break;
		case 'E':
			skipblank (s);
			i= getnum (s);
			if (nopaint)
				graphics::move (lastx + i, lasty - i);
			else
				graphics::line (lastx + i, lasty - i);
			break;
		case 'F':
			skipblank (s);
			i= getnum (s);
			if (nopaint)
				graphics::move (lastx + i, lasty + i);
			else
				graphics::line (lastx + i, lasty + i);
			break;
		case 'G':
			skipblank (s);
			i= getnum (s);
			if (nopaint)
				graphics::move (lastx - i, lasty + i);
			else
				graphics::line (lastx - i, lasty + i);
			break;
		case 'H':
			skipblank (s);
			i= getnum (s);
			if (nopaint)
				graphics::move (lastx - i, lasty - i);
			else
				graphics::line (lastx - i, lasty - i);
			break;
		case 'C':
			c= skipblank (s);
			if (! isdigit (c) )
				//throw ErrSyntax;
				drawsyntaxerror ();
			graphics::setcolor (c - '0');
			++s;
			break;
		case 'X':
			{
				std::string x;
				while ( (c= *s) != ';' && c != '\0')
				{
					x+= c;
					++s;
				}
				if (c != ';')
					//throw ErrSyntax;
					drawsyntaxerror ();
				++s;
				if (typeofvar (x) != VarString)
					throw ErrMismatch;
				std::string xx= evaluatevarstring (x);
				draw (xx);
			}
			break;
		case ';':
			break;
		default:
			//throw ErrSyntax;
			drawsyntaxerror ();
		}
	}
	//cerr << endl;

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (str);

	#endif
}

graphics::Point graphics::getlast ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return Point (lastx, lasty);

	#else

	return Point (0, 0);

	#endif
}

std::string graphics::inkey ()
{
	#ifdef  BLASSIC_HAS_GRAPHICS

	idle ();
	if (queuekey.empty () )
		return std::string ();

	#if 0

	std::string str= queuekey.front ();
	queuekey.pop ();
	return str;

	#else

	return queuekey.pop ();

	#endif

	#else
	// No BLASSIC_HAS_GRAPHICS

	ASSERT (false);
	throw ErrBlassicInternal; // Make the compiler happy.

	#endif
}

bool graphics::pollin ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	idle ();
	return ! queuekey.empty ();

	#else

	throw ErrBlassicInternal; // Make the compiler happy.

	#endif
}

std::string graphics::getkey ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	#ifdef BLASSIC_USE_X

	while (queuekey.empty () )
		wait_X_event ();
	return queuekey.pop ();

	#else

	for (;;)
	{
		idle ();
		if (! queuekey.empty () )
			return queuekey.pop ();

		// Reduces cpu usage.
		#ifdef BLASSIC_USE_WINDOWS
		Sleep (0);
		#endif
	}

	#endif

	#else
	// No BLASSIC_HAS_GRAPHICS

	ASSERT (false); // Must not came here.
	throw ErrBlassicInternal; // Make the compiler happy.

	#endif
}

int graphics::keypressed (int keynum)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	#if 0

	if (keynum < 0 || keynum > static_cast <int> (MAXINKEYCODE) )
		return -1;
	idle ();
	if (! keypressedmap [presscode [keynum] ] )
		return -1;
	int r= 0;
	const int shiftpressed= 32, ctrlpressed= 128;

	#ifdef BLASSIC_USE_X
	if (keypressedmap [XK_Shift_L] || keypressedmap [XK_Shift_R] )
		r|= shiftpressed;
	if (keypressedmap [XK_Control_L] || keypressedmap [XK_Control_R] )
		r|= ctrlpressed;
	#endif

	return r;

	#else
	return ::keypressed (keynum);
	#endif

	#else
	// No graphics.

	touch (keynum);
	ASSERT (false); // Must not came here.
	throw ErrBlassicInternal; // Make the compiler happy.

	#endif
}

namespace {

int symbol_after_is;

inline bool iscontrolchar (char c)
{
	//return iscntrl (static_cast <unsigned char> (c) );
	unsigned char c1= static_cast <unsigned char> (c);
	return c1 < 32;
}

} // namespace

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

class BlWindow {
	size_t collecting_params;
	std::string params;
	unsigned char actual_control;
	struct DefControlChar {
		size_t nparams;
		bool force;
		void (BlWindow::* action) ();
		DefControlChar (size_t nparams, bool force,
				void (BlWindow::* action) () ) :
			nparams (nparams),
			force (force),
			action (action)
		{ }
		// Default constructor required by the initialization
		// of the escape map (I don't want to use insert
		// instead of [ ] ).
		DefControlChar ()
		{ }
	};
	static const DefControlChar control [];
	typedef std::map <char, DefControlChar> escape_t;
	static const escape_t escape;
	static escape_t init_escape ();
	void ignore ()
	{ /* Nothing to do */ }
	void do_SOH () // 1
	{
		do_charout (params [0] );
	}
	void do_STX () // 2
	{
		cursor_visible= false;
	}
	void do_ETX () // 3
	{
		cursor_visible= true;
	}
	void do_ENQ () // 5
	{
		tag_charout (params [0] );
	}
	void do_BEL () // 7
	{
		graphics::ring ();
	}
	void do_BS () // 8
	{
		--x;
	}
	void do_TAB () // 9
	{
		++x;
	}
	void do_LF () // 10
	{
		//x= 0;
		++y;
	}
	void do_VT () // 11
	{
		--y;
	}
	void do_FF () // 12
	{
		cls ();
	}
	void do_CR () // 13
	{
		x= 0;
	}
	void do_SO () // 14
	{
		setbackground (static_cast <unsigned char> (params [0] ) % 16);
	}
	void do_SI () // 15
	{
		setcolor (static_cast <unsigned char> (params [0] ) % 16 );
	}
	void do_DLE () // 16
	{
		clear_rectangle (x, x, y, y);
	}
	void do_DC1 () // 17
	{
		clear_from_left ();
	}
	void do_DC2 () // 18
	{
		clear_to_right ();
	}
	void do_DC3 () // 19
	{
		clear_from_begin ();
	}
	void do_DC4 () // 20
	{
		clear_to_end ();
	}
	void do_SYN () // 22
	{
		graphics::settransparent
			(static_cast <unsigned char> (params [0] ) );
	}
	void do_ETB () // 23
	{
		// Ink mode, only CPC modes are allowed.
		graphics::setdrawmode
			(static_cast <unsigned char> (params [0] ) % 4);
	}
	void do_CAN () // 24
	{
		std::swap (foreground, background);
	}
	void do_EM () // 25
	{
		int symbol= static_cast <unsigned char> (params [0] );
		// Avoid generate an error if out of range.
		if (symbol < symbol_after_is)
			return;
		unsigned char byte [8];
		params.copy (reinterpret_cast <char *> (& byte [0] ), 8, 1);
		graphics::definesymbol (symbol, byte);
	}
	void do_SUB () // 26
	{
		set (static_cast <unsigned char> (params [0] ),
			static_cast <unsigned char> (params [1] ),
			static_cast <unsigned char> (params [2] ),
			static_cast <unsigned char> (params [3] ) );
	}
	void do_ESC () // 27
	{
		char c= params [0];
		escape_t::const_iterator it= escape.find (c);
		if (it == escape.end () )
		{
			do_charout (c);
			return;
		}
		const DefControlChar & defcontrol= it->second;
		if (defcontrol.nparams == 0)
		{
			if (defcontrol.force)
				forcelegalposition ();
			(this->* defcontrol.action) ();
			return;
		}
		collecting_params= defcontrol.nparams;
		// All escapes used have non-control codes,
		// then we can use the character code
		// without confusion.
		ASSERT (! iscontrolchar (c) );

		actual_control= c;
		params.erase ();
	}
	void do_FS () // 28
	{
		int ink= static_cast <unsigned char> (params [0] ) % 16;
		int color= static_cast <unsigned char> (params [1] ) % 32;
		if (color > 26)
			return;
		// Third parameter (flashing ink) ignored.
		graphics::ink (ink, color);
	}
	void do_RS () // 30
	{
		x= 0; y= 0;
	}
	void do_US () // 31
	{
		int xx= static_cast <unsigned char> (params [0] );
		int yy= static_cast <unsigned char> (params [1] );
		if (xx == 0 || yy == 0)
			return;
		x= xx - 1; y= yy - 1;
	}
	void do_ESC_A ()
	{
		if (y > 0)
			--y;
	}
	void do_ESC_B ()
	{
		if (y < height - 1)
			++y;
	}
	void do_ESC_C ()
	{
		if (x < width - 1)
			++x;
	}
	void do_ESC_D ()
	{
		if (x > 0)
			--x;
	}
	void do_ESC_E ()
	{
		clear_rectangle (0, width - 1, 0, height - 1);
	}
	void do_ESC_H ()
	{
		x= 0; y= 0;
	}
	void do_ESC_I ()
	{
		--y;
	}
	void do_ESC_J ()
	{
		clear_to_end ();
	}
	void do_ESC_K ()
	{
		clear_to_right ();
	}
	void do_ESC_L ()
	{
		textscrollinverse (y);
	}
	void do_ESC_M ()
	{
		textscroll (y);
	}
	void do_ESC_N ()
	{
		deletechar ();
	}
	void do_ESC_Y ()
	{
		y= static_cast <unsigned char> (params [0] ) - 32;
		x= static_cast <unsigned char> (params [1] ) - 32;
		if (y > height - 1)
			y= height - 1;
		if (x > width - 1)
			x= width - 1;
	}
	void do_ESC_d ()
	{
		clear_from_begin ();
	}
	void do_ESC_e ()
	{
		cursor_visible= true;
	}
	void do_ESC_f ()
	{
		cursor_visible= false;
	}
	void do_ESC_j ()
	{
		savex= x; savey= y;
	}
	void do_ESC_k ()
	{
		x= savex; y= savey;
	}
	void do_ESC_l ()
	{
		clear_rectangle (0, width - 1, y, y);
	}
	void do_ESC_o ()
	{
		clear_from_left ();
	}
	void do_ESC_p ()
	{
		inverse= true;
		//std::swap (foreground, background);
	}
	void do_ESC_q ()
	{
		inverse= false;
	}
	void do_ESC_r ()
	{
		underline= true;
	}
	void do_ESC_u ()
	{
		underline= false;
	}
	void do_ESC_x ()
	{
		// set mode 24 x 80
		setdefault ();
		cls ();
		set (0, 79, 0, 23);
	}
	void do_ESC_y ()
	{
		// unset mode 24 x 80
		setdefault ();
		cls ();
	}
public:
	BlWindow () :
		collecting_params (0),
		fTag (false),
		cursor_visible (true),
		inverse (false),
		bright (false),
		underline (false)
	{
		setdefault ();
		defaultcolors ();
	}
	BlWindow (int x1, int x2, int y1, int y2) :
		collecting_params (0),
		fTag (false),
		cursor_visible (true),
		inverse (false),
		bright (false),
		underline (false)
	{
		set (x1, x2, y1, y2);
		defaultcolors ();
	}
	void setdefault ()
	{
		TRACEFUNC (tr, "BlWindow::setdefault");

		set (0, maxtcol - 1, 0, maxtrow - 1);
	}
	void set (int x1, int x2, int y1, int y2)
	{
		TRACEFUNC (tr, "BlWindow::set");

		if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0)
		{
			TRMESSAGE (tr, "Invalid window values");
			throw ErrImproperArgument;
		}
		if (x1 > x2) std::swap (x1, x2);
		if (y1 > y2) std::swap (y1, y2);
		if (x1 >= maxtcol) x1= maxtcol - 1;
		if (x2 >= maxtcol) x2= maxtcol - 1;
		if (y1 >= maxtrow) y1= maxtrow - 1;
		if (y2 >= maxtrow) y2= maxtrow - 1;
		orgx= x1; orgy= y1;
		width= x2 - x1 + 1;
		height= y2 - y1 + 1;
		x= y= 0;
	}
	void defaultcolors ()
	{
		pen= default_pen;
		paper= default_paper;
		foreground= mapcolor (default_pen).pc;
		background= mapcolor (default_paper).pc;
	}
	void setinverse (bool active) { inverse= active; }
	bool getinverse () { return inverse; }
	void setbright (bool active)
	{
		bool previous= bright;
		bright= active;
		// PENDIENTE
		if (bright != previous)
		{
			int color= pen;
			if (color >= 0 && color <= 7)
			{
				if (bright)
					color+= 8;
				foreground= mapcolor (color).pc;
			}
			color= paper;
			if (color >= 0 && color <= 7)
			{
				if (bright)
					color+= 8;
				background= mapcolor (color).pc;
			}
		}
	}
	bool getbright () { return bright; }
	int getwidth () const { return width; }
	int getxpos () const { return x; }
	int getypos () const { return y; }
	void gotoxy (int x, int y)
	{
		this->x= x; this->y= y;
	}
	void forcelegalposition ()
	{
		if (x >= width)
			{ x= 0; ++y; }
		if (x < 0)
			{ x= width - 1; --y; }
		if (y < 0)
		{
			y= 0;
			textscrollinverse (0);
		}
		if (y >= height)
		{
			textscroll (0);
			y= height - 1;
		}
	}
	void tab ()
	{
		forcelegalposition ();
		int zone= sysvar::get16 (sysvar::Zone);
		if (zone == 0)
			zone= 8;
		if (x >= (width / zone) * zone)
		{
			//cerr << "Fin de linea" << endl;
			int yy= orgy + y;
			for ( ; x < width; ++x)
				print (orgx + x, yy, ' ', inverse, underline);
			x= 0;
			++y;
		}
		else
		{
			int yy= orgy + y;
			do {
				print (orgx + x, yy, ' ', inverse, underline);
				++x;
			} while (x % zone);
		}
	}
	void tab (size_t n)
	{
		forcelegalposition ();
		int col= n;
		if (x > col)
		{
			do {
				charout (' ');
			} while (x < width);
		}
		int maxpos= std::min (col, width);
		while (x < maxpos)
			charout (' ');
		x= col;
	}
	void setcolor (int color)
	{
		pen= color;
		if (bright && color >=0 && color <= 7)
			color+= 8;
		foreground= mapcolor (color).pc;
	}
	int getcolor ()
	{
		return pen;
	}
	void setbackground (int color)
	{
		paper= color;
		if (bright && color >=0 && color <= 7)
			color+= 8;
		background= mapcolor (color).pc;
	}
	int getbackground ()
	{
		return paper;
	}
	void movecharforward ()
	{
		forcelegalposition ();
		++x;
	}
	void movecharforward (size_t n)
	{
		for ( ; n > 0; --n)
			movecharforward ();
	}
	void movecharback ()
	{
		forcelegalposition ();
		--x;
	}
	void movecharback (size_t n)
	{
		for ( ; n > 0; --n)
			movecharback ();
	}
	void movecharup ()
	{
		forcelegalposition ();
		--y;
	}
	void movecharup (size_t n)
	{
		for ( ; n > 0; --n)
			movecharup ();
	}
	void movechardown ()
	{
		forcelegalposition ();
		++y;
	}
	void movechardown (size_t n)
	{
		for ( ; n > 0; --n)
			movechardown ();
	}
	void clear_from_left ()
	{
		clear_rectangle (0, x, y, y);
	}
	void clear_to_right ()
	{
		clear_rectangle (x, width - 1, y, y);
	}
	void clear_from_begin ()
	{
		if (y > 0)
			clear_rectangle (0, width - 1, 0, y - 1);
		clear_from_left ();
	}
	void clear_to_end ()
	{
		clear_to_right ();
		if (y < height - 1)
			clear_rectangle (0, width - 1, y + 1, height - 1);
	}
	void cls ()
	{
		x= y= 0;
		clear_rectangle (0, width - 1, 0, height - 1);
	}
	void clear_rectangle (int left, int right, int top, int bottom)
	{
		#if 1

		int x1= (orgx + left) * charwidth;
		int x2= (orgx + right + 1) * charwidth;
		int y1= (orgy + top) * charheight;
		int y2= (orgy + bottom + 1) * charheight;
		setactivecolor (background);
		do_fill_rectangle (x1, y1, x2 - 1, y2 - 1, false);
		setactivecolor (pforeground);

		#else

		int x1= (orgx + left) * charwidth;
		int w= (right - left + 1) * charwidth;
		int y1= (orgy + top) * charheight;
		int h= (bottom - top + 1) * charheight;

		#ifdef BLASSIC_USE_WINDOWS

		RECT r= { x1, y1, x1 + w, y1 + h };
		LOGPEN logpen;
		GetObject (* background, sizeof (LOGPEN), & logpen);
		HBRUSH hbrush= CreateSolidBrush (logpen.lopnColor);
		if (! fSynchro)
			FillRect (hdc, & r, hbrush);
		FillRect (hdcPixmap, & r, hbrush);
		DeleteObject (hbrush);

		#endif

		#ifdef BLASSIC_USE_X

		setactivecolor (background);
		XSetFunction (display, gcp, drawmode_copy);
		XFillRectangle (display, pixmap, gcp,
			x1, y1, w, h);
		XSetFunction (display, gcp, drawmode);
		if (! fSynchro)
		{
			XSetFunction (display, gc, drawmode_copy);
			XFillRectangle (display, window, gc,
				x1, y1, w, h);
			XSetFunction (display, gc, drawmode);
			// Inserted an idle call because without it
			// the window sometimes is not updated.
			graphics::idle ();
		}
		setactivecolor (pforeground);

		#endif

		#endif
	}
	void deletechar ()
	{
		int x1= (orgx + x) * charwidth;
		int y1= (orgy + y) * charheight;
		int w= (width - x - 1) * charwidth;
		int h= charheight;

		#ifdef BLASSIC_USE_X

		XSetFunction (display, gcp, drawmode_copy);
		XCopyArea (display, pixmap, pixmap, gcp,
			x1 + charwidth, y1, w, h, x1, y1);
		setactivecolor (background);
		XFillRectangle (display, pixmap, gcp,
			x1 + w, y1, charwidth, h);
		if (! fSynchro)
			reinit_window (x1, y1, w + charwidth, h);
		setactivecolor (foreground);

		#elif defined BLASSIC_USE_WINDOWS

		RECT r= { x1 + w, y1, x1 + w + charwidth, y1 + h };
		BitBlt (hdcPixmap, x1, y1, w, h,
			hdcPixmap, x1 + charwidth, y1, SRCCOPY);
		LOGPEN logpen;
		GetObject (* background, sizeof (LOGPEN), & logpen);
		HBRUSH hbrush= CreateSolidBrush (logpen.lopnColor);
		FillRect (hdcPixmap, & r, hbrush);
		DeleteObject (hbrush);
		if (! fSynchro)
			reinit_window (x1, y1, w + charwidth, h);

		#endif
	}
	void textscroll (int fromline)
	{
		int x1= orgx * charwidth;
		int y1= (orgy + fromline) * charheight;
		int w= width * charwidth;
		int h= (height - 1 - fromline) * charheight;
		int x2= x1 + w;
		int y2= y1 + h;

		// Reinit rectangle. This are not rotated because
		// do_fill_rectangle will do it.
		int x1reinit= x1;
		int y1reinit= y1;
		int x2reinit= x1 + w;
		int y2reinit= y1 + h + charheight;

		int x1fill= x1;
		int y1fill= y1 + h;
		int x2fill= x1 + w - 1;
		int y2fill= y1 + h + charheight - 1;

		do_rotate (x1, y1);
		do_rotate (x2, y2);
		if (x1 > x2)
			std::swap (x1, x2);
		if (y1 > y2)
			std::swap (y1, y2);
		do_rotate_rel (w, h);

		int x1src= x1;
		int y1src= y1;
		switch (rotate)
		{
		case RotateNone:
			y1src+= charheight;
			break;
		case Rotate90:
			x1src+= charheight;
			break;
		}

		do_rotate (x1reinit, y1reinit);
		do_rotate (x2reinit, y2reinit);
		if (x1reinit > x2reinit)
			std::swap (x1reinit, x2reinit);
		if (y1reinit > y2reinit)
			std::swap (y1reinit, y2reinit);

		// Operate only in the pixmap, when finished update
		// the visible window if not in synchro mode.

		// Do the scrolling.

		#ifdef BLASSIC_USE_X

		XSetFunction (display, gcp, drawmode_copy);
		XCopyArea (display, pixmap, pixmap, gcp,
			x1src, y1src, w, h, x1, y1);

		#if 0
		setactivecolor (background);
		XFillRectangle (display, pixmap, gcp,
			x1, y1 + h, w, charheight);
		#endif

		XSetFunction (display, gcp, drawmode);

		#elif defined BLASSIC_USE_WINDOWS

		BitBlt (hdcPixmap, x1, y1, w, h,
			hdcPixmap, x1src, y1src, SRCCOPY);

		#if 0
		LOGPEN logpen;
		GetObject (* background, sizeof (LOGPEN), & logpen);
		HBRUSH hbrush= CreateSolidBrush (logpen.lopnColor);
		RECT r = { x1, y1 + h, x1 + w, y1 + h + charheight};
		FillRect (hdcPixmap, & r, hbrush);
		DeleteObject (hbrush);
		#endif

		#endif

		// Fill the new line with the background color.
		setactivecolor (background);
		do_fill_rectangle (x1fill, y1fill, x2fill, y2fill, false);
		setactivecolor (foreground);

		// Test.
		//#ifdef BLASSIC_USE_X
		#if 0

		// Generate a expose event for the scrolled area.
		XExposeEvent event;
		event.type= Expose;
		event.display= display;
		event.window= window;
		event.x= x1;
		event.y= y1;
		event.width= w;
		event.height= h;
		event.count= 0;
		XSendEvent (display, window, False, 0,
			reinterpret_cast <XEvent *> (& event) );

		#else

		// And update window if not in synchro mode.
		if (! fSynchro)
			//reinit_window (x1, y1, w, h + charheight);
			reinit_window (x1reinit, y1reinit,
				x2reinit - x1reinit, y2reinit - y1reinit);

		#endif
	}
	void textscrollinverse (int fromline)
	{
		int x1= orgx * charwidth;
		int y1= (orgy + fromline) * charheight;
		int w= width * charwidth;
		int h= (height - 1 - fromline) * charheight;

		do_rotate (x1, y1);
		do_rotate (w, h);

		#ifdef BLASSIC_USE_X

		XSetFunction (display, gcp, drawmode_copy);
		XCopyArea (display, pixmap, pixmap, gcp,
			x1, y1, w, h, x1, y1 + charheight);
		setactivecolor (background);
		XFillRectangle (display, pixmap, gcp,
			x1, y1, w, charheight);
		XSetFunction (display, gcp, drawmode);

		if (! fSynchro)
			reinit_window (x1, y1, w, h + charheight);
		setactivecolor (foreground);

		#elif defined BLASSIC_USE_WINDOWS

		RECT r = { x1, y1, x1 + w, y1 + charheight};
		BitBlt (hdcPixmap, x1, y1 + charheight, w, h,
			hdcPixmap, x1, y1, SRCCOPY);
		LOGPEN logpen;
		GetObject (* background, sizeof (LOGPEN), & logpen);
		HBRUSH hbrush= CreateSolidBrush (logpen.lopnColor);
		FillRect (hdcPixmap, & r, hbrush);
		DeleteObject (hbrush);
		if (! fSynchro)
			reinit_window (x1, y1, w, h + charheight);

		#endif
	}
	void scroll (int n)
	{
		forcelegalposition ();
		if (n < 0)
		{
			for ( ; n < 0; ++n)
				textscrollinverse (0);
			gotoxy (0, 0);
		}
		else
		{
			for ( ; n > 0; --n)
				textscroll (0);
			gotoxy (0, height - 1);
		}
	}
	void tag_charout (char c)
	{
		int x= lastx, y= lasty;
		lastx+= charwidth;
		transform_x (x);
		transform_y (y);
		printxy (x, y, c, true);
	}
	void charout (char c)
	{
		TRACEFUNC (tr, "BlWindow::charout");

		if (fTag)
		{
			tag_charout (c);
			return;
		}
		if (collecting_params)
		{
			TRMESSAGE (tr, "collecting params");
			params+= c;
			if (--collecting_params == 0)
			{
				const DefControlChar & defcontrol=
					actual_control < 32 ?
					control [actual_control] :
					escape.find (actual_control)->second;
				if (defcontrol.force)
					forcelegalposition ();
				(this->* defcontrol.action) ();
			}
			return;
		}
		if (iscontrolchar (c) )
		{
			TRMESSAGE (tr, "Is control char");
			actual_control= c;
			params.erase ();
			const DefControlChar & defcontrol=
				control [actual_control];
			if (defcontrol.nparams > 0)
				collecting_params= defcontrol.nparams;
			else
			{
				if (defcontrol.force)
					forcelegalposition ();
				(this->* defcontrol.action) ();
			}
			return;
		}
		forcelegalposition ();
		pcolor foresave= pforeground;
		pforeground= foreground;
		pcolor backsave= pbackground;
		pbackground= background;
		#if 0
		switch (c)
		{
		case '\n':
			x= 0;
			++y;
			break;
		case '\b':
			--x;
			break;
		case '\r':
			x= 0;
			break;
		case '\t':
			if (x >= (width / zone) * zone)
			{
				//cerr << "Fin de linea" << endl;
				int yy= orgy + y;
				for ( ; x < width; ++x)
					print (orgx + x, yy, ' ',
						inverse, underline);
				x= 0;
				++y;
			}
			else
			{
				int yy= orgy + y;
				do {
					print (orgx + x, yy, ' ',
						inverse, underline);
					++x;
				} while (x % zone);
			}
			break;
		default:
		#endif
			print (orgx + x, orgy + y, c, inverse, underline);
			++x;
		#if 0
		}
		#endif
		pforeground= foresave;
		pbackground= backsave;
	}
	void do_charout (char c)
	{
		pcolor foresave= pforeground;
		pforeground= foreground;
		pcolor backsave= pbackground;
		pbackground= background;
		print (orgx + x, orgy + y, c, inverse, underline);
		pforeground= foresave;
		pbackground= backsave;
		++x;
	}
	void invertcursor ()
	{
		forcelegalposition ();
		if (! cursor_visible)
			return;
		int x1= (orgx + x) * charwidth;
		int y1= (orgy + y) * charheight;
		int x1ini= x1;
		int y1ini= y1 + charheight - 2;
		int x1end= x1 + charwidth;
		int y1end= y1 + charheight;
		do_rotate (x1, y1);
		do_rotate (x1ini, y1ini);
		do_rotate (x1end, y1end);
		if (x1ini > x1end)
			std::swap (x1ini, x1end);
		if (y1ini > y1end)
			std::swap (y1ini, y1end);

		#ifdef BLASSIC_USE_X

		XSetFunction (display, gc, drawmode_invert);
		XSetFunction (display, gcp, drawmode_invert);
		XFillRectangle (display, window, gc,
			x1ini, y1ini, x1end - x1ini, y1end - y1ini);
		XFillRectangle (display, pixmap, gcp,
			x1ini, y1ini, x1end - x1ini, y1end - y1ini);
		XSetFunction (display, gc, drawmode);
		XSetFunction (display, gcp, drawmode);

		#elif defined BLASSIC_USE_WINDOWS

		HBRUSH hbrush= (HBRUSH) GetStockObject (BLACK_BRUSH);
		HDC ahdc [2]= { hdc, hdcPixmap };
		for (size_t i= 0; i < 2; ++i)
		{
			HDC hdc= ahdc [i];
			SetROP2 (hdc, drawmode_invert);
			HBRUSH hold= (HBRUSH) SelectObject (hdc, hbrush);
			//Rectangle (hdc, x1, y1 + 6, x1 + 8, y1 + 8);
			Rectangle (hdc, x1ini, y1ini, x1end, y1end);
			SelectObject (hdc, hold);
			SetROP2 (hdc, drawmode);
		}

		#endif
	}
	std::string copychr (BlChar from, BlChar to)
	{
		// I don't tested yet if that is done in the cpc
		forcelegalposition ();

		int x1= (orgx + x) * charwidth;
		int y1= (orgy + y) * charheight;
		return copychrat (x1, y1, from , to);
	}
	void tag ()
	{
		fTag= true;
	}
	void tagoff ()
	{
		fTag= false;
	}
	bool istagactive ()
	{
		return fTag;
	}
private:
	int orgx, orgy, width, height;
	pcolor foreground;
	pcolor background;
	int x, y, savex, savey;
	bool fTag;
	bool cursor_visible;
	bool inverse;
	bool bright;
	bool underline;
	int pen;
	int paper;
};

const BlWindow::DefControlChar BlWindow::control [32]= {
	BlWindow::DefControlChar (0, false, & BlWindow::ignore), // NUL
	BlWindow::DefControlChar (1, true,  & BlWindow::do_SOH), // SOH
	BlWindow::DefControlChar (0, false, & BlWindow::do_STX), // STX
	BlWindow::DefControlChar (0, false, & BlWindow::do_ETX), // ETX
	BlWindow::DefControlChar (1, false, & BlWindow::ignore), // EOT
	BlWindow::DefControlChar (1, false, & BlWindow::do_ENQ), // ENQ
	BlWindow::DefControlChar (0, false, & BlWindow::ignore), // ACK
	BlWindow::DefControlChar (0, false, & BlWindow::do_BEL), // BEL
	BlWindow::DefControlChar (0, true,  & BlWindow::do_BS ), // BS
	BlWindow::DefControlChar (0, true,  & BlWindow::do_TAB), // TAB
	BlWindow::DefControlChar (0, true,  & BlWindow::do_LF ), // LF
	BlWindow::DefControlChar (0, true,  & BlWindow::do_VT ), // VT
	BlWindow::DefControlChar (0, false, & BlWindow::do_FF ), // FF
	BlWindow::DefControlChar (0, true,  & BlWindow::do_CR ), // CR
	BlWindow::DefControlChar (1, false, & BlWindow::do_SO ), // SO
	BlWindow::DefControlChar (1, false, & BlWindow::do_SI ), // SI
	BlWindow::DefControlChar (0, true,  & BlWindow::do_DLE), // DLE
	BlWindow::DefControlChar (0, true,  & BlWindow::do_DC1), // DC1
	BlWindow::DefControlChar (0, true,  & BlWindow::do_DC2), // DC2
	BlWindow::DefControlChar (0, true,  & BlWindow::do_DC3), // DC3
	BlWindow::DefControlChar (0, true,  & BlWindow::do_DC4), // DC4
	BlWindow::DefControlChar (0, false, & BlWindow::ignore), // NAK
	BlWindow::DefControlChar (1, false, & BlWindow::do_SYN), // SYN
	BlWindow::DefControlChar (1, false, & BlWindow::do_ETB), // ETB
	BlWindow::DefControlChar (0, false, & BlWindow::do_CAN), // CAN
	BlWindow::DefControlChar (9, false, & BlWindow::do_EM ), // EM
	BlWindow::DefControlChar (4, false, & BlWindow::ignore), // SUB
	BlWindow::DefControlChar (1, false, & BlWindow::do_ESC), // ESC
	BlWindow::DefControlChar (3, false, & BlWindow::do_FS ), // FS
	BlWindow::DefControlChar (2, false, & BlWindow::ignore), // GS
	BlWindow::DefControlChar (0, false, & BlWindow::do_RS ), // RS
	BlWindow::DefControlChar (2, false, & BlWindow::do_US ), // US
};

BlWindow::escape_t BlWindow::init_escape ()
{
	escape_t aux;
	DefControlChar ignore0 (0, false, & BlWindow::ignore);
	DefControlChar ignore1 (1, false, & BlWindow::ignore);
	aux ['0']= ignore0; // Status line inactive.
	aux ['1']= ignore0; // Status line active.
	aux ['2']= ignore1; // Select national character set
	aux ['3']= ignore1; // Set mode
	aux ['A']= DefControlChar (0, true,  & BlWindow::do_ESC_A);
	aux ['B']= DefControlChar (0, true,  & BlWindow::do_ESC_B);
	aux ['C']= DefControlChar (0, true,  & BlWindow::do_ESC_C);
	aux ['D']= DefControlChar (0, true,  & BlWindow::do_ESC_D);
	aux ['E']= DefControlChar (0, false, & BlWindow::do_ESC_E);
	aux ['H']= DefControlChar (0, false, & BlWindow::do_ESC_H);
	aux ['I']= DefControlChar (0, true,  & BlWindow::do_ESC_I);
	aux ['J']= DefControlChar (0, true,  & BlWindow::do_ESC_J);
	aux ['K']= DefControlChar (0, true,  & BlWindow::do_ESC_K);
	aux ['L']= DefControlChar (0, true,  & BlWindow::do_ESC_L);
	aux ['M']= DefControlChar (0, true,  & BlWindow::do_ESC_M);
	aux ['N']= DefControlChar (0, true,  & BlWindow::do_ESC_N);
	aux ['Y']= DefControlChar (2, false, & BlWindow::do_ESC_Y);
	aux ['d']= DefControlChar (0, true,  & BlWindow::do_ESC_d);
	aux ['e']= DefControlChar (0, false, & BlWindow::do_ESC_e);
	aux ['f']= DefControlChar (0, false, & BlWindow::do_ESC_f);
	aux ['j']= DefControlChar (0, false, & BlWindow::do_ESC_j);
	aux ['k']= DefControlChar (0, false, & BlWindow::do_ESC_k);
	aux ['l']= DefControlChar (0, true,  & BlWindow::do_ESC_l);
	aux ['o']= DefControlChar (0, true,  & BlWindow::do_ESC_o);
	aux ['p']= DefControlChar (0, false, & BlWindow::do_ESC_p);
	aux ['q']= DefControlChar (0, false, & BlWindow::do_ESC_q);
	aux ['r']= DefControlChar (0, false, & BlWindow::do_ESC_r);
	aux ['u']= DefControlChar (0, false, & BlWindow::do_ESC_u);
	aux ['x']= DefControlChar (0, false, & BlWindow::do_ESC_x);
	aux ['y']= DefControlChar (0, false, & BlWindow::do_ESC_y);
	return aux;
}

const BlWindow::escape_t BlWindow::escape= BlWindow::init_escape ();

BlWindow windowzero;

//typedef std::map <BlChannel, BlWindow *> MapWindow;

// Map encapsultated to check access. Windows must be accessed only
// throug file, then accesing a window that not exists is a internal
// error. Also destruction of windows is now granted.

class MapWindow {
	typedef std::map <BlChannel, BlWindow *> map_type;
	map_type mw;
public:
	typedef map_type::iterator iterator;
	typedef map_type::const_iterator const_iterator;
	typedef map_type::value_type value_type;
	typedef map_type::key_type key_type;
	typedef map_type::mapped_type mapped_type;

	~MapWindow ();
	iterator begin ();
	iterator end ();
	const_iterator begin () const;
	const_iterator end () const;
	// operators [] are checked, fail if the key not exist.
	mapped_type operator [] (key_type n) const;
	iterator find (key_type n);
	static void killwindowifnotzero (const value_type & vt);
	void clear ();
	void insert (const value_type & val);
	void erase (iterator pos);
};

MapWindow::~MapWindow ()
{
	// Ensure window destruction on exit.
	clear ();
}

MapWindow::iterator MapWindow::begin ()
{
	return mw.begin ();
}

MapWindow::iterator MapWindow::end ()
{
	return mw.end ();
}

MapWindow::const_iterator MapWindow::begin () const
{
	return mw.begin ();
}

MapWindow::const_iterator MapWindow::end () const
{
	return mw.end ();
}

MapWindow::mapped_type MapWindow::operator [] (key_type n) const
{
	const_iterator it= mw.find (n);
	if (it == end () )
	{
		if (showdebuginfo () )
			cerr << "Trying to access to window " <<
				n << " that not exists" << endl;
		throw ErrBlassicInternal;
	}
	return it->second;
}

MapWindow::iterator MapWindow::find (key_type n)
{
	return mw.find (n);
}

void MapWindow::killwindowifnotzero (const value_type & vt)
{
	if (vt.first != BlChannel (0) )
		delete vt.second;
}

void MapWindow::clear ()
{
	std::for_each (begin (), end (),
		& MapWindow::killwindowifnotzero);
	mw.clear ();
}

void MapWindow::insert (const value_type & val)
{
	// Fail if already exist.
	std::pair <iterator, bool> r= mw.insert (val);
	if (! r.second)
	{
		if (showdebuginfo () )
			cerr << "Trying to create window " <<
				val.first << " that already exists" <<
				endl;
		throw ErrBlassicInternal;
	}
}

void MapWindow::erase (iterator pos)
{
	// Unchecked, actually.
	mw.erase (pos);
}


MapWindow mapwindow;

void destroy_text_windows ()
{
	mapwindow.clear ();
}

void recreate_windows ()
{
	TRACEFUNC (tr, "recreate_windows");

	windowzero.setdefault ();
	windowzero.defaultcolors ();
	windowzero.cls ();

	//std::for_each (mapwindow.begin (), mapwindow.end (),
	//	killwindowifnotzero);
	//mapwindow.clear ();
	destroy_text_windows ();

	//mapwindow [0]= & windowzero;
	mapwindow.insert (std::make_pair (BlChannel (0), & windowzero) );
}

#if 0
inline void do_charout (char c)
{
	switch (c)
	{
	case '\n':
		tcol= 0;
		if (++trow >= maxtrow)
		{
			textscroll ();
			trow= maxtrow - 1;
		}
		return;
	case '\r':
		tcol= 0;
		return;
	case '\t':
		{
			int zone= sysvar::get16 (sysvar::Zone);
			if (zone == 0)
				zone= 8;
			if (tcol >= (maxtcol / zone) * zone)
			{
				//cerr << "Fin de linea" << endl;
				for ( ; tcol < maxtcol; ++tcol)
					print (tcol, trow, ' ', false);
				tcol= 0;
				if (++trow >= maxtrow)
				{
					textscroll ();
					trow= maxtrow - 1;
				}
			}
			else
			{
				do {
					print (tcol, trow, ' ', false);
					++tcol;
				} while (tcol % zone);
			}
			return;
		}
	}
	print (tcol, trow, c, false);
	if (++tcol >= maxtcol)
	{
		tcol= 0;
		if (++trow >= maxtrow)
		{
			textscroll ();
			trow= maxtrow - 1;
		}
	}
}
#endif

} // namespace

#endif
// BLASSIC_HAS_GRAPHICS

void graphics::setcolor (BlChannel ch, int color)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->setcolor (color);

	#else

	touch (ch, color);
	no_graphics_support ();

	#endif
}

int graphics::getcolor (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return mapwindow [ch]->getcolor ();

	#else

	touch (ch);
	no_graphics_support ();
	throw ErrBlassicInternal; // Make the compiler happy

	#endif
}

void graphics::setbackground (BlChannel ch, int color)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->setbackground (color);

	#else

	touch (ch, color);
	no_graphics_support ();

	#endif
}

int graphics::getbackground (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return mapwindow [ch]->getbackground ();

	#else

	touch (ch);
	no_graphics_support ();
	throw ErrBlassicInternal;

	#endif
}

void graphics::cls (BlChannel n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [n]->cls ();

	#else

	touch (n);
	no_graphics_support ();

	#endif
}

void graphics::definewindow (BlChannel n, int x1, int x2, int y1, int y2)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	--x1; --x2; --y1; --y2;
	MapWindow::iterator it= mapwindow.find (n);
	if (it != mapwindow.end () )
		it->second->set (x1, x2, y1, y2);
	else
	{
		//mapwindow [n]= new BlWindow (x1, x2, y1, y2);

		// auto_ptr protects the new window in case
		// that mapwindows.insert throws.
		auto_ptr <BlWindow> pwin (new BlWindow (x1, x2, y1, y2) );
		mapwindow.insert (std::make_pair (n, pwin.get () ) );
		pwin.release ();
	}

	#else

	touch (n, x1, x2, y1, y2);

	#endif
}

void graphics::undefinewindow (BlChannel n)
{
	if (n == 0)
		return;

	#ifdef BLASSIC_HAS_GRAPHICS

	MapWindow::iterator it= mapwindow.find (n);
	if (it != mapwindow.end () )
	{
		delete it->second;
		mapwindow.erase (it);
	}

	#else

	ASSERT (false);
	no_graphics_support ();

	#endif
}

#if 0
size_t graphics::getlinewidth ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	#if 0
	return maxtcol;
	#else
	return windowzero.getwidth ();
	#endif

	#else

	return 0;

	#endif
}
#endif

size_t graphics::getlinewidth (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return mapwindow [ch]->getwidth ();

	#else

	touch (ch);
	no_graphics_support ();
	return 0; // Make the compiler happy

	#endif
}

#if 0
void graphics::charout (char c)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	//do_charout (c);
	windowzero.charout (c);
	//idle ();

	#else

	touch (c);

	#endif
}

void graphics::stringout (const std::string & str)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	for (std::string::size_type i= 0, l= str.size (); i < l; ++i)
		windowzero.charout (str [i]);

	#else

	touch (str);

	#endif
}
#endif

void graphics::charout (BlChannel ch, char c)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->charout (c);

	#else

	touch (ch, c);

	#endif
}

void graphics::stringout (BlChannel ch, const std::string & str)
{
	TRACEFUNC (tr, "graphics::stringout");
	TRMESSAGE (tr, "Channel: " + to_string (ch) );
	TRMESSAGE (tr, "String: " + str);

	#ifdef BLASSIC_HAS_GRAPHICS

	BlWindow * pwin= mapwindow [ch];
	if (pwin == NULL)
		throw ErrBlassicInternal;
	for (std::string::size_type i= 0, l= str.size (); i < l; ++i)
		pwin->charout (str [i]);

	#else

	touch (ch, str);

	#endif
}

std::string graphics::copychr (BlChannel ch, BlChar from, BlChar to)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	BlWindow * pwin= mapwindow [ch];
	return pwin->copychr (from, to);

	#else

	touch (ch, from, to);
	ASSERT (false);
	throw ErrBlassicInternal;

	#endif
}

std::string graphics::screenchr (int x, int y)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	return copychrat (x * charwidth, y * charheight, 0, 255);

	#else

	touch (x, y);
	ASSERT (false);
	throw ErrBlassicInternal;

	#endif
}

#if 0
void graphics::gotoxy (int x, int y)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	trow= y;
	tcol= x;

	#else

	touch (x, y);

	#endif
}
#endif

void graphics::gotoxy (BlChannel ch, int x, int y)
{
	TRACEFUNC (tr, "graphics::gotoxy");
	TRMESSAGE (tr, "Channel: " + to_string (ch) + ", x= " +
		to_string (x) + ", y= " + to_string (y) );

	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->gotoxy (x, y);

	#else

	touch (ch, x, y);

	#endif
}

#if 0
void graphics::tab (size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	int col (n - 1);
	if (tcol >= col)
	{
		do {
			do_charout (' ');
		} while (tcol > 0);
	}
	if (col >= maxtcol)
		throw ErrFunctionCall;
	do {
		do_charout (' ');
	} while (tcol < col);

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (n);

	#endif
}
#endif

void graphics::tab (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->tab ();

	#else

	touch (ch);

	#endif
}

void graphics::tab (BlChannel ch, size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->tab (n);

	#else

	touch (ch, n);

	#endif
}

#if 0
void graphics::movecharforward (size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	windowzero.movecharforward (n);

	#else

	touch (n);

	#endif
}
#endif

void graphics::movecharforward (BlChannel ch, size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->movecharforward (n);

	#else

	touch (ch, n);

	#endif
}

#if 0
void graphics::movecharback (size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	windowzero.movecharback (n);

	#else

	touch (n);

	#endif
}
#endif

void graphics::movecharback (BlChannel ch, size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->movecharback (n);

	#else

	touch (ch, n);

	#endif
}

#if 0
void graphics::movecharup (size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	windowzero.movecharup (n);

	#else

	touch (n);

	#endif
}
#endif

void graphics::movecharup (BlChannel ch, size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->movecharup (n);

	#else

	touch (ch, n);

	#endif
}

#if 0
void graphics::movechardown (size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	windowzero.movechardown (n);

	#else

	touch (n);

	#endif
}
#endif

void graphics::movechardown (BlChannel ch, size_t n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->movechardown (n);

	#else

	touch (ch, n);

	#endif
}

void graphics::scroll (BlChannel ch, int n)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->scroll (n);

	#else

	touch (ch, n);

	#endif
}

void graphics::symbolafter (int symbol)
{
	if (symbol < 0 || symbol > 256)
		throw ErrFunctionCall;
	memcpy (charset::data, * charset::default_charset,
		sizeof (charset::chardataset) );
	symbol_after_is= symbol;
}

void graphics::definesymbol (int symbol, const unsigned char (& byte) [8] )
{
	if (symbol < 0 || symbol > 255)
		throw ErrFunctionCall;
	if (symbol < symbol_after_is)
		throw ErrImproperArgument;
	memcpy (charset::data + symbol, byte, sizeof (byte) );
}

void graphics::synchronize (bool mode)
{
	TRACEFUNC (tr, "graphics::synchronize");

	#ifdef BLASSIC_HAS_GRAPHICS

	bool previous= fSynchro;
	TRMESSAGE (tr, "Was " + to_string (previous) );

	fSynchro= mode;

	if (previous == true && mode == false && ingraphicsmode () )
		reinit_window ();

	TRMESSAGE (tr, "Set to " + to_string (mode) );

	#else

	touch (mode);

	#endif
}

void graphics::synchronize ()
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	reinit_window ();
	idle ();

	#endif
}

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

size_t synchrosaved= 0;
bool fSynchroSaved= false;

} // namespace

#endif
// BLASSIC_HAS_GRAPHICS

void graphics::synchronize_suspend ()
{
	TRACEFUNC (tr, "graphics::synchronize_suspend");

	#ifdef BLASSIC_HAS_GRAPHICS

	if (synchrosaved++ > 0)
	{
		if (showdebuginfo () )
			cerr << "synchronize_suspend called several times" <<
				endl;
	}
	else
		fSynchroSaved= fSynchro;

	//if (fSynchro)
	//	reinit_window ();
	//fSynchro= false;
	synchronize (false);

	#endif
}

void graphics::synchronize_restart ()
{
	TRACEFUNC (tr, "graphics::synchronize_restart");

	#ifdef BLASSIC_HAS_GRAPHICS

	if (synchrosaved == 0)
	{
		if (showdebuginfo () )
			cerr << "uexpected call to synchronize_restart" <<
				endl;
	}
	else
	{
		if (--synchrosaved == 0)
			//fSynchro= fSynchroSaved;
			synchronize (fSynchroSaved);
	}

	#endif
}

int graphics::xmouse ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return xmousepos;

	#else

	return 0;

	#endif
}

int graphics::ymouse ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return ymousepos;

	#else

	return 0;

	#endif
}

int graphics::xpos ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return lastx;

	#else

	return 0;

	#endif
}

int graphics::xpos (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return mapwindow [ch]->getxpos ();

	#else

	touch (ch);
	throw ErrFunctionCall;

	#endif
}

int graphics::ypos ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return lasty;

	#else

	return 0;

	#endif
}

int graphics::ypos (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return mapwindow [ch]->getypos ();

	#else

	touch (ch);
	throw ErrFunctionCall;

	#endif
}

void graphics::tag (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->tag ();

	#else

	touch (ch);

	#endif
}

void graphics::tagoff (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->tagoff ();

	#else

	touch (ch);

	#endif
}

bool graphics::istagactive (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return mapwindow [ch]->istagactive ();

	#else

	touch (ch);
	throw ErrFunctionCall;

	#endif
}

#if 0
void graphics::showcursor ()
{
	#ifdef BLASSIC_HAS_GRAPHICS
	windowzero.invertcursor ();
	#endif
}

void graphics::hidecursor ()
{
	#ifdef BLASSIC_HAS_GRAPHICS
	windowzero.invertcursor ();
	#endif
}
#endif

void graphics::showcursor (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->invertcursor ();

	#else

	touch (ch);

	#endif
}

void graphics::hidecursor (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->invertcursor ();

	#else

	touch (ch);

	#endif
}

void graphics::inverse (BlChannel ch, bool active)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->setinverse (active);

	#else

	touch (ch, active);

	#endif
}

bool graphics::getinverse (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return mapwindow [ch]->getinverse ();

	#else

	touch (ch);
	throw ErrFunctionCall;

	#endif
}

void graphics::bright (BlChannel ch, bool active)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	mapwindow [ch]->setbright (active);

	#else

	touch (ch, active);

	#endif
}

bool graphics::getbright (BlChannel ch)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	return mapwindow [ch]->getbright ();

	#else

	touch (ch);
	throw ErrFunctionCall;

	#endif
}

void graphics::clean_input ()
{
	TRACEFUNC (tr, "graphics::clean_input");

	#ifdef BLASSIC_HAS_GRAPHICS

	graphics::idle ();
	queuekey.erase ();

	#endif
}

void graphics::ring ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	#ifdef BLASSIC_USE_X

	XBell (display, 100);

	#elif defined BLASSIC_USE_WINDOWS

	MessageBeep (MB_ICONEXCLAMATION);

	#endif

	#endif
}

void graphics::set_title (const std::string & title)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	#ifdef BLASSIC_USE_WINDOWS

	SetWindowText (window, title.c_str () );

	#elif defined BLASSIC_USE_X

	XmbSetWMProperties (display, window, title.c_str (), title.c_str (),
		NULL, 0, NULL, NULL, NULL);

	#endif

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (title);

	#endif
}

void graphics::set_default_title (const std::string & title)
{
	#ifdef BLASSIC_HAS_GRAPHICS

	default_title= title;

	#else

	touch (title);

	#endif
}

//*********************************************************
//		Graphics get and put.
//*********************************************************

#ifdef BLASSIC_HAS_GRAPHICS

namespace {

class Image {
public:
	Image (int x1, int y1, int x2, int y2);
	~Image ();
	void put (int x, int y, int mode);
private:
	int width;
	int height;

	#ifdef BLASSIC_USE_X

	XImage * img;

	#elif defined BLASSIC_USE_WINDOWS

	HBITMAP img;
	HDC hdcImg;

	#endif
};

Image::Image (int x1, int y1, int x2, int y2)
{
	// Adjust coordinates to current rotate mode.
	do_rotate (x1, y1);
	do_rotate (x2, y2);
	if (x1 > x2) std::swap (x1, x2);
	if (y1 > y2) std::swap (y1, y2);
	if (x1 >= screenwidth || y1 >= screenheight)
		throw ErrFunctionCall;
	if (x2 >= screenwidth)
		x2= screenwidth - 1;
	if (y2 >= screenheight)
		y2= screenheight - 1;

	width= x2 - x1 + 1;
	height= y2 - y1 + 1;

	#ifdef BLASSIC_USE_X

	img= XGetImage (display, pixmap, x1, y1, width, height,
		AllPlanes, XYPixmap);
	if (img == NULL)
		throw ErrFunctionCall;

	#elif defined BLASSIC_USE_WINDOWS

	img= CreateCompatibleBitmap (hdcPixmap, width, height);
	if (img == NULL)
		throw ErrFunctionCall;
	hdcImg= CreateCompatibleDC (hdcPixmap);
	if (hdcImg == NULL)
	{
		DeleteObject (img);
		throw ErrFunctionCall;
	}
	HGDIOBJ hgdiobj= SelectObject (hdcImg, img);
	if (hgdiobj == NULL ||
		hgdiobj == reinterpret_cast <HGDIOBJ> (GDI_ERROR) )
	{
		DeleteDC (hdcImg);
		DeleteObject (img);
		throw ErrFunctionCall;
	}

	if (BitBlt (hdcImg, 0, 0, width, height, hdcPixmap, x1, y1, SRCCOPY)
		== 0)
	{
		DeleteDC (hdcImg);
		DeleteObject (img);
		throw ErrFunctionCall;
	}

	#endif
}

Image::~Image ()
{
	#ifdef BLASSIC_USE_X

	XDestroyImage (img);

	#elif defined BLASSIC_USE_WINDOWS

	DeleteDC (hdcImg);
	DeleteObject (img);

	#endif
}

void Image::put (int x, int y, int mode)
{
	TRACEFUNC (tr, "Image::put");

	TRMESSAGE (tr, "at " + to_string (x) + ',' + to_string (y) );

	// Adjust coordinates to current rotate mode.
	do_rotate (x, y);
	switch (rotate)
	{
	case RotateNone:
		// Nothing to do.
		break;
	case Rotate90:
		y-= width - 1;
		break;
	}
	if (x >= screenwidth || y >= screenheight)
		throw ErrFunctionCall;
	int maxwidth= screenwidth - x + 1;
	int maxheight= screenheight - y + 1;
	int w= std::min (width, maxwidth);
	int h= std::min (height, maxheight);

	TRMESSAGE (tr, "At " + to_string (x) + ',' + to_string (y) );

	#ifdef BLASSIC_USE_X

	int modeused= getdrawmode (mode);

	XSetFunction (display, gcp, modeused);
	XPutImage (display, pixmap, gcp, img,
		0, 0, x, y, w, h);
	XSetFunction (display, gcp, drawmode);

	#elif defined BLASSIC_USE_WINDOWS

	int modeused= getbitbltmode (mode);

	if (BitBlt (hdcPixmap, x, y, w, h, hdcImg, 0, 0, modeused) == 0)
		throw ErrFunctionCall;

	#endif

	if (! fSynchro)
		reinit_window (x, y, width, height);
}

typedef std::map <std::string, Image *> imagemap_t;
imagemap_t imagemap;

} // namespace

#endif
// BLASSIC_HAS_GRAPHICS

void graphics::get_image (int x1, int y1, int x2, int y2,
	const std::string & name)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	imagemap_t::iterator it= imagemap.find (name);
	if (it != imagemap.end () )
	{
		delete it->second;
		imagemap.erase (it);
	}

	auto_ptr <Image> pimg (new Image (x1, y1, x2, y2) );
	imagemap [name]= pimg.get ();
	pimg.release ();

	#else
	// No BLASSIC_HAS_GRAPHICS

	touch (x1, y1, x2, y2, name);

	#endif
}

void graphics::put_image (int x, int y, const std::string & name, int mode)
{
	requiregraphics ();

	#ifdef BLASSIC_HAS_GRAPHICS

	imagemap_t::iterator it= imagemap.find (name);
	if (it != imagemap.end () )
	{
		it->second->put (x, y, mode);
	}
	else
		throw ErrFunctionCall;

	#else

	touch (x, y, name, mode);

	#endif
	// BLASSIC_HAS_GRAPHICS
}

void graphics::clear_images ()
{
	#ifdef BLASSIC_HAS_GRAPHICS

	for (imagemap_t::iterator it= imagemap.begin ();
		it != imagemap.end ();
		++it)
	{
		delete it->second;
	}
	imagemap.clear ();

	#endif
	// BLASSIC_HAS_GRAPHICS
}

// Fin de graphics.cpp
