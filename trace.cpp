// trace.cpp
// Revision 7-feb-2005

#include "trace.h"


#if __BORLANDC__ >= 0x0560
#pragma warn -8091
#endif

#include <iostream>
#include <fstream>
#include <string>

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <stdexcept>

#include <string.h>

using std::cerr;
using std::endl;
using std::ostream;
using std::ofstream;
using std::string;


namespace {

ostream * pout= 0;
bool flag= true;
size_t indent= 0;

TraceFunc * initial= NULL;
TraceFunc * * lastpos= & initial;

ostream * opentracefile (const char * str)
{
	std::ofstream * pof=
		new ofstream (str, std::ios::app | std::ios::out);
	if (! pof->is_open () )
	{
		cerr << "Error opening " << str << endl;
		delete pof;
		pof= 0;
	}
	return pof;
}

void showinfo (const char * strenterexit, const char * strfunc)
{
	if (pout)
	{
		* pout << string (indent, ' ') << strenterexit << ' ';

		if (std::uncaught_exception () )
			* pout << "(throwing) ";

		* pout << strfunc << endl;
	}
}

const char BAD_USE []= "Bad use of TraceFunc";

} // namespace

TraceFunc::TraceFunc (const char * strFuncName) :
	strfunc (strFuncName),
	next (NULL)
{
	if (flag)
	{
		flag= false;
		char * aux= getenv ("TRACEFUNC");
		if (aux)
		{
			if (strcmp (aux, "-") == 0)
				pout= & std::cerr;
			else
				pout= opentracefile (aux);
		}
	}

	//tracelist.push_back (this);
	previous= lastpos;
	* lastpos= this;
	lastpos= & next;

	#if 0
	if (pout)
	{
		* pout << string (indent, ' ') << "Enter ";

		if (std::uncaught_exception () )
			* pout << "(throwing) ";

		* pout << strfunc << endl;
	}
	#else
	showinfo ("Enter", strfunc);
	#endif
	++indent;
}

TraceFunc::~TraceFunc ()
{
	--indent;

	#if 0
	if (pout)
	{
		* pout << string (indent, ' ') << "Exit ";

		if (std::uncaught_exception () )
			* pout << "(throwing) ";

		* pout << strfunc << endl;
	}
	#else
	showinfo ("Exit", strfunc);
	#endif

	if (next != NULL)
	{
		cerr << BAD_USE << endl;
		abort ();
	}
	if (lastpos != & next)
	{
		//throw std::logic_error ("Bad use of TraceFunc");
		cerr << BAD_USE << endl;
		abort ();
	}
	lastpos= previous;
	if (* lastpos != this)
	{
		cerr << BAD_USE << endl;
		abort ();
	}
	* lastpos= NULL;
}

void TraceFunc::message (const std::string & strMes)
{
	if (pout)
	{
		* pout << string (indent, ' ') << strfunc;

		if (std::uncaught_exception () )
			* pout << "(throwing) ";

		* pout << ": " << strMes << endl;
	}
}

void TraceFunc::show (int)
{
	cerr << "\r\n";

	if (initial == NULL)
		cerr << "TraceFunc: no calls.";
	else
	{
		cerr << "TraceFunc dump of calls: \r\n";
		for (TraceFunc * act= initial; act != NULL; act= act->next)
			cerr << act->strfunc << "\r\n";
		cerr << "TraceFunc dump ended.";
	}
	cerr << "\r\n";
}

// Fin de trace.cpp
