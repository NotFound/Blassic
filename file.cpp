// file.cpp
// Revision 9-jan-2005

#include "blassic.h"
#include "file.h"
#include "trace.h"
#include "error.h"
#include "var.h"

//#include "cursor.h"
//#include "edit.h"
//#include "graphics.h"

#include "sysvar.h"
//#include "socket.h"
#include "util.h"
using util::to_string;

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// para strerror (errno)
#include <string.h>
#include <errno.h>

#include <iostream>
using std::cerr;
using std::endl;

#include <cassert>
#define ASSERT assert

//***********************************************
//		Auxiliary functions
//***********************************************

namespace {

BlInteger lengthoffileread (std::fstream & fs)
{
	// WARNING: The C++ standard does not guarantee that
	// this method obtains the length of the file.
	std::streamsize old= fs.tellg ();
	fs.seekg (0, std::ios::end);
	std::streamsize l= fs.tellg ();
	fs.seekg (old, std::ios::beg);
	// Not leave the file unusable in case something were wrong.
	fs.clear ();
	return static_cast <BlInteger> (l);
}

BlInteger lengthoffilewrite (std::fstream & fs)
{
	// WARNING: The C++ standard does not guarantee that
	// this method obtains the length of the file.
	std::streamsize old= fs.tellp ();
	fs.seekp (0, std::ios::end);
	std::streamsize l= fs.tellp ();
	fs.seekp (old, std::ios::beg);
	// Not leave the file unusable in case something were wrong.
	fs.clear ();
	return static_cast <BlInteger> (l);
}

} // namespace

namespace blassic {

namespace file {

//***********************************************
//              BlFile
//***********************************************

BlFile::BlFile (OpenMode nmode) :
	mode (nmode),
	cDelimiter (','),
	cQuote ('"'),
	cEscape ('\0')
{
}

BlFile::~BlFile ()
{
}

void BlFile::closein ()
	{ throw ErrFileMode; }
void BlFile::closeout ()
	{ throw ErrFileMode; }

void BlFile::reset (int, int, int, int)
	{ throw ErrFileMode; }

bool BlFile::istextwindow () const
{
	return false;
}

bool BlFile::eof ()
{
	if (showdebuginfo () )
		cerr << "This file type does not implement EOF" << endl;
	throw ErrFileMode;
}

size_t BlFile::loc ()
	{ throw ErrFileMode; }
void BlFile::flush ()
	{ throw ErrFileMode; }
void BlFile::getline (std::string &, bool)
	{ throw ErrFileMode; }
void BlFile::outstring (const std::string &)
	{ throw ErrFileMode; }
void BlFile::outchar (char)
	{ throw ErrFileMode; }

#if 0

void BlFile::outnumber (BlNumber n)
{
	//outstring (to_string (n) );
	std::ostringstream oss;
	oss << std::setprecision (16) << n;
	outstring (oss.str () );
}

void BlFile::outinteger (BlInteger n)
{
	outstring (to_string (n) );
}

void BlFile::outlinenumber (BlLineNumber l)
{
	std::ostringstream oss;
	oss << std::setw (7) << l;
	outstring (oss.str () );
}

#endif

BlFile & operator << (BlFile & bf, const std::string & str)
{
	TRACEFUNC (tr, "operator << (BlFile &, const string &)");

	bf.outstring (str);
	return bf;
}

BlFile & operator << (BlFile & bf, char c)
{
	bf.outchar (c);
	return bf;
}

BlFile & operator << (BlFile & bf, BlNumber n)
{
	//bf.outnumber (n);
	std::ostringstream oss;
	oss << std::setprecision (16) << n;
	bf.outstring (oss.str () );
	return bf;
}

BlFile & operator << (BlFile & bf, BlInteger n)
{
	//bf.outinteger (n);
	bf.outstring (to_string (n) );
	return bf;
}

BlFile & operator << (BlFile & bf, BlLineNumber l)
{
	//bf.outlinenumber (l);
	std::ostringstream oss;
	oss << std::setw (7) << l;
	bf.outstring (oss.str () );
	return bf;
}

BlFile & operator << (BlFile & bf, unsigned short n)
{
	bf.outstring (to_string (n) );
	return bf;
}

void BlFile::putspaces (size_t n)
{
	outstring (std::string (n, ' ') );
}

void BlFile::tab ()
{
	outchar ('\t');
}

void BlFile::tab (size_t n)
{
	// Provisional
	outstring (std::string (n, ' ') );
}

void BlFile::endline ()
{
	outchar ('\n');
}

void BlFile::put (size_t)
{ throw ErrFileMode; }

void BlFile::get (size_t)
{ throw ErrFileMode; }

void BlFile::field_clear ()
{ throw ErrFileMode; }

void BlFile::field (const std::vector <field_element> &)
{ throw ErrFileMode; }

void BlFile::field_append (const std::vector <field_element> &)
{ throw ErrFileMode; }

// assign doesn't throw because we call it for all open files,
// those that are no random files or does nor have the var in
// their fields just ignore it.
bool BlFile::assign (const std::string &, const Dimension &,
	const std::string &, Align)
{ return false; }
bool BlFile::assign_mid (const std::string &, const Dimension &,
	const std::string &, size_t, std::string::size_type)
{ return false; }

size_t BlFile::getwidth () const
{ throw ErrFileMode; }

void BlFile::movecharforward ()
{ throw ErrFileMode; }

void BlFile::movecharforward (size_t)
{ throw ErrFileMode; }

void BlFile::movecharback ()
{ throw ErrFileMode; }

void BlFile::movecharback (size_t)
{ throw ErrFileMode; }

void BlFile::movecharup ()
{ throw ErrFileMode; }

void BlFile::movecharup (size_t)
{ throw ErrFileMode; }

void BlFile::movechardown ()
{ throw ErrFileMode; }

void BlFile::movechardown (size_t)
{ throw ErrFileMode; }

void BlFile::showcursor ()
{ throw ErrFileMode; }

void BlFile::hidecursor ()
{ throw ErrFileMode; }

std::string BlFile::getkey ()
{ throw ErrFileMode; }

std::string BlFile::inkey ()
{ throw ErrFileMode; }

std::string BlFile::read (size_t)
{ throw ErrFileMode; }

void BlFile::gotoxy (int, int)
{ throw ErrFileMode; }

void BlFile::setcolor (int)
{ throw ErrFileMode; }

int BlFile::getcolor ()
{ throw ErrFileMode; }

void BlFile::setbackground (int)
{ throw ErrFileMode; }

int BlFile::getbackground ()
{ throw ErrFileMode; }

void BlFile::cls ()
{ throw ErrFileMode; }

std::string BlFile::copychr (BlChar, BlChar)
{ throw ErrFileMode; }

int BlFile::pos ()
{ throw ErrFileMode; }

int BlFile::vpos ()
{ throw ErrFileMode; }

void BlFile::tag ()
{ throw ErrFileMode; }

void BlFile::tagoff ()
{ /* Ignored */ }

bool BlFile::istagactive ()
{ return false; }

void BlFile::inverse (bool)
{ throw ErrFileMode; }

bool BlFile::getinverse ()
{ throw ErrFileMode; }

void BlFile::bright (bool)
{ throw ErrFileMode; }

bool BlFile::getbright ()
{ throw ErrFileMode; }

void BlFile::setwidth (size_t )
{ }

void BlFile::setmargin (size_t )
{ }

BlInteger BlFile::lof ()
{ throw ErrFileMode; }

bool BlFile::poll ()
{
	return false;
}

void BlFile::scroll (int)
{ throw ErrFileMode; }


//***********************************************
//		BlFileOut
//***********************************************

BlFileOut::BlFileOut () : BlFile (Output)
{ }

BlFileOut::BlFileOut (OpenMode mode) : BlFile (mode)
{ }

void BlFileOut::flush ()
{
	ofs () << std::flush;
}

void BlFileOut::outstring (const std::string & str)
{
	ofs () << str;
}

void BlFileOut::outchar (char c)
{
	ofs () << c;
}

#if 0

void BlFileOut::outnumber (BlNumber n)
{
	ofs () << n;
}

void BlFileOut::outinteger (BlInteger n)
{
	ofs () << n;
}

void BlFileOut::outlinenumber (BlLineNumber l)
{
	ofs () << std::setw (7) << l;
}

#endif

//***********************************************
//		BlFileOutString
//***********************************************

BlFile * newBlFileOutString ()
{
	return new BlFileOutString;
}

BlFileOutString::BlFileOutString ()
{ }

std::string BlFileOutString::str ()
{
	return oss.str ();
}

std::ostream & BlFileOutString::ofs ()
{
	return oss;
}

//***********************************************
//		BlFileOutput
//***********************************************

class BlFileOutput : public BlFileOut {
public:
	BlFileOutput (std::ostream & os);
	bool isfile () const { return true; }
private:
	std::ostream & ofs ();
	std::ostream & os;
};

class BlFileRegular : public BlFileOut {
public:
	BlFileRegular (const std::string & name, OpenMode mode);
	bool isfile () const { return true; }
	void getline (std::string & str, bool endline= true);
	bool eof ();
	void flush ();
	virtual std::string getkey ();
	virtual std::string inkey ();
	std::string read (size_t n);
	BlInteger lof ();
private:
	std::ostream & ofs ();
	std::fstream fs;
};

BlFile * newBlFileOutput (std::ostream & os)
{
	return new BlFileOutput (os);
}

BlFileOutput::BlFileOutput (std::ostream & os) :
	os (os)
{
}

std::ostream & BlFileOutput::ofs ()
{
	return os;
}

//***********************************************
//              BlFileRegular
//***********************************************

BlFile * newBlFileRegular (const std::string & name, OpenMode mode)
{
	return new BlFileRegular (name, mode);
}

BlFileRegular::BlFileRegular (const std::string & name, OpenMode nmode) :
	BlFileOut (nmode)
{
	TRACEFUNC (tr, "BlFileRegular::BlFileRegular");

	using std::ios;

	ios::openmode mode= ios::in;
	switch (nmode & ~ Binary)
	{
	case Input:
		mode= ios::in; break;
	case Output:
		mode= ios::out; break;
	case Append:
		//mode= ios::out | ios::ate; break;
		mode= ios::out | ios::app; break;
	default:
		if (showdebuginfo () )
			cerr << "Unexpected mode value" << endl;
		TRMESSAGE (tr, std::string ("Invalid mode ") +
			util::to_string (nmode) );
		throw ErrBlassicInternal;
	}
	if (nmode & Binary)
		mode|= ios::binary;
	fs.open (name.c_str (), mode);
	if (! fs.is_open () )
	{
		if (showdebuginfo () )
			cerr << "open (" << name << ", " << mode <<
				") failed: " << strerror (errno) <<
				endl;
		throw ErrFileNotFound;
	}
}

bool BlFileRegular::eof ()
{
	int c= fs.get ();
	if (! fs || c == EOF)
		return true;
	fs.unget ();
	return false;
}

void BlFileRegular::flush ()
{
	fs << std::flush;
}

void BlFileRegular::getline (std::string & str, bool)
{
	std::getline (fs, str);
	if (! fs)
	{
		if (fs.eof () )
			throw ErrPastEof;
		else
			throw ErrFileRead;
	}
}

std::string BlFileRegular::getkey ()
{
	return read (1);
}

std::string BlFileRegular::inkey ()
{
	int c= fs.get ();
	if (! fs || c == EOF)
	{
		fs.clear ();	// Allow further reads.
		return std::string ();
	}
	return std::string (1, static_cast <char> (c) );
}

std::string BlFileRegular::read (size_t n)
{
	util::auto_buffer <char> buf (n);
	fs.read (buf, n);
	if (! fs)
	{
		if (fs.eof () )
			throw ErrPastEof;
		else
			throw ErrFileRead;
	}
	return std::string (buf, n);
}

std::ostream & BlFileRegular::ofs ()
{
	return fs;
}

BlInteger BlFileRegular::lof ()
{
	if (getmode () & Input)
		return lengthoffileread (fs);
	else
		return lengthoffilewrite (fs);
}

//***********************************************
//              BlFileRandom
//***********************************************

class BlFileRandom : public BlFile {
public:
	BlFileRandom (const std::string & name, size_t record_len);
	bool isfile () const { return true; }
	bool eof ();
	virtual size_t loc ();
	void put (size_t pos);
	void get (size_t pos);
	void field_clear ();
	void field (const std::vector <field_element> & elem);
	void field_append (const std::vector <field_element> & elem);
	virtual bool assign (const std::string & name, const Dimension & dim,
		const std::string & value, Align align);
	virtual bool assign_mid (const std::string & name,
		const Dimension & dim,
		const std::string & value,
		size_t inipos, std::string::size_type len);
	struct field_chunk {
		std::string name;
		Dimension dim;
		size_t pos;
		size_t size;
		inline void getvar (char * buf) const;
	};
	typedef std::vector <field_chunk> vchunk;
	BlInteger lof ();
private:
	std::fstream fs;
	size_t len;
	size_t len_assigned;
	bool used;
	size_t actual;
	//auto_array buf;
	util::auto_buffer <char> buf;
	vchunk chunk;
};

BlFile * newBlFileRandom (const std::string & name, size_t record_len)
{
	return new BlFileRandom (name, record_len);
}

inline void BlFileRandom::field_chunk::getvar (char * buf) const
{
	if (dim.empty () )
		assignvarstring (name, std::string (buf + pos, size) );
	else
		assigndimstring (name, dim, std::string (buf + pos, size) );
}

BlFileRandom::BlFileRandom (const std::string & name, size_t record_len) :
	BlFile (Random),
	len (record_len),
	len_assigned (0),
	used (false),
	actual (0),
	buf (record_len)
{
	using std::ios;

	const ios::openmode iobin (ios::in | ios::out | ios::binary);
	fs.open (name.c_str (), iobin);
	if (! fs.is_open () )
	{
		// This can be necessary or not depending of the compiler
		// to create the file if not exist. We test it, anyway.
		if (errno == ENOENT)
		{
			fs.clear ();
			fs.open (name.c_str (), ios::out | ios::binary);
			if (fs.is_open () )
			{
				fs.close ();
				fs.open (name.c_str (), iobin);
			}

		}
		if (! fs.is_open () )
		{
			if (showdebuginfo () )
				cerr << "open " << name << " failed: " <<
					strerror (errno) << endl;
			throw ErrFileNotFound;
		}
	}
	std::fill_n (buf.begin (), len, '\0');
}

bool BlFileRandom::eof ()
{
	fs.clear ();
	fs.seekg (actual * len);
	if (! fs)
	{
		if (fs.eof () )
			return true;
		throw ErrFileRead;
	}
	char testchar;
	fs.read (& testchar, 1);
	if (! fs)
	{
		if (fs.eof () )
			return true;
		throw ErrFileRead;
	}
	return fs.gcount () != 1;
}

size_t BlFileRandom::loc ()
{
	return used ? actual + 1 : size_t (0);
}


void BlFileRandom::put (size_t pos)
{
	used= true;
	if (pos != 0)
	{
		actual= pos - 1;
		fs.seekp (actual * len);
	}
	fs.write (buf, len);
	++actual;
}

namespace {

class GetVar {
public:
	GetVar (char * buf) : buf (buf) { }
	void operator () (const BlFileRandom::field_chunk & chunk)
	{
		chunk.getvar (buf);
	}
private:
	char * buf;
};

} // namespace

void BlFileRandom::get (size_t pos)
{
	using std::string;

	used= true;
	if (pos != 0)
		actual= pos - 1;
	fs.clear ();
	fs.seekg (actual * len);
	fs.read (buf, len);
	std::streamsize r= fs.gcount ();
	if (r < std::streamsize (len) )
		std::fill_n (buf.begin () + r, len - r, ' ');
	std::for_each (chunk.begin (), chunk.end (), GetVar (buf) );
	++actual;
}

namespace {

class MakeChunk {
public:
	MakeChunk (size_t len, size_t & pos) :
		len (len),
		pos (pos)
	{ }
	BlFileRandom::field_chunk operator ()
		(const BlFile::field_element & elem)
	{
		size_t size= elem.size;
		BlFileRandom::field_chunk fc;
		fc.name= elem.name;
		fc.dim= elem.dim;
		fc.pos= pos;
		fc.size= size;
		pos+= size;
		if (pos > len)
			throw ErrFieldOverflow;
		return fc;
	}
	size_t getpos () const { return pos; }
private:
	const size_t len;
	size_t & pos;
};

} // namespace

void BlFileRandom::field_clear ()
{
	chunk.clear ();
	len_assigned= 0;
}

void BlFileRandom::field_append (const std::vector <field_element> & elem)
{
	TRACEFUNC (tr, "BlFileRandom::field_append");

	// Create the field chunks.
	//MakeChunk maker (len, len_assigned);
	std::transform (elem.begin (), elem.end (),
		std::back_inserter (chunk),
		MakeChunk (len, len_assigned) );
	// Check the assigned mark.
	TRMESSAGE (tr, std::string ("len_assigned= ") +
		util::to_string (len_assigned) );

	// Assign initial values to the field variables (filled with
	// zeroes if the file is opened and not touched).
	// Do it for all fields, not only the added now.
	std::for_each (chunk.begin (), chunk.end (), GetVar (buf) );
}

void BlFileRandom::field (const std::vector <field_element> & elem)
{
	field_clear ();
	field_append (elem);
}

namespace {

class field_var_is {
public:
	field_var_is (const std::string & name, const Dimension & dim) :
		name (name), dim (dim)
	{ }
	bool operator () (const BlFileRandom::field_chunk & chunk)
	{
		return chunk.name == name && chunk.dim == dim;
	}
private:
	const std::string & name;
	const Dimension & dim;
};

} // namespace

bool BlFileRandom::assign (const std::string & name, const Dimension & dim,
		const std::string & value, Align align)
{
	vchunk::iterator pe= std::find_if (chunk.begin (), chunk.end (),
		field_var_is (name, dim) );
	if (pe == chunk.end () )
		return false;
	if (pe->size == 0)
		return true;
	char * init= buf + pe->pos;
	std::string str;

	// Changed the behaviour: now does the same as gwbasic.

	#if 0
	std::string::size_type l= value.size ();
	if (align == AlignLeft)
	{
		if (l < pe->size)
			str= value + std::string (pe->size - l, ' ');
		else
			str= value.substr (0, pe->size);
	}
	else
	{
		if (l < pe->size)
			str= std::string (pe->size - l, ' ') + value;
		else
			str= value.substr (l - pe->size);
	}
	#else
	if (align == AlignLeft)
		str= util::stringlset (value, pe->size);
	else
		str= util::stringrset (value, pe->size);
	#endif
	ASSERT (str.size () == pe->size);
	#if 0
	std::copy (str.begin (), str.end (), init);
	#else
	str.copy (init, pe->size);
	#endif
	// Now also assign to the standalone var, so that the
	// value assiged can be checked.
	if (dim.empty () )
		assignvarstring (name, str);
	else
		assigndimstring (name, dim, str);
	return true;
}

bool BlFileRandom::assign_mid (const std::string & name,
	const Dimension & dim,
	const std::string & value,
	size_t inipos, std::string::size_type len)
{
	vchunk::iterator pe= std::find_if (chunk.begin (), chunk.end (),
		field_var_is (name, dim) );
	if (pe == chunk.end () )
		return false;
	const size_t size= pe->size;
	if (size == 0)
		return true;
	if (inipos >= size)
		return true;
	char * const init= buf + pe->pos;
	char * const initmid= init + inipos;
	size_t l= size - inipos;
	if (len > l)
		len= l;
	std::string str= value.substr (0, len);
	if (str.size () < len)
		str+= std::string (len - str.size (), ' ');
	str.copy (initmid, str.size () );
	// Now also assign to the standalone var, so that the
	// value assiged can be checked.
	if (dim.empty () )
		assignvarstring (name, std::string (init, size) );
	else
		assigndimstring (name, dim, std::string (init, size) );
	return true;
}

BlInteger BlFileRandom::lof ()
{
	return lengthoffileread (fs);
}

} // namespace file

} // namespace blassic

// Fin de file.cpp
