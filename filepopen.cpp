// filepopen.cpp
// Revision 6-feb-2005

#include "file.h"

#include "blassic.h"
#include "error.h"
#include "showerror.h"
#include "sysvar.h"
#include "util.h"
using util::to_string;

#include "trace.h"

#include <iostream>
using std::cerr;
using std::endl;

#ifndef BLASSIC_USE_WINDOWS

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

// Suggested in autoconf manual.
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(star_val) ((unsigned) (stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif


#include <fcntl.h>
#include <sys/poll.h>

#else

#include <windows.h>
#undef min
#undef max

#endif

// para strerror (errno)
#include <string.h>
#include <errno.h>

#include <assert.h>

#define ASSERT assert

namespace blassic {

namespace file {

//***********************************************
//              BlFilePopen
//***********************************************

class BlFilePopen : public BlFile {
protected:
	virtual char getcharfrombuffer ()= 0;
	virtual void outstring (const std::string & str)= 0;
	virtual void outchar (char c)= 0;
	bool forread;
	bool forwrite;
	bool witherr;
public:
	BlFilePopen (OpenMode nmode);
	bool isfile () const { return true; }
	virtual void closein ()= 0;
	virtual void closeout ()= 0;
	virtual bool eof ()= 0;
	void flush ();
	virtual void endline ()= 0;
	virtual std::string read (size_t n)= 0;
	virtual void getline (std::string & str, bool endline= true)= 0;
	virtual bool poll ()= 0;
};

BlFilePopen::BlFilePopen (OpenMode nmode) :
	BlFile (nmode)
{
	if (nmode & Input)
		forread= true;
	if (nmode & Output)
		forwrite= true;
	if (nmode & WithErr)
		witherr= true;
	const OpenMode checkmode= OpenMode (Input | Output | WithErr);
	if ( (nmode | checkmode) != checkmode)
		throw ErrFileMode;
}

void BlFilePopen::flush ()
{
}

#ifdef BLASSIC_USE_WINDOWS

//***********************************************
// Auxiliary functions and classes for windows
//***********************************************

namespace {

class ProtectHandle {
public:
	ProtectHandle () :
		handle (INVALID_HANDLE_VALUE)
	{ }
	ProtectHandle (HANDLE handle) :
		handle (handle)
	{ }
	~ProtectHandle ()
	{
		close ();
	}
	void reset (HANDLE newhandle)
	{
		close ();
		handle= newhandle;
	}
	void close ()
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle (handle);
			handle= INVALID_HANDLE_VALUE;
		}
	}
	void release ()
	{
		handle= INVALID_HANDLE_VALUE;
	}
private:
	HANDLE handle;
};

std::string makecommand (const std::string & str)
{
	const char * comspec= getenv ("COMSPEC");
	if (comspec == NULL)
		comspec= "C:\\COMMAND.COM";
	std::string command= comspec;
	command+= " /C ";
	command+= str;
	return command;
}

void create_pipe (HANDLE & hread, HANDLE & hwrite)
{
	if (CreatePipe (& hread, & hwrite, NULL, 0) == 0)
	{
		showlasterror ("CreatePipe failed");
		throw ErrFunctionCall;
	}
}

HANDLE duplicate_inherit (HANDLE handle, HANDLE current)
{
	HANDLE newhandle;
	if (DuplicateHandle (current, handle,
		current, & newhandle,
		0,
		TRUE, // Inherited
		DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS) == 0)
	{
		showlasterror ("DuplicateHandle hread failed");
		throw ErrFunctionCall;
	}
	return newhandle;
}

HANDLE createnull ()
{
	SECURITY_ATTRIBUTES sec;
	sec.nLength= sizeof sec;
	sec.lpSecurityDescriptor= NULL;
	sec.bInheritHandle= TRUE;

	const HANDLE hnull= CreateFile ("NUL",
		GENERIC_READ | GENERIC_WRITE,
		0,
		& sec, // Inheritable.
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hnull == INVALID_HANDLE_VALUE)
	{
		showlasterror ("Error creating /dev/null handle");
		throw ErrOperatingSystem;
	}
	return hnull;
}

void launchchild (const std::string & str,
	bool forread, bool forwrite, bool witherr,
	HANDLE & hreadpipe, HANDLE & hwritepipe, HANDLE & childprocess)
{
	TRACEFUNC (tr, "launchchild");

	const std::string command= makecommand (str);

	HANDLE current= GetCurrentProcess ();
	const HANDLE hnull= createnull ();
	ProtectHandle prnull (hnull);

	STARTUPINFO start;
	start.cb= sizeof start;
	GetStartupInfo (& start);
	start.dwFlags= STARTF_USESTDHANDLES;
	start.hStdInput= hnull;
	start.hStdOutput= hnull;
	start.hStdError= hnull;
	DWORD creationflags= 0;

	HANDLE hwritechild;
	ProtectHandle prreadpipe;
	ProtectHandle prwritechild;
	if (forread)
	{
		TRMESSAGE (tr, "Creating pipe input");

		HANDLE hwrite;
		create_pipe (hreadpipe, hwrite);
		prreadpipe.reset (hreadpipe);
		hwritechild= duplicate_inherit (hwrite, current);
		prwritechild.reset (hwritechild);
		start.hStdOutput= hwritechild;
		if (witherr)
			start.hStdError= hwritechild;
	}
	HANDLE hreadchild;
	ProtectHandle prwritepipe;
	ProtectHandle prreadchild;
	if (forwrite)
	{
		TRMESSAGE (tr, "Creating pipe output");

		// In Windows 95 or 98 detach the process from the
		// actual console, without that the parent process
		// gets blocked.
		// Now seems inneccessary, and conflicts with
		// bidirectional mode.
		#if 0
		{
			OSVERSIONINFO osv;
			osv.dwOSVersionInfoSize= sizeof (OSVERSIONINFO);
			GetVersionEx (& osv);
			if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				creationflags|= DETACHED_PROCESS;
		}
		#endif

		HANDLE hread;
		create_pipe (hread, hwritepipe);
		prwritepipe.reset (hwritepipe);
		hreadchild= duplicate_inherit (hread, current);
		prreadchild.reset (hreadchild);
		start.hStdInput= hreadchild;
	}

	TRMESSAGE (tr, "Creating pipe process");

	PROCESS_INFORMATION procinfo;
	if (CreateProcess (
		NULL, (char *) command.c_str (),
		NULL, NULL,
		TRUE, // Inherit handles
		creationflags,
		NULL, NULL, & start, & procinfo) == 0)
	{
		showlasterror ( ("CreateProcess " + command +
			" failed in POPEN").c_str () );
		throw ErrFunctionCall;
	}

	// No need to close here the child handles, ProtectHandle
	// takes care of him.

	// Release protection of the parent handles.
	prreadpipe.release ();
	prwritepipe.release ();

	childprocess= procinfo.hProcess;
	CloseHandle (procinfo.hThread);
}

class CriticalSection {
public:
	CriticalSection ()
	{
		InitializeCriticalSection (& object);
	}
	~CriticalSection ()
	{
		DeleteCriticalSection (& object);
	}
	CriticalSection (const CriticalSection &); // Forbidden
	void operator= (const CriticalSection &); // Forbidden
	void enter ()
	{
		EnterCriticalSection (& object);
	}
	void leave ()
	{
		LeaveCriticalSection (& object);
	}
private:
	CRITICAL_SECTION object;
};

class CriticalLock {
public:
	CriticalLock (CriticalSection & crit) :
		crit (crit)
	{
		crit.enter ();
	}
	~CriticalLock ()
	{
		crit.leave ();
	}
private:
	CriticalSection & crit;
};

class Event
{
public:
	Event ()
	{
		hevent= CreateEvent (
			NULL, // Default security
			FALSE, // Automatic
			FALSE, // Initially not signaled
			NULL // Without name
			);
		if (hevent == NULL)
		{
			showlasterror ();
			throw ErrOperatingSystem;
		}
	}
	~Event ()
	{
		CloseHandle (hevent);
	}
	void wait ()
	{
		WaitForSingleObject (hevent, INFINITE);
	}
	void set ()
	{
		SetEvent (hevent);
	}
	void reset ()
	{
		ResetEvent (hevent);
	}
private:
	HANDLE hevent;
};

} // namespace

//***********************************************
//              BlFilePopenWindows
//***********************************************

class BlFilePopenWindows : public BlFilePopen {
public:
	BlFilePopenWindows (const std::string & str, OpenMode mode);
	~BlFilePopenWindows ();
	void closein ();
	void closeout ();
	bool eof ();
	void endline ();
	std::string read (size_t n);
	void getline (std::string & str, bool endline= true);
	bool poll ();
private:
	static const size_t bufsize= 4096;
	char buffer [bufsize];
	size_t bufpos, bufread;
	void readbuffer ();
	char getcharfrombuffer ();

	void read_in_thread ();

	void outstring (const std::string & str);
	void outchar (char c);

	CriticalSection critical;
	Event canread;
	bool nowreading;
	Event hasread;
	HANDLE hreadpipe, hwritepipe;
	HANDLE hprocess;
	HANDLE hreadthread;
	bool finishthread;
	bool error_reading;
	bool eof_found;

	friend DWORD WINAPI thread_popen_read (LPVOID param);
	friend class ProtectThread;
};

class ProtectThread {
public:
	ProtectThread (BlFilePopenWindows & popen) :
		popen (popen),
		released (false)
	{ }
	~ProtectThread ()
	{
		if (! released && popen.hreadthread != NULL)
		{
			popen.finishthread= true;
			ResumeThread (popen.hreadthread);
			CloseHandle (popen.hreadthread);
		}
	}
	void release ()
	{
		released= true;
	}
private:
	BlFilePopenWindows & popen;
	bool released;
};

DWORD WINAPI thread_popen_read (LPVOID param)
{
	BlFilePopenWindows & popen= *
		reinterpret_cast <BlFilePopenWindows *> (param);
	if (popen.finishthread)
		return 0;

	try
	{
		popen.read_in_thread ();
	}
	catch (...)
	{
		popen.error_reading= true;
		popen.nowreading= false;
		popen.hasread.set ();
	}

	return 0;
}

BlFilePopenWindows::BlFilePopenWindows
		(const std::string & str, OpenMode nmode) :
	BlFilePopen (nmode)
{
	TRACEFUNC (tr, "BlFilePopenWindows::BlFilePopenWindows");

	hprocess= INVALID_HANDLE_VALUE;
	hreadpipe= INVALID_HANDLE_VALUE;
	hwritepipe= INVALID_HANDLE_VALUE;
	hreadthread= NULL;
	nowreading= false;
	finishthread= false;
	error_reading= false;
	eof_found= false;

	if (forread)
	{
		DWORD idthread;
		hreadthread= CreateThread (NULL, 1024,
			thread_popen_read, this,
			CREATE_SUSPENDED, & idthread);
		if (hreadthread == NULL)
		{
			showlasterror ();
			throw ErrOperatingSystem;
		}
	}
	ProtectThread protect (* this);
	launchchild (str, forread, forwrite, witherr,
		hreadpipe, hwritepipe, hprocess);
	if (forread)
	{
		ResumeThread (hreadthread);
		nowreading= true;
		canread.set ();
	}
	protect.release ();
}

BlFilePopenWindows::~BlFilePopenWindows ()
{
	TRACEFUNC (tr, "BlFilePopenWindows::~BlFilePopenWindows");

	// This is to workaround errors in C++ Builder.
	if (hprocess == INVALID_HANDLE_VALUE)
		return;

	if (hreadthread != NULL)
	{
		finishthread= true;
		CloseHandle (hreadthread);
		hreadthread= NULL;
		nowreading= true;
		hasread.reset ();
		canread.set ();
	}
	// Close the pipe before wait for termination,
	// the child process can be waiting for
	// input.
	//CloseHandle (hpipe);
	if (hreadpipe != INVALID_HANDLE_VALUE)
		CloseHandle (hreadpipe);
	if (hwritepipe != INVALID_HANDLE_VALUE)
		CloseHandle (hwritepipe);

	WaitForSingleObject (hprocess, INFINITE);
	DWORD exitcode;
	BOOL r= GetExitCodeProcess (hprocess, & exitcode);
	if (r == 0)
	{
		showlasterror ();
	}
	CloseHandle (hprocess);
	if (r == 0)
	{
		throw ErrOperatingSystem;
	}
	TRMESSAGE (tr, "Exit code: " + to_string (exitcode) );
	sysvar::set (sysvar::ShellResult,
		static_cast <BlChar> (exitcode & 0xFF) );
}

char BlFilePopenWindows::getcharfrombuffer ()
{
	{
		CriticalLock lock (critical);
		if (bufpos < bufread)
			return buffer [bufpos++];
	}

	if (error_reading)
		throw ErrOperatingSystem;
	if (eof_found)
		return 0;

	readbuffer ();
	hasread.wait ();

	if (error_reading)
		throw ErrOperatingSystem;
	if (eof_found)
		return 0;

	return buffer [bufpos++];
}

void BlFilePopenWindows::closein ()
{
	TRACEFUNC (tr, "BlFilePopenWindows::closein");

	if (! getmode () & Input)
		throw ErrFileMode;
	finishthread= true;
	CloseHandle (hreadpipe);
	hreadpipe= INVALID_HANDLE_VALUE;

	critical.enter ();
	nowreading= true;
	hasread.reset ();
	canread.set ();
	critical.leave ();

	CloseHandle (hreadthread);
	hreadthread= INVALID_HANDLE_VALUE;
}

void BlFilePopenWindows::closeout ()
{
	TRACEFUNC (tr, "BlFilePopenWindows::closeout");

	if (! getmode () & Output)
		throw ErrFileMode;
	CloseHandle (hwritepipe);
	hwritepipe= INVALID_HANDLE_VALUE;
}

bool BlFilePopenWindows::eof ()
{
	TRACEFUNC (tr, "BlFilePopenWindows::eof");

	if (! getmode () & Input)
		throw ErrFileMode;

	{
		CriticalLock lock (critical);
		if (bufpos < bufread)
			return false;
	}

	readbuffer ();
	hasread.wait ();

	if (error_reading)
		throw ErrOperatingSystem;
	if (!eof_found)
	{
		ASSERT (bufpos < bufread);
	}
	return eof_found;
}

void BlFilePopenWindows::endline ()
{
	outstring ("\r\n");
}

std::string BlFilePopenWindows::read (size_t n)
{
	if (! getmode () & Input)
		throw ErrFileMode;

	std::string str;
	for (size_t i= 0; i < n; ++i)
	{
		char c= getcharfrombuffer ();
		if (c == 0 && eof_found)
			break;
		str+= c;
	}
	return str;
}

void BlFilePopenWindows::getline (std::string & str, bool)
{
	TRACEFUNC (tr, "BlFilePopenWindows::getline");

	if (! getmode () & Input)
		throw ErrFileMode;

	str= std::string ();
	char c;
	while ( (c= getcharfrombuffer () ) != '\r' && c != '\n')
	{
		if (c == 0)
		{
			if (eof_found)
			{
				if (str.empty () )
					throw ErrPastEof;
				else
					break;
			}
		}
		else
			str+= c;
	}

	if (c == '\r')
		getcharfrombuffer ();
}

void BlFilePopenWindows::readbuffer ()
{
	if (finishthread | error_reading | eof_found)
		return;
	CriticalLock lock (critical);
	if (nowreading)
		return;
	nowreading= true;
	hasread.reset ();
	canread.set ();
}

void BlFilePopenWindows::read_in_thread ()
{
	do
	{
		canread.wait ();
		if (finishthread)
			break;
		if (hreadpipe == INVALID_HANDLE_VALUE)
			break;
		DWORD bytesread= 0;
		if (ReadFile (hreadpipe, buffer, bufsize, & bytesread, NULL) == 0)
		{
			DWORD r= GetLastError ();
			if (r != ERROR_BROKEN_PIPE)
			{
				error_reading= true;
			}
			else
			{
				eof_found= true;
			}

			CriticalLock lock (critical);
			nowreading= false;
		}
		else
		{
			CriticalLock lock (critical);
			if (bytesread == 0)
			{
				// This is not supposed to happen
				// in an anonymous pipe.
				error_reading= true;
			}
			bufpos= 0;
			bufread= bytesread;
			nowreading= false;
		}
		hasread.set ();
	} while (! error_reading & ! eof_found & ! finishthread);
}

void BlFilePopenWindows::outstring (const std::string & str)
{
	TRACEFUNC (tr, "BlFilePopenWindows::outstring");

	if (! getmode () & Output)
		throw ErrFileMode;

	const char * to= str.data ();
	std::string::size_type l= str.size ();
	DWORD written;
	//WriteFile (hpipe, to, l, & written, NULL);
	if (WriteFile (hwritepipe, to, l, & written, NULL) == 0)
	{
		showlasterror ();
		throw ErrOperatingSystem;
	}
}

void BlFilePopenWindows::outchar (char c)
{
	if (! getmode () & Output)
		throw ErrFileMode;

	DWORD written;
	//WriteFile (hpipe, & c, 1, & written, NULL);
	if (WriteFile (hwritepipe, & c, 1, & written, NULL) == 0)
	{
		showlasterror ();
		throw ErrOperatingSystem;
	}
}

bool BlFilePopenWindows::poll ()
{
	if (! getmode () & Input)
		throw ErrFileMode;

	{
		CriticalLock lock (critical);
		if (bufpos < bufread)
			return true;
	}

	readbuffer ();

	return false;
}

#else
// No Windows

//***********************************************
// Auxiliary functions and classes for unix
//***********************************************

namespace {

class ProtectHandle {
public:
	ProtectHandle (int handle) :
		handle (handle)
	{ }
	~ProtectHandle ()
	{
		close ();
	}
	void close ()
	{
		if (handle != -1)
		{
			::close (handle);
			handle= -1;
		}
	}
	void release ()
	{
		handle= -1;
	}
private:
	int handle;
};

void exec_command (const std::string & str)
{
	const char * strShell= getenv ("SHELL");
	if (! strShell)
		strShell= "/bin/sh";

	execlp (strShell, strShell, "-c",
		str.c_str (), (char *) 0);

	// If something fails:
	exit (127);
}

void createpipe (int & hread, int & hwrite)
{
	int handlepair [2];
	if (pipe (handlepair) != 0)
	{
		showlasterror ("Creating pipe");
		throw ErrOperatingSystem;
	}
	hread= handlepair [0];
	hwrite= handlepair [1];
}

void duporfail (int oldfd, int newfd)
{
	if (dup2 (oldfd, newfd) == -1)
		exit (127);
}

void stdtonull ()
{
	int newstd= open ("/dev/null", O_RDWR);
	if (newstd == -1)
		exit (127);
	duporfail (newstd, STDIN_FILENO);
	duporfail (newstd, STDOUT_FILENO);
	duporfail (newstd, STDERR_FILENO);
	close (newstd);
}

void launchchild (const std::string & str,
	bool forread, bool forwrite, bool witherr,
	int & readhandle, int & writehandle, pid_t & childpid)
{
	TRACEFUNC (tr, "launchchild");

	if (! forread && ! forwrite)
	{
		ASSERT (false);
		throw ErrBlassicInternal;
	}

	int writechild= -1;
	if (forread)
		createpipe (readhandle, writechild);
	ProtectHandle prrhandle (readhandle);
	ProtectHandle prwchild (writechild);

	int readchild= -1;
	if (forwrite)
		createpipe (readchild, writehandle);
	ProtectHandle prwhandle (writehandle);
	ProtectHandle prrchild (readchild);

	childpid= fork ();
	switch (childpid)
	{
	case pid_t (-1):
		// Fork has failed.
		showlasterror ();
		throw ErrOperatingSystem;
	case pid_t (0):
		// Child process.
		// No need to take care of handles in case of fail
		// here, the process will be terminated.
		stdtonull ();
		if (forread)
		{
			prrhandle.close ();
			duporfail (writechild, STDOUT_FILENO);
			if (witherr)
				duporfail (writechild, STDERR_FILENO);
			prwchild.close ();
		}
		if (forwrite)
		{
			prwhandle.close ();
			duporfail (readchild, STDIN_FILENO);
			prrchild.close ();
		}
		exec_command (str);
		break; // To avoid warnings.
	default:
		// Parent process.
		if (forread)
		{
			prwchild.close ();
			prrhandle.release ();
		}
		if (forwrite)
		{
			prrchild.close ();
			prwhandle.release ();
		}
	}
}

} // namespace

//***********************************************
//              BlFilePopenUnix
//***********************************************

class BlFilePopenUnix : public BlFilePopen {
public:
	BlFilePopenUnix (const std::string & str, OpenMode mode);
	~BlFilePopenUnix ();
	void closein ();
	void closeout ();
	bool eof ();
	void endline ();
	std::string read (size_t n);
	void getline (std::string & str, bool endline= true);
	bool poll ();
private:
	static const size_t bufsize= 4096;
	char buffer [bufsize];
	size_t bufpos, bufread;
	void readbuffer ();
	char getcharfrombuffer ();
	size_t do_readbuffer (char * buffer, size_t bytes);

	void closeread ();
	void closewrite ();
	void outstring (const std::string & str);
	void outchar (char c);

	int readhandle;
	int writehandle;
	pid_t childpid;
};

BlFilePopenUnix::BlFilePopenUnix
		(const std::string & str, OpenMode nmode) :
	BlFilePopen (nmode)
{
	TRACEFUNC (tr, "BlFilePopenUnix::BlFilePopenUnix");
	TRMESSAGE (tr, str);

	readhandle= -1;
	writehandle= -1;
	childpid= -1;

	launchchild (str, forread, forwrite, witherr,
		readhandle, writehandle, childpid);
}

BlFilePopenUnix::~BlFilePopenUnix ()
{
	TRACEFUNC (tr, "BlFilePopenUnix::~BlFilePopenUnix");

	// First close the pipes, the child can be waiting for input.
	closeread ();
	closewrite ();

	int status;
	if (waitpid (childpid, & status, 0) == -1 || ! WIFEXITED (status) )
	{
		showlasterror ();
		throw ErrOperatingSystem;
	}
	TRMESSAGE (tr, "Exit code: " + to_string (WEXITSTATUS (status) ) );
	sysvar::set (sysvar::ShellResult, WEXITSTATUS (status) );
}

char BlFilePopenUnix::getcharfrombuffer ()
{
	if (bufpos >= bufread)
	{
		readbuffer ();
		if (bufread == 0)
			return '\0';
	}
	return buffer [bufpos++];
}

void BlFilePopenUnix::closein ()
{
	TRACEFUNC (tr, "BlFilePopenUnix::closein");

	if (! getmode () & Input)
		throw ErrFileMode;

	closeread ();
}

void BlFilePopenUnix::closeout ()
{
	TRACEFUNC (tr, "BlFilePopenUnix::closeout");

	if (! getmode () & Output)
		throw ErrFileMode;

	closewrite ();
}

bool BlFilePopenUnix::eof ()
{
	TRACEFUNC (tr, "BlFilePopenUnix::eof");

	if (! getmode () & Input)
		throw ErrFileMode;

	if (bufpos < bufread)
		return false;
	readbuffer ();
	return bufread == 0;
}

void BlFilePopenUnix::endline ()
{
	outchar ('\n');
}

std::string BlFilePopenUnix::read (size_t n)
{
	if (! getmode () & Input)
		throw ErrFileMode;

	std::string str;
	for (size_t i= 0; i < n; ++i)
	{
		char c= getcharfrombuffer ();
		if (c == '\0')
		{
			if (eof () )
				break;
			else
				str+= c;
		}
		else
			str+= c;
	}
	return str;
}

void BlFilePopenUnix::getline (std::string & str, bool)
{
	TRACEFUNC (tr, "BlFilePopenUnix::getline");

	if (! getmode () & Input)
		throw ErrFileMode;

	str= std::string ();
	char c;
	while ( (c= getcharfrombuffer () ) != '\r' && c != '\n')
	{
		if (c == '\0')
		{
			if (eof () )
			{
				if (str.empty () )
					throw ErrPastEof;
				else
					break;
			}
			else
				break;
		}
		else
			str+= c;
	}
}

void BlFilePopenUnix::closeread ()
{
	if (readhandle != -1)
	{
		close (readhandle);
		readhandle= -1;
	}
}

void BlFilePopenUnix::closewrite ()
{
	if (writehandle != -1)
	{
		close (writehandle);
		writehandle= -1;
	}
}

void BlFilePopenUnix::readbuffer ()
{
	bufpos= 0;
	bufread= do_readbuffer (buffer, bufsize);
}

size_t BlFilePopenUnix::do_readbuffer (char * buffer, size_t bytes)
{
	TRACEFUNC (tr, "BlFilePopenUnix::do_readbuffer");

	size_t bytesread= ::read (readhandle, buffer, bytes);
	if (bytesread == size_t (-1) )
		bytesread= 0;
	return bytesread;
}

void BlFilePopenUnix::outstring (const std::string & str)
{
	TRACEFUNC (tr, "BlFilePopenUnix::outstring");

	if (! getmode () & Output)
		throw ErrFileMode;

	const char * to= str.data ();
	std::string::size_type l= str.size ();
	if (write (writehandle, to, l) == -1)
	{
		showlasterror ();
		throw ErrOperatingSystem;
	}
}

void BlFilePopenUnix::outchar (char c)
{
	if (! getmode () & Output)
		throw ErrFileMode;

	if (write (writehandle, & c, 1) == -1)
	{
		showlasterror ();
		throw ErrOperatingSystem;
	}
}

bool BlFilePopenUnix::poll ()
{
	TRACEFUNC (tr, "BlFilePopenUnix::poll");

	if (! getmode () & Input)
		throw ErrFileMode;
	struct pollfd data [1];
	data [0].fd= readhandle;
	data [0].events= POLLIN | POLLPRI;
	switch (::poll (data, 1, 0) )
	{
	case 0:
		return false;
	case 1:
		return true;
	default:
		showlasterror ();
		throw ErrOperatingSystem;
	}
}

#endif
// Windows or unix

//***********************************************
//		newBlFilePopen
//***********************************************

BlFile * newBlFilePopen (const std::string & name, OpenMode mode)
{
	#ifdef BLASSIC_USE_WINDOWS

	return new BlFilePopenWindows (name, mode);

	#else

	return new BlFilePopenUnix (name, mode);

	#endif
}

} // namespace file

} // namespace blassic

// End of filepopen.cpp
