#ifndef INCLUDE_BLASSIC_ERROR_H
#define INCLUDE_BLASSIC_ERROR_H

// error.h
// Revision 7-feb-2005


#include "blassic.h"

#include <string>
#include <iostream>


const BlErrNo
	ErrNoError=             0,
	ErrSyntax=              1,
	ErrMismatch=            2,
	ErrGosubWithoutReturn=  3,
	ErrReturnWithoutGosub=  4,
	ErrNextWithoutFor=      5,
	ErrNotImplemented=      6,
	ErrDivZero=             7,
	ErrDataExhausted=       8,
	ErrInvalidCommand=      9,
	ErrPolite=             10,
	ErrBadSubscript=       11,
	ErrOutMemory=          12,
	ErrAlreadyDim=         13,
	ErrNoContinue=         14,
	ErrFileNumber=         15,
	ErrFileMode=           16,
	ErrFileAlreadyOpen=    17,
	ErrFileRead=           18,
	ErrFileWrite=          19,
	ErrUntilWithoutRepeat= 20,
	ErrWendWithoutWhile=   21,
	ErrWhileWithoutWend=   22,
	ErrBlassicInternal=    23,
	ErrNoDynamicLibrary=   24,
	ErrNoDynamicSymbol=    25,
	ErrCannotResume=       26,
	ErrNoLabel=            27,
	ErrMisplacedLocal=     28,
	ErrFieldOverflow=      29,
	ErrFileNotFound=       30,
	ErrLineExhausted=      31,
	ErrFunctionNoDefined=  32,
	ErrIncompleteDef=      33,
	ErrInvalidDirect=      34,
	ErrBadRecord=          35,
	ErrFunctionCall=       36,
	ErrSocket=             37,
	ErrRenameFile=         38,
	ErrOperatingSystem=    39,
	ErrPastEof=            40,
	ErrNoGraphics=         41,
	ErrImproperArgument=   42,
	ErrDomain=             43,
	ErrRange=              44,
	ErrLineNotExist=       45,
	ErrFnRecursion=        46,
	ErrOverflow=           48,
	ErrRegexp=             49,
	ErrDynamicUnsupported= 50,
	ErrRepeatWithoutUntil= 51,
	ErrUnexpectedFnEnd=    52,
	ErrNoFnEnd=            53,
	ErrDuplicateLabel=     54,
	ErrNoTeDejo=           55;

class BlError {
public:
	BlError ();
	BlError (BlErrNo nerr);
	BlError (BlErrNo nerr, ProgramPos npos);
	void clear ();
	void set (BlErrNo nerr, ProgramPos npos);
	void seterr (BlErrNo nerr);
	BlErrNo geterr () const;
	ProgramPos getpos () const;
	friend std::ostream & operator << (std::ostream & os,
		const BlError & be);
private:
	BlErrNo err;
	ProgramPos pos;
};

class BlBreak { };

class BlBreakInPos {
	ProgramPos pos;
public:
	BlBreakInPos (ProgramPos pos);
	ProgramPos getpos () const;
};

std::ostream & operator << (std::ostream & os, const BlBreakInPos & bbip);

std::string ErrStr (BlErrNo err);

bool showdebuginfo ();

#endif

// Fin de error.h
