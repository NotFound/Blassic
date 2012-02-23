// showerror.cpp
// Revision 9-jan-2005

#include "showerror.h"

#include "blassic.h"

#include "error.h"

#include "trace.h"

#include <iostream>
using std::cerr;
using std::endl;

#ifdef BLASSIC_USE_WINDOWS

#include <windows.h>

void showlasterror ()
{
	TRACEFUNC (tr, "showlasterror");

	if (! showdebuginfo () )
		return;

	char * lpMsgBuf;
	DWORD r= FormatMessage (
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError (),
		MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char *) & lpMsgBuf,
		0,
		NULL);
	if (r)
	{
		TRMESSAGE (tr, lpMsgBuf);
		CharToOemBuff (lpMsgBuf, lpMsgBuf, r);
		try
		{
			cerr << lpMsgBuf << endl;
		}
		catch (...)
		{
			r= 0;
		}
		LocalFree (lpMsgBuf);
	}
	if (! r)
	{
		static const char FAILED []=
			"FormatMessage failed, can't show error info";
		TRMESSAGE (tr, FAILED);
		cerr << FAILED << endl;
	}
}

#else

#include <string.h>
#include <errno.h>

void showlasterror ()
{
	TRACEFUNC (tr, "showlasterror");

	if (! showdebuginfo () )
		return;

	const char * message= strerror (errno);
	TRMESSAGE (tr, message);
	cerr << message << endl;
}

#endif

void showlasterror (const char * str)
{
	TRACEFUNC (tr, "showlasterror (str)");

	if (! showdebuginfo () )
		return;

	cerr << str << ": ";
	showlasterror ();
}

// End of showerror.cpp
