// filewindow.cpp
// Revision 23-jan-2005

#include "file.h"

#include "blassic.h"
#include "error.h"
#include "graphics.h"
#include "edit.h"

#include "trace.h"

namespace blassic {

namespace file {

//***********************************************
//		BlFileWindow
//***********************************************

class BlFileWindow : public BlFile {
public:
	BlFileWindow (BlChannel ch);
	BlFileWindow (BlChannel ch, int x1, int x2, int y1, int y2);
	~BlFileWindow ();
	void reset (int x1, int x2, int y1, int y2);
	bool isfile () const;
	bool istextwindow () const;
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
	void endline ();
	void gotoxy (int x, int y);
	virtual void setcolor (int color);
	virtual int getcolor ();
	virtual void setbackground (int color);
	virtual int getbackground ();
	virtual void cls ();
	std::string copychr (BlChar from, BlChar to);
	int pos ();
	int vpos ();
	void tag ();
	void tagoff ();
	bool istagactive ();
	void inverse (bool active);
	bool getinverse ();
	void bright (bool active);
	bool getbright ();
	bool poll ();
	void scroll (int nlines);
private:
	void outstring (const std::string & str);
	void outchar (char c);

	BlChannel ch;
};

BlFile * newBlFileWindow (BlChannel ch)
{
	return new BlFileWindow (ch);
}

BlFile * newBlFileWindow (BlChannel ch, int x1, int x2, int y1, int y2)
{
	return new BlFileWindow (ch, x1, x2, y1, y2);
}

BlFileWindow::BlFileWindow (BlChannel ch) :
	BlFile (OpenMode (Input | Output) ),
	ch (ch)
{
	if (ch != 0)
		throw ErrBlassicInternal;
}

BlFileWindow::BlFileWindow (BlChannel ch, int x1, int x2, int y1, int y2) :
	BlFile (OpenMode (Input | Output) ),
	ch (ch)
{
	graphics::definewindow (ch, x1, x2, y1, y2);
}

BlFileWindow::~BlFileWindow ()
{
	graphics::undefinewindow (ch);
}

void BlFileWindow::reset (int x1, int x2, int y1, int y2)
{
	graphics::definewindow (ch, x1, x2, y1, y2);
}

bool BlFileWindow::isfile () const
{
	return false;
}

bool BlFileWindow::istextwindow () const
{
	return true;
}

bool BlFileWindow::eof ()
{
	return false;
}

void BlFileWindow::flush ()
{
	// Nothing to do
}

size_t BlFileWindow::getwidth () const
{
	return graphics::getlinewidth (ch);
}

void BlFileWindow::movecharforward ()
{
	graphics::movecharforward (ch, 1);
}

void BlFileWindow::movecharforward (size_t n)
{
	graphics::movecharforward (ch, n);
}

void BlFileWindow::movecharback ()
{
	graphics::movecharback (ch, 1);
}

void BlFileWindow::movecharback (size_t n)
{
	graphics::movecharback (ch, n);
}

void BlFileWindow::movecharup ()
{
	graphics::movecharup (ch, 1);
}

void BlFileWindow::movecharup (size_t n)
{
	graphics::movecharup (ch, n);
}

void BlFileWindow::movechardown ()
{
	graphics::movechardown (ch, 1);
}

void BlFileWindow::movechardown (size_t n)
{
	graphics::movechardown (ch, n);
}

void BlFileWindow::showcursor ()
{
	graphics::showcursor (ch);
}

void BlFileWindow::hidecursor ()
{
	graphics::hidecursor (ch);
}

std::string BlFileWindow::getkey ()
{
	return graphics::getkey ();
}

std::string BlFileWindow::inkey ()
{
	return graphics::inkey ();
}

void BlFileWindow::getline (std::string & str, bool endline)
{
	using blassic::edit::editline;

	std::string auxstr;
	int inicol= graphics::xpos (ch);
	while (! editline (* this, auxstr, 0, inicol, endline) )
		continue;
	swap (str, auxstr);
}

std::string BlFileWindow::read (size_t)
{
	throw ErrNotImplemented;
}

void BlFileWindow::tab ()
{
	graphics::tab (ch);
}

void BlFileWindow::tab (size_t n)
{
	graphics::tab (ch, n);
}

void BlFileWindow::endline ()
{
	outstring ("\r\n");
}

void BlFileWindow::gotoxy (int x, int y)
{
	graphics::gotoxy (ch, x, y);
}

void BlFileWindow::setcolor (int color)
{
	graphics::setcolor (ch, color);
}

int BlFileWindow::getcolor ()
{
	return graphics::getcolor (ch);
}

void BlFileWindow::setbackground (int color)
{
	graphics::setbackground (ch, color);
}

int BlFileWindow::getbackground ()
{
	return graphics::getbackground (ch);
}

void BlFileWindow::outstring (const std::string & str)
{
	TRACEFUNC (tr, "BlFileWindow::outstring");
	TRMESSAGE (tr, str);

	graphics::stringout (ch, str);
}

void BlFileWindow::outchar (char c)
{
	graphics::charout (ch, c);
}

void BlFileWindow::cls ()
{
	graphics::cls (ch);
}

std::string BlFileWindow::copychr (BlChar from, BlChar to)
{
	return graphics::copychr (ch, from, to);
}

int BlFileWindow::pos ()
{
	return graphics::xpos (ch);
}

int BlFileWindow::vpos ()
{
	return graphics::ypos (ch);
}

void BlFileWindow::tag ()
{
	graphics::tag (ch);
}

void BlFileWindow::tagoff ()
{
	graphics::tagoff (ch);
}

bool BlFileWindow::istagactive ()
{
	return graphics::istagactive (ch);
}

void BlFileWindow::inverse (bool active)
{
	graphics::inverse (ch, active);
}

bool BlFileWindow::getinverse ()
{
	return graphics::getinverse (ch);
}

void BlFileWindow::bright (bool active)
{
	graphics::bright (ch, active);
}

bool BlFileWindow::getbright ()
{
	return graphics::getbright (ch);
}

bool BlFileWindow::poll ()
{
	return graphics::pollin ();
}

void BlFileWindow::scroll (int nlines)
{
	graphics::scroll (ch, nlines);
}

} // namespace file

} // namespace blassic

// End of filewindow.cpp
