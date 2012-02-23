// fileprinter.cpp
// Revision 9-jan-2005

#include "file.h"

#include "blassic.h"
#include "error.h"
#include "sysvar.h"
#include "showerror.h"

#include "trace.h"

// para strerror (errno)
//#include <string.h>
//#include <errno.h>

#include <iostream>
using std::cerr;
using std::endl;

#include <memory>
using std::auto_ptr;

#include <cassert>
#define ASSERT assert

#ifdef BLASSIC_USE_WINDOWS

#include <windows.h>
#undef min
#undef max

#else

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#endif

//***********************************************
//		Auxiliary functions
//***********************************************

namespace {

#ifdef BLASSIC_USE_WINDOWS

class GuardCharp {
public:
	GuardCharp (char * pc) :
		pc (pc)
	{ }
	~GuardCharp ()
	{
		delete [] pc;
	}
private:
	char * pc;
};

class GuardHMODULE {
public:
	GuardHMODULE (HMODULE h) :
		h (h)
	{ }
	~GuardHMODULE ()
	{
		FreeLibrary (h);
	}
private:
	HMODULE h;
};

class GuardHPRINTER {
public:
	GuardHPRINTER (HANDLE h) :
		h (h)
	{ }
	~GuardHPRINTER ()
	{
		ClosePrinter (h);
	}
private:
	HANDLE h;
};

std::string getdefaultprinter ()
{
	// This routine is based in several founded on the web,
	// correcting several errors and adapted to C++.
	OSVERSIONINFO osv;
	osv.dwOSVersionInfoSize= sizeof (OSVERSIONINFO);
	GetVersionEx (& osv);
	switch (osv.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_WINDOWS:
		// Windows 95 or 98, use EnumPrinters
		{
			SetLastError (0);
			DWORD dwNeeded= 0, dwReturned= 0;
			EnumPrinters (PRINTER_ENUM_DEFAULT, NULL, 2,
				NULL, 0, & dwNeeded, & dwReturned);
			if (GetLastError () != ERROR_INSUFFICIENT_BUFFER ||
				dwNeeded == 0)
			{
				if (showdebuginfo () )
				{
					cerr << "EnumPrinters get buffer "
						"failed" <<
						endl;
					showlasterror ();
				}
				throw ErrBlassicInternal; // Provisional
			}
			char * aux= new char [dwNeeded];
			GuardCharp guardcharp (aux);
			PRINTER_INFO_2 * ppi2=
				reinterpret_cast <PRINTER_INFO_2 *> (aux);
			if (! EnumPrinters (PRINTER_ENUM_DEFAULT, NULL, 2,
				(LPBYTE) ppi2, dwNeeded,
				&dwNeeded, & dwReturned) )
			{
				if (showdebuginfo () )
				{
					cerr << "EnumPrinters failed" << endl;
					showlasterror ();
				}
				throw ErrBlassicInternal; // Provisional
			}
			return ppi2->pPrinterName;
		}
	case VER_PLATFORM_WIN32_NT:
		if (osv.dwMajorVersion >= 5) // Windows 2000 or later
		{
			HMODULE hWinSpool= LoadLibrary ("winspool.drv");
			if (! hWinSpool)
			{
				if (showdebuginfo () )
				{
					cerr << "LoadLibrary failed" << endl;
					showlasterror ();
				}
				throw ErrBlassicInternal;
			}
			GuardHMODULE guardh (hWinSpool);
			#ifdef UNICODE
			static const TCHAR GETDEFAULTPRINTER []=
				"GetDefaultPrinterW";
			#else
			static const TCHAR GETDEFAULTPRINTER []=
				"GetDefaultPrinterA";
			#endif
			PROC fnGetDefaultPrinter= GetProcAddress (hWinSpool,
				GETDEFAULTPRINTER);
			if (! fnGetDefaultPrinter)
			{
				if (showdebuginfo () )
				{
					cerr << "GetProcAddress failed" <<
						endl;
					showlasterror ();
				}
				throw ErrBlassicInternal;
			}
			typedef BOOL (* GetDefaultPrinter_t) (LPTSTR, LPDWORD);
			GetDefaultPrinter_t callGetDefaultPrinter=
				(GetDefaultPrinter_t) fnGetDefaultPrinter;
			DWORD dwBufferLength= 0;
			SetLastError (0);
			callGetDefaultPrinter (NULL, & dwBufferLength);
			if (GetLastError () != ERROR_INSUFFICIENT_BUFFER)
			{
				if (showdebuginfo () )
				{
					cerr << "GetDefaultPrinter get buffer "
						"failed" << endl;
					showlasterror ();
				}
				throw ErrBlassicInternal;
			}
			char * buffer= new char [dwBufferLength];
			GuardCharp guardcharp (buffer);
			if (! callGetDefaultPrinter
				(buffer, & dwBufferLength) )
			{
				if (showdebuginfo () )
				{
					cerr << "GetDefaultPrinter failed" <<
						endl;
					showlasterror ();
				}
				throw ErrBlassicInternal;
			}
			return std::string (buffer);
		}
		else // NT 4.0 or earlier
		{
			const DWORD MAXBUFFERSIZE= 250;
			TCHAR cBuffer [MAXBUFFERSIZE];
			if (GetProfileString ("windows", "device", ",,,",
				cBuffer, MAXBUFFERSIZE) <= 0)
			{
				if (showdebuginfo () )
					cerr << "GetProfileString failed" <<
						endl;
				throw ErrBlassicInternal;
			}
			strtok (cBuffer, ",");
			return std::string (cBuffer);
		}
	default:
		ASSERT (false);
		return std::string (); // Make the compiler happy.
	}
}

#else

class GuardHandle {
public:
	GuardHandle (int handle) :
		handle (handle)
	{
		TRACEFUNC (tr, "GuardHandle::GuardHandle");

		if (handle == -1)
		{
			if (showdebuginfo () )
				cerr << "GuardHandle with invalid handle" <<
					endl;
			throw ErrBlassicInternal;
		}
	}
	~GuardHandle ()
	{
		do_close (true);
	}
	void close ()
	{
		do_close (false);
	}
private:
	void do_close (bool destructing)
	{
		TRACEFUNC (tr, "GuardHandle::do_close");

		if (handle != CLOSED)
		{
			int aux= handle;
			handle= CLOSED;
			if (::close (aux) != 0)
			{
				// Problem: this message can be lost because
				// stdeerr may be redirected to null when
				// using this.

				#if 0
				const char * message= strerror (errno);
				TRMESSAGE (tr, message);
				if (showdebuginfo () )
					cerr << "Error closing handle: " <<
						message << endl;
				#else

				showlasterror ("Error closing handle");

				#endif

				if (! destructing)
					throw ErrBlassicInternal;
			}
		}
	}

	static const int CLOSED= -1;
	int handle;
};

class Dup2Save {
	// Checks are not done because the functions can fail
	// if Blassic is running in detached mode.
	// More work required to make checks that works in
	// all cases.
public:
	Dup2Save (int newhandle, int oldhandle) :
		oldhandle (oldhandle)
	{
		savedhandle= dup (oldhandle);
		dup2 (newhandle, oldhandle);
	}
	~Dup2Save ()
	{
		release ();
	}
	void release ()
	{
		if (savedhandle != RELEASED)
		{
			dup2 (savedhandle, oldhandle);
			close (savedhandle);
			savedhandle= RELEASED;
		}
	}
private:
	static const int RELEASED= -1;
	int oldhandle;
	int savedhandle;
};

#endif

} // namespace

namespace blassic {

namespace file {

class BlFilePrinter : public BlFile {
public:
	BlFilePrinter ();
	~BlFilePrinter ();
	bool isfile () const { return false; }
	void flush ();
	size_t getwidth () const;
	void tab ();
	void tab (size_t n);
	void endline ();
	void setwidth (size_t w);
	void setmargin (size_t m);
private:
	void outstring (const std::string & str);
	void outchar (char c);

	class Internal;
	Internal * pin;
};

//***********************************************
//              BlFilePrinter::Internal
//***********************************************

class BlFilePrinter::Internal {
public:
	Internal () :
		pos (0),
		width (80),
		margin (0)
	{ }
	~Internal ();
	void close ();
	size_t getwidth () const { return width; }
	void checkmargin ();
	void tab ();
	void tab (size_t n);
	void endline ();
	void outstring (std::string str);
	void outchar (char c);
	void setwidth (size_t w)
	{ width= w; }
	void setmargin (size_t m)
	{ margin= m - 1; }
private:
	void do_printing (const char * printcommand);

	std::string text;
	size_t pos;
	size_t width;
	size_t margin;
};

BlFilePrinter::Internal::~Internal ()
{
	TRACEFUNC (tr, "BlFilePrinter::Internal::~Internal");

	try
	{
		close ();
	}
	catch (...)
	{
		if (showdebuginfo () )
			cerr << "Error closing printer buffer" << endl;
	}
}

void BlFilePrinter::Internal::close ()
{
	TRACEFUNC (tr, "BlFilePrinter::Internal::close");

	static const char blassic_print_command []= "BLASSIC_PRINT_COMMAND";

	// Print only if there is somethnig to print.
	if (text.empty () )
	{
		TRMESSAGE (tr, "Nothing to print.");
		return;
	}
	TRMESSAGE (tr, "Begin printing");

	const char * printcommand= getenv (blassic_print_command);
	do_printing (printcommand);
}



#ifdef BLASSIC_USE_WINDOWS

void BlFilePrinter::Internal::do_printing (const char * printcommand)
{
	TRACEFUNC (tr, "BlFilePrinter::Internal::do_printing");

	if (printcommand != NULL)
	{
		TRMESSAGE (tr, std::string ("Popening to ") + printcommand);
		//BlFilePopen bfp (printcommand, Output);
		//bfp << text;
		auto_ptr <BlFile> pbfp (newBlFilePopen (printcommand, Output) );
		* pbfp << text;
		return;
	}

	std::string printername= getdefaultprinter ();
	TRMESSAGE (tr, std::string ("Printer is ") + printername);

	HANDLE hPrinter;
	if (! OpenPrinter ( (char *) printername.c_str (), & hPrinter, NULL) )
	{
		if (showdebuginfo () )
		{
			cerr << "OpenPrinter failed" << endl;
			showlasterror ();
		}
		throw ErrBlassicInternal;
	}
	GuardHPRINTER guardhprinter (hPrinter);
	DOC_INFO_1 doc_info;
	doc_info.pDocName= (char *) "Blassic printing output";
	doc_info.pOutputFile= NULL;
	doc_info.pDatatype= NULL;
	DWORD jobid= StartDocPrinter (hPrinter, 1, (LPBYTE) & doc_info);
	if (jobid == 0)
	{
		if (showdebuginfo () )
		{
			cerr << "StartDocPrinter failed" << endl;
			showlasterror ();
		}
		throw ErrBlassicInternal;
	}
	DWORD written= 0;
	WritePrinter (hPrinter, (void *) text.data (), text.size (),
		& written);
	if (written != text.size () )
	{
		if (showdebuginfo () )
		{
			cerr << "WritePrinter failed" << endl;
			showlasterror ();
		}
		throw ErrBlassicInternal;
	}
	EndDocPrinter (hPrinter);
	// ClosePrinter is done by the guard.
}

#else

void BlFilePrinter::Internal::do_printing (const char * printcommand)
{
	TRACEFUNC (tr, "BlFilePrinter::Internal::do_printing");

	if (printcommand == NULL)
		printcommand= "lp";

	FILE * f;

	// Block opened to automatically restore
	// saved handles when closing it.
	{
		// Redirect standard handles not used to /dev/null
		int newstd= open ("/dev/null", O_RDWR);
		if (newstd == -1)
		{
			#if 0
			if (showdebuginfo () )
				cerr << "open /dev/null failed: " <<
					strerror (errno) << endl;
			#else

			showlasterror ("open /dev/null failed");

			#endif

			throw ErrFunctionCall;
		}

		// Save standard handles not used
		#if 0
		int savestdout= dup (STDOUT_FILENO);
		int savestderr= dup (STDERR_FILENO);

		dup2 (newstd, STDOUT_FILENO);
		dup2 (newstd, STDERR_FILENO);
		::close (newstd);
		#else
		GuardHandle guardnew (newstd);
		Dup2Save saveout (newstd, STDOUT_FILENO);
		Dup2Save saveerr (newstd, STDERR_FILENO);
		guardnew.close ();
		#endif

		f= popen (printcommand, "w");

		#if 0
		// Restore saved standard handles
		dup2 (savestdout, STDOUT_FILENO);
		dup2 (savestderr, STDERR_FILENO);
		::close (savestdout);
		::close (savestderr);
		#endif
	}

	if (f == NULL)
	{
		if (showdebuginfo () )
			cerr << "Error in popen print command" << endl;
		throw ErrBlassicInternal;
	}
	size_t size= text.size ();
	size_t written= write (fileno (f), text.data (), size);
	if (written != size)
		showlasterror ("Error writing to print command");
	pclose (f);
}

#endif

void BlFilePrinter::Internal::checkmargin ()
{
	if (pos == 0)
		text+= std::string (margin, ' ');
}

void BlFilePrinter::Internal::tab ()
{
	size_t zone= sysvar::get16 (sysvar::Zone);
	if (zone == 0)
	{
		outchar ('\t');
		return;
	}
	if (width > 0 && pos >= (width / zone) * zone)
		endline ();
	else
	{
		do
		{
			outchar (' ');
		} while (pos % zone);
	}
}

void BlFilePrinter::Internal::tab (size_t n)
{
	if (pos > n)
		endline ();
	size_t maxpos= std::min (width, n);
	while (pos < maxpos)
		outchar (' ');
}

void BlFilePrinter::Internal::endline ()
{
	switch (sysvar::get (sysvar::PrinterLine) )
	{
	case 0:
		text+= '\n'; break;
	case 1:
		text+= "\r\n"; break;
	case 2:
		text+= '\r'; break;
	}
	pos= 0;
}

void BlFilePrinter::Internal::outstring (std::string str)
{
	if (width > 0)
	{
		size_t l= str.size ();
		while (pos + l > width)
		{
			checkmargin ();
			text+= str.substr (0, width - pos);
			str.erase (0, width - pos);
			endline ();
			l= str.size ();
		}
	}
	checkmargin ();
	text+= str;
	pos+= str.size ();
}

void BlFilePrinter::Internal::outchar (char c)
{
	if (width > 0 && pos>= width)
		endline ();
	checkmargin ();
	text+= c;
	++pos;
}

//***********************************************
//              BlFilePrinter
//***********************************************

BlFile * newBlFilePrinter ()
{
	return new BlFilePrinter;
}

BlFilePrinter::BlFilePrinter () :
	BlFile (Output),
	pin (new Internal)
{ }

BlFilePrinter::~BlFilePrinter ()
{
	delete pin;
}

void BlFilePrinter::flush ()
{
}

size_t BlFilePrinter::getwidth () const
{
	return pin->getwidth ();
}

void BlFilePrinter::tab ()
{
	pin->tab ();
}

void BlFilePrinter::tab (size_t n)
{
	pin->tab (n);
}

void BlFilePrinter::endline ()
{
	pin->endline ();
}

void BlFilePrinter::outstring (const std::string & str)
{
	pin->outstring (str);
}

void BlFilePrinter::outchar (char c)
{
	pin->outchar (c);
}

void BlFilePrinter::setwidth (size_t w)
{
	pin->setwidth (w);
}

void BlFilePrinter::setmargin (size_t m)
{
	pin->setmargin (m);
}

} // namespace file

} // namespace blassic

// End of fileprinter.cpp
