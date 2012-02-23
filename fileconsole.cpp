// fileconsole.cpp
// Revision 6-feb-2005

#ifdef __BORLANDC__
#pragma warn -8022
#endif

#include "file.h"

#include "blassic.h"
#include "error.h"
#include "cursor.h"
#include "edit.h"
#include "sysvar.h"
#include "util.h"

#include "trace.h"

#include <iostream>
using std::cerr;
using std::endl;
#include <string>
#include <algorithm>

#ifndef BLASSIC_USE_WINDOWS

#include <unistd.h>

#else

#include <windows.h>
#undef max
#undef min
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined HAVE_IO_H || ! defined BLASSIC_CONFIG
#include <io.h>
#endif

#endif

//***********************************************
//		Auxiliary functions
//***********************************************

namespace {

class updateposchar {
public:
	updateposchar (int & pos) :
		pos (pos)
	{ }
	void operator () (const char c)
	{
		switch (c)
		{
		case '\r':
		case '\n':
			pos= 0;
			break;
		case '\b':
			if (pos > 0)
				--pos;
			break;
		case '\a':
			// Bell does not use space in screen.
			break;
		case '\t':
			pos= ( (pos / 8) + 1) * 8;
			break;
		default:
			++pos;
		}
	}
private:
	int & pos;
};

void updatepos (int & pos, const std::string & str)
{
	std::for_each (str.begin (), str.end (), updateposchar (pos) );
}

} // namespace

namespace blassic {

namespace file {

//***********************************************
//              BlFileConsole
//***********************************************

class BlFileConsole : public BlFile {
public:
	//BlFileConsole (std::istream & nin, std::ostream & nout);
	BlFileConsole ();
	bool isfile () const { return false; }
	virtual bool eof ();
	virtual void flush ();
	virtual size_t getwidth () const;
	virtual void movecharforward ();
	virtual void movecharforward (size_t n);
	virtual void movecharback ();
	virtual void movecharback (size_t n);
	virtual void movecharup ();
	virtual void movecharup (size_t n);
	virtual void movechardown ();
	virtual void movechardown (size_t n);
	virtual void showcursor ();
	virtual void hidecursor ();
	virtual std::string getkey ();
	virtual std::string inkey ();
	void getline (std::string & str, bool endline= true);
	std::string read (size_t n);
	void tab ();
	void tab (size_t n);
	void gotoxy (int x, int y);
	virtual void setcolor (int color);
	virtual void setbackground (int color);
	virtual void cls ();
	int pos ();
	bool poll ();
private:
	void outstring (const std::string & str);
	void outchar (char c);
	//void outnumber (BlNumber n);
	//void outinteger (BlInteger n);

	std::istream & in;
	std::ostream & out;
	bool ttyin, ttyout;
	#ifndef BLASSIC_USE_WINDOWS
	int xpos;
	#endif
};

BlFile * newBlFileConsole ()
{
	return new BlFileConsole ();
}

//BlFileConsole::BlFileConsole (std::istream & nin, std::ostream & nout) :
BlFileConsole::BlFileConsole () :
	BlFile (OpenMode (Input | Output) ),
	//in (nin),
	//out (nout),
	in (std::cin),
	out (std::cout),
	ttyin (isatty (0) ),
	ttyout (isatty (1) )
	//#ifndef _Windows
	#ifndef BLASSIC_USE_WINDOWS
	, xpos (0)
	#endif
{
	TRACEFUNC (tr, "BlFileConsole::BlFileConsole");
	TRMESSAGE (tr, std::string ("ttyin ") +
		(ttyin ? "is" : "is not") + " a tty");
	TRMESSAGE (tr, std::string ("ttyout ") +
		(ttyout ? "is" : "is not") + " a tty");
}

bool BlFileConsole::eof ()
{
	if (! ttyin)
	{
		int c= in.get ();
		if (! in || c == EOF)
			return true;
		else
		{
			in.unget ();
			return false;
		}
	}
	else
	{
		return false;
	}
}

void BlFileConsole::flush ()
{
	out << std::flush;
}

size_t BlFileConsole::getwidth () const
{
	return cursor::getwidth ();
}

void BlFileConsole::movecharforward ()
{
	cursor::movecharforward ();
}

void BlFileConsole::movecharforward (size_t n)
{
	cursor::movecharforward (n);
}

void BlFileConsole::movecharback ()
{
	cursor::movecharback ();
}

void BlFileConsole::movecharback (size_t n)
{
	cursor::movecharback (n);
}

void BlFileConsole::movecharup ()
{
	cursor::movecharup ();
}

void BlFileConsole::movecharup (size_t n)
{
	cursor::movecharup (n);
}

void BlFileConsole::movechardown ()
{
	cursor::movechardown ();
}

void BlFileConsole::movechardown (size_t n)
{
	cursor::movechardown (n);
}

void BlFileConsole::showcursor ()
{
	cursor::showcursor ();
}

void BlFileConsole::hidecursor ()
{
	cursor::hidecursor ();
}

std::string BlFileConsole::getkey ()
{
	//TRACEFUNC (tr, "BlFileConsole::getkey");

	if (ttyin)
	{
		std::string str= cursor::getkey ();

		#ifdef BLASSIC_USE_WINDOWS

		if (str.size () == 1)
		{
			char c= str [0];
			OemToCharBuff (& c, & c, 1);
			return std::string (1, c);
		}

		#endif

		return str;
	}
	else
	{
		int c= in.get ();
		if (! in || c == EOF)
			throw ErrPastEof;
		return std::string (1, static_cast <char> (c) );
	}
}

std::string BlFileConsole::inkey ()
{
	std::string str= cursor::inkey ();

	#ifdef BLASSIC_USE_WINDOWS

	if (ttyin && str.size () == 1)
	{
		char c= str [0];
		OemToCharBuff (& c, & c, 1);
		return std::string (1, c);
	}

	#endif

	return str;
}

void BlFileConsole::getline (std::string & str, bool endline)
{
	using blassic::edit::editline;

	TRACEFUNC (tr, "BlFileConsole::getline");

	if (ttyin)
	{
		std::string auxstr;
		//int inicol= getcursorx ();
		int inicol= pos ();
		while (! editline (* this, auxstr, 0, inicol, endline) )
			continue;
		swap (str, auxstr);
	}
	else
	{
		std::getline (in, str);
		if (! in)
			throw ErrPastEof;
	}

	if (fInterrupted)
	{
		in.clear ();
		str.erase ();
		return;
	}

	#ifdef BLASSIC_USE_WINDOWS

	if (ttyin)
	{
		size_t l= str.size ();
		util::auto_buffer <char> aux (l);
		OemToCharBuff (str.data (), aux, l);
		str= std::string (aux, l);
	}

	#endif
}

std::string BlFileConsole::read (size_t n)
{
	util::auto_buffer <char> buf (n);
	in.read (buf, n);
	return std::string (buf, n);
}

void BlFileConsole::tab ()
{
	int zone= static_cast <int> (sysvar::get16 (sysvar::Zone) );
	if (zone == 0)
	{
		outchar ('\t');
		return;
	}

	#if 0

	#ifdef BLASSIC_USE_WINDOWS

	int newpos= getcursorx ();

	#else

	int pos= xpos;

	#endif

	#else

	int newpos= pos ();

	#endif

	const int width= 80; // This may need another approach.
	if (newpos >= (width / zone) * zone)
		endline ();
	else
	{
		do
		{
			outchar (' ');
			++newpos;
		} while (newpos % zone);
	}
}

void BlFileConsole::tab (size_t n)
{
	int p= pos ();
	if (p > static_cast <int> (n) )
	{
		outchar ('\n');
		p= pos ();
	}
	outstring (std::string (n - p, ' ') );
}

void BlFileConsole::outstring (const std::string & str)
{
	TRACEFUNC (tr, "BlFileConsole::outstring");

	#ifdef BLASSIC_USE_WINDOWS

	if (ttyout)
	{
		size_t l= str.size ();
		util::auto_buffer <char> aux (l + 1);
		CharToOemBuff (str.data (), aux, l);
		aux [l]= 0;
		out << aux;
	}
	else
		out << str;

	#else

	out << str;
	updatepos (xpos, str);

	#endif

	#ifndef NDEBUG
	out << std::flush;
	#endif

	if (! out)
	{
		out.clear ();
		//throw std::runtime_error ("Al diablo");
	}
}

void BlFileConsole::outchar (char c)
{
	#ifdef BLASSIC_USE_WINDOWS

	if (ttyout)
		CharToOemBuff (& c, & c, 1);

	#endif

	if (c == '\n')
		out << endl;
	else
		out << c;

	#ifndef BLASSIC_USE_WINDOWS

	updateposchar (xpos).operator () (c);

	#endif

	if (! out)
	{
		out.clear ();
		//throw std::runtime_error ("Al diablo");
	}
}

#if 0

void BlFileConsole::outnumber (BlNumber n)
{
	#if 0
	if (graphics::ingraphicsmode () )
		graphics::stringout (to_string (n) );
	else
	#endif
		out << n;
}

void BlFileConsole::outinteger (BlInteger n)
{
	#if 0
	if (graphics::ingraphicsmode () )
		graphics::stringout (to_string (n) );
	else
	#endif
		out << n;
}

#endif

void BlFileConsole::gotoxy (int x, int y)
{
	cursor::gotoxy (x, y);

	#ifndef BLASSIC_USE_WINDOWS

	xpos= x;

	#endif
}

void BlFileConsole::setcolor (int color)
{
	cursor::textcolor (color);
}

void BlFileConsole::setbackground (int color)
{
	cursor::textbackground (color);
}

void BlFileConsole::cls ()
{
	cursor::cls ();

	#ifndef BLASSIC_USE_WINDOWS

	xpos= 0;

	#endif
}

int BlFileConsole::pos ()
{
	#ifdef BLASSIC_USE_WINDOWS

	return cursor::getcursorx ();

	#else

	return xpos;

	#endif
}

bool BlFileConsole::poll ()
{
	return cursor::pollin ();
}

} // namespace file

} // namespace blassic

// End of fileconsole.cpp
