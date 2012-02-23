// error.cpp
// Revision 7-feb-2005

#include "error.h"
#include "sysvar.h"
#include "util.h"

#include <sstream>
#include <algorithm>

namespace sysvar= blassic::sysvar;


namespace {

struct errcode {
	const BlErrNo err;
	const char * const str;
	errcode (BlErrNo nerr, const char * nstr) :
		err (nerr),
		str (nstr)
	{ }
};

// Can't declare const or Borland can't expand find_if. Why?
errcode table []= {
	errcode (ErrNoError,            "No error"),
	errcode (ErrSyntax,             "Syntax horror"),
	errcode (ErrMismatch,           "Type mismatch"),
	errcode (ErrGosubWithoutReturn, "GOSUB without RETURN"),
	errcode (ErrReturnWithoutGosub, "RETURN without GOSUB"),
	errcode (ErrNextWithoutFor,     "NEXT without FOR"),
	errcode (ErrNotImplemented,     "Not implemented"),
	errcode (ErrDivZero,            "Division by zero"),
	errcode (ErrDataExhausted,      "Data exhausted"),
	errcode (ErrInvalidCommand,     "Invalid command"),
	errcode (ErrPolite,             "Programmer is too polite"),
	errcode (ErrBadSubscript,       "Bad Subscript"),
	errcode (ErrOutMemory,          "Out of memory"),
	errcode (ErrAlreadyDim,         "Array already dimensioned"),
	errcode (ErrNoContinue,         "Cannot CONTinue"),
	errcode (ErrFileNumber,         "Bad file number"),
	errcode (ErrFileMode,           "Bad file mode"),
	errcode (ErrFileAlreadyOpen,    "File already open"),
	errcode (ErrFileRead,           "Error reading file"),
	errcode (ErrFileWrite,          "Error writing file"),
	errcode (ErrUntilWithoutRepeat, "UNTIL without REPEAT"),
	errcode (ErrWendWithoutWhile,   "WEND without WHILE"),
	errcode (ErrWhileWithoutWend,   "WHILE without WEND"),
	errcode (ErrBlassicInternal,    "Internal Blassic error"),
	errcode (ErrNoDynamicLibrary,   "Dynamic library not found"),
	errcode (ErrNoDynamicSymbol,    "Symbol not found in dynamic library"),
	errcode (ErrCannotResume,       "Cannot RESUME"),
	errcode (ErrNoLabel,            "Label does not exist"),
	errcode (ErrMisplacedLocal,     "LOCAL out of subroutine"),
	errcode (ErrFieldOverflow,      "FIELD overflow"),
	errcode (ErrFileNotFound,       "File not found"),
	errcode (ErrLineExhausted,      "Line numbers exhausted"),
	errcode (ErrFunctionNoDefined,  "User function undefined"),
	errcode (ErrIncompleteDef,      "User function incomplete"),
	errcode (ErrInvalidDirect,      "Invalid direct command"),
	errcode (ErrBadRecord,          "Bad record number"),
	errcode (ErrFunctionCall,       "Illegal function call"),
	errcode (ErrSocket,             "Socket error"),
	errcode (ErrRenameFile,         "Rename file error"),
	errcode (ErrOperatingSystem,    "Operating system error"),
	errcode (ErrPastEof,            "Input past EOF"),
	errcode (ErrNoGraphics,         "Graphics mode required"),
	errcode (ErrImproperArgument,   "Improper argument"),
	errcode (ErrDomain,             "Domain error"),
	errcode (ErrRange,              "Result out of range"),
	errcode (ErrLineNotExist,       "Line does not exist"),
	errcode (ErrFnRecursion,        "FN recursion too deep"),
	errcode (ErrOverflow,           "Overflow"),
	errcode (ErrRegexp,             "Bad regular expression"),
	errcode (ErrDynamicUnsupported, "Dymanic link not supported"),
	errcode (ErrRepeatWithoutUntil, "REPEAT without UNTIL"),
	errcode (ErrUnexpectedFnEnd,    "FN END outside DEF FN"),
	errcode (ErrNoFnEnd,            "FN without FN END"),
	errcode (ErrDuplicateLabel,     "Duplicate LABEL"),
	errcode (ErrNoTeDejo,           "That won't work"),
};

errcode * table_end= table + util::dim_array (table);

class is_err {
public:
	is_err (BlErrNo err) : err (err)
	{ }
	bool operator () (const errcode & ec) const
	{ return ec.err == err; }
private:
	BlErrNo err;
};

} // namespace

//***********************************************
//		class BlError
//***********************************************

BlError::BlError () :
	err (ErrNoError)
{ }

BlError::BlError (BlErrNo nerr) :
	err (nerr)
{ }

BlError::BlError (BlErrNo nerr, ProgramPos npos) :
	err (nerr),
	pos (npos)
{ }

void BlError::clear ()
{
	err= ErrNoError;
	pos= ProgramPos ();
}

void BlError::set (BlErrNo nerr, ProgramPos npos)
{
	err= nerr;
	pos= npos;
}

void BlError::seterr (BlErrNo nerr)
{
	err= nerr;
}

BlErrNo BlError::geterr () const
{
	return err;
}

ProgramPos BlError::getpos () const
{
	return pos;
}

std::ostream & operator << (std::ostream & os, const BlError & bl)
{
	os << ErrStr (bl.err);
	const BlLineNumber line= bl.getpos ().getnum ();
	if (line != LineDirectCommand)
		os << " in " << line;
	//os << '\n';
	return os;
}

//***********************************************
//		class BlBreakInPos
//***********************************************

BlBreakInPos::BlBreakInPos (ProgramPos pos) :
	pos (pos)
{ }

ProgramPos BlBreakInPos::getpos () const
{
	return pos;
}

std::ostream & operator << (std::ostream & os, const BlBreakInPos & bbip)
{
	os << "**BREAK**";
	BlLineNumber line= bbip.getpos ().getnum ();
	if (line != LineDirectCommand)
		os << " in " << line;
	return os;
}

//***********************************************
//		Other functions
//***********************************************

std::string ErrStr (BlErrNo err)
{
	const errcode * perr= std::find_if (table, table_end, is_err (err) );
	if (perr != table_end)
		return perr->str;
	std::ostringstream strbuf;
	strbuf << "Error " << err;
	return strbuf.str ();
}

bool showdebuginfo ()
{
	//return sysvar::getFlags1 () & sysvar::ShowDebugInfo;
	return sysvar::hasFlags1 (sysvar::ShowDebugInfo);
}

// Fin de error.cpp
