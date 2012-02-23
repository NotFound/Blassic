// cursor.cpp
// Revision 24-apr-2009

#include "cursor.h"

#include "blassic.h"

#include "key.h"
#include "util.h"
#include "error.h"
#include "showerror.h"

#include "trace.h"

#include <iostream>
using std::cerr;
using std::endl;

#include <map>
#include <queue>
#include <sstream>

#include <string.h>

#ifdef BLASSIC_USE_WINDOWS


#include <windows.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined HAVE_IO_H || ! defined BLASSIC_CONFIG
#include <io.h>
#endif


#else


#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <errno.h>
#include <cassert>
#define ASSERT assert

#include <termios.h>

// We check for terminfo only if not in windows,
// use windows console functions even in Cygwin.

// Now curses can be disabled from configure.
#ifndef BLASSIC_CONFIG_NO_CURSES

#ifdef BLASSIC_CONFIG_USE_NCURSES

#include <ncurses.h>

#elif defined BLASSIC_CONFIG_USE_CURSES

#include <curses.h>

#else

#error Bad configuration of curses options.

#endif

#ifdef HAVE_TERM_H
// I suppose that if this is not availabe, then is not required.
#include <term.h>
#endif

#define BLASSIC_USE_TERMINFO

// curses can define erase as macro, invalidating string::erase.
#ifdef erase
#undef erase
#endif
// curses can define bool as macro.
#ifdef bool
#undef bool
#endif

#endif


// Stuff needed by getwidth

#include <sys/ioctl.h>
#include <termios.h>

// This is from ncurses.
#ifdef TIOCGSIZE
# define IOCTL_WINSIZE TIOCGSIZE
# define STRUCT_WINSIZE struct ttysize
# define WINSIZE_ROWS(n) (int)n.ts_lines
# define WINSIZE_COLS(n) (int)n.ts_cols
#else
# ifdef TIOCGWINSZ
#  define IOCTL_WINSIZE TIOCGWINSZ
#  define STRUCT_WINSIZE struct winsize
#  define WINSIZE_ROWS(n) (int)n.ws_row
#  define WINSIZE_COLS(n) (int)n.ws_col
# endif
#endif

#endif

#include <iostream>
#include <map>

using util::touch;


namespace {

using namespace cursor;


#ifndef BLASSIC_USE_WINDOWS

#ifdef BLASSIC_USE_TERMINFO

bool fInit= true;

int background= 8;

const char
	* strKeypadXmit= NULL,    * strKeypadLocal= NULL,
	* strCls= NULL,           * strCup= NULL,
	* strCursorNormal= NULL,  * strCursorInvisible= NULL,
	* strForeground= NULL,    * strBackground= NULL,
	* strEnterBold= NULL,     * strExitBold= NULL,
	* strMoveForward= NULL,   * strMoveBack= NULL,
	* strMoveForwardN= NULL,  * strMoveBackN= NULL,
	* strMoveUp= NULL,        * strMoveDown= NULL,
	* strMoveUpN= NULL,       * strMoveDownN= NULL,
	* strSaveCursorPos= NULL, * strRestoreCursorPos= NULL,
	* strBell= NULL;

const char * newstr (const char * str)
{
	if (str == NULL)
		return NULL;
	size_t l= strlen (str);
	char *n= new char [l + 1];
	strcpy (n, str);
	return n;
}

inline const char * calltigetstr (const char * id)
{
	#ifdef BLASSIC_CONFIG_USE_NCURSES
	const char * str= tigetstr ( (char *) id);
	#else
	char buffer [128];
	char * area= buffer;
	const char * str= tgetstr (const_cast <char *> (id), & area);
	#endif
	if (str == (char *) -1)
		return NULL;
	return str;
}

inline const char * mytigetstr (const char * id)
{
	return newstr (calltigetstr (id) );
}

int putfunc (int ic)
{
	char c= ic;
	write (STDOUT_FILENO, & c, 1);
	return c;
}

inline void calltputs (const char * str)
{
	if (str != NULL)
		tputs (str, 1, putfunc);
}

inline void calltparm (const char * str, int n)
{
	if (str != NULL)
	{
		#ifdef BLASSIC_CONFIG_USE_NCURSES
		calltputs (tparm ( (char *) str, n) );
		#else
		calltputs (tgoto ( (char *) str, n, 0) );
		#endif
	}
}

void initkeytable ();

struct str_terminfo {
	const char * & str;
	const char * tinfoname;
	str_terminfo (const char * & str, const char * tinfoname) :
		str (str), tinfoname (tinfoname)
	{ }
};

#ifdef BLASSIC_CONFIG_USE_NCURSES

const str_terminfo strinfo []= {
	str_terminfo (strKeypadXmit, "smkx"),
	str_terminfo (strKeypadLocal, "rmkx"),

	str_terminfo (strCls, "clear" ),
	str_terminfo (strCup, "cup" ),

	str_terminfo (strCursorNormal, "cnorm" ),
	str_terminfo (strCursorInvisible, "civis" ),

	str_terminfo (strForeground, "setaf" ),
	str_terminfo (strBackground, "setab" ),

	str_terminfo (strEnterBold, "bold" ),
	str_terminfo (strExitBold, "sgr0" ),

	str_terminfo (strMoveForward, "cuf1" ),
	str_terminfo (strMoveBack, "cub1" ),
	str_terminfo (strMoveForwardN, "cuf" ),
	str_terminfo (strMoveBackN, "cub" ),
	str_terminfo (strMoveUp, "cuu1" ),
	str_terminfo (strMoveDown, "cud1" ),
	str_terminfo (strMoveUpN, "cuu" ),
	str_terminfo (strMoveDownN, "cud" ),

	str_terminfo (strSaveCursorPos, "sc" ),
	str_terminfo (strRestoreCursorPos, "rc" ),

	str_terminfo (strBell, "bel"),
};

#else

const str_terminfo strinfo []= {
	str_terminfo (strKeypadXmit, "ks"),
	str_terminfo (strKeypadLocal, "ke"),

	str_terminfo (strCls, "cl" ),
	str_terminfo (strCup, "cm" ),

	str_terminfo (strCursorNormal, "ve" ),
	str_terminfo (strCursorInvisible, "vi" ),

	str_terminfo (strForeground, "AF" ),
	str_terminfo (strBackground, "AB" ),

	str_terminfo (strEnterBold, "md" ),
	str_terminfo (strExitBold, "me" ),

	str_terminfo (strMoveForward, "nd" ),
	str_terminfo (strMoveBack, "le" ),
	str_terminfo (strMoveForwardN, "RI" ),
	str_terminfo (strMoveBackN, "LE" ),
	str_terminfo (strMoveUp, "up" ),
	str_terminfo (strMoveDown, "do" ),
	str_terminfo (strMoveUpN, "UP" ),
	str_terminfo (strMoveDownN, "DO" ),

	str_terminfo (strSaveCursorPos, "sc" ),
	str_terminfo (strRestoreCursorPos, "rc" ),

	str_terminfo (strBell, "bl"),
};

#endif

#ifndef BLASSIC_CONFIG_USE_NCURSES

char tgetent_buffer [1024];

#endif

void init ()
{
	TRACEFUNC (tr, "init");

	fInit= false;
	#ifdef BLASSIC_CONFIG_USE_NCURSES
	{
		int errret;
		setupterm (0, 1, & errret);
	}
	#else
	{
		char * strterm= getenv ("TERM");
		if (strterm != NULL && strterm [0] != '\0')
			tgetent (tgetent_buffer, strterm);
	}
	#endif

	for (size_t i= 0; i < util::dim_array (strinfo); ++i)
		strinfo [i].str= mytigetstr (strinfo [i].tinfoname);

	initkeytable ();

	if (isatty (STDOUT_FILENO) )
	{
		#if 0
		const char * str_keypad_xmit= calltigetstr ("smkx");
		calltputs (str_keypad_xmit);
		#else
		calltputs (strKeypadXmit);
		#endif
	}

}

inline void checkinit ()
{
	if (fInit)
		init ();
}

#else

inline void checkinit () { }

#endif

#endif

} // namespace

void cursor::initconsole ()
{
	TRACEFUNC (tr, "initconsole");

	cursorinvisible ();
}

void cursor::quitconsole ()
{
	TRACEFUNC (tr, "quitconsole");

	cursorvisible ();

	#ifdef BLASSIC_USE_TERMINFO

	if (! fInit)
	{
		if (isatty (STDOUT_FILENO) )
		{
			#if 0
			const char * str_keypad_local= calltigetstr ("rmkx");
			calltputs (str_keypad_local);
			#else
			calltputs (strKeypadLocal);
			#endif
		}

	}

	#endif
}

size_t cursor::getwidth ()
{
	const size_t default_value= 80;
	size_t width;

	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo (h, & info) )
		width= info.dwSize.X;
	else
		width= default_value;

	#elif defined BLASSIC_USE_TERMINFO

	STRUCT_WINSIZE win;
	if (ioctl (0, IOCTL_WINSIZE, & win) == 0)
		width= WINSIZE_COLS (win);
	else
	{
		const char * aux= getenv ("COLUMNS");
		if (aux)
			width= atoi (aux);
		else
			width= default_value;
	}

	#else

	width= default_value;

	#endif

	return width;
}

void cursor::cursorvisible ()
{
	TRACEFUNC (tr, "cursorvisible");

	// checkinit not needed, is done by showcursor
	showcursor ();

	#ifndef BLASSIC_USE_WINDOWS

	struct termios ter;
	tcgetattr (STDIN_FILENO, & ter);
	//ter.c_lflag|= (ECHO | ICANON | PENDIN);
	ter.c_lflag|= (ECHO | ICANON);
	tcsetattr (STDIN_FILENO, TCSANOW, & ter);

	#endif
}

void cursor::cursorinvisible ()
{
	TRACEFUNC (tr, "cursorinvisible");

	// checkinit not needed, is done by hidecursor
	hidecursor ();

	#ifndef BLASSIC_USE_WINDOWS

	struct termios ter;
	tcgetattr (STDIN_FILENO, & ter);
	ter.c_lflag&= ~ (ECHO | ICANON);
	ter.c_cc [VMIN]= 1;
	tcsetattr (STDIN_FILENO, TCSANOW, & ter);

	#endif
}

void cursor::showcursor ()
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	GetConsoleCursorInfo (h, & info);
	info.bVisible= TRUE;
	SetConsoleCursorInfo (h, & info);

	#elif defined BLASSIC_USE_TERMINFO

	checkinit ();

	if (isatty (STDOUT_FILENO) )
		calltputs (strCursorNormal );

	#endif
}

void cursor::hidecursor ()
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	GetConsoleCursorInfo (h, & info);
	info.bVisible= FALSE;
	SetConsoleCursorInfo (h, & info);

	#elif defined BLASSIC_USE_TERMINFO

	checkinit ();

	if (isatty (STDOUT_FILENO) )
		calltputs (strCursorInvisible);

	#endif
}

#ifdef BLASSIC_USE_WINDOWS

const WORD init_attributes=
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD attributes= init_attributes;

#endif

void cursor::cls ()
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	if (h != INVALID_HANDLE_VALUE)
	{
		CONSOLE_SCREEN_BUFFER_INFO info;
		if (GetConsoleScreenBufferInfo (h, & info) )
		{
			DWORD l= info.dwSize.X * info.dwSize.Y;
			COORD coord= {0, 0};
			DWORD notused;
			FillConsoleOutputAttribute (h,
				attributes,
				l, coord, & notused);
			FillConsoleOutputCharacter (h,
				' ', l, coord, & notused);
			SetConsoleCursorPosition (h, coord);
		}
	}

	#elif defined BLASSIC_USE_TERMINFO

	checkinit ();

	calltputs (strCls);

	#endif
}

void cursor::gotoxy (int x, int y)
{
	#ifdef BLASSIC_USE_WINDOWS

	COORD coord= { SHORT (x), SHORT (y) };
	SetConsoleCursorPosition (GetStdHandle (STD_OUTPUT_HANDLE), coord);

	#elif defined BLASSIC_USE_TERMINFO

	checkinit ();

	if (strCup)
		calltputs (tgoto (const_cast <char *> (strCup), x, y) );

	#else

	touch (x, y);

	#endif
}

int cursor::getcursorx ()
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo (h, & info) == 0)
		return 0;
	return info.dwCursorPosition.X;

	#else

	return 0;

	#endif
}

void cursor::movecharforward ()
{
	#ifdef BLASSIC_USE_WINDOWS

	movecharforward (1);

	#elif defined BLASSIC_USE_TERMINFO

	calltputs (strMoveForward);

	#endif
}

void cursor::movecharback ()
{
	#ifdef BLASSIC_USE_WINDOWS

	movecharback (1);

	#elif defined BLASSIC_USE_TERMINFO

	calltputs (strMoveBack);

	#endif
}

void cursor::movecharup ()
{
	#ifdef BLASSIC_USE_WINDOWS

	movecharup (1);

	#elif defined BLASSIC_USE_TERMINFO

	calltputs (strMoveUp);

	#endif
}

void cursor::movechardown ()
{
	#ifdef BLASSIC_USE_WINDOWS

	movechardown (1);

	#elif defined BLASSIC_USE_TERMINFO

	calltputs (strMoveDown);

	#endif
}

namespace {

#ifdef BLASSIC_USE_TERMINFO

inline void auxmovechar (const char * strN, const char * str, size_t n)
{
	if (n != 0)
	{
		if (strN)
			//calltputs (tparm ( (char *) strN, n) );
			calltparm (strN, n);
		else
			if (str)
				for (size_t i= 0; i < n; ++i)
					calltputs (str);
	}
}

#endif

} // namespace

void cursor::movecharforward (size_t n)
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo (h, & info) )
	{
		info.dwCursorPosition.X+= SHORT (n);
		SetConsoleCursorPosition (h, info.dwCursorPosition);
	}

	#elif defined BLASSIC_USE_TERMINFO

	auxmovechar (strMoveForwardN, strMoveForward, n);

	#else

	touch (n);

	#endif
}

void cursor::movecharback (size_t n)
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo (h, & info) )
	{
		info.dwCursorPosition.X-= SHORT (n);
		SetConsoleCursorPosition (h, info.dwCursorPosition);
	}

	#elif defined BLASSIC_USE_TERMINFO

	auxmovechar (strMoveBackN, strMoveBack, n);

	#else

	touch (n);

	#endif
}

void cursor::movecharup (size_t n)
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo (h, & info) )
	{
		info.dwCursorPosition.Y-= SHORT (n);
		SetConsoleCursorPosition (h, info.dwCursorPosition);
	}

	#elif defined BLASSIC_USE_TERMINFO

	auxmovechar (strMoveUpN, strMoveUp, n);

	#else

	touch (n);

	#endif
}

void cursor::movechardown (size_t n)
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo (h, & info) )
	{
		info.dwCursorPosition.Y+= SHORT (n);
		SetConsoleCursorPosition (h, info.dwCursorPosition);
	}

	#elif defined BLASSIC_USE_TERMINFO

	auxmovechar (strMoveDownN, strMoveDown, n);

	#else

	touch (n);

	#endif
}

void cursor::savecursorpos ()
{
	#ifdef BLASSIC_USE_WINDOWS

	#elif defined BLASSIC_USE_TERMINFO

	calltputs (strSaveCursorPos);

	#endif
}

void cursor::restorecursorpos ()
{
	#ifdef BLASSIC_USE_WINDOWS

	#elif defined BLASSIC_USE_TERMINFO

	calltputs (strRestoreCursorPos);

	#endif
}


#ifndef BLASSIC_USE_WINDOWS

namespace {

static const int newcolor []=
	{ 0, 4, 2, 6, 1, 5, 3, 7};

inline int mapcolor (int n)
{
	// Intensity bit unchanged
	return newcolor [n & 7] | (n & 8);
}

} // namespace

#endif

void cursor::textcolor (int color)
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	attributes= (attributes & WORD (0xF0) ) | WORD (color & 0x0F);
	SetConsoleTextAttribute (h, attributes);

	#elif defined BLASSIC_USE_TERMINFO

	color= mapcolor (color & 0xF);
	bool intensity= color > 7;
	if (intensity)
	{
		color&= 7;
		calltputs (strEnterBold);
	}
	else
	{
		if (strExitBold)
		{
			calltputs (strExitBold);
			// sgr0 reset the background, then we need to set it.
			textbackground (background);
		}
	}
	//if (strForeground)
	//	calltputs (tparm ( (char *) strForeground, color) );
	calltparm (strForeground, color);

	#else

	touch (color);

	#endif
}

void cursor::textbackground (int color)
{
	#ifdef BLASSIC_USE_WINDOWS

	HANDLE h= GetStdHandle (STD_OUTPUT_HANDLE);
	attributes= (attributes & WORD (0xF) ) | WORD ( (color & 0xF) << 4);
	SetConsoleTextAttribute (h, attributes);

	#elif defined BLASSIC_USE_TERMINFO

	background= color;
	color= mapcolor (color & 0xF);
	//if (strBackground)
	//	calltputs (tparm ( (char *) strBackground, color) );
	calltparm (strBackground, color);

	#else

	touch (color);

	#endif
}

namespace {

enum ReadType { ReadWait, ReadNoWait };

#ifdef BLASSIC_USE_WINDOWS

std::string string_from_key_event (const KEY_EVENT_RECORD & kr)
{

	char c= kr.uChar.AsciiChar;
	if (c != '\0')
		return std::string (1, c);
	WORD k= kr.wVirtualKeyCode;
	std::string str= string_from_key (k);
	if (! str.empty () )
		return str;

	if (k != VK_SHIFT &&
		k != VK_CONTROL &&
		k != VK_MENU &&
		k != VK_CAPITAL &&
		k != VK_NUMLOCK &&
		k != VK_SCROLL)
	{
		std::string str (1, '\0');
		str+= char (kr.wVirtualScanCode);
		return str;
	}
	return std::string ();
}

std::string string_from_input (const INPUT_RECORD & input)
{
	std::string str;
	if (input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown)
	{
		str= string_from_key_event (input.Event.KeyEvent);
	}
	return str;
}

std::string readkey (ReadType type)
{
	std::string str;

	HANDLE h= GetStdHandle (STD_INPUT_HANDLE);
	if (h == INVALID_HANDLE_VALUE)
	{
		showlasterror ();
		throw ErrFileRead;
	}

	DWORD mode, orgmode;
	GetConsoleMode (h, & mode);
	orgmode= mode;
	//mode&= ~ (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
	mode= 0;
	SetConsoleMode (h, mode);
	DWORD n= 0;
	INPUT_RECORD input;
	if (type == ReadNoWait)
	{
		if (PeekConsoleInput (h, & input, 1, & n) == 0)
		{
			showlasterror ();
			throw ErrFileRead;
		}
	}
	else
		n= 1;
	if (n)
	{
		do
		{
			if (ReadConsoleInput (h, & input, 1, & n) == 0)
			{
				showlasterror ();
				throw ErrFileRead;
			}
			str= string_from_input (input);
		} while (type == ReadWait && str.empty () );
	}
	SetConsoleMode (h, orgmode);

	return str;
}

#else

#if defined BLASSIC_USE_TERMINFO

class MapSpecial {
public:
	enum Result { NoMapped, Found, MoreNeeded };
	void addkey (const std::string & str, std::string::size_type pos,
		const std::string & keyname)
	{
		//TRACEFUNC (tr, "MapSpecial::addkey");

		ASSERT (pos < str.size () );
		char c= str [pos];
		if (pos == str.size () - 1)
			kname [c]= keyname;
		else
		{
			//if (kmap.find (c) == kmap.end () )
			//	kmap [c]= MapSpecial ();
			kmap [c].addkey (str, pos + 1, keyname);
		}
	}
	Result findkey (const std::string & str, std::string::size_type pos,
		std::string & keyname, std::string::size_type & consumed)
	{
		if (pos >= str.size () )
			return MoreNeeded;
		char c= str [pos];
		//cout << "Buscando: " << c << endl;
		{
			kname_t::iterator it= kname.find (c);
			if (it != kname.end () )
			{
				keyname= it->second;
				consumed= pos;
				return Found;
			}
		}
		std::map <char, MapSpecial>::iterator it= kmap.find (c);
		if (it != kmap.end () )
			return it->second.findkey
				(str, pos + 1, keyname, consumed);
		else
			return NoMapped;
	}
private:
	typedef std::map <char, std::string> kname_t;
	kname_t kname;
	std::map <char, MapSpecial> kmap;
};

struct KeyDescription {
	const char * tiId;
	//const char * blName;
	const std::string & blName;
	KeyDescription (const char * tiId, const std::string & blName) :
		tiId (tiId),
		blName (blName)
	{ }
};

const std::string
	strMULT ("*"),
	strMINUS ("-"),
	strPLUS ("+"),
	strDIV ("/");

#ifdef BLASSIC_CONFIG_USE_NCURSES

const KeyDescription keyname [] = {
	KeyDescription ("kpp",   strPAGEUP),    // previous-page key
	KeyDescription ("knp",   strPAGEDOWN),  // next-page key
	KeyDescription ("kend",  strEND),       // end key
	KeyDescription ("kslt",  strEND),       // select key
	KeyDescription ("kc1",   strEND),       // lower left of keypad
	KeyDescription ("khome", strHOME),      // home key
	KeyDescription ("kfnd",  strHOME),      // find key
	KeyDescription ("ka1",   strHOME),      // upper left of keypad
	KeyDescription ("kcub1", strLEFT),      // left-arrow key
	KeyDescription ("kcuu1", strUP),        // up-arrow key
	KeyDescription ("kcuf1", strRIGHT),     // right-arrow key
	KeyDescription ("kcud1", strDOWN),      // down-arrow key
	KeyDescription ("kich1", strINSERT),    // insert-character key
	KeyDescription ("kdch1", strDELETE),    // delete-character key
	KeyDescription ("kent",  strENTER),     // enter/send key
	KeyDescription ("kf1",   strF1),        // F1 function key
	KeyDescription ("kf2",   strF2),        // F2 function key
	KeyDescription ("kf3",   strF3),        // F3 function key
	KeyDescription ("kf4",   strF4),        // F4 function key
	KeyDescription ("kf5",   strF5),        // F5 function key
	KeyDescription ("kf6",   strF6),        // F6 function key
	KeyDescription ("kf7",   strF7),        // F7 function key
	KeyDescription ("kf8",   strF8),        // F8 function key
	KeyDescription ("kf9",   strF9),        // F9 function key
	KeyDescription ("kf10",  strF10),       // F10 function key
	KeyDescription ("kf11",  strF11),       // F11 function key
	KeyDescription ("kf12",  strF12),       // F12 function key
	KeyDescription ("kf54",  strDIV),       // F54 function key, / in xterm
	KeyDescription ("kf55",  strMULT),      // F55 function key, * in xterm
	KeyDescription ("kf56",  strMINUS),     // F56 function key, - in xterm
	KeyDescription ("kf57",  strPLUS),      // f57 function key, + in xterm
};

#else

const KeyDescription keyname [] = {
	KeyDescription ("kP",    strPAGEUP),    // previous-page key
	KeyDescription ("kN",    strPAGEDOWN),  // next-page key
	KeyDescription ("@7",    strEND),       // end key
	KeyDescription ("*6",    strEND),       // select key
	KeyDescription ("K4",    strEND),       // lower left of keypad
	KeyDescription ("kh",    strHOME),      // home key
	KeyDescription ("@0",    strHOME),      // find key
	KeyDescription ("K1",    strHOME),      // upper left of keypad
	KeyDescription ("kl",    strLEFT),      // left-arrow key
	KeyDescription ("ku",    strUP),        // up-arrow key
	KeyDescription ("kr",    strRIGHT),     // right-arrow key
	KeyDescription ("kd",    strDOWN),      // down-arrow key
	KeyDescription ("kI",    strINSERT),    // insert-character key
	KeyDescription ("kD",    strDELETE),    // delete-character key
	KeyDescription ("@8",    strENTER),     // enter/send key
	KeyDescription ("k1",    strF1),        // F1 function key
	KeyDescription ("k2",    strF2),        // F2 function key
	KeyDescription ("k3",    strF3),        // F3 function key
	KeyDescription ("k4",    strF4),        // F4 function key
	KeyDescription ("k5",    strF5),        // F5 function key
	KeyDescription ("k6",    strF6),        // F6 function key
	KeyDescription ("k7",    strF7),        // F7 function key
	KeyDescription ("k8",    strF8),        // F8 function key
	KeyDescription ("k9",    strF9),        // F9 function key
	KeyDescription ("k;",    strF10),       // F10 function key
	KeyDescription ("F1",    strF11),       // F11 function key
	KeyDescription ("F2",    strF12),       // F12 function key
	KeyDescription ("Fi",    strDIV),       // F54 function key, / in xterm
	KeyDescription ("Fj",    strMULT),      // F55 function key, * in xterm
	KeyDescription ("Fk",    strMINUS),     // F56 function key, - in xterm
	KeyDescription ("Fl",    strPLUS),      // f57 function key, + in xterm
};

#endif

#ifndef NDEBUG

bool checktable ()
{
	const size_t nkeys= util::dim_array (keyname);

	for (size_t i= 0; i < nkeys - 1; ++i)
		for (size_t j= i + 1; j < nkeys; ++j)
			if (strcmp (keyname [i].tiId, keyname [j].tiId) == 0)
			{
				std::cerr << "Code repeated in keyname: " <<
					keyname [i].tiId << std::endl;
				throw 1;
			}
	return true;
}

bool tablechecked= checktable ();

#endif

MapSpecial ms;

void initkeytable ()
{
	TRACEFUNC (tr, "initkeytable");

	const size_t nkeys= util::dim_array (keyname);

	for (size_t i= 0; i < nkeys; ++i)
	{
		const KeyDescription & keydesc= keyname [i];
		const char * const strkey= keydesc.tiId;
		const char * str= calltigetstr (strkey);
		if (str != NULL)
		{
			#if 0
			cerr << keydesc.blName << "=";
			for (size_t i= 0, l= strlen (str); i < l; ++i)
			{
				char c= str [i];
				if (c >= 32) cerr << c;
				else cerr << "\\(" << hex << int (c) << ')';
			}
			cerr << endl;
			#endif
			TRMESSAGE (tr, std::string ("Adding ") + keydesc.blName);
			ms.addkey (str, 0, keydesc.blName);
		}
	}

}

#endif // BLASSIC_USE_TERMINFO

class PollInput {
public:
	PollInput ()
	{
		pfd.fd= STDIN_FILENO;
		pfd.events= POLLIN;
	}
	int poll ()
	{
		int r= ::poll (& pfd, 1, 100);
		if (r == 1 && pfd.revents != POLLIN)
			throw ErrFileRead;
		return r;
	}
private:
	struct pollfd pfd;
};

void wait_event ()
{
	PollInput pi;
	int r;
	do {
		blassic::idle ();
	} while ( (r= pi.poll () ) == 0);
	if (r < 0)
	{
		std::cerr << "Error in poll: " << strerror (errno) <<
			std::endl;
	}
}

void do_poll ()
{
	PollInput ().poll ();
}

std::string readkey (ReadType type)
{
	checkinit ();

	static std::string charpending;
	std::string str;
	bool reset_blocking_mode= false;
	int l;
	//char c;
	const int lbuf= 32;
	char buffer [lbuf + 1];

	if (! charpending.empty () )
		goto check_it;

	#if 0

	if (type == ReadWait)
	{
		//fcntl (STDIN_FILENO, F_SETFL, 0);
		wait_event ();
	}
	else
	{
		fcntl (STDIN_FILENO, F_SETFL, O_NONBLOCK);
		reset_blocking_mode= true;
	}

	//read_another:

	l= read (STDIN_FILENO, & c, 1);
	if (l == 1)
		str+= c;

	#else

	fcntl (STDIN_FILENO, F_SETFL, O_NONBLOCK);
	reset_blocking_mode= true;
	//l= read (STDIN_FILENO, & c, 1);
	l= read (STDIN_FILENO, buffer, lbuf);
	//if (l != 1 && type == ReadWait)
	if (l < 1 && type == ReadWait)
	{
		do {
			//wait_event ();
			//l= read (STDIN_FILENO, & c, 1);
			do_poll ();
			l= read (STDIN_FILENO, buffer, lbuf);
		//} while (l != 1);
		} while (l < 1);
	}
	//if (l == 1)
	//	str+= c;
	if (l >= 1)
	{
		buffer [l]= '\0';
		str+= buffer;
	}

	#endif

	#ifdef BLASSIC_USE_TERMINFO
	read_another:
	#endif

	//std::cerr << "Adding: >" << str << '<' << std::endl;
	charpending+= str;
	str.erase ();

	check_it:

	std::string keyname;
	//std::string::size_type pos;
	if (! charpending.empty () )
	{
		#ifdef BLASSIC_USE_TERMINFO
		std::string::size_type pos;
		MapSpecial::Result r=
			ms.findkey (charpending, 0, keyname, pos);
		switch (r)
		{
		case MapSpecial::NoMapped:
			str= charpending [0];
			charpending.erase (0, 1);
			break;
		case MapSpecial::Found:
			str= keyname;
			charpending.erase (0, pos + 1);
			break;
		case MapSpecial::MoreNeeded:
			fcntl (STDIN_FILENO, F_SETFL, O_NONBLOCK);
			reset_blocking_mode= true;
			do_poll ();
			//l= read (STDIN_FILENO, & c, 1);
			//if (l == 1)
			//{
			//	str= c;
			//	goto read_another;
			//}
			l= read (STDIN_FILENO, buffer, lbuf);
			if (l >= 1)
			{
				buffer [l]= '\0';
				str= buffer;
				goto read_another;
			}
			str= charpending [0];
			charpending.erase (0, 1);
			break;
		}
		#else
		str= charpending [0];
		charpending.erase (0, 1);
		#endif
	}

	//if (type == ReadWait)
	//	cursorinvisible ();

	if (reset_blocking_mode)
		fcntl (STDIN_FILENO, F_SETFL, 0);

	return str;
}

#endif // ! BLASSIC_USE_WINDOWS

// A provisional solution to pollin

std::queue <std::string> keypending;

} // namespace

std::string cursor::inkey ()
{
	if (! keypending.empty () )
	{
		std::string r= keypending.front ();
		keypending.pop ();
		return r;
	}
	else
		return readkey (ReadNoWait);
}

std::string cursor::getkey ()
{
	if (! keypending.empty () )
	{
		std::string r= keypending.front ();
		keypending.pop ();
		return r;
	}
	return readkey (ReadWait);
}

bool cursor::pollin ()
{
	if (! keypending.empty () )
		return true;
	else
	{
		std::string key= readkey (ReadNoWait);
		if (key.empty () )
			return false;
		else
		{
			keypending.push (key);
			return true;
		}
	}
}

void cursor::clean_input ()
{
	TRACEFUNC (tr, "clean_input");

	#ifdef BLASSIC_USE_WINDOWS

	Sleep (100);
	HANDLE h= GetStdHandle (STD_INPUT_HANDLE);
	INPUT_RECORD input;
	DWORD n= 0;
	PeekConsoleInput (h, & input, 1, & n);
	if (n && input.EventType == KEY_EVENT &&
			! input.Event.KeyEvent.bKeyDown)
		ReadConsoleInput (h, & input, 1, & n);

	#else

	fcntl (STDIN_FILENO, F_SETFL, O_NONBLOCK);
	int l;
	const int lbuf= 32;
	char buffer [lbuf + 1];
	do
	{
		l= read (STDIN_FILENO, buffer, lbuf);
	} while (l > 0);
	fcntl (STDIN_FILENO, F_SETFL, 0);

	#endif
}

void cursor::ring ()
{
	#ifdef BLASSIC_USE_WINDOWS

	MessageBeep (MB_ICONEXCLAMATION);

	#elif defined BLASSIC_USE_TERMINFO

	calltputs (strBell);

	#else

	// Last resource
	char c= '\a';
	write (STDOUT_FILENO, & c, 1);

	#endif
}

//************************************************
//	set_title
//************************************************

#ifdef BLASSIC_USE_TERMINFO

// Escape sequences from the "How to change the title of an xterm",
// by Ric Lister, http://www.tldp.org/HOWTO/mini/Xterm-Title.html

namespace
{

void write_it (std::ostringstream & oss)
{
	const std::string & str (oss.str () );
	write (STDOUT_FILENO, str.c_str (), str.size () );
}

void set_title_xterm (const std::string & title)
{
	std::ostringstream oss;
	oss << "\x1B]0;" << title << "\x07";
	write_it (oss);
}

void set_title_iris_ansi (const std::string & title)
{
	std::ostringstream oss;
	oss << "\x1BP1.y" << title << "\x1B\\"; // Set window title
	oss << "\x1BP3.y" << title << "\x1B\\"; // Set icon title
	write_it (oss);
}

void set_title_sun_cmd (const std::string & title)
{
	std::ostringstream oss;
	oss << "\x1B]l;" << title << "\x1B\\"; // Set window title
	oss << "\x1B]L;" << title << "\x1B\\"; // Set icon title
	write_it (oss);
}

void set_title_hpterm (const std::string & title)
{
	std::ostringstream oss;
	const std::string::size_type l= title.size ();
	oss << "\x1B&f0k" << l << 'D' << title; // Set window title
	oss << "\x1B&f-1k" << l << 'D' << title; // Set icon title
	write_it (oss);
}

typedef void (* set_title_t) (const std::string &);

struct Cstring_less {
	bool operator () (const char * p, const char * q)
	{ return strcmp (p, q) < 0; }
};

typedef std::map <const char *, set_title_t, Cstring_less> maptitle_t;
maptitle_t maptitle;

bool initmaptitle ()
{
	maptitle ["xterm"]=     set_title_xterm;
	maptitle ["aixterm"]=   set_title_xterm;
	maptitle ["dtterm"]=    set_title_xterm;
	maptitle ["iris-ansi"]= set_title_iris_ansi;
	maptitle ["sun-cmd"]=   set_title_sun_cmd;
	maptitle ["hpterm"]=    set_title_hpterm;
	return true;
}

bool maptitle_inited= initmaptitle ();

void set_title_terminfo (const std::string & title)
{
	TRACEFUNC (tr, "set_title_terminfo");

	if (! isatty (STDOUT_FILENO) )
		return;

	if (const char * term= getenv ("TERM") )
	{
		maptitle_t::iterator it= maptitle.find (term);
		if (it != maptitle.end () )
			(* it->second) (title);
		else
		{
			TRMESSAGE (tr, "TERM not found");
		}
	}
}

} // namespace

#endif

void cursor::set_title (const std::string & title)
{
	TRACEFUNC (tr, "set_title");

	#ifdef BLASSIC_USE_WINDOWS

	SetConsoleTitle (title.c_str () );

	#elif defined BLASSIC_USE_TERMINFO

	set_title_terminfo (title);

	#else

	touch (title);

	#endif
}

// Fin de cursor.cpp
