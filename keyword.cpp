// keyword.cpp
// Revision 7-feb-2005

#include "keyword.h"
#include "util.h"
#include "sysvar.h"
#include "trace.h"

#include <iostream>
#include <set>
#include <algorithm>
#include <iterator>
#include <functional>
#include <cctype>

// I don't understand why, but with this using the older version
// of C++ Builder fails.
#if __BORLANDC__ >= 0x0560
using std::toupper;
#endif

namespace sysvar= blassic::sysvar;
using util::dim_array;


namespace {

struct keycode {
	std::string key;
	BlCode code;
	keycode (const char * str, BlCode c) :
		key (str), code (c)
	{ }
};

// Can't declare const on Borland or fail at instantiate find_if,
// don't know why.
#ifdef __BORLANDC__
#define const_keycode keycode
#else
#define const_keycode const keycode
#endif

#define m_keycode(n) keycode (#n, key ## n)

#define m_keycode_s(n) keycode (#n "$", key ## n ## _S)

const_keycode table []= {
	m_keycode (END),
	m_keycode (LIST),
	m_keycode (REM),
	m_keycode (LOAD),
	m_keycode (SAVE),
	m_keycode (EXIT),
	m_keycode (NEW),
	m_keycode (RUN),
	m_keycode (PRINT),
	m_keycode (FOR),
	m_keycode (NEXT),
	m_keycode (TO),
	m_keycode (STEP),
	m_keycode (IF),
	m_keycode (THEN),
	m_keycode (ELSE),
	m_keycode (TRON),
	m_keycode (TROFF),
	m_keycode (LET),
	m_keycode (GOTO),
	m_keycode (STOP),
	m_keycode (CONT),
	m_keycode (CLEAR),
	m_keycode (GOSUB),
	m_keycode (RETURN),
	m_keycode (POKE),
	m_keycode (DATA),
	m_keycode (READ),
	m_keycode (RESTORE),
	m_keycode (INPUT),
	m_keycode (LINE),
	m_keycode (RANDOMIZE),
	m_keycode (PLEASE),
	m_keycode (AUTO),
	m_keycode (DIM),
	m_keycode (SYSTEM),
	m_keycode (ON),
	m_keycode (ERROR),
	m_keycode (OPEN),
	m_keycode (CLOSE),
	m_keycode (OUTPUT),
	m_keycode (AS),
	m_keycode (LOCATE),
	m_keycode (CLS),
	m_keycode (APPEND),
	m_keycode (WRITE),
	m_keycode (MODE),
	m_keycode (MOVE),
	m_keycode (COLOR),
	m_keycode (GET),
	m_keycode (LABEL),
	m_keycode (DELIMITER),
	m_keycode (REPEAT),
	m_keycode (UNTIL),
	m_keycode (WHILE),
	m_keycode (WEND),
	m_keycode (PLOT),
	m_keycode (POPEN),
	m_keycode (RESUME),
	m_keycode (DELETE),
	m_keycode (LOCAL),
	m_keycode (RANDOM),
	m_keycode (PUT),
	m_keycode (FIELD),
	m_keycode (LSET),
	m_keycode (RSET),
	m_keycode (SOCKET),
	m_keycode (DRAW),
	m_keycode (DEF),
	m_keycode (FN),
	m_keycode (ERASE),
	m_keycode (SWAP),
	m_keycode (SYMBOL),
	m_keycode (ZONE),
	m_keycode (POP),
	m_keycode (NAME),
	m_keycode (KILL),
	m_keycode (FILES),
	m_keycode (PAPER),
	m_keycode (PEN),
	m_keycode (SHELL),
	m_keycode (MERGE),
	m_keycode (CHDIR),
	m_keycode (MKDIR),
	m_keycode (RMDIR),
	m_keycode (BREAK),
	m_keycode (SYNCHRONIZE),
	m_keycode (PAUSE),
	m_keycode (CHAIN),
	m_keycode (STR),
	m_keycode (REAL),
	m_keycode (ENVIRON),
	m_keycode (EDIT),
	m_keycode (DRAWR),
	m_keycode (PLOTR),
	m_keycode (MOVER),
	m_keycode (POKE16),
	m_keycode (POKE32),
	m_keycode (RENUM),
	m_keycode (CIRCLE),
	m_keycode (MASK),
	m_keycode (WINDOW),
	m_keycode (GRAPHICS),
	m_keycode (AFTER),
	m_keycode (BEEP),
	m_keycode (DEFINT),
	m_keycode (DEFSTR),
	m_keycode (DEFREAL),
	m_keycode (DEFSNG),
	m_keycode (DEFDBL),
	m_keycode (INK),
	m_keycode (SET_TITLE),
	m_keycode (TAG),
	m_keycode (TAGOFF),
	m_keycode (ORIGIN),
	m_keycode (DEG),
	m_keycode (RAD),
	m_keycode (INVERSE),
	m_keycode (IF_DEBUG),
	m_keycode (LPRINT),
	m_keycode (LLIST),
	m_keycode (WIDTH),
	m_keycode (BRIGHT),
	m_keycode (BINARY),
	m_keycode (DRAWARC),
	m_keycode (PULL),
	m_keycode (PAINT),
	m_keycode (FREE_MEMORY),
	m_keycode (SCROLL),
	m_keycode (ZX_PLOT),
	m_keycode (ZX_UNPLOT),

	m_keycode_s (MID),
	m_keycode_s (LEFT),
	m_keycode_s (RIGHT),
	m_keycode_s (CHR),
	m_keycode_s (ENVIRON),
	m_keycode_s (STRING),
	m_keycode_s (OSFAMILY),
	m_keycode_s (HEX),
	m_keycode_s (SPACE),
	m_keycode_s (UPPER),
	m_keycode_s (LOWER),
	m_keycode_s (STR),
	m_keycode_s (OCT),
	m_keycode_s (BIN),
	m_keycode_s (INKEY),
	m_keycode_s (PROGRAMARG),
	m_keycode_s (DATE),
	m_keycode_s (TIME),
	m_keycode_s (INPUT),
	m_keycode_s (MKI),
	m_keycode_s (MKS),
	m_keycode_s (MKD),
	m_keycode_s (MKL),
	m_keycode_s (TRIM),
	m_keycode_s (LTRIM),
	m_keycode_s (RTRIM),
	m_keycode_s (OSNAME),
	m_keycode_s (FINDFIRST),
	m_keycode_s (FINDNEXT),
	m_keycode_s (COPYCHR),
	m_keycode_s (STRERR),
	m_keycode_s (DEC),
	m_keycode_s (VAL),
	m_keycode_s (SCREEN),
	m_keycode_s (MKSMBF),
	m_keycode_s (MKDMBF),
	m_keycode_s (REGEXP_REPLACE),
	m_keycode_s (UCASE),
	m_keycode_s (LCASE),

	m_keycode (ASC),
	m_keycode (LEN),
	m_keycode (PEEK),
	m_keycode (PROGRAMPTR),
	m_keycode (RND),
	m_keycode (INT),
	m_keycode (SIN),
	m_keycode (COS),
	m_keycode (PI),
	m_keycode (TAN),
	m_keycode (SQR),
	m_keycode (ASIN),
	m_keycode (ACOS),
	m_keycode (INSTR),
	m_keycode (ATAN),
	m_keycode (ABS),
	m_keycode (USR),
	m_keycode (VAL),
	m_keycode (EOF),
	m_keycode (VARPTR),
	m_keycode (SYSVARPTR),
	m_keycode (SGN),
	m_keycode (LOG),
	m_keycode (LOG10),
	m_keycode (EXP),
	m_keycode (TIME),
	m_keycode (ERR),
	m_keycode (ERL),
	m_keycode (CVI),
	m_keycode (CVS),
	m_keycode (CVD),
	m_keycode (CVL),
	m_keycode (MIN),
	m_keycode (MAX),
	m_keycode (CINT),
	m_keycode (FIX),
	m_keycode (XMOUSE),
	m_keycode (YMOUSE),
	m_keycode (XPOS),
	m_keycode (YPOS),
	m_keycode (PEEK16),
	m_keycode (PEEK32),
	m_keycode (RINSTR),
	m_keycode (FIND_FIRST_OF),
	m_keycode (FIND_LAST_OF),
	m_keycode (FIND_FIRST_NOT_OF),
	m_keycode (FIND_LAST_NOT_OF),
	m_keycode (SINH),
	m_keycode (COSH),
	m_keycode (TANH),
	m_keycode (ASINH),
	m_keycode (ACOSH),
	m_keycode (ATANH),
	m_keycode (ATAN2),
	m_keycode (TEST),
	m_keycode (TESTR),
	m_keycode (POS),
	m_keycode (VPOS),
	m_keycode (LOF),
	m_keycode (FREEFILE),
	m_keycode (INKEY),
	m_keycode (ROUND),
	m_keycode (CVSMBF),
	m_keycode (CVDMBF),
	m_keycode (REGEXP_INSTR),
	m_keycode (ALLOC_MEMORY),
	m_keycode (LOC),

	m_keycode (NOT),
	m_keycode (OR),
	m_keycode (AND),
	m_keycode (TAB),
	m_keycode (SPC),
	m_keycode (AT),
	m_keycode (XOR),
	m_keycode (MOD),
	m_keycode (USING),

	keycode ("<>",                keyDISTINCT),
	keycode ("<=",                keyMINOREQUAL),
	keycode (">=",                keyGREATEREQUAL),
	keycode ("=<",                keyEQUALMINOR),
	keycode ("=>",                keyEQUALGREATER),
	keycode ("><",                keyGREATERMINOR),

	// table_end points here, then if find_if (table, table_end, ...)
	// fails the result is:
	keycode ("???",         0)
};

const_keycode * table_end= table + dim_array (table) - 1;

class key_is : public std::unary_function <keycode, bool> {
public:
	key_is (const std::string & str) : str (str)
	{ }
	bool operator () (const keycode & k) const
	{ return k.key == str; }
private:
	const std::string & str;
};

class code_is : public std::unary_function <keycode, bool> {
public:
	code_is (BlCode code) : code (code)
	{ }
	bool operator () (const keycode & k) const
	{ return k.code == code; }
private:
	BlCode code;
};

inline std::string stringupper (const std::string & str)
{
	std::string u (str.size (), 0);
	std::transform (str.begin (), str.end (), u.begin (), toupper);
	return u;
}

std::set <std::string> exclude;

} // namespace

void excludekeyword (const std::string & str)
{
	TRACEFUNC (tr, "excludekeyword");

	std::string stru= stringupper (str);
	if (find_if (table, table_end, key_is (stru) ) != table_end)
	{
		exclude.insert (stru);
		TRMESSAGE (tr, std::string ("Excluding ") + stru);
	}
}

BlCode keyword (const std::string & str)
{
	std::string stru= stringupper (str);
	BlCode code= std::find_if (table, table_end, key_is (stru) )->code;
	if (code != 0)
		if (exclude.find (stru) != exclude.end () )
			return 0;
	return code;
}

std::string decodekeyword (BlCode s)
{
	if (s == keyGOTO || s == keyGOSUB)
	{
		if (sysvar::hasFlags2 (sysvar::SeparatedGoto) )
			return (s == keyGOTO) ?
				"GO TO" :
				"GO SUB";
	}
	return std::find_if (table, table_end, code_is (s) )->key;
}

// Fin de keyword.cpp
