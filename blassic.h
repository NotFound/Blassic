#ifndef INCLUDE_BLASSIC_H
#define INCLUDE_BLASSIC_H

// blassic.h
// Revision 6-feb-2005

// Now do not use hash_map at all.
//#if ! defined __BORLANDC__ && __GNUC__ < 3
//#if defined __GNUC__
//#define USE_HASH_MAP
//#endif


#if defined _Windows || defined __CYGWIN__ || defined __MINGW32__


#define BLASSIC_USE_WINDOWS

#ifndef __MT__
#define __MT__
#endif


#else


// This is controlled with the configure option --disable-graphics
#ifndef BLASSIC_CONFIG_NO_GRAPHICS

#define BLASSIC_USE_X

#endif


#ifdef __linux__

// Uncomment next #define if you want to use the svgalib option
// or tell it to configure.
// Support for svgalib is currently outdated.
//#define BLASSIC_USE_SVGALIB

#endif

#endif

// Borland define _M_IX86, gcc define __i386__
#if defined (_M_IX86) || defined (__i386__)

#define BLASSIC_INTEL

// In other processor used the endianess is different and / or
// there are restrictions of alignment.

#endif


#ifdef __BORLANDC__
#pragma warn -8027
#endif


#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <string>
#include <vector>
#include <climits>

// Now defined here instead of in var.h to reduce dependencies.

enum VarType { VarUndef, VarNumber, VarInteger,
	VarString, VarStringSlice };

inline bool is_numeric_type (VarType v)
{ return v == VarNumber || v == VarInteger; }

inline bool is_string_type (VarType v)
{ return v == VarString || v == VarStringSlice; }


typedef unsigned char BlChar;
typedef unsigned short BlCode;
typedef double BlNumber;

#if ULONG_MAX == 4294967295UL

typedef long BlInt32;
const BlInt32 BlInt32Max= LONG_MAX;
const BlInt32 BlInt32Min= LONG_MIN;

typedef unsigned long BlUint32;
const BlUint32 BlUint32Max= ULONG_MAX;

#elif UINT_MAX == 4294967295UL

typedef int BlInt32;
const BlInt32 BlInt32Max= INT_MAX;
const BlInt32 BlInt32Min= INT_MIN;

typedef unsigned int BlUint32;
const BlUint32 BlUint32Max= UINT_MAX;

#elif USHRT_MAX == 4294967295UL

typedef short BlInt32;
const BlInt32 BlInt32Max= SHRT_MAX;
const BlInt32 BlInt32Min= SHRT_MIN;

typedef unsigned short BlUint32;
const BlUint32 BlUint32Max= USHRT_MAX;

#else

#error Unsupported platform

#endif

typedef BlInt32 BlInteger;
typedef BlUint32 BlLineNumber;
typedef BlUint32 BlLineLength;

const BlInteger BlIntegerMax= BlInt32Max;
const BlInteger BlIntegerMin= BlInt32Min;

// We limit the max line number as if it were signed.
const BlLineNumber BlMaxLineNumber= BlIntegerMax;

// Special line number values.

const BlLineNumber LineEndProgram= BlUint32Max;
const BlLineNumber LineBeginProgram= BlUint32Max - 1;
const BlLineNumber LineDirectCommand= BlUint32Max - 2;
const BlLineNumber LineNoDelete= BlUint32Max - 3;

typedef unsigned short BlChunk;
typedef unsigned short BlErrNo;
typedef unsigned short BlChannel;

const BlChannel DefaultChannel= 0;
const BlChannel PrinterChannel= 65535;

class ProgramPos {
public:
	ProgramPos () :
		num (LineEndProgram), chunk (0)
	{ }
	ProgramPos (BlLineNumber num) :
		num (num), chunk (0)
	{ }
	ProgramPos (BlLineNumber num, BlChunk chunk) :
		num (num), chunk (chunk)
	{ }
	void operator= (BlLineNumber num)
	{
		this->num= num;
		chunk= 0;
	}
	void nextchunk () { ++chunk; }
	void nextline () { ++num; chunk= 0; }
	operator bool () { return num != 0 || chunk != 0; }
	BlLineNumber getnum () const { return num; }
	BlChunk getchunk () const { return chunk; }
	void setchunk (BlChunk n) { chunk= n; }
private:
	BlLineNumber num;
	BlChunk chunk;
};

// Global variables:

//extern BlLineNumber blnAuto, blnAutoInc;

extern bool fInterrupted;

extern const std::string strPrompt;

// version.cpp
namespace version {

extern const unsigned short Major, Minor, Release;

} // namespace version

class Exit {
public:
	Exit (int ncode= 0);
	int code () const;
private:
	int exitcode;
};

std::string getprogramarg (size_t n);
void setprogramargs (const std::vector <std::string> & nargs);

inline BlInteger peek16 (const BlChar * p)
{
	#ifdef BLASSIC_INTEL
	return * reinterpret_cast <const unsigned short *> (p);
	#else
	return p [0] | (static_cast <unsigned short> (p [1]) << 8);
	#endif
}

inline void poke16 (BlChar * p, short n)
{
	#ifdef BLASSIC_INTEL
	* reinterpret_cast <short *> (p)= n;
	#else
	p [0]= BlChar (n & 0xFF);
	p [1]= BlChar ( (n >> 8) & 0xFF);
	#endif
}

inline BlInteger peek32 (const BlChar * p)
{
	#ifdef BLASSIC_INTEL
	return * reinterpret_cast <const BlInteger *> (p);
	#else
	return p [0] |
		(BlInteger (p [1]) << 8) |
		(BlInteger (p [2]) << 16) |
		(BlInteger (p [3]) << 24);
	#endif
}

inline void poke32 (BlChar * p, BlUint32 n)
{
	#ifdef BLASSIC_INTEL
	* reinterpret_cast <BlInteger *> (p)= n;
	#else
	p [0]= BlChar (n & 0xFF);
	p [1]= BlChar ( (n >> 8) & 0xFF);
	p [2]= BlChar ( (n >> 16) & 0xFF);
	p [3]= BlChar (n >> 24);
	#endif
}

inline void poke32 (BlChar * p, BlInteger n)
{
	poke32 (p, static_cast <BlUint32> (n) );
}

namespace blassic {

void idle (); // Implemented in graphics.cpp

} // namespace blassic

#endif

// Fin de blassic.h
