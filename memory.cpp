// memory.cpp
// Revision 24-apr-2009

#include "memory.h"

#include "blassic.h"
#include "error.h"
#include "trace.h"

//#ifdef HAVE_SYS_MMAN_H
#ifdef HAVE_MMAP

#include <sys/mman.h>

#if ! defined MAP_ANON

#if defined MAP_ANONYMOUS

#define MAP_ANON MAP_ANONYMOUS

#else

#error "Don't know how to flag anonymous map."

#endif

#endif

#endif
// HAVE_SYS_MMAN_H

#include <iostream>
using std::cerr;
using std::endl;

#include <string.h>
#include <errno.h>

//#include <set>
#include <map>

namespace {

//typedef std::set <void *> memused_t;
typedef std::map <void *, size_t> memused_t;

memused_t memused;

#ifdef HAVE_SYS_MMAN_H

void do_munmap (void * start, size_t length)
{
	TRACEFUNC (tr, "do_munmap");

	int r= munmap (start, length);
	if (r != 0)
	{
		const char * errormessage= strerror (errno);
		TRMESSAGE (tr, std::string ("munmap failed: ") + errormessage);
		if (showdebuginfo () )
			cerr << "munmap (" << start << ", " <<
				length << ") failed: " << errormessage <<
				endl;
	}
}

#endif

} // namespace

size_t blassic::memory::dyn_alloc (size_t memsize)
{
	TRACEFUNC (tr, "memory::alloc");

	if (memsize == 0)
		throw ErrFunctionCall;

	#ifndef HAVE_SYS_MMAN_H

	void * aux= malloc (memsize);

	#else

	void * aux= mmap (NULL, memsize, PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANON, -1, 0);
	if (aux == (void *) -1)
		aux= NULL;

	#endif

	if (aux == NULL)
	{
		if (showdebuginfo () )
			cerr << "Failed allocation of " << memsize <<
				" bytes: " << strerror (errno) << endl;
		throw ErrOutMemory;
	}

	//memused.insert (aux);
	memused [aux]= memsize;
	return reinterpret_cast <size_t> (aux);
}

void blassic::memory::dyn_free (size_t mempos)
{
	TRACEFUNC (tr, "memory::free");

	void * aux= reinterpret_cast <void *> (mempos);
	memused_t::iterator it= memused.find (aux);
	if (it == memused.end () )
	{
		if (showdebuginfo () )
			cerr << "Trying to free address " << mempos <<
				" but is not allocated" << endl;
		throw ErrFunctionCall;
	}

	#ifdef HAVE_SYS_MMAN_H

	size_t memsize= it->second;

	#endif

	memused.erase (it);

	#ifndef HAVE_SYS_MMAN_H

	::free (aux);

	#else

	//munmap (aux, memsize);
	do_munmap (aux, memsize);

	#endif
}

void blassic::memory::dyn_freeall ()
{
	TRACEFUNC (tr, "memory::freeall");

	bool something= false;
	while (! memused.empty () )
	{
		something= true;
		memused_t::iterator it= memused.begin ();

		#ifndef HAVE_SYS_MMAN_H

		::free (it->first);

		#else

		//munmap (it->first, it->second);
		do_munmap (it->first, it->second);

		#endif

		memused.erase (it);
	}

	if (! something)
		TRMESSAGE (tr, "Nothing to free");
}

// End of memory.cpp
