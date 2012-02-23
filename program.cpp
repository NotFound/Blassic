// program.cpp
// Revision 24-apr-2009

#include "program.h"

#include "keyword.h"
#include "error.h"
#include "sysvar.h"
#include "util.h"
using util::to_string;
#include "trace.h"

#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <sstream>

using std::string;
// For debugging.
using std::cerr;
using std::endl;
using std::flush;

#include <cassert>
#define ASSERT assert

#include <string.h>

#ifndef USE_HASH_MAP

#include <map>
#define MAP std::map

#else

#if __GNUC__ < 3
#include <hash_map>
#define N_MAP std
#else
#include <ext/hash_map>
#define N_MAP __gnu_cxx
#endif

#define MAP N_MAP::hash_map

namespace N_MAP {

template <> struct hash <std::string>
{
	hash () : hashstr (hash <const char *> () ) { }
	size_t operator () (const std::string & str) const
	{ return hashstr (str.c_str () ); }
private:
	hash <const char *> hashstr;
};

} // namespace N_MAP

#endif

#ifdef __BORLANDC__
#pragma warn -inl
#endif

namespace sysvar= blassic::sysvar;
using namespace blassic::file;

//**********************************************************
//			Program
//**********************************************************

Program::Program ()
{
}

Program::~Program ()
{
}

//**********************************************************
//			Auxiliar
//**********************************************************

namespace {

typedef size_t Position;

inline BlLineNumber getLineNumber (const BlChar * p)
{
	return peek32 (p);
}

inline BlLineNumber getLineNumberAt (const BlChar * p)
{
	return peek32 (p);
}

inline void setLineNumber (BlChar * p, BlLineNumber n)
{
	poke32 (p, n);
}

inline BlLineLength getLineLength (BlChar * p)
{
	return peek32 (p);
}

inline BlLineLength getLineLengthAt (BlChar * p)
{
	return peek32 (p + sizeof (BlLineNumber) );
}

inline void setLineLength (BlChar * p, BlLineLength n)
{
	poke32 (p, n);
}

inline BlChar * getLineContentAt (BlChar * p)
{
	return p + sizeof (BlLineNumber) + sizeof (BlLineLength);
}

inline BlChar * getNextLineAt (BlChar * p)
{
	return p + getLineLengthAt (p) +
		sizeof (BlLineNumber) + sizeof (BlLineLength);
}

inline void getLineAt (BlChar * p, CodeLine & codeline)
{
	codeline.assign (getLineContentAt (p),
		getLineNumberAt (p),
		getLineLengthAt (p) );
}

// Used in renum.
typedef MAP <BlLineNumber, BlLineNumber> MapLine;

} // namespace

//**********************************************************
//			ProgramImpl
//**********************************************************

//#define OLD_LABEL_CACHE

//#define CACHE_ALL_LINES

class ProgramImpl : public Program {
public:
	ProgramImpl ();
	~ProgramImpl ();
	BlChar * programptr () { return program; }

	BlLineNumber getlabel (const std::string & str);
	CodeLine getfirstline ();
	void getnextline (CodeLine & codeline);
	void getline (BlLineNumber num, CodeLine & codeline);
	void getline (ProgramPos pos, CodeLine & codeline);
	void do_insert (const CodeLine & codeline);
	void insert (const CodeLine & codeline);
	void deletelines (BlLineNumber iniline, BlLineNumber endline);
	void listline (const CodeLine & codeline, BlFile & out) const;
	void list (BlLineNumber iniline, BlLineNumber endline,
		BlFile & out) const;
	void save (const std::string & name) const;
	void load (const std::string & name);
	void load (std::istream & is);
	void merge (const std::string & name,
		BlLineNumber inidel, BlLineNumber enddel);
	void renew ();
	void renum (BlLineNumber blnNew, BlLineNumber blnOld,
		BlLineNumber blnInc, BlLineNumber blnStop);
private:
	BlChar * program;
	typedef size_t ProgSize;
	ProgSize size;

	static const size_t BLOCK= 16 * 1024;
	static size_t blockrounded (size_t size);
	void resize (ProgSize newsize);
	void transfer_content (ProgramImpl & other);

	//typedef MAP <BlLineNumber, Position> linecache_t;
	typedef MAP <BlLineNumber, BlChar *> linecache_t;
	linecache_t linecache;

	typedef MAP <string, BlLineNumber> labelcache_t;
	labelcache_t labelcache;

public:
	// Public to allow use for LabelCacheGuard.
	void clear_label_cache ();
private:
	void clear_cache ();

	#ifndef NDEBUG
	size_t linecache_hits;
	size_t linecache_fails;
	size_t labelcache_hits;
	size_t labelcache_fails;
	#endif

	#ifndef OLD_LABEL_CACHE
	bool labelcache_inited;
	void generate_label_cache ();
	#endif

	#ifdef CACHE_ALL_LINES
	bool line_cache_inited;
	void generate_line_cache ();
	#endif

	void changeline (Position pos, const MapLine & mapline);
	void setlabel (const std::string & label, CodeLine & codeline);
	void getlineinpos (Position pos, CodeLine & codeline) const;
	CodeLine getlineinpos (Position pos) const;
	Position nextline (Position pos) const;
	BlLineLength sizeline (Position) const;
	BlLineNumber numline (Position pos) const;
	const BlChar * linecontent (Position pos) const;
	BlChar * linecontent (Position pos);
	void loadtext (std::istream & is);
	void loadbinary (std::istream & is);
};

ProgramImpl::ProgramImpl () :
	program (0),
	size (0)

	#ifndef NDEBUG
		,
	linecache_hits (0),
	linecache_fails (0),
	labelcache_hits (0),
	labelcache_fails (0)
	#endif

	#ifndef OLD_LABEL_CACHE
		,
	labelcache_inited (false)
	#endif

	#ifdef CACHE_ALL_LINES
		,
	line_cache_inited (false)
	#endif
{
	TRACEFUNC (tr, "ProgramImpl::ProgramImpl");

	resize (0);
}

ProgramImpl::~ProgramImpl ()
{
	TRACEFUNC (tr, "ProgramImpl::~ProgramImpl");

	if (program)
		free (program);

	#ifndef NDEBUG
	std::ostringstream oss;
	oss << "Line cache: hits " << linecache_hits <<
		", fails " << linecache_fails <<
		" Label cache: hits " << labelcache_hits <<
		", fails " << labelcache_fails;
	TRMESSAGE (tr, oss.str () );
	#endif
}

Program * newProgram ()
{
	return new ProgramImpl;
}

namespace {

class ProgramTail {
	BlLineNumber n;
	BlLineLength l;
	ProgramTail ()
	{
		setLineNumber (reinterpret_cast <BlChar *> (& n),
			LineEndProgram);
		setLineLength (reinterpret_cast <BlChar *> (& l), 0);
	}
public:
	static const ProgramTail tail;
};

const ProgramTail ProgramTail::tail;

} // namespace

inline size_t ProgramImpl::blockrounded (size_t size)
{
	return ( (size + sizeof (ProgramTail) + BLOCK - 1) / BLOCK) * BLOCK;
}

void ProgramImpl::resize (ProgSize newsize)
{
	// No need to clear cache here, the callers
	// must do it.

	ProgSize newblock= blockrounded (newsize);
	if (newblock != blockrounded (size) || ! program)
	{
		BlChar * newprog=
			(BlChar *) realloc (program, newblock);
		if (! newprog)
			throw ErrOutMemory;
		program= newprog;
	}
	size= newsize;
	memcpy (program + size, & ProgramTail::tail, sizeof (ProgramTail) );
}

void ProgramImpl::transfer_content (ProgramImpl & other)
{
	// No need to clear cache here, the callers
	// must do it.

	if (this == & other)
	{
		ASSERT (0);
		throw ErrBlassicInternal;
	}
	program= other.program;
	size= other.size;
	other.program= NULL;
	other.size= 0;
}

void ProgramImpl::renew ()
{
	TRACEFUNC (tr, "ProgramImpl::renew");

	clear_cache ();
	resize (0);
}

inline BlLineLength ProgramImpl::sizeline (Position pos) const
{
	BlChar * aux= program + pos + sizeof (BlLineNumber);
	return getLineLength (aux);
}

inline Position ProgramImpl::nextline (Position pos) const
{
	return pos + sizeline (pos) +
		sizeof (BlLineNumber) + sizeof (BlLineLength);
}

inline BlLineNumber ProgramImpl::numline (Position pos) const
{
	return getLineNumber (program + pos);
}

inline const BlChar * ProgramImpl::linecontent (Position pos) const
{
	return program + pos + sizeof (BlLineNumber) + sizeof (BlLineLength);
}

inline BlChar * ProgramImpl::linecontent (Position pos)
{
	return program + pos + sizeof (BlLineNumber) + sizeof (BlLineLength);
}

inline CodeLine ProgramImpl::getfirstline ()
{
	//if (size == 0)
	//	return CodeLine ();
	return CodeLine (
		//program + sizeof (BlLineNumber) + sizeof (BlLineLength),
		linecontent (0),
		numline (0), sizeline (0) );
}

inline void ProgramImpl::getnextline (CodeLine & codeline)
{
	//if (codeline.number () == 0)
	if (codeline.number () == LineDirectCommand)
	{
		//codeline.assign (0, 0, 0);
		codeline.assign (0, LineEndProgram, 0);
		return;
	}

	#if 0

	Position pos= codeline.content () - program;
	pos+= codeline.length ();
	if (pos >= size)
	{
		//codeline.assign (0, 0, 0);
		codeline.assign (0, LineEndProgram, 0);
		return;
	}
	codeline.assign (linecontent (pos), numline (pos), sizeline (pos) );

	#else

	// Testing a micro optimization.

	BlChar * p= const_cast <BlChar *>
		(codeline.content () + codeline.length () );
	//codeline.assign (p + sizeof (BlLineNumber) + sizeof (BlLineLength),
	//	getLineNumber (p),
	//	getLineLength (p + sizeof (BlLineNumber) ) );

	getLineAt (p, codeline);

	#endif
}

void ProgramImpl::clear_label_cache ()
{
	TRACEFUNC (tr, "ProgramImpl::clear_label_cache");

	labelcache.clear ();
	#ifndef OLD_LABEL_CACHE
	labelcache_inited= false;
	#endif
}

void ProgramImpl::clear_cache ()
{
	TRACEFUNC (tr, "ProgramImpl::clear_cache");

	clear_label_cache ();
	linecache.clear ();

	#ifdef CACHE_ALL_LINES
	line_cache_inited= false;
	#endif
}

void ProgramImpl::setlabel (const std::string & label,
	CodeLine & codeline)
{
	BlLineNumber n (codeline.number () );

	#if 0
	labelcache [label]= n;
	#else
	std::pair <labelcache_t::iterator, bool> r =
		labelcache.insert (std::make_pair (label, n) );
	if (! r.second)
	{
		if (showdebuginfo () )
			cerr << "Duplicate label '" << label <<
				"' in lines " << r.first->second <<
				" and " << n <<
				endl;
		throw ErrDuplicateLabel;
	}
	#endif

	// Put it in the line number cache, to avoid one search.
	#ifndef CACHE_ALL_LINES
	//linecache [n]= codeline.content () - program
	//	- sizeof (BlLineNumber)
	//	- sizeof (BlLineLength);
	linecache [n]= const_cast <BlChar *> (codeline.content () ) -
		sizeof (BlLineNumber) - sizeof (BlLineLength);
	#endif
}

#ifdef OLD_LABEL_CACHE

inline BlLineNumber ProgramImpl::getlabel (const std::string & label)
{
	labelcache_t::iterator  it= labelcache.find (label);
	if (it != labelcache.end () )
	{
		#ifndef NDEBUG
		++labelcache_hits;
		#endif
		return it->second;
	}
	else
	{
		#ifndef NDEBUG
		++labelcache_fails;
		#endif
	}
	CodeLine codeline= getfirstline ();
	CodeLine::Token token;
	//while (codeline.number () != 0)
	while (codeline.number () != LineEndProgram)
	{
		codeline.gettoken (token);
		if (token.code == keyLABEL)
		{
			codeline.gettoken (token);
			if (label == token.str)
			{
				setlabel (label, codeline);
				return codeline.number ();
			}
		}
		getnextline (codeline);
	}
	//return 0;
	return LineEndProgram;
}

#else

// New label cache

namespace {

class LabelCacheGuard {
public:
	LabelCacheGuard (ProgramImpl & pin);
	~LabelCacheGuard ();
	void release ();
private:
	ProgramImpl & pi;
	bool released;
};

LabelCacheGuard::LabelCacheGuard (ProgramImpl & pin) :
	pi (pin),
	released (false)
{ }

LabelCacheGuard::~LabelCacheGuard ()
{
	if (! released)
		pi.clear_label_cache ();
}

void LabelCacheGuard::release ()
{
	released= true;
}

} // namespace

void ProgramImpl::generate_label_cache ()
{
	TRACEFUNC (tr, "ProgramImpl::generate_label_cache");

	LabelCacheGuard guard (* this);

	CodeLine codeline= getfirstline ();
	CodeLine::Token token;
	while (codeline.number () != LineEndProgram)
	{
		codeline.gettoken (token);
		if (token.code == keyLABEL)
		{
			codeline.gettoken (token);
			if (token.code != keyIDENTIFIER)
			{
				if (showdebuginfo () )
					cerr << "Invalid LABEL "
						"in line " <<
						codeline.number () <<
						endl;
				throw ErrSyntax;
			}
			setlabel (token.str, codeline);
		}
		getnextline (codeline);
	}

	guard.release ();
	labelcache_inited= true;
}

inline BlLineNumber ProgramImpl::getlabel (const std::string & label)
{
	if (! labelcache_inited)
	{
		#ifndef NDEBUG
		++labelcache_fails;
		#endif
		generate_label_cache ();
	}

	labelcache_t::iterator  it= labelcache.find (label);
	if (it != labelcache.end () )
	{
		#ifndef NDEBUG
		++labelcache_hits;
		#endif
		return it->second;
	}
	else
	{
		#ifndef NDEBUG
		++labelcache_fails;
		#endif
		//return 0;
		return LineEndProgram;
	}
}

#endif

#if 0
inline BlLineNumber ProgramImpl::getnextnum (CodeLine & line)
{
	if (line.number () == 0)
		return 0;
	Position pos= line.content () - program;
	pos+= line.length ();
	if (pos >= size)
		return 0;
	return numline (pos);
}
#endif

inline void ProgramImpl::getlineinpos (Position pos, CodeLine & codeline)
	const
{
	codeline.assign (linecontent (pos), numline (pos), sizeline (pos) );
}

CodeLine ProgramImpl::getlineinpos (Position pos) const
{
	return CodeLine
		(linecontent (pos), numline (pos), sizeline (pos) );
}


#ifdef CACHE_ALL_LINES

void ProgramImpl::generate_line_cache ()
{
	TRACEFUNC (tr, "ProgramImpl::generate_line_cache");

	//for (Position pos= 0; pos < size; pos= nextline (pos) )
	//{
	//	linecache [numline (pos) ]= pos;
	//}
	for (BlChar * p= program; ; p= getNextLineAt (p) )
	{
		BlLineNumber n= getLineNumberAt (p);
		linecache [n]= p;
		// The end mark is also included in the cache.
		if (n > BlMaxLineNumber)
			break;
	}

	line_cache_inited= true;
}

#endif

inline void ProgramImpl::getline (BlLineNumber num, CodeLine & codeline)
{
	//TRACEFUNC (tr, "ProgramImpl::getline");

	#ifdef CACHE_ALL_LINES
	if (! line_cache_inited)
		generate_line_cache ();
	#endif

	//Position pos= 0;
	BlChar * p= program;

	if (num != LineBeginProgram)
	{
		#ifndef CACHE_ALL_LINES

		linecache_t::iterator it= linecache.find (num);
		if (it != linecache.end () )
		{
			//pos= it->second;
			p= it->second;
			#ifndef NDEBUG
			++linecache_hits;
			#endif
		}
		else
		{
			#ifndef NDEBUG
			++linecache_fails;
			//TRMESSAGE (tr, "Line " + to_string (num) +
			//	" missing in cache");
			#endif

			//while (pos < size && numline (pos) < num)
			//	pos= nextline (pos);
			//if (pos >= size)
			//{
			//	codeline.assign (0, LineEndProgram, 0);
			//	return;
			//}
			while (getLineNumberAt (p) < num)
				p= getNextLineAt (p);

			//linecache [num]= pos;
			linecache [num]= p;
		}
	
		#else

		linecache_t::iterator it= linecache.lower_bound (num);
//		if (it == linecache.end () )
//		{
//			#ifndef NDEBUG
//			++linecache_fails;
//			#endif
//			//codeline.assign (0, 0, 0);
//			codeline.assign (0, LineEndProgram, 0);
//			return;
//		}
		#ifndef NDEBUG
		++linecache_hits;
		#endif
		//pos= it->second;
		p= it->second;

		#endif
	}

	//getlineinpos (pos, codeline);
	getLineAt (p, codeline);
}

void ProgramImpl::getline (ProgramPos pos, CodeLine & codeline)
{
	BlLineNumber n= pos.getnum ();
	getline (n, codeline);
	if (codeline.number () == n)
	{
		BlChunk ch= pos.getchunk ();
		if (ch != 0)
			codeline.gotochunk (ch);
	}
}

void ProgramImpl::do_insert (const CodeLine & codeline)
{
	Position pos= 0;
	while (pos < size && numline (pos) < codeline.number () )
		pos= nextline (pos);

	const BlChar * strnew= codeline.content ();
	BlLineLength linesize= codeline.length () +
		sizeof (BlLineNumber) + sizeof (BlLineLength);

	//#define USE_PADDING

	#ifdef USE_PADDING
	BlLineLength paddedsize= linesize;
	unsigned int pad= linesize % 4;
	if (pad > 0)
	{
		pad= 4 - pad;
		paddedsize+= pad;
	}
	#endif

	//unsigned long osize= size;
	ProgSize newsize= size;

	#ifndef USE_PADDING
	Position destpos= pos + linesize;
	#else
	Position destpos= pos + paddedsize;
	#endif

	Position origpos;
	if (pos < size && numline (pos) == codeline.number () )
	{
		origpos= nextline (pos);
		//destpos= pos + sizenew;
		newsize+= codeline.length () - sizeline (pos);
		//size+= codeline.length () - sizeline (pos);
	}
	else
	{
		origpos= pos;
		//destpos= pos + sizenew;

		#ifndef USE_PADDING
		newsize+= linesize;
		//size+= linesize;
		#else
		newsize+= paddedsize;
		//size+= paddedsize;
		#endif
	}

	if (destpos > origpos)
	{
		ASSERT (newsize > size);
		//ASSERT (size > osize);

		#if 0
		size_t newblock= blockrounded (size);
		if (newblock != blockrounded (osize) )
		{
			unsigned char * newprog=
				(unsigned char *) realloc (program, newblock);
			if (! newprog)
				throw ErrOutMemory;
			program= newprog;
		}
		#else

		ProgSize osize= size;
		resize (newsize);

		#endif

		if (pos < osize)
		{
			memmove (program + destpos,
				program + origpos,
				osize - origpos);
		}
	}
	else if (destpos < origpos)
	{
		ASSERT (newsize < size);
		//ASSERT (size < osize);
		memmove (program + destpos,
			program + origpos,
			size - origpos);
			//osize - origpos);

		#if 0
		size_t newblock= blockrounded (size);
		if (newblock != blockrounded (osize) )
		{
			unsigned char * newprog=
				(unsigned char *) realloc (program, newblock);
			if (! newprog)
				throw ErrOutMemory;
			program= newprog;
		}
		#else

		resize (newsize);

		#endif
	}

	setLineNumber (program + pos, codeline.number () );

	#ifndef USE_PADDING
	setLineLength (program + pos + sizeof (BlLineNumber),
		codeline.length () );
	#else
	setLineLength (program + pos + sizeof (BlLineNumber),
		codeline.length () + pad);
	#endif

	memcpy (linecontent (pos), strnew, codeline.length () );

	#ifdef USE_PADDING
	if (pad > 0)
		memset (linecontent (pos) + codeline.length (), 0, pad);
	#endif
}

void ProgramImpl::insert (const CodeLine & codeline)
{
	clear_cache ();
	do_insert (codeline);
}

void ProgramImpl::deletelines
	(BlLineNumber iniline, BlLineNumber endline)
{
	TRACEFUNC (tr, "ProgramImpl::deletelines");

	if ( (iniline > BlMaxLineNumber && iniline != LineBeginProgram) ||
		(endline > BlMaxLineNumber && endline != LineEndProgram) )
	{
		ASSERT (false);
		throw ErrBlassicInternal;
	}

	if (iniline == LineBeginProgram && endline == LineEndProgram)
	{
		renew ();
	}
	else
	{
		// Evaluate initial position.

		Position pos= 0;
		if (iniline != LineBeginProgram)
		{
			while (pos < size && numline (pos) < iniline)
				pos= nextline (pos);
		}
		if (pos >= size)
			return;

		// Evaluate final position.

		Position posend= pos;
		while (posend < size && numline (posend) <= endline)
			posend= nextline (posend);
		if (posend == pos)
			return;

		#if 0
		cout << "Deleting from " << numline (pos) << " to ";
		if (posend < size)
			cout << "(not including) " << numline (posend);
		else
			cout << "the end";
		cout << endl;
		#endif

		if (posend < size)
			memmove (program + pos,
				program + posend,
				size - posend);
		//size_t osize= size;
		//size-= posend - pos;
		ProgSize newsize= size - posend + pos;

		if (newsize > 0)
		{
			clear_cache ();
			#if 0
			size_t newblock= blockrounded (newsize);
			if (newblock != blockrounded (size) )
				realloc (program, newblock);
			#else

			resize (newsize);

			#endif
		}
		else
		{
			renew ();
		}
	}
}

void ProgramImpl::listline (const CodeLine & codeline, BlFile & out)
	const
{
	BlLineNumber number= codeline.number ();
	BlLineLength linesize= codeline.length ();
	out << /*std::setw (7) << */ number << ' ';
	const BlChar * aux= codeline.content ();
	std::string line;
	for (unsigned long i= 0; i < linesize; ++i)
	{
		unsigned char c= aux [i];
		if (c == '\0') // Skip garbage
			break;
		if (iskey (c) )
		{
			BlCode s= c;
			s<<= 8;
			s|= aux [++i];
			line+= decodekeyword (s);
		}
		else if (c == INTEGER_PREFIX)
		{
			//BlInteger n;
			//n= * (BlInteger *) (aux + i + 1);
			BlInteger n= peek32 (aux + i + 1);
			std::ostringstream oss;
			oss << n;
			line+= oss.str ();
			i+= 4;
		}
		else if (c == '"')
		{
			line+= c;
			while ( (c= aux [++i]) != 0)
				if (c == '"')
					line+= "\"\"";
				else
					line+= c;
			line+= '"';
		}
		else if (c == '\t')
		{
			const size_t l= line.size ();
			line.insert (l, 8 - l % 8, ' ');
		}
		else
			line+= c;
	}
	out << line;
	out.endline ();
}

void ProgramImpl::list
	(BlLineNumber iniline, BlLineNumber endline, BlFile & out) const
{
	TRACEFUNC (tr, "ProgramImpl::list");

	if ( (iniline > BlMaxLineNumber && iniline != LineBeginProgram) ||
		(endline > BlMaxLineNumber && endline != LineEndProgram) )
	{
		ASSERT (false);
		throw ErrBlassicInternal;
	}

	Position pos= 0;
	if (iniline != LineBeginProgram)
	{
		while (pos < size && numline (pos) < iniline)
			pos= nextline (pos);
	}
	while (pos < size)
	{
		BlLineNumber number= numline (pos);
		if (number > endline || number == LineEndProgram)
			break;
		listline (getlineinpos (pos), out);
		pos= nextline (pos);
		if (fInterrupted)
			break;
	}
}

namespace {

bool hasblassicextension (const std::string & name)
{
	std::string::size_type l= name.size ();
	if (l < 4)
		return false;
	std::string ext= name.substr (l - 4);
	//#ifdef _Windows
	std::transform (ext.begin (), ext.end (), ext.begin (), tolower);
	//#endif
	if (ext == ".blc" || ext == ".bas")
		return true;
	return false;
}

void openblassicprogram (std::ifstream & is, const std::string & name)
{
	const std::ios::openmode mode= std::ios::binary | std::ios::in;
	is.open (name.c_str (), mode);
	if (! is)
	{
		if (! hasblassicextension (name) )
		{
			std::string namex= name;
			namex+= ".blc";
			is.clear ();
			is.open (namex.c_str (), mode);
			if (! is.is_open () )
			{
				namex= name;
				namex+= ".bas";
				is.clear ();
				is.open (namex.c_str (), mode);
				if (! is.is_open () )
					throw ErrFileNotFound;
			}
		}
		else
			throw ErrFileNotFound;
	}
}

const char signature []=
	{ 'B', 'l', 'a', 's', 's', 'i', 'c', '\0' };
const size_t lsig= sizeof (signature);

bool isblassicbinary (std::istream & is)
{
	char magicstring [lsig];
	is.read (magicstring, lsig);
	if (! is || memcmp (magicstring, signature, lsig) != 0)
		return false;
	return true;
}

inline void checkread (std::istream & is, size_t readed)
{
	if (! is || size_t (is.gcount () ) != readed)
		throw ErrFileRead;
}

typedef BlUint32 EndianType;
const EndianType endian_mark= 0x12345678;

class TextLoader {
public:
	TextLoader (ProgramImpl & program) :
		program (program),
		nextnumline (sysvar::get32 (sysvar::AutoInit) ),
		incnumline (sysvar::get32 (sysvar::AutoInc) ),
		maxnumline (BlMaxLineNumber - incnumline)
	{ }
	bool directive (std::string str);
	void load (std::istream & is);
private:
	ProgramImpl & program;
	BlLineNumber nextnumline;
	BlLineNumber incnumline;
	BlLineNumber maxnumline;
};

bool TextLoader::directive (std::string str)
{
	TRACEFUNC (tr, "TextLoader::directive");

	static std::string include ("include");
	static const std::string::size_type linc= include.size ();
	if (str.substr (1, linc) == include)
	{
		str.erase (0, linc + 1);
		std::string::size_type l= str.find_first_not_of (" \t");
		if (l > 0)
			str.erase (0, l);
		if (str.empty () )
			return false;
		if (str [0] == '"')
		{
			l= str.find ('"',  1);
			str= str.substr (1, l - 1);
		}
		else if (str [0] == '<')
		{
			l= str.find ('>',  1);
			str= str.substr (1, l - 1);
		}
		else
		{
			l= str.find_first_of (" \t");
			if (l != std::string::npos)
				str.erase (l);
		}
		TRMESSAGE (tr, str);
		std::ifstream is;
		openblassicprogram (is, str);
		load (is);
		return true;
	}
	return false;
}

void TextLoader::load (std::istream & is)
{
	TRACEFUNC (tr, "TextLoader::load");

	bool blankcomment= sysvar::hasFlags2 (sysvar::BlankComment);

	std::string str;
	std::getline (is, str);
	if (!str.empty () && str [0] == '#')
	{
		str.erase ();
		std::getline (is, str);
	}
	CodeLine codeline;
	bool fExhausted= false;
	for ( ; is; std::getline (is, str) )
	{
		if (! str.empty () )
		{
			// EOF char on windows
			if (str [0] == '\x1A')
				break;

			if (str [str.size () - 1] == '\r')
				str.erase (str.size () - 1);
		}

		// Quick & dirty implemantation of #include
		if (! str.empty () && str [0] == '#' && directive (str) )
			continue;

		if (str.empty () && blankcomment)
			str= "'";

		codeline.scan (str);
		//if (codeline.number () == 0)
		if (codeline.number () == LineDirectCommand)
		{
			if (fExhausted)
			{
				TRMESSAGE (tr, "Line exhausted");
				throw ErrLineExhausted;
			}
			codeline.setnumber (nextnumline);
			fExhausted= nextnumline > maxnumline;
			nextnumline+= incnumline;
		}
		else
		{
			fExhausted= codeline.number () > maxnumline;
			nextnumline= codeline.number () + incnumline;
		}
		if (codeline.length () > 0)
			program.do_insert (codeline);
	}
}

} // namespace

void ProgramImpl::save (const std::string & name) const
{
	TRACEFUNC (tr, "ProgramImpl::save");

	std::ofstream os (name.c_str (), std::ios::binary | std::ios::out);

	// Blassic signature.
	if (! os)
		return;
	os.write (signature, lsig);

	// Endian mark.
	ASSERT (sizeof (endian_mark) == 4);
	os.write ( (char *) & endian_mark, 4);

	// Size.
	BlChar caux [4];
	poke32 (caux, static_cast <BlUint32> (size) );
	os.write ( (char *) caux, 4);

	// Program body.
	os.write ( (char *) program, size);
	if (! os)
		throw ErrFileWrite;
}

void ProgramImpl::loadtext (std::istream & is)
{
	TRACEFUNC (tr, "ProgramImpl::loadtext");

	TextLoader loader (* this);
	loader.load (is);
}

void ProgramImpl::loadbinary (std::istream & is)
{
	TRACEFUNC (tr, "ProgramImpl::loadbinary");

	// This was intended to check endianess, but is
	// currently unused.
	EndianType endian_check;
	ASSERT (sizeof endian_check == 4);

	is.read ( (char *) & endian_check, 4);
	checkread (is, 4);

	#define SHOW_ENDIAN_CHECK
	#ifdef SHOW_ENDIAN_CHECK
	if (showdebuginfo () )
	{
		std::ostringstream oss;
		oss << "Endian check: " << std::hex << endian_check;
		cerr << oss.str () << endl;
	}
	#ifndef NDEBUG
	{
		std::ostringstream oss;
		oss << "Endian check: " << std::hex << endian_check;
		TRMESSAGE (tr, oss.str () );
	}
	#endif
	#endif

	// Get the program size.
	BlChar caux [4];
	is.read ( (char *) caux, 4);
	checkread (is, 4);
	unsigned long newsize= peek32 (caux);

	// Get program body.
	if (newsize > 0)
	{
		//size_t newblock= blockrounded (newsize);
		//util::auto_alloc <BlChar> newprog (newblock);
		//is.read (reinterpret_cast <char *> (newprog.data () ),
		//	newsize);

		ProgramImpl other;
		other.resize (newsize);
		is.read (reinterpret_cast <char *> (other.program), newsize);

		checkread (is, newsize);

		//renew ();
		//program= newprog;
		//newprog.release ();
		//size= newsize;

		clear_cache ();
		transfer_content (other);
	}
	else
		renew ();
}

void ProgramImpl::load (const std::string & name)
{
	TRACEFUNC (tr, "ProgramImpl::load (const string &)");

	std::ifstream (is);
	openblassicprogram (is, name);

	if (! isblassicbinary (is) )
	{
		renew ();
		is.clear ();
		is.seekg (0);
		loadtext (is);
	}
	else
	{
		loadbinary (is);
	}
}

void ProgramImpl::load (std::istream & is)
{
	TRACEFUNC (tr, "ProgramImpl::load (istream &)");

	renew ();
	loadtext (is);
}

void ProgramImpl::merge (const std::string & name,
	BlLineNumber inidel,
	BlLineNumber enddel)
{
	TRACEFUNC (tr, "ProgramImpl::merge");

	bool dellines= true;
	if (inidel == LineNoDelete)
	{
		if (enddel != LineNoDelete)
		{
			ASSERT (false);
			throw ErrBlassicInternal;
		}
		dellines= false;
	}
	else
	{
		if (enddel == LineNoDelete)
		{
			ASSERT (false);
			throw ErrBlassicInternal;
		}
		if ( (inidel > BlMaxLineNumber &&
			inidel != LineBeginProgram) ||
			(enddel > BlMaxLineNumber &&
			enddel != LineEndProgram) )
		{
			ASSERT (false);
			throw ErrBlassicInternal;
		}
	}

	clear_cache ();

	#if 0
	std::ifstream is;
	openblassicprogram (is, name);

	ProgramImpl inload;
	if (! isblassicbinary (is) )
	{
		is.clear ();
		is.seekg (0);
		inload.loadtext (is);
	}
	else
	{
		inload.loadbinary (is);
	}
	is.close ();
	#else

	ProgramImpl inload;
	inload.load (name);

	#endif

	if (dellines)
		deletelines (inidel, enddel);

	for (CodeLine codeline= inload.getfirstline ();
		//codeline.number () != 0;
		codeline.number () != LineEndProgram;
		//codeline= inload.getnextline (codeline)
		inload.getnextline (codeline)
		)
	{
		do_insert (codeline);
	}
}

namespace {

bool iscodewithnumber (BlCode code)
{
	return (code == keyGOTO || code == keyGOSUB || code == keyRUN ||
		code == keyRESTORE || code == keyRESUME ||
		code == keyDELETE ||
		code == keyLIST || code == keyLLIST ||
		code == keyEDIT ||
		code == keyTHEN || code == keyELSE);
}

void changenumber (BlChar * pos, const MapLine & mapline)
{
	BlLineNumber old= getLineNumber (pos);
	//cerr << " Find " << old << flush;
	MapLine::const_iterator it= mapline.find (old);
	if (it != mapline.end () )
	{
		//cerr << " Changed " << it->second << flush;
		setLineNumber (pos, it->second);
	}
}

} // namespace

void ProgramImpl::changeline (Position pos, const MapLine & mapline)
{
	const BlLineLength l= sizeline (pos);
	BlChar * s= linecontent (pos);
	BlLineLength p= 0;
	BlChar c;
	while (p < l)
	{
		c= s [p];
		if (iskey (c) )
		{
			BlCode code= BlCode ( (BlCode (c) << 8 ) ) |
				BlCode (s [p+1] );
			p+= 2;
			//cerr << "Key " << decodekeyword (code) << flush;
			if (iscodewithnumber (code) )
			{
				//cerr << " analyzing" << flush;
				for (;;)
				{
					while (p < l && isspace (s [p] ) )
						++p;
					if (p >= l)
						break;
					c= s [p];
					if (c == INTEGER_PREFIX)
					{
						BlChar * const pos= s + p + 1;
						changenumber (pos, mapline);
						p+= 1 + sizeof (BlInteger);
					}
					else if (c == ':')
					{
						++p;
						break;
					}
					else if (c == '"')
					{
						while (s [++p] != '\0')
							continue;
						++p;
					}
					else if (iskey (c) )
						p+= 2;
					else if (c == '\'')
					{
						p= l;
						break;
					}
					else ++p;
				}
				//cerr << endl;
			}
			//else cerr << endl;
		}
		else if (c == INTEGER_PREFIX)
			p+= 1 + sizeof (BlInteger);
		else if (c == '"')
		{
			while (s [++p] != '\0')
				continue;
			++p;
		}
		else if (c == '\'')
			break;
		else ++p;
	}
}

void ProgramImpl::renum (BlLineNumber blnNew, BlLineNumber blnOld,
	BlLineNumber blnInc, BlLineNumber blnStop)
{
	TRACEFUNC (tr, "ProgramImpl::renum");
	TRMESSAGE (tr, "args: " + to_string (blnNew) + ", " +
		to_string (blnOld) + ", " +
		to_string (blnInc) + ", " +
		to_string (blnStop) );

	if (blnNew > BlMaxLineNumber || blnInc > BlMaxLineNumber ||
		(blnOld > BlMaxLineNumber && blnOld != LineBeginProgram) ||
		(blnStop > BlMaxLineNumber && blnStop != LineEndProgram) )
	{
		ASSERT (false);
		throw ErrBlassicInternal;
	}

	bool showinfo= showdebuginfo ();

	if (size == 0)
	{
		if (showinfo)
			cerr << "Trying to renum but program is empty" <<
				endl;
		return;
	}
	if (blnInc == 0)
		throw ErrImproperArgument;

	// Find first line to renum.

	Position pos= 0;
	BlLineNumber previous= LineBeginProgram;
	if (blnOld != LineBeginProgram)
	{
		TRMESSAGE (tr, "Searching for " + to_string (blnOld) );
		while (pos < size && numline (pos) < blnOld)
		{
			previous= numline (pos);
			//cerr << "Skipping line " << previous << endl;
			pos= nextline (pos);
		}
		TRMESSAGE (tr, "Found " + to_string (numline (pos) ) );
	}
	if (previous != LineBeginProgram && previous >= blnNew)
		throw ErrImproperArgument;

	// Evaluate changes required.

	MapLine mapline;
	BlLineNumber actual;
	BlLineNumber blnMax= BlMaxLineNumber - blnInc;
	bool overflow= false;
	for ( ; pos < size; pos= nextline (pos) )
	{
		actual= numline (pos);
		if (actual >= blnStop)
		{
			TRMESSAGE (tr, "Stop line reached");
			if (previous >= blnStop)
			{
				if (showinfo)
					cerr << "Renumbered line " <<
						previous <<
						" is greater than " <<
						blnStop <<
						endl;
				throw ErrImproperArgument;
			}
			break;
		}
		if (actual != blnNew)
		{
			if (overflow)
				throw ErrLineExhausted;
			TRMESSAGE (tr, "changing " + to_string (actual) +
				" by " + to_string (blnNew) );
			mapline [actual]= blnNew;
		}
		previous= blnNew;
		if (blnNew > blnMax)
			overflow= true;
		else
			blnNew+= blnInc;
	}

	if (mapline.empty () )
	{
		if (showinfo)
			cerr << "renum: no changes needed" << endl;
		return;
	}

	// Do the changes.

	clear_cache ();

	MapLine::iterator it, mapend= mapline.end ();
	for (pos= 0; pos < size; pos= nextline (pos) )
	{
		actual= numline (pos);
		it= mapline.find (actual);
		if (it != mapend)
		{
			//cerr << "Changing line " << actual <<
			//	" by " << it->second << endl;
			setLineNumber (program + pos, it->second);
		}
		changeline (pos, mapline);
	}
}

// End of program.cpp
