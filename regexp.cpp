// regexp.cpp
// Revision 9-jan-2005

#include "regexp.h"

#include "blassic.h"
#include "error.h"
#include "trace.h"
#include "runner.h"
#include "runnerline.h"

#include "util.h"
using util::touch;

#ifndef BLASSIC_USE_WINDOWS

#include <vector>

#include <regex.h>

#endif

//***********************************************
//		Regexp::Internal
//***********************************************


class Regexp::Internal {
public:
	typedef Regexp::string string;
	typedef Regexp::size_type size_type;

	Internal (const string & exp, flag_t flags);
	~Internal ();
	size_type find (const string & searched, size_type init);
	string replace (const string & searched, size_type init,
		const string & replaceby);
	string replace (const string & searched, size_type init,
		RunnerLine & runnerline, const string & fname);
private:
	flag_t flags;
	#ifndef BLASSIC_USE_WINDOWS
	regex_t reg;
	#endif
};

Regexp::Internal::Internal (const string & exp, flag_t flags) :
	flags (flags)
{
	#ifdef BLASSIC_USE_WINDOWS

	touch (exp, flags);
	throw ErrNotImplemented;

	#else
	int cflags= REG_EXTENDED;
	if (flags & FLAG_NOCASE)
		cflags|= REG_ICASE;
	if (flags & FLAG_NEWLINE)
		cflags|= REG_NEWLINE;
	if (regcomp (& reg, exp.c_str (), cflags) != 0)
		throw ErrRegexp;

	#endif
}

Regexp::Internal::~Internal ()
{
	#ifndef BLASSIC_USE_WINDOWS

	regfree (& reg);

	#endif
}

Regexp::size_type Regexp::Internal::find (const string & searched,
	size_type init)
{
	#ifdef BLASSIC_USE_WINDOWS

	touch (searched, init);
	throw ErrNotImplemented;

	#else

	regmatch_t match;
	int eflags= 0;
	if (flags & FLAG_NOBEG)
		eflags|= REG_NOTBOL;
	if (flags & FLAG_NOEND)
		eflags|= REG_NOTEOL;
	int r= regexec (& reg, searched.c_str () + init, 1, & match, eflags);
	switch (r)
	{
	case 0:
		return match.rm_so + init;
	case REG_NOMATCH:
		return std::string::npos;
	default:
		throw ErrRegexp;
	}

	return std::string::npos;

	#endif
}

Regexp::string Regexp::Internal::replace
	(const string & searched, size_type init, const string & replaceby)
{
	#ifdef BLASSIC_USE_WINDOWS

	touch (searched, init, replaceby);
	throw ErrNotImplemented;

	#else

	TRACEFUNC (tr, "Regexp::Internal::replace");

	const size_t nmatch= reg.re_nsub + 1;
	std::vector <regmatch_t> vmatch (nmatch);
	int eflags= 0;
	if (flags & FLAG_NOBEG)
		eflags|= REG_NOTBOL;
	if (flags & FLAG_NOEND)
		eflags|= REG_NOTEOL;
	int r= regexec (& reg, searched.c_str () + init,
		nmatch, & vmatch [0], eflags);
	switch (r)
	{
	case 0:
		break;
	case REG_NOMATCH:
		return searched;
	default:
		throw ErrRegexp;
	}

	#if 0
	TRMESSAGE (tr, "Coincidence list");
	for (size_t i= 0; i < nmatch; ++i)
	{
		regoff_t ini= vmatch [i].rm_so;
		if (ini == regoff_t (-1) )
			TRMESSAGE (tr, "(empty)");
		else
		{
			regoff_t len= vmatch [i].rm_eo - ini;
			TRMESSAGE (tr, searched.substr (init + ini, len) );
		}
	}
	TRMESSAGE (tr, "End of coincidence list");
	#endif

	string replace= replaceby;
	size_type pos= replace.find ('$');
	while (pos != std::string::npos)
	{
		//TRMESSAGE (tr, replace.substr (pos) );

		if (pos == replace.size () - 1)
			break;
		char c= replace [pos + 1];
		if (c == '$')
		{
			replace.erase (pos, 1);
			++pos;
		}
		else
		{
			if (isdigit (c) )
			{
				size_t n= c - '0';
				if (n >= nmatch)
					replace.erase (pos, 2);
				else
				{
					regoff_t ini= vmatch [n].rm_so;
					if (ini == regoff_t (-1) )
					{
						replace.erase (pos, 2);
					}
					else
					{
						regoff_t l= vmatch [n].rm_eo
							- ini;
						replace.replace (pos, 2,
							searched, init + ini,
							l);
						pos-= 2;
						pos+= l;
					}
				}
			}
			else
				pos+= 2;
		}
		pos= replace.find ('$', pos);
	}
	return searched.substr (0, vmatch [0].rm_so + init) +
		replace +
		searched.substr (vmatch [0].rm_eo + init);

	#endif
}

Regexp::string Regexp::Internal::replace
	(const string & searched, size_type init,
	RunnerLine & runnerline, const string & fname)
{
	#ifdef BLASSIC_USE_WINDOWS

	touch (searched, init, runnerline, fname);
	throw ErrNotImplemented;

	#else

	TRACEFUNC (tr, "Regexp::Internal::replace");

	const size_t nmatch= reg.re_nsub + 1;
	std::vector <regmatch_t> vmatch (nmatch);
	int eflags= 0;
	if (flags & FLAG_NOBEG)
		eflags|= REG_NOTBOL;
	if (flags & FLAG_NOEND)
		eflags|= REG_NOTEOL;
	int r= regexec (& reg, searched.c_str () + init,
		nmatch, & vmatch [0], eflags);
	switch (r)
	{
	case 0:
		break;
	case REG_NOMATCH:
		return searched;
	default:
		throw ErrRegexp;
	}

	Function f= Function::get (fname);
	LocalLevel ll;
	const ParameterList & param= f.getparam ();
	const size_t l= param.size ();
	for (size_t i= 0; i < l; ++i)
	{
		std::string var= param [i];
		TRMESSAGE (tr, "paramter: " + var);

		std::string value;
		if (i < nmatch)
		{
			regoff_t ini= vmatch [i].rm_so;
			if (ini != regoff_t (-1) )
			{
				regoff_t l= vmatch [i].rm_eo - ini;
				value= std::string (searched, ini + init, l);
			}
		}
		assignvarstring (var, value);
	}
	blassic::result::BlResult result;
	runnerline.callfn (f, fname, ll, result);

	return searched.substr (0, vmatch [0].rm_so + init) +
		result.str () +
		searched.substr (vmatch [0].rm_eo + init);

	#endif
}

//***********************************************
//			Regexp
//***********************************************


Regexp::Regexp (const string & exp, flag_t flags) :
	pin (new Internal (exp, flags) )
{
}

Regexp::~Regexp ()
{
	delete pin;
}

Regexp::size_type Regexp::find (const string & searched, size_type init)
{
	return pin->find (searched, init);
}

Regexp::string Regexp::replace
	(const string & searched, size_type init, const string & replaceby)
{
	return pin->replace (searched, init, replaceby);
}

Regexp::string Regexp::replace (const string & searched, size_type init,
	RunnerLine & runnerline, const string & fname)
{
	return pin->replace (searched, init, runnerline, fname);
}

// End of regexp.cpp
