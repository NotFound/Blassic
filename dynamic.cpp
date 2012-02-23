// dynamic.cpp
// Revision 1-jan-2005

#include "dynamic.h"
#include "error.h"
#include "showerror.h"

// For debugging.
#include <errno.h>
#include <iostream>
using std::cerr;
using std::endl;


#if (defined __unix__ || defined __linux__ || defined __NetBSD__) &&  \
	! defined __CYGWIN__
// Kylix defines only __linux__


#ifdef __hpux__


#include <dl.h>

class DynamicHandle::Internal {
	shl_t handle;
public:
	Internal (const std::string & name);
	~Internal ();
	DynamicUsrFunc addr (const std::string & str);
};

DynamicHandle::Internal::Internal (const std::string & name)
{
	handle= shl_load (name.c_str (), BIND_DEFERRED, 0);
	if (handle == NULL)
		throw ErrNoDynamicLibrary;
}

DynamicHandle::Internal::~Internal ()
{
	// HP-UX seems to unload the library even if it was
	// opened several times, then we don't unload it.
	//shl_unload (handle);
}

DynamicUsrFunc DynamicHandle::Internal::addr (const std::string & str)
{
	void * value;
	if (shl_findsym (& handle, str.c_str (), TYPE_UNDEFINED,
			& value) == 0)
	{
		//return reinterpret_cast <DynamicUsrFunc> (value);
		return (DynamicUsrFunc) (value);
	}
	else
		throw ErrNoDynamicSymbol;
}

#else
// Unix no hp-ux


#include <dlfcn.h>

class DynamicHandle::Internal {
	void * handle;
public:
	Internal (const std::string & name);
	~Internal ();
	DynamicUsrFunc addr (const std::string & str);
};

DynamicHandle::Internal::Internal (const std::string & name)
{
	handle= dlopen (name.c_str (), RTLD_LAZY);
	if (handle == NULL)
	{
		if (showdebuginfo () )
		{
			cerr << "Error loading " << name << ": " <<
				dlerror () << endl;
		}
		throw ErrNoDynamicLibrary;
	}
}

DynamicHandle::Internal::~Internal ()
{
	dlclose (handle);
}

DynamicUsrFunc DynamicHandle::Internal::addr (const std::string & str)
{
	void * value;
	if ( (value= dlsym (handle, str.c_str () ) ) != NULL)
	{
		//return reinterpret_cast <DynamicUsrFunc> (value);
		return (DynamicUsrFunc) value;
	}
	else
		throw ErrNoDynamicSymbol;
}

#endif


#elif defined BLASSIC_USE_WINDOWS


#include <windows.h>
#undef min
#undef max

namespace {

void * GetAddress (HMODULE handle, const std::string & str)
{
	FARPROC result= GetProcAddress (handle, str.c_str () );
	if (result == NULL)
	{
		std::string str_ (1, '_');
		str_+= str;
		result= GetProcAddress (handle, str_.c_str () );
	}
	if (result == NULL)
	{
	        showlasterror ();
		throw ErrNoDynamicSymbol;
	}
	return (void *) (result);
}

} // namespace

class DynamicHandle::Internal {
	HMODULE handle;
public:
	Internal (const std::string & name);
	~Internal ();
	DynamicUsrFunc addr (const std::string & str);
};

DynamicHandle::Internal::Internal (const std::string & name)
{
	handle= LoadLibrary (name.c_str () );
	if (handle == NULL)
	{
		showlasterror ();
		throw ErrNoDynamicLibrary;
	}
}

DynamicHandle::Internal::~Internal ()
{
	FreeLibrary (handle);
}

DynamicUsrFunc DynamicHandle::Internal::addr (const std::string & str)
{
	return (DynamicUsrFunc) GetAddress (handle, str);
}

#else


#warning Dynamic link unsupported

class DynamicHandle::Internal {
public:
	Internal (const std::string & name);
	~Internal ();
	DynamicUsrFunc addr (const std::string & str);
};

DynamicHandle::Internal::Internal (const std::string & name)
{
	throw ErrDynamicUnsupported;
}

DynamicHandle::Internal::~Internal ()
{
}

DynamicUsrFunc DynamicHandle::Internal::addr (const std::string & str)
{
	throw ErrDynamicUnsupported;
}

#endif


DynamicHandle::DynamicHandle () :
	pin (0)
{
}

DynamicHandle::DynamicHandle (const std::string & name) :
	pin (new Internal (name) )
{
}

void DynamicHandle::assign (const std::string & name)
{
	// Create the new before destructing the old, thus leaving
	// intouched in case of exception.
	Internal * npin= new Internal (name);
	delete pin;
	pin= npin;
}

DynamicHandle::~DynamicHandle ()
{
	delete pin;
}

DynamicUsrFunc DynamicHandle::addr (const std::string & str)
{
	return pin->addr (str);
}

// Fin de dymanic.cpp
