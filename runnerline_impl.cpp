// runnerline_impl.cpp
// Revision 9-feb-2005

#include "runnerline_impl.h"

#include "error.h"
#include "dynamic.h"
#include "runner.h"
#include "program.h"
#include "directory.h"
#include "sysvar.h"
#include "graphics.h"
#include "util.h"
#include "using.h"
#include "mbf.h"
#include "regexp.h"
#include "memory.h"
#include "trace.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <memory>
#include <cstdio>
#include <cerrno>
#include <ctime>
#include <cctype>
#include <iostream>

// cmath is not used because in some platforms asinh and others
// are declared only in math.h
#include <math.h>

#if defined __unix__ || defined __linux__
// Using uname on unix, linux and cygwin.
#include <sys/utsname.h>
#endif

#include <cassert>
#define ASSERT assert

using std::cerr;
using std::endl;
using std::auto_ptr;
using std::isalpha;

namespace sysvar= blassic::sysvar;
namespace onbreak= blassic::onbreak;

using namespace blassic::file;

using util::dim_array;
using util::touch;


namespace {

#if defined __unix__ || defined __linux__
// Even in windows with cygwin.
const char * os_family= "unix";

#else

const char * os_family= "windows";

#endif

// Workaround for a problem in some versions of gcc.
// Do not define zero as const, that does not solve the problem.
// But do not modify it! You have been warned!
double zero= 0.0;

#if 0
inline bool iscomp (BlCode code)
{
	return code == '=' || code == keyDISTINCT ||
		code == '<' || code == keyMINOREQUAL ||
		code == '>' || code == keyGREATEREQUAL;
}

inline bool islogical2 (BlCode code)
{
	return code == keyOR || code == keyAND || code == keyXOR;
}

#else

// Testing as macro for speed.

#define iscomp(code) \
	((code) == '=' || (code) == keyDISTINCT || \
		(code) == '<' || (code) == keyMINOREQUAL || \
		(code) == '>' || (code) == keyGREATEREQUAL)

#define islogical2(code) ((code) == keyOR || (code) == keyAND || \
	(code) == keyXOR)

#endif

#ifdef M_PIl
const BlNumber value_pi= M_PIl;
#else
const BlNumber value_pi= 4.0 * atan (1);
#endif

const BlNumber value_pi_div_180= value_pi / 180.0;
const BlNumber value_180_div_pi= 180.0 / value_pi;

// **********  Hyperbolic trigonometric arc functions  **********


namespace auxmath {

#if HAVE_DECL_ASINH
using ::asinh;
#else
double asinh (double x)
{
	return log (x + sqrt (x * x + 1) );
}
#endif

#if HAVE_DECL_ACOSH
using ::acosh;
#else
double acosh (double x)
{
	errno= 0;
	double r= sqrt (x * x - 1);
	if (errno != 0)
		return 0;
	return log (x + r );
}
#endif

#if HAVE_DECL_ATANH
using ::atanh;
#else
double atanh (double x)
{
	return log ( (1 + x) / (1 - x) ) / 2;
}
#endif

} // namespace auxmath


// ***************** Auxiliary math functions *************

double auxFIX (double n)
{
	double r;
	modf (n, & r);
	return r;
}

} // namespace


RunnerLineImpl::RunnerLineImpl
	(Runner & runner, const CodeLine & newcodeline) :
		RunnerLine (runner, newcodeline),
		program (runner.getprogram () ),
		pdirectory (0),
		fInElse (false)
{
	TRACEFUNC (tr, "RunnerLineImpl::RunnerLineImpl");
}

RunnerLineImpl::RunnerLineImpl (Runner & runner) :
		RunnerLine (runner),
		program (runner.getprogram () ),
		fInElse (false)
{
	TRACEFUNC (tr, "RunnerLineImpl::RunnerLineImpl");
}

RunnerLineImpl::~RunnerLineImpl ()
{
	TRACEFUNC (tr, "RunnerLineImpl::~RunnerLineImpl");
}

RunnerLine * newRunnerLine (Runner & runner, const CodeLine & newcodeline)
{
	return new RunnerLineImpl (runner, newcodeline);
}

#ifdef ONE_TABLE

#define insfunc_element(elem) \
	{key##elem, \
		{ & RunnerLineImpl::do_##elem, \
		& RunnerLineImpl::valsyntax_error} }

#define insfunc_alias(elem,alias) \
	{key##elem, \
		{ & RunnerLineImpl::do_##alias, \
		& RunnerLineImpl::valsyntax_error} }

#define valfunc_element(elem) \
	{key##elem, \
		{ & RunnerLineImpl::syntax_error, \
		& RunnerLineImpl::val_##elem} }

#define valfunc_alias(elem,alias) \
	{key##elem, \
		{ & RunnerLineImpl::syntax_error, \
		& RunnerLineImpl::val_##alias} }

#define mixfunc_element(elem) \
	{key##elem, \
		{ & RunnerLineImpl::do_##elem, \
		& RunnerLineImpl::val_##elem} }


const RunnerLineImpl::tfunctions_t RunnerLineImpl::tfunctions []=
{
	valfunc_element (OpenPar),
	insfunc_element (Colon),

	insfunc_element (END),
	insfunc_element (LIST),
	insfunc_element (REM),
	insfunc_element (LOAD),
	insfunc_element (SAVE),
	insfunc_element (NEW),
	insfunc_element (EXIT),
	insfunc_element (RUN),
	insfunc_element (PRINT),
	insfunc_element (FOR),
	insfunc_element (NEXT),
	insfunc_element (IF),
	insfunc_element (ELSE),
	insfunc_element (TRON),
	insfunc_element (TROFF),
	mixfunc_element (LET),
	insfunc_element (GOTO),
	insfunc_element (STOP),
	insfunc_element (CONT),
	insfunc_element (CLEAR),
	insfunc_element (GOSUB),
	insfunc_element (RETURN),
	insfunc_element (POKE),
	insfunc_element (DATA),
	insfunc_element (READ),
	insfunc_element (RESTORE),
	insfunc_element (INPUT),
	insfunc_element (LINE),
	insfunc_element (RANDOMIZE),
	insfunc_element (PLEASE),
	insfunc_element (AUTO),
	insfunc_element (DIM),
	insfunc_element (SYSTEM),
	insfunc_element (ON),
	insfunc_element (ERROR),
	insfunc_element (OPEN),
	insfunc_element (CLOSE),
	insfunc_element (LOCATE),
	insfunc_element (CLS),
	insfunc_element (WRITE),
	insfunc_element (MODE),
	insfunc_element (MOVE),
	insfunc_element (COLOR),
	insfunc_element (GET),
	mixfunc_element (LABEL),
	insfunc_element (DELIMITER),
	insfunc_element (REPEAT),
	insfunc_element (UNTIL),
	insfunc_element (WHILE),
	insfunc_element (WEND),
	insfunc_element (PLOT),
	insfunc_element (POPEN),
	insfunc_element (RESUME),
	insfunc_element (DELETE),
	insfunc_element (LOCAL),
	insfunc_element (PUT),
	insfunc_element (FIELD),
	insfunc_element (LSET),

	// Lset and rset use same function.
	insfunc_alias (RSET, LSET),

	insfunc_element (SOCKET),
	insfunc_element (DRAW),
	insfunc_element (DEF),
	mixfunc_element (FN),
	insfunc_element (ERASE),
	insfunc_element (SWAP),
	insfunc_element (SYMBOL),
	insfunc_element (ZONE),
	insfunc_element (POP),
	insfunc_element (NAME),
	insfunc_element (KILL),
	insfunc_element (FILES),
	insfunc_element (PAPER),
	insfunc_element (PEN),
	insfunc_element (SHELL),
	insfunc_element (MERGE),
	insfunc_element (CHDIR),
	insfunc_element (MKDIR),
	insfunc_element (RMDIR),
	insfunc_element (SYNCHRONIZE),
	insfunc_element (PAUSE),
	insfunc_element (CHAIN),
	insfunc_element (ENVIRON),
	insfunc_element (EDIT),
	insfunc_element (DRAWR),
	insfunc_element (PLOTR),
	insfunc_element (MOVER),
	insfunc_element (POKE16),
	insfunc_element (POKE32),
	insfunc_element (RENUM),
	insfunc_element (CIRCLE),
	insfunc_element (MASK),
	insfunc_element (WINDOW),
	insfunc_element (GRAPHICS),
	insfunc_element (BEEP),
	insfunc_element (DEFINT),

	// DEFINT, DEFSTR, DEFREAL, DEFSNG and DEFDBL use same function.
	insfunc_alias (DEFSTR, DEFINT),
	insfunc_alias (DEFREAL, DEFINT),
	insfunc_alias (DEFSNG, DEFINT),
	insfunc_alias (DEFDBL, DEFINT),

	insfunc_element (INK),
	insfunc_element (SET_TITLE),
	insfunc_element (TAG),

	// TAG and TAGOFF use same function.
	insfunc_alias (TAGOFF, TAG),

	insfunc_element (ORIGIN),
	insfunc_element (DEG),

	// DEG and RAD use same function.
	insfunc_alias (RAD, DEG),

	insfunc_element (INVERSE),
	insfunc_element (IF_DEBUG),

	// LPRINT and PRINT use same function.
	insfunc_alias (LPRINT, PRINT),

	insfunc_element (LLIST),
	insfunc_element (WIDTH),
	insfunc_element (BRIGHT),
	insfunc_element (DRAWARC),
	insfunc_element (PULL),
	insfunc_element (PAINT),
	insfunc_element (FREE_MEMORY),
	insfunc_element (SCROLL),
	insfunc_element (ZX_PLOT),
	insfunc_element (ZX_UNPLOT),

	mixfunc_element (MID_S),
	valfunc_element (LEFT_S),
	valfunc_element (RIGHT_S),
	valfunc_element (CHR_S),
	valfunc_element (ENVIRON_S),
	valfunc_element (STRING_S),
	valfunc_element (OSFAMILY_S),
	valfunc_element (HEX_S),
	valfunc_element (SPACE_S),
	valfunc_element (UPPER_S),
	valfunc_element (LOWER_S),
	valfunc_element (STR_S),
	valfunc_element (OCT_S),
	valfunc_element (BIN_S),
	valfunc_element (INKEY_S),
	mixfunc_element (PROGRAMARG_S),
	valfunc_element (DATE_S),
	valfunc_element (TIME_S),
	valfunc_element (INPUT_S),
	valfunc_element (MKI_S),
	valfunc_element (MKS_S),
	valfunc_element (MKD_S),
	valfunc_element (MKL_S),
	valfunc_element (TRIM_S),
	valfunc_element (LTRIM_S),
	valfunc_element (RTRIM_S),
	valfunc_element (OSNAME_S),
	valfunc_element (FINDFIRST_S),
	valfunc_element (FINDNEXT_S),
	valfunc_element (COPYCHR_S),
	valfunc_element (STRERR_S),
	valfunc_element (DEC_S),
	valfunc_element (VAL_S),
	valfunc_element (SCREEN_S),
	valfunc_element (MKSMBF_S),
	valfunc_element (MKDMBF_S),
	valfunc_element (REGEXP_REPLACE_S),
	// UCASE$ and LCASE$ are alias for UPPER$ and LOWER$
	valfunc_alias (UCASE_S, UPPER_S),
	valfunc_alias (LCASE_S, LOWER_S),

	valfunc_element (ASC),
	valfunc_element (LEN),
	valfunc_element (PEEK),
	valfunc_element (PROGRAMPTR),
	valfunc_element (RND),
	valfunc_element (INT),
	valfunc_element (SIN),
	valfunc_element (COS),
	valfunc_element (PI),
	valfunc_element (TAN),
	valfunc_element (SQR),
	valfunc_element (ASIN),
	valfunc_element (ACOS),
	valfunc_element (INSTR),
	valfunc_element (ATAN),
	valfunc_element (ABS),
	valfunc_element (USR),
	valfunc_element (VAL),
	valfunc_element (EOF),
	valfunc_element (VARPTR),
	valfunc_element (SYSVARPTR),
	valfunc_element (SGN),
	valfunc_element (LOG),
	valfunc_element (LOG10),
	valfunc_element (EXP),
	valfunc_element (TIME),
	valfunc_element (ERR),
	valfunc_element (ERL),
	valfunc_element (CVI),
	valfunc_element (CVS),
	valfunc_element (CVD),
	valfunc_element (CVL),
	valfunc_element (MIN),
	valfunc_element (MAX),
	valfunc_element (CINT),
	valfunc_element (FIX),
	valfunc_element (XMOUSE),
	valfunc_element (YMOUSE),
	valfunc_element (XPOS),
	valfunc_element (YPOS),
	valfunc_element (PEEK16),
	valfunc_element (PEEK32),
	valfunc_element (RINSTR),
	valfunc_element (FIND_FIRST_OF),
	valfunc_element (FIND_LAST_OF),
	valfunc_element (FIND_FIRST_NOT_OF),
	valfunc_element (FIND_LAST_NOT_OF),
	valfunc_element (SINH),
	valfunc_element (COSH),
	valfunc_element (TANH),
	valfunc_element (ASINH),
	valfunc_element (ACOSH),
	valfunc_element (ATANH),
	valfunc_element (ATAN2),
	valfunc_element (TEST),
	valfunc_element (TESTR),
	valfunc_element (POS),
	valfunc_element (VPOS),
	valfunc_element (LOF),
	valfunc_element (FREEFILE),
	valfunc_element (INKEY),
	valfunc_element (ROUND),
	valfunc_element (CVSMBF),
	valfunc_element (CVDMBF),
	valfunc_element (REGEXP_INSTR),
	valfunc_element (ALLOC_MEMORY),
	valfunc_element (LOC),

	mixfunc_element (IDENTIFIER),
	mixfunc_element (NUMBER),
	valfunc_element (STRING),
	mixfunc_element (INTEGER),
	insfunc_element (ENDLINE),

	// Table used ends here.
	// Serch returning end to indicate fail point
	// to the next entry.

	{0, { & RunnerLineImpl::syntax_error,
		& RunnerLineImpl::valsyntax_error } },
};

const RunnerLineImpl::tfunctions_t * RunnerLineImpl::tfunctionsend=
	tfunctions + dim_array (tfunctions) - 1;

#else
// No ONE_TABLE

#ifndef INSTRUCTION_SWITCH

#define tfunc_t_element(elem) {key##elem, & RunnerLineImpl::do_##elem}

const RunnerLineImpl::tfunc_t RunnerLineImpl::tfunc []=
{
	tfunc_t_element (Colon),
	tfunc_t_element (END),
	tfunc_t_element (LIST),
	tfunc_t_element (REM),
	tfunc_t_element (LOAD),
	tfunc_t_element (SAVE),
	tfunc_t_element (NEW),
	tfunc_t_element (EXIT),
	tfunc_t_element (RUN),
	tfunc_t_element (PRINT),
	tfunc_t_element (FOR),
	tfunc_t_element (NEXT),
	tfunc_t_element (IF),
	tfunc_t_element (ELSE),
	tfunc_t_element (TRON),
	tfunc_t_element (TROFF),
	tfunc_t_element (LET),
	tfunc_t_element (GOTO),
	tfunc_t_element (STOP),
	tfunc_t_element (CONT),
	tfunc_t_element (CLEAR),
	tfunc_t_element (GOSUB),
	tfunc_t_element (RETURN),
	tfunc_t_element (POKE),
	tfunc_t_element (DATA),
	tfunc_t_element (READ),
	tfunc_t_element (RESTORE),
	tfunc_t_element (INPUT),
	tfunc_t_element (LINE),
	tfunc_t_element (RANDOMIZE),
	tfunc_t_element (PLEASE),
	tfunc_t_element (AUTO),
	tfunc_t_element (DIM),
	tfunc_t_element (SYSTEM),
	tfunc_t_element (ON),
	tfunc_t_element (ERROR),
	tfunc_t_element (OPEN),
	tfunc_t_element (CLOSE),
	tfunc_t_element (LOCATE),
	tfunc_t_element (CLS),
	tfunc_t_element (WRITE),
	tfunc_t_element (MODE),
	tfunc_t_element (MOVE),
	tfunc_t_element (COLOR),
	tfunc_t_element (GET),
	tfunc_t_element (LABEL),
	tfunc_t_element (DELIMITER),
	tfunc_t_element (REPEAT),
	tfunc_t_element (UNTIL),
	tfunc_t_element (WHILE),
	tfunc_t_element (WEND),
	tfunc_t_element (PLOT),
	tfunc_t_element (POPEN),
	tfunc_t_element (RESUME),
	tfunc_t_element (DELETE),
	tfunc_t_element (LOCAL),
	tfunc_t_element (PUT),
	tfunc_t_element (FIELD),
	tfunc_t_element (LSET),

	// Lset and rset use same function.
	{keyRSET,         & RunnerLineImpl::do_LSET},

	tfunc_t_element (SOCKET),
	tfunc_t_element (DRAW),
	tfunc_t_element (DEF),
	tfunc_t_element (FN),
	tfunc_t_element (ERASE),
	tfunc_t_element (SWAP),
	tfunc_t_element (SYMBOL),
	tfunc_t_element (ZONE),
	tfunc_t_element (POP),
	tfunc_t_element (NAME),
	tfunc_t_element (KILL),
	tfunc_t_element (FILES),
	tfunc_t_element (PAPER),
	tfunc_t_element (PEN),
	tfunc_t_element (SHELL),
	tfunc_t_element (MERGE),
	tfunc_t_element (CHDIR),
	tfunc_t_element (MKDIR),
	tfunc_t_element (RMDIR),
	tfunc_t_element (SYNCHRONIZE),
	tfunc_t_element (PAUSE),
	tfunc_t_element (CHAIN),
	tfunc_t_element (ENVIRON),
	tfunc_t_element (EDIT),
	tfunc_t_element (DRAWR),
	tfunc_t_element (PLOTR),
	tfunc_t_element (MOVER),
	tfunc_t_element (POKE16),
	tfunc_t_element (POKE32),
	tfunc_t_element (RENUM),
	tfunc_t_element (CIRCLE),
	tfunc_t_element (MASK),
	tfunc_t_element (WINDOW),
	tfunc_t_element (GRAPHICS),
	tfunc_t_element (BEEP),
	tfunc_t_element (DEFINT),

	// DEFINT, DEFSTR, DEFREAL, DEFSNG and DEFDBL use same function.
	{keyDEFSTR,       & RunnerLineImpl::do_DEFINT},
	{keyDEFREAL,      & RunnerLineImpl::do_DEFINT},
	{keyDEFSNG,       & RunnerLineImpl::do_DEFINT},
	{keyDEFDBL,       & RunnerLineImpl::do_DEFINT},

	tfunc_t_element (INK),
	tfunc_t_element (SET_TITLE),
	tfunc_t_element (TAG),

	// TAG and TAGOFF use same function.
	{keyTAGOFF,       & RunnerLineImpl::do_TAG},

	tfunc_t_element (ORIGIN),
	tfunc_t_element (DEG),

	// DEG and RAD use same function.
	{keyRAD,          & RunnerLineImpl::do_DEG},

	tfunc_t_element (INVERSE),
	tfunc_t_element (IF_DEBUG),

	// LPRINT and PRINT use same function.
	{keyLPRINT,       & RunnerLineImpl::do_PRINT},

	tfunc_t_element (LLIST),
	tfunc_t_element (WIDTH),
	tfunc_t_element (BRIGHT),
	tfunc_t_element (DRAWARC),
	tfunc_t_element (PULL),
	tfunc_t_element (PAINT),
	tfunc_t_element (FREE_MEMORY),
	tfunc_t_element (SCROLL),
	tfunc_t_element (ZX_PLOT),
	tfunc_t_element (ZX_UNPLOT),

	tfunc_t_element (MID_S),
	tfunc_t_element (PROGRAMARG_S),

	{keyIDENTIFIER,   & RunnerLineImpl::do_IDENTIFIER},
	tfunc_t_element (NUMBER),
	{keyINTEGER,      & RunnerLineImpl::do_NUMBER},
	tfunc_t_element (ENDLINE),

	// Table used ends here.
	// Serch returning end to indicate fail point
	// to the next entry.

	{0,               & RunnerLineImpl::syntax_error },
};

const RunnerLineImpl::tfunc_t * RunnerLineImpl::tfuncend=
	tfunc + dim_array (tfunc) - 1;

#endif
// INSTRUCTION_SWITCH


#ifndef OPERATION_SWITCH

#define valfunction_t_element(elem) {key##elem, & RunnerLineImpl::val_##elem}

const RunnerLineImpl::valfunction_t RunnerLineImpl::valfunction []=
{
	valfunction_t_element (OpenPar),

	valfunction_t_element (LET),
	valfunction_t_element (LABEL),
	valfunction_t_element (FN),

	valfunction_t_element (MID_S),
	valfunction_t_element (LEFT_S),
	valfunction_t_element (RIGHT_S),
	valfunction_t_element (CHR_S),
	valfunction_t_element (ENVIRON_S),
	valfunction_t_element (STRING_S),
	valfunction_t_element (OSFAMILY_S),
	valfunction_t_element (HEX_S),
	valfunction_t_element (SPACE_S),
	valfunction_t_element (UPPER_S),
	valfunction_t_element (LOWER_S),
	valfunction_t_element (STR_S),
	valfunction_t_element (OCT_S),
	valfunction_t_element (BIN_S),
	valfunction_t_element (INKEY_S),
	valfunction_t_element (PROGRAMARG_S),
	valfunction_t_element (DATE_S),
	valfunction_t_element (TIME_S),
	valfunction_t_element (INPUT_S),
	valfunction_t_element (MKI_S),
	valfunction_t_element (MKS_S),
	valfunction_t_element (MKD_S),
	valfunction_t_element (MKL_S),
	valfunction_t_element (TRIM_S),
	valfunction_t_element (LTRIM_S),
	valfunction_t_element (RTRIM_S),
	valfunction_t_element (OSNAME_S),
	valfunction_t_element (FINDFIRST_S),
	valfunction_t_element (FINDNEXT_S),
	valfunction_t_element (COPYCHR_S),
	valfunction_t_element (STRERR_S),
	valfunction_t_element (DEC_S),
	valfunction_t_element (VAL_S),
	valfunction_t_element (SCREEN_S),
	valfunction_t_element (MKSMBF_S),
	valfunction_t_element (MKDMBF_S),
	valfunction_t_element (REGEXP_REPLACE_S),
	// UCASE$ and LCASE$ are alias for UPPER$ and LOWER$
	{ keyUCASE_S,           & RunnerLineImpl::val_UPPER_S},
	{ keyLCASE_S,           & RunnerLineImpl::val_LOWER_S},

	valfunction_t_element (ASC),
	valfunction_t_element (LEN),
	valfunction_t_element (PEEK),
	valfunction_t_element (PROGRAMPTR),
	valfunction_t_element (RND),
	valfunction_t_element (INT),
	valfunction_t_element (SIN),
	valfunction_t_element (COS),
	valfunction_t_element (PI),
	valfunction_t_element (TAN),
	valfunction_t_element (SQR),
	valfunction_t_element (ASIN),
	valfunction_t_element (ACOS),
	valfunction_t_element (INSTR),
	valfunction_t_element (ATAN),
	valfunction_t_element (ABS),
	valfunction_t_element (USR),
	valfunction_t_element (VAL),
	valfunction_t_element (EOF),
	valfunction_t_element (VARPTR),
	valfunction_t_element (SYSVARPTR),
	valfunction_t_element (SGN),
	valfunction_t_element (LOG),
	valfunction_t_element (LOG10),
	valfunction_t_element (EXP),
	valfunction_t_element (TIME),
	valfunction_t_element (ERR),
	valfunction_t_element (ERL),
	valfunction_t_element (CVI),
	valfunction_t_element (CVS),
	valfunction_t_element (CVD),
	valfunction_t_element (CVL),
	valfunction_t_element (MIN),
	valfunction_t_element (MAX),
	valfunction_t_element (CINT),
	valfunction_t_element (FIX),
	valfunction_t_element (XMOUSE),
	valfunction_t_element (YMOUSE),
	valfunction_t_element (XPOS),
	valfunction_t_element (YPOS),
	valfunction_t_element (PEEK16),
	valfunction_t_element (PEEK32),
	valfunction_t_element (RINSTR),
	valfunction_t_element (FIND_FIRST_OF),
	valfunction_t_element (FIND_LAST_OF),
	valfunction_t_element (FIND_FIRST_NOT_OF),
	valfunction_t_element (FIND_LAST_NOT_OF),
	valfunction_t_element (SINH),
	valfunction_t_element (COSH),
	valfunction_t_element (TANH),
	valfunction_t_element (ASINH),
	valfunction_t_element (ACOSH),
	valfunction_t_element (ATANH),
	valfunction_t_element (ATAN2),
	valfunction_t_element (TEST),
	valfunction_t_element (TESTR),
	valfunction_t_element (POS),
	valfunction_t_element (VPOS),
	valfunction_t_element (LOF),
	valfunction_t_element (FREEFILE),
	valfunction_t_element (INKEY),
	valfunction_t_element (ROUND),
	valfunction_t_element (CVSMBF),
	valfunction_t_element (CVDMBF),
	valfunction_t_element (REGEXP_INSTR),
	valfunction_t_element (ALLOC_MEMORY),
	valfunction_t_element (LOC),

	valfunction_t_element (IDENTIFIER),
	valfunction_t_element (NUMBER),
	valfunction_t_element (STRING),
	valfunction_t_element (INTEGER),

	// Table used ends here.
	// Serch returning end to indicate fail point
	// to the next entry.

	{0,               & RunnerLineImpl::valsyntax_error },
};

const RunnerLineImpl::valfunction_t * RunnerLineImpl::valfunctionend=
	valfunction + dim_array (valfunction) - 1;

#endif
// OPERATION_SWITCH


#endif
// ONE_TABLE


#ifndef NDEBUG

bool RunnerLineImpl::checktfunc ()
{
	#ifdef ONE_TABLE

	for (size_t i= 1; i < dim_array (tfunctions) - 1; ++i)
	{
		if (tfunctions [i - 1].code >= tfunctions [i].code)
		{
			cerr << "Failed check of tfunctions in " <<
				i << endl;
			abort ();
		}
	}

	#else
	// No ONE_TABLE

	#ifndef INSTRUCTION_SWITCH

	for (size_t i= 1; i < dim_array (tfunc) - 1; ++i)
	{
		if (tfunc [i - 1].code >= tfunc [i].code)
		{
			cerr << "Failed check of tfunc in " <<
				i << endl;
			abort ();
		}
	}

	#endif
	// INSTRUCTION_SWITCH

	#ifndef OPERATION_SWITCH

	for (size_t i= 1; i < dim_array (valfunction) - 1; ++i)
	{
		if (valfunction [i - 1].code >= valfunction [i].code)
		{
			cerr << "Failed check on valfunction in " <<
				i << endl;
			abort ();
		}
	}

	#endif
	// OPERATION_SWITCH

	#endif
	// ONE_TABLE

	return true;
}

const bool RunnerLineImpl::tfuncchecked= checktfunc ();

#endif

#ifdef BRUTAL_MODE

#ifdef ONE_TABLE

RunnerLineImpl::functions_t
	RunnerLineImpl::array_functions [keyMAX_CODE_USED + 1];

#else

RunnerLineImpl::do_func RunnerLineImpl::array_func [keyMAX_CODE_USED + 1];

RunnerLineImpl::do_valfunction
	RunnerLineImpl::array_valfunction [keyMAX_CODE_USED + 1];

#endif

bool RunnerLineImpl::init_array_func ()
{
	#ifdef ONE_TABLE

	// This intializer generates an internal error in some
	// old versions of gcc.
	//functions_t emptyfunc= { & RunnerLineImpl::syntax_error,
	//	& RunnerLineImpl::valsyntax_error };
	functions_t emptyfunc;
	emptyfunc.inst_func= & RunnerLineImpl::syntax_error;
	emptyfunc.val_func= & RunnerLineImpl::valsyntax_error;

	std::fill (array_functions,
		array_functions + dim_array (array_functions),
		emptyfunc);
	for (const tfunctions_t * p= tfunctions; p != tfunctionsend; ++p)
		array_functions [p->code]= p->f;

	#else

	std::fill (array_func, array_func + dim_array (array_func),
		& RunnerLineImpl::syntax_error);
	for (const tfunc_t * p= tfunc; p != tfuncend; ++p)
		array_func [p->code]= p->f;

	std::fill (array_valfunction,
		array_valfunction + dim_array (array_valfunction),
		& RunnerLineImpl::valsyntax_error);
	for (const valfunction_t * p= valfunction; p != valfunctionend; ++p)
		array_valfunction [p->code]= p->f;

	#endif

	return true;
}

bool RunnerLineImpl::array_func_inited= init_array_func ();

#endif


#ifdef BRUTAL_MODE

#ifdef ONE_TABLE

RunnerLineImpl::do_func RunnerLineImpl::findfunc (BlCode code)
{
	return array_functions [code].inst_func;
}

RunnerLineImpl::do_valfunction RunnerLineImpl::findvalfunc (BlCode code)
{
	return array_functions [code].val_func;
}

#else
// No ONE_TABLE

RunnerLineImpl::do_func RunnerLineImpl::findfunc (BlCode code)
{
	return array_func [code];
}

RunnerLineImpl::do_valfunction RunnerLineImpl::findvalfunc (BlCode code)
{
	return array_valfunction [code];
}

#endif

#else
// No BRUTAL_MODE


#ifdef ONE_TABLE

RunnerLineImpl::do_func RunnerLineImpl::findfunc (BlCode code)
{
	using std::lower_bound;
	const tfunctions_t tfs= {code, {NULL, NULL} };
	const tfunctions_t * ptf=
		lower_bound <const RunnerLineImpl::tfunctions_t *,
				const RunnerLineImpl::tfunctions_t>
			(tfunctions, tfunctionsend, tfs);
	if (ptf->code != code)
		return & RunnerLineImpl::syntax_error;
	else
		return ptf->f.inst_func;
}

RunnerLineImpl::do_valfunction RunnerLineImpl::findvalfunc (BlCode code)
{
	using std::lower_bound;
	const tfunctions_t tfs= {code, {NULL, NULL} };
	const tfunctions_t * ptf=
		lower_bound <const RunnerLineImpl::tfunctions_t *,
				const RunnerLineImpl::tfunctions_t>
			(tfunctions, tfunctionsend, tfs);
	if (ptf->code != code)
		return & RunnerLineImpl::valsyntax_error;
	else
		return ptf->f.val_func;
}

#else
// No ONE_TABLE

#ifndef INSTRUCTION_SWITCH

RunnerLineImpl::do_func RunnerLineImpl::findfunc (BlCode code)
{
	using std::lower_bound;
	const tfunc_t tfs= {code, NULL};
	// C++ Builder can't deduce the lower_bound template argments,
	// I don't know why.
	const tfunc_t * ptf=
		lower_bound <const RunnerLineImpl::tfunc_t *,
				const RunnerLineImpl::tfunc_t>
			(tfunc, tfuncend, tfs);
	if (ptf->code != code)
		return & RunnerLineImpl::syntax_error;
	else
		return ptf->f;
}

#endif
// INSTRUCTION_SWITCH

#ifndef OPERATION_SWITCH

RunnerLineImpl::do_valfunction RunnerLineImpl::findvalfunc (BlCode code)
{
	using std::lower_bound;
	const valfunction_t vfs= { code, NULL};
	const valfunction_t * pvf=
		lower_bound <const RunnerLineImpl::valfunction_t *,
				const RunnerLineImpl::valfunction_t>
			(valfunction, valfunctionend, vfs);
	if (pvf->code != code)
		return & RunnerLineImpl::valsyntax_error;
	else
		return pvf->f;
}

#endif
// OPERATION_SWITCH

#endif
// ONE_TABLE

#endif
// BRUTAL_MODE


#if 0
// Use macros instead to avoid strange errors on hp-ux.

void RunnerLineImpl::requiretoken (BlCode code) const throw (BlErrNo)
{
	if (token.code != code)
		throw ErrSyntax;
}

void RunnerLineImpl::expecttoken (BlCode code) throw (BlErrNo)
{
	gettoken ();
	requiretoken (code);
}

#else

#define requiretoken(c) if (token.code == c) ; else throw ErrSyntax

#define expecttoken(c) \
	do { \
		gettoken (); \
		if (token.code != c) throw ErrSyntax; \
	} while (0)

#endif


void RunnerLineImpl::getnextchunk ()
{
	while (! endsentence () )
		gettoken ();
	if (token.code != keyENDLINE)
		gettoken ();
}

BlFile & RunnerLineImpl::getfile (BlChannel channel) const
{
	return runner.getfile (channel);
}

BlFile & RunnerLineImpl::getfile0 () const
{
	return runner.getfile0 ();
}

BlNumber RunnerLineImpl::evalnum ()
{
	BlResult result;
	eval (result);
	return result.number ();
}

BlNumber RunnerLineImpl::expectnum ()
{
	gettoken ();
	return evalnum ();
}

BlInteger RunnerLineImpl::evalinteger ()
{
	BlResult result;
	eval (result);
	return result.integer ();
}

BlInteger RunnerLineImpl::expectinteger ()
{
	gettoken ();
	return evalinteger ();
}

std::string RunnerLineImpl::evalstring ()
{
	BlResult result;
	eval (result);
	return result.str ();
}

std::string RunnerLineImpl::expectstring ()
{
	gettoken ();
	return evalstring ();
}

BlChannel RunnerLineImpl::evalchannel ()
{
	BlResult result;
	eval (result);
	return util::checked_cast <BlChannel> (result.integer (), ErrMismatch);
}

BlChannel RunnerLineImpl::expectchannel ()
{
	gettoken ();
	return evalchannel ();
}

BlChannel RunnerLineImpl::evaloptionalchannel (BlChannel defchan)
{
	if (token.code == keySharp)
	{
		gettoken ();
		BlInteger n= evalinteger ();
		return util::checked_cast <BlChannel> (n, ErrMismatch);
	}
	else
		return defchan;
}

BlChannel RunnerLineImpl::expectoptionalchannel (BlChannel defchan)
{
	gettoken ();
	return evaloptionalchannel (defchan);
}

BlChannel RunnerLineImpl::evalrequiredchannel ()
{
	if (token.code == keySharp)
		gettoken ();
	BlInteger n= evalinteger ();
	return util::checked_cast <BlChannel> (n, ErrMismatch);
}

BlChannel RunnerLineImpl::expectrequiredchannel ()
{
	gettoken ();
	return evalrequiredchannel ();
}

std::string::size_type RunnerLineImpl::evalstringindex ()
{
	BlInteger n= evalinteger ();
	if (n < 1)
		throw ErrBadSubscript;
	return static_cast <std::string::size_type> (n);
}

void RunnerLineImpl::evalstringslice (const std::string & str,
	std::string::size_type & from, std::string::size_type & to)
{
	const std::string::size_type limit= str.size ();
	from= 0;
	to= limit;
	gettoken ();
	if (token.code != ']')
	{
		if (token.code != keyTO)
		{
			from= evalstringindex () - 1;
			if (token.code == keyTO)
			{
				gettoken ();
				if (token.code != ']')
					to= evalstringindex ();
			}
			else
				to= from + 1;
		}
		else
		{
			gettoken ();
			if (token.code != ']')
				to= evalstringindex ();
		}
		requiretoken (']');
	}
	gettoken ();
	if (from < to)
	{
		if (from >= limit || to > limit)
			throw ErrBadSubscript;
	}
}

void RunnerLineImpl::assignslice (VarPointer & vp, const BlResult & result)
{
	ASSERT (vp.type == VarStringSlice);

	using std::string;

	string & str= * vp.pstring;
	string value= result.str ();
	if (vp.from  >= vp.to)
		return;
	const string::size_type l= vp.to - vp.from;
	const string::size_type vsize= value.size ();
	if (l < vsize)
		value.erase (l);
	else if (l > vsize)
		value+= string (l - vsize, ' ');
	str.replace (vp.from, l, value);
}

VarPointer RunnerLineImpl::evalvarpointer ()
{
	requiretoken (keyIDENTIFIER);
	std::string varname= token.str;
	gettoken ();
	Dimension d;
	bool isarray= false;
	bool isslice= false;
	VarPointer vp;
	switch (token.code)
	{
	case '(':
		d= getdims ();
		isarray= true;
		break;
	case '[':
		{
			if (typeofvar (varname) != VarString)
				throw ErrMismatch;
			vp.type= VarStringSlice;
			vp.pstring= addrvarstring (varname);
			evalstringslice (* vp.pstring, vp.from, vp.to);
		}
		isslice= true;
		break;
	default:
		break;
	}

	if (! isslice)
		vp.type= typeofvar (varname);

	switch (vp.type)
	{
	case VarNumber:
		vp.pnumber= isarray ?
			addrdimnumber (varname, d) :
			addrvarnumber (varname);
		break;
	case VarInteger:
		vp.pinteger= isarray ?
			addrdiminteger (varname, d) :
			addrvarinteger (varname);
		break;
	case VarString:
		vp.pstring= isarray ?
			addrdimstring (varname, d) :
			addrvarstring (varname);
		break;
	case VarStringSlice:
		break;
	default:
		throw ErrBlassicInternal;
	}
	return vp;
}

void RunnerLineImpl::evalmultivarpointer (ListVarPointer & lvp)
{
	for (;;)
	{
		VarPointer vp= evalvarpointer ();
		lvp.push_back (vp);
		if (token.code != ',')
			break;
		gettoken ();
	}
}

VarPointer RunnerLineImpl::eval_let ()
{
	VarPointer vp= evalvarpointer ();
	requiretoken ('=');
	BlResult result;
	expect (result);
	switch (vp.type)
	{
	case VarNumber:
		* vp.pnumber= result.number ();
		break;
	case VarInteger:
		* vp.pinteger= result.integer ();
		break;
	case VarString:
		* vp.pstring= result.str ();
		break;
	case VarStringSlice:
		assignslice (vp, result);
		break;
	default:
		throw ErrBlassicInternal;
	}
	return vp;
}

void RunnerLineImpl::parenarg (BlResult & result)
{
	expect (result);
	requiretoken (')');
	gettoken ();
}

void RunnerLineImpl::getparenarg (BlResult & result)
{
	expecttoken ('(');
	expect (result);
	requiretoken (')');
	gettoken ();
}

void RunnerLineImpl::getparenarg (BlResult & result, BlResult & result2)
{
	expecttoken ('(');
	expect (result);
	requiretoken (',');
	expect (result2);
	requiretoken (')');
	gettoken ();
}

BlFile & RunnerLineImpl::getparenfile ()
{
	expecttoken ('(');
	//expecttoken ('#');
	//BlChannel c= expectchannel ();
	BlChannel c= expectrequiredchannel ();
	requiretoken (')');
	gettoken ();
	return getfile (c);
}

namespace {

inline void checkfinite (double n)
{
	// Some errors do not set errno, this check can
	// detect some.
	#if (defined __unix__ && ! defined __hpux__) \
		|| defined __linux__
	if (! finite (n) )
		throw ErrDomain;
	#else
	touch (n);
	#endif
}

inline double callnumericfunc (double (* f) (double), double n)
{
	errno= 0;
	n= f (n);
	switch (errno)
	{
	case 0:
		checkfinite (n);
		break;
	case EDOM:
		throw ErrDomain;
	case ERANGE:
		throw ErrRange;
	default:
		if (showdebuginfo () )
			cerr << "Math error, errno= " << errno << endl;
		throw ErrBlassicInternal;
	}
	return n;
}

inline double callnumericfunc (double (* f) (double, double),
	double n, double n2)
{
	errno= 0;
	n= f (n, n2);
	switch (errno)
	{
	case 0:
		checkfinite (n);
		break;
	case EDOM:
		throw ErrDomain;
	case ERANGE:
		throw ErrRange;
	default:
		if (showdebuginfo () )
			cerr << "Math error, errno= " << errno << endl;
		throw ErrBlassicInternal;
	}
	return n;
}

} // namespace

void RunnerLineImpl::valnumericfunc (double (* f) (double), BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	BlNumber n= result.number ();
	result= callnumericfunc (f, n);
}

void RunnerLineImpl::valnumericfunc2 (double (* f) (double, double),
	BlResult & result)
{
	BlResult result2;
	getparenarg (result, result2);
	BlNumber n= result.number ();
	BlNumber n2= result2.number ();
	result= callnumericfunc (f, n, n2);
}

void RunnerLineImpl::valtrigonometricfunc
	(double (* f) (double), BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	BlNumber n= result.number ();
	switch (runner.trigonometric_mode () )
	{
	case TrigonometricDeg:
		n*= value_pi_div_180;
		break;
	case TrigonometricRad:
		break;
	}
	result= callnumericfunc (f, n);
}

void RunnerLineImpl::valtrigonometricinvfunc
	(double (* f) (double), BlResult & result)
{
	getparenarg (result);
	BlNumber n= result.number ();
	n= callnumericfunc (f, n);
	// Provisional
	switch (runner.trigonometric_mode () )
	{
	case TrigonometricDeg:
		n*= value_180_div_pi;
		break;
	case TrigonometricRad:
		break;
	}
	result= n;
}

void RunnerLineImpl::val_ASC (BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	const std::string & str= result.str ();
	if (str.empty () ) result= 0L;
	else result= BlInteger ( (unsigned char) str [0] );
}

void RunnerLineImpl::val_LEN (BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	result= result.str ().size ();
}

void RunnerLineImpl::val_PEEK (BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	BlChar * addr= (BlChar *) size_t (result.number () );
	//result= BlNumber (size_t (* addr) );
	result= BlInteger (size_t (* addr) );
}

void RunnerLineImpl::val_PEEK16 (BlResult & result)
{
	getparenarg (result);
	BlChar * addr= (BlChar *) size_t (result.number () );
	result= peek16 (addr);
}

void RunnerLineImpl::val_PEEK32 (BlResult & result)
{
	getparenarg (result);
	BlChar * addr= (BlChar *) size_t (result.number () );
	//result= BlNumber (static_cast <unsigned long> (peek32 (addr) ) );
	result= peek32 (addr);
}

void RunnerLineImpl::val_PROGRAMPTR (BlResult & result)
{
	gettoken ();
	//result= BlNumber (size_t (program.programptr () ) );
	result= static_cast <BlInteger> (size_t (program.programptr () ) );
}

void RunnerLineImpl::val_SYSVARPTR (BlResult & result)
{
	gettoken ();
	result= sysvar::address ();
}

void RunnerLineImpl::val_RND (BlResult & result)
{
	BlNumber n;
	gettoken ();
	if (token.code == '(')
	{
		parenarg (result);
		n= result.number ();
	}
	else n= 1;

	static BlNumber previous= 0;

	if (n == 0)
	{
		result= previous;
		return;
	}

	if (n < 0)
		//srand (time (0) );
		runner.seedrandom (time (0) );

	BlNumber r= runner.getrandom ();
	result= r;
	previous= r;
}

void RunnerLineImpl::val_INT (BlResult & result)
{
	valnumericfunc (floor, result);
}

void RunnerLineImpl::val_SIN (BlResult & result)
{
	valtrigonometricfunc (sin, result);
}

void RunnerLineImpl::val_COS (BlResult & result)
{
	valtrigonometricfunc (cos, result);
}

void RunnerLineImpl::val_PI (BlResult & result)
{
	result= value_pi;
	gettoken ();
}

void RunnerLineImpl::val_TAN (BlResult & result)
{
	valtrigonometricfunc (tan, result);
}

void RunnerLineImpl::val_SQR (BlResult & result)
{
	valnumericfunc (sqrt, result);
}

void RunnerLineImpl::val_ASIN (BlResult & result)
{
	valtrigonometricinvfunc (asin, result);
}

void RunnerLineImpl::val_ACOS (BlResult & result)
{
	valtrigonometricinvfunc (acos, result);
}

void RunnerLineImpl::val_ATAN (BlResult & result)
{
	valtrigonometricinvfunc (atan, result);
}

void RunnerLineImpl::val_ABS (BlResult & result)
{
	valnumericfunc (fabs, result);
}

void RunnerLineImpl::val_LOG (BlResult & result)
{
	valnumericfunc (log, result);
}

void RunnerLineImpl::val_LOG10 (BlResult & result)
{
	valnumericfunc (log10, result);
}

void RunnerLineImpl::val_EXP (BlResult & result)
{
	valnumericfunc (exp, result);
}

void RunnerLineImpl::val_TIME (BlResult & result)
{
	result= BlInteger (time (NULL) );
	gettoken ();
}

void RunnerLineImpl::val_ERR (BlResult & result)
{
	result= static_cast <BlInteger> (runner.geterr () );
	gettoken ();
}

void RunnerLineImpl::val_ERL (BlResult & result)
{
	result= static_cast <BlInteger> (runner.geterrline () );
	gettoken ();
}

void RunnerLineImpl::val_FIX (BlResult & result)
{
	valnumericfunc (auxFIX, result);
}

void RunnerLineImpl::val_XMOUSE (BlResult & result)
{
	result= static_cast <BlInteger> (graphics::xmouse () );
	gettoken ();
}

void RunnerLineImpl::val_YMOUSE (BlResult & result)
{
	result= static_cast <BlInteger> (graphics::ymouse () );
	gettoken ();
}

void RunnerLineImpl::val_XPOS (BlResult & result)
{
	result= graphics::xpos ();
	gettoken ();
}

void RunnerLineImpl::val_YPOS (BlResult & result)
{
	result= graphics::ypos ();
	gettoken ();
}

void RunnerLineImpl::val_SINH (BlResult & result)
{
	valnumericfunc (sinh, result);
}

void RunnerLineImpl::val_COSH (BlResult & result)
{
	valnumericfunc (cosh, result);
}

void RunnerLineImpl::val_TANH (BlResult & result)
{
	valnumericfunc (tanh, result);
}

void RunnerLineImpl::val_ASINH (BlResult & result)
{
	valnumericfunc (auxmath::asinh, result);
}

void RunnerLineImpl::val_ACOSH (BlResult & result)
{
	valnumericfunc (auxmath::acosh, result);
}

void RunnerLineImpl::val_ATANH (BlResult & result)
{
	valnumericfunc (auxmath::atanh, result);
}

void RunnerLineImpl::val_ATAN2 (BlResult & result)
{
	valnumericfunc2 (atan2, result);
}

namespace {

// Flags to valinstr calls.

const bool instr_direct= false;
const bool instr_reverse= true;

} // namespace

void RunnerLineImpl::valinstrbase (BlResult & result, bool reverse)
{
	expecttoken ('(');
	std::string str;
	std::string::size_type init= reverse ? std::string::npos : 0;

	expect (result);
	switch (result.type () )
	{
	case VarString:
		str= result.str ();
		break;
	case VarNumber:
	#if 0
		init= std::string::size_type (result.number () );
		if (init > 0)
			--init;
		requiretoken (',');
		str= expectstring ();
		break;
	#endif
	case VarInteger:
		init= result.integer ();
		//if (init > 0)
		//	--init;
		if (init < 1)
			throw ErrFunctionCall;
		--init;
		requiretoken (',');
		str= expectstring ();
		break;
	default:
		throw ErrBlassicInternal;
	}
	requiretoken (',');
	std::string tofind= expectstring ();
	requiretoken (')');
	gettoken ();
	std::string::size_type pos;
	if (tofind.empty () )
	{
		if (str.empty () )
			pos= 0;
		else
			if (init < str.size () )
				pos= init + 1;
			else
				pos= 0;
	}
	else
	{
		pos= reverse ?
			str.rfind (tofind, init) :
			str.find (tofind, init);
		if (pos == std::string::npos)
			pos= 0;
		else ++pos;
	}
	result= BlInteger (pos);
}

void RunnerLineImpl::val_INSTR (BlResult & result)
{
	valinstrbase (result, instr_direct);
}

void RunnerLineImpl::val_RINSTR (BlResult & result)
{
	valinstrbase (result, instr_reverse);
}

namespace {

// Flags to valfindfirstlast calls.

const bool find_first= true;
const bool find_last= false;
const bool find_yes= true;
const bool find_not= false;

} // namespace

void RunnerLineImpl::valfindfirstlast (BlResult & result, bool first, bool yesno)
{
	expecttoken ('(');
	std::string str;
	std::string::size_type init= first ? 0 : std::string::npos;

	expect (result);
	switch (result.type () )
	{
	case VarString:
		str= result.str ();
		break;
	case VarNumber:
		init= std::string::size_type (result.number () );
		if (init > 0)
			--init;
		requiretoken (',');
		str= expectstring ();
		break;
	case VarInteger:
		init= result.integer ();
		if (init > 0)
			--init;
		requiretoken (',');
		str= expectstring ();
		break;
	default:
		throw ErrBlassicInternal;
	}
	requiretoken (',');
	std::string tofind= expectstring ();
	requiretoken (')');
	gettoken ();
	std::string::size_type pos;
	if (tofind.empty () )
	{
		if (str.empty () )
			pos= 0;
		else
			if (init < str.size () )
				pos= init + 1;
			else
				pos= 0;
	}
	else
	{
		pos= first ?
			( yesno ? str.find_first_of (tofind, init) :
			str.find_first_not_of (tofind, init) )
			:
			(yesno ? str.find_last_of (tofind, init) :
			str.find_last_not_of (tofind, init) );
		if (pos == std::string::npos)
			pos= 0;
		else ++pos;
	}
	result= BlInteger (pos);
}

void RunnerLineImpl::val_FIND_FIRST_OF (BlResult & result)
{
	valfindfirstlast (result, find_first, find_yes);
}

void RunnerLineImpl::val_FIND_LAST_OF (BlResult & result)
{
	valfindfirstlast (result, find_last, find_yes);
}

void RunnerLineImpl::val_FIND_FIRST_NOT_OF (BlResult & result)
{
	valfindfirstlast (result, find_first, find_not);
}

void RunnerLineImpl::val_FIND_LAST_NOT_OF (BlResult & result)
{
	valfindfirstlast (result, find_last, find_not);
}

namespace {


unsigned long spectrumUDGcode (const std::string & str)
{
	if (str.size () != 1)
		throw ErrImproperArgument;
	unsigned char c= str [0];
	const unsigned char base= 144;

	// Changed this. Now instead of the Spectrum limit of 'u'
	// a to z are allowed.

	if (c >= 'a' && c <= 'z')
		c= static_cast <unsigned char>
			(c - 'a' + base);
	else if (c >= 'A' && c <= 'Z')
		c= static_cast <unsigned char>
			(c - 'A' + base);
	else if (c < 144 || c > 169)
		throw ErrImproperArgument;

	return sysvar::get32 (sysvar::CharGen) + c * 8;
}

} // namespace

void RunnerLineImpl::val_USR (BlResult & result)
{
	gettoken ();
	if (token.code != '(')
	{
		// Without parenthesis only Spectrum style
		// USR char is allowed.
		//valparen (result);
		valbase (result);
		result= spectrumUDGcode (result.str() );
		return;
	}
	DynamicUsrFunc symaddr;
	DynamicHandle libhandle;

	// These are now defined here to avoid an error
	// in C++ Builder.
	std::vector <int> vparam;
	util::auto_buffer <int> param;

	expect (result);
	switch (result.type () )
	{
	case VarNumber:
	case VarInteger:
		symaddr= (DynamicUsrFunc) result.integer ();
		break;
	case VarString:
		{
			std::string libname= result.str ();
			switch (token.code)
			{
			case ',':
				break;
			case ')':
				// Spectrum USR character.
				result= spectrumUDGcode (libname);
				gettoken ();
				return;
			default:
				throw ErrSyntax;
			}
			std::string funcname= expectstring ();
			libhandle.assign (libname);
			symaddr= libhandle.addr (funcname);
		}
		break;
	default:
		throw ErrBlassicInternal;
	}

	int nparams= 0;
	//std::vector <int> vparam;
	while (token.code == ',')
	{
		expect (result);
		vparam.push_back (result.integer () );
		++nparams;
	}
	requiretoken (')');
	gettoken ();
	//util::auto_buffer <int> param;
	if (nparams)
	{
		param.alloc (nparams);
		std::copy (vparam.begin (), vparam.end (), param.begin () );
	}

	result= (* symaddr) (nparams, param);
}

void RunnerLineImpl::val_VAL (BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	std::string str= result.str ();
	switch (sysvar::get (sysvar::TypeOfVal) )
	{
	case 0:
		// VAL simple.
		{
			#if 0
			size_t i= 0, l= str.size ();
			while (i < l && str [i] == ' ')
				++i;
			#else
			std::string::size_type i=
				str.find_first_not_of (" \t");
			if (i > 0)
				if (i == std::string::npos)
					str.erase ();
				else
					str= str.substr (i);
			#endif
		}
		result= CodeLine::Token::number (str);
		break;
	case 1:
		// VAL with expression evaluation (Sinclair ZX)
		if (str.find_first_not_of (" \t")
			== std::string::npos)
		{
			result= 0L;
			break;
		}
		str= std::string ("0 ") + str;
		{
			#if 1

			CodeLine valcodeline;
			valcodeline.scan (str);
			RunnerLineImpl valrunnerline (runner, valcodeline);

			#else

			RunnerLineImpl valrunnerline (runner);
			CodeLine & valcodeline= valrunnerline.getcodeline ();
			valcodeline.scan (str);

			#endif

			valrunnerline.expect (result);
			if (valrunnerline.token.code != keyENDLINE)
				throw ErrSyntax;
		}
		if (! result.is_numeric () )
			throw ErrMismatch;
		break;
	default:
		throw ErrNotImplemented;
	}
}

void RunnerLineImpl::val_EOF (BlResult & result)
{
	// Change to accept the form: EOF (#file)
	//getparenarg (result);

	expecttoken (keyOpenPar);
	gettoken ();
	if (token.code == keySharp)
		gettoken ();
	eval (result);
	requiretoken (')');
	gettoken ();

	BlChannel channel= BlChannel (result.integer () );
	BlFile & file= getfile (channel);
	result= BlInteger (file.eof () ? -1 : 0);
}

void RunnerLineImpl::val_VARPTR (BlResult & result)
{
	expecttoken ('(');
	expecttoken (keyIDENTIFIER);
	std::string varname (token.str);
	VarType type= typeofvar (varname);
	size_t addr= 0;
	gettoken ();
	switch (token.code)
	{
	case ')':
		// Simple
		switch (type)
		{
		case VarNumber:
			addr= reinterpret_cast <size_t>
				(addrvarnumber (varname) );
			break;
		case VarInteger:
			addr= reinterpret_cast <size_t>
				(addrvarinteger (varname) );
			break;
		case VarString:
			addr= reinterpret_cast <size_t>
				(addrvarstring (varname) );
			break;
		default:
			throw ErrBlassicInternal;
		}
		result= 0L;
		gettoken ();
		break;
	case '(':
		// Array
		{
		Dimension dims= getdims ();
		result= 0L;
		requiretoken (')');
		switch (type)
		{
		case VarNumber:
			addr= reinterpret_cast <size_t>
				(addrdimnumber (varname, dims) );
			break;
		case VarInteger:
			addr= reinterpret_cast <size_t>
				(addrdiminteger (varname, dims) );
			break;
		case VarString:
			addr= reinterpret_cast <size_t>
				(addrdimstring (varname, dims) );
			break;
		default:
			throw ErrBlassicInternal;
		}
		gettoken ();
		}
		break;
	default:
		throw ErrSyntax;
	}
	//result= BlNumber (addr);
	result= static_cast <BlInteger> (addr);
}

void RunnerLineImpl::val_SGN (BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	BlNumber d= result.number ();
	// Do not use 0.0 instead of zero, is a workaround
	// for an error on some versions of gcc.
	result= d < zero ? -1L : d > zero ? 1L : 0L;
}

void RunnerLineImpl::val_CVI (BlResult & result)
{
	getparenarg (result);
	std::string str (result.str () );
	if (str.size () < 2)
		throw ErrFunctionCall;
	result= BlInteger (short ( (unsigned char) str [0] ) |
		short ( ( (unsigned char) str [1] ) << 8) );
}

void RunnerLineImpl::val_CVS (BlResult & result)
{
	#define SIZE_S 4
	ASSERT (sizeof (float) == 4);

	getparenarg (result);
	const std::string & str (result.str () );
	if (str.size () < SIZE_S)
		throw ErrFunctionCall;
	BlNumber bn= BlNumber
		(* reinterpret_cast <const float *> (str.data () ) );
	result= bn;

	#undef SIZE_S
}

void RunnerLineImpl::val_CVD (BlResult & result)
{
	#define SIZE_D 8
	ASSERT (sizeof (double) == 8);

	getparenarg (result);
	const std::string & str (result.str () );
	if (str.size () < SIZE_D)
		throw ErrFunctionCall;
	BlNumber bn= static_cast <BlNumber>
		( (* reinterpret_cast <const double *> (str.data () ) ) );
	result= bn;
	#undef SIZE_D
}

void RunnerLineImpl::val_CVL (BlResult & result)
{
	getparenarg (result);
	std::string str (result.str () );
	if (str.size () < 4)
		throw ErrFunctionCall;
	result=
		long ( (unsigned char) str [0] ) |
		long ( ( (unsigned char) str [1] ) << 8) |
		long ( ( (unsigned char) str [2] ) << 16) |
		long ( ( (unsigned char) str [3] ) << 24);
}

void RunnerLineImpl::val_MIN (BlResult & result)
{
	expecttoken ('(');
	BlNumber bnMin= expectnum ();
	while (token.code == ',')
		bnMin= std::min (bnMin, expectnum () );
	requiretoken (')');
	gettoken ();
	result= bnMin;
}

void RunnerLineImpl::val_MAX (BlResult & result)
{
	expecttoken ('(');
	BlNumber bnMax= expectnum ();
	while (token.code == ',')
		bnMax= std::max (bnMax, expectnum () );
	requiretoken (')');
	gettoken ();
	result= bnMax;
}

void RunnerLineImpl::val_CINT (BlResult & result)
{
	gettoken ();
	#if 0
	if (token.code == '(')
	{
		expect (result);
		requiretoken (')');
		gettoken ();
	}
	else
		eval (result);
	#else
	//valparen (result);
	valbase (result);
	#endif
	result= result.integer ();
}

void RunnerLineImpl::val_TEST (BlResult & result)
{
	expecttoken ('(');
	int x= expectinteger ();
	requiretoken (',');
	int y= expectinteger ();
	requiretoken (')');
	gettoken ();
	result= static_cast <BlInteger> (graphics::test (x, y, false) );
}

void RunnerLineImpl::val_TESTR (BlResult & result)
{
	expecttoken ('(');
	int x= expectinteger ();
	requiretoken (',');
	int y= expectinteger ();
	requiretoken (')');
	gettoken ();
	result= static_cast <BlInteger> (graphics::test (x, y, true) );
}

void RunnerLineImpl::val_POS (BlResult & result)
{
	result= static_cast <BlInteger> (getparenfile ().pos () + 1);
}

void RunnerLineImpl::val_VPOS (BlResult & result)
{
	result= getparenfile ().vpos () + 1;
}

void RunnerLineImpl::val_LOF (BlResult & result)
{
	//expecttoken ('(');
	//gettoken ();
	//if (token.code == '#')
	//	gettoken ();
	//BlChannel ch= evalchannel ();
	//requiretoken (')');
	//gettoken ();
	//result= getfile (ch).lof ();
	result= getparenfile ().lof ();
}

void RunnerLineImpl::val_FREEFILE (BlResult & result)
{
	gettoken ();
	result= runner.freefile ();
}

void RunnerLineImpl::val_INKEY (BlResult & result)
{
	getparenarg (result);
	result= graphics::keypressed (result.integer () );
}

void RunnerLineImpl::val_ROUND (BlResult & result)
{
	using blassic::result::round;
	expecttoken ('(');
	BlNumber n= expectnum ();
	BlInteger d= 0;
	if (token.code == ',')
		d= expectinteger ();
	requiretoken (')');
	gettoken ();
	if (d == 0)
		result= round (n);
	else if (d > 0)
	{
		for (BlInteger i= 0; i < d; ++i)
			n*= 10;
		n= round (n);
		for (BlInteger i= 0; i < d; ++i)
			n/= 10;
		result= n;
	}
	else
	{
		for (BlInteger i= 0; i > d; --i)
			n/= 10;
		n= round (n);
		for (BlInteger i= 0; i > d; --i)
			n*= 10;
		result= n;
	}
}

void RunnerLineImpl::val_CVSMBF (BlResult & result)
{
	getparenarg (result);
	const std::string & str (result.str () );
	result= mbf::mbf_s (str);
}

void RunnerLineImpl::val_CVDMBF (BlResult & result)
{
	getparenarg (result);
	const std::string & str (result.str () );
	result= mbf::mbf_d (str);
}

void RunnerLineImpl::val_REGEXP_INSTR (BlResult & result)
{
	expecttoken ('(');
	std::string searched;
	std::string::size_type init= 0;

	expect (result);
	switch (result.type () )
	{
	case VarString:
		searched= result.str ();
		break;
	case VarNumber:
	case VarInteger:
		init= result.integer ();
		if (init < 1)
			throw ErrFunctionCall;
		--init;
		requiretoken (',');
		searched= expectstring ();
		break;
	default:
		throw ErrBlassicInternal;
	}
	requiretoken (',');
	std::string expr= expectstring ();
	BlInteger flags= 0;
	if (token.code == ',')
	{
		flags= expectinteger ();
	}
	else
	{
		// Default flags value depending on the initial position.
		if (init > 0)
			flags= Regexp::FLAG_NOBEG;
	}
	requiretoken (')');
	gettoken ();

	Regexp regexp (expr, flags);
	Regexp::size_type r= regexp.find (searched, init);
	if (r == std::string::npos)
		result= 0;
	else
		result= r + 1;
}

void RunnerLineImpl::val_ALLOC_MEMORY (BlResult & result)
{
	getparenarg (result);
	result= blassic::memory::dyn_alloc (result.integer () );
}

void RunnerLineImpl::val_LOC (BlResult & result)
{
	expecttoken (keyOpenPar);
	gettoken ();
	if (token.code == keySharp)
		gettoken ();
	eval (result);
	requiretoken (')');
	gettoken ();

	BlChannel channel= BlChannel (result.integer () );
	BlFile & file= getfile (channel);
	result= BlInteger (file.loc () );
}

void RunnerLineImpl::val_MID_S (BlResult & result)
{
	expecttoken ('(');
	std::string str= expectstring ();
	requiretoken (',');
	BlNumber blfrom= expectnum ();
	size_t from= size_t (blfrom) - 1;
	size_t len;
	if (token.code == ',')
	{
		BlNumber bllen= expectnum ();
		len= size_t (bllen);
	}
	else
		len= std::string::npos;
	requiretoken (')');
	if (from >= str.size () )
		result= std::string ();
	else
		result= str.substr (from, len);
	gettoken ();
}

void RunnerLineImpl::val_LEFT_S (BlResult & result)
{
	expecttoken ('(');
	std::string str= expectstring ();
	requiretoken (',');
	BlNumber blfrom= expectnum ();
	requiretoken (')');
	size_t from= size_t (blfrom);
	result= str.substr (0, from);
	gettoken ();
}

void RunnerLineImpl::val_RIGHT_S (BlResult & result)
{
	expecttoken ('(');
	std::string str= expectstring ();
	requiretoken (',');
	BlNumber blfrom= expectnum ();
	requiretoken (')');
	size_t from= size_t (blfrom);
	size_t l= str.size ();
	if (from < l)
		result= str.substr (str.size () - from);
	else
		result= str;
	gettoken ();
}

void RunnerLineImpl::val_CHR_S (BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	result= std::string (1, (unsigned char) result.number () );
}

void RunnerLineImpl::val_ENVIRON_S (BlResult & result)
{
	getparenarg (result);
	char * str= getenv (result.str ().c_str () );
	if (str)
		result= std::string (str);
	else
		result= std::string ();
}

void RunnerLineImpl::val_STRING_S (BlResult & result)
{
	expecttoken ('(');
	expect (result);
	size_t rep= result.integer ();
	requiretoken (',');
	expect (result);
	requiretoken (')');
	gettoken ();
	BlChar charrep= '\0';
	switch (result.type () )
	{
	case VarNumber:
		charrep= BlChar ( (unsigned int) result.number () );
		break;
	case VarInteger:
		charrep= BlChar (result.integer () );
		break;
	case VarString:
		{
			const std::string & aux= result.str ();
			if (aux.empty () )
				charrep= '\0';
			else
				charrep= aux [0];
		}
		break;
	default:
		throw ErrBlassicInternal;
	}
	result= std::string (rep, charrep);
}

void RunnerLineImpl::val_OSFAMILY_S (BlResult & result)
{
	gettoken ();
	result= std::string (os_family);
}

void RunnerLineImpl::val_OSNAME_S (BlResult & result)
{
	gettoken ();

	#if defined __unix__ || defined __linux__
	// Even in cygwin

	struct utsname buf;
	if (uname (&buf) != 0)
		result= "unknown";
	else
		result= buf.sysname;

	#elif defined BLASSIC_USE_WINDOWS

	result= "Windows";

	#else

	result= "unknown";

	#endif
}

void RunnerLineImpl::val_HEX_S (BlResult & result)
{
	expecttoken ('(');
	expect (result);
	if (result.type () == VarNumber)
	{
		BlNumber m= blassic::result::round (result.number () );
		if (m <= BlUint32Max && m > BlInt32Max)
			result= m - BlUint32Max - 1;
	}
	BlInteger n= result.integer ();
	size_t w= 0;
	if (token.code == ',')
	{
		expect (result);
		w= result.integer ();
	}
	requiretoken (')');
	gettoken ();
	std::ostringstream oss;
	oss.setf (std::ios::uppercase);
	oss << std::hex <<
		std::setw (w) << std::setfill ('0') <<
		n;
	result= oss.str ();
}

void RunnerLineImpl::val_SPACE_S (BlResult & result)
{
	getparenarg (result);
	result= std::string (size_t (result.number () ), ' ');
}

void RunnerLineImpl::val_UPPER_S (BlResult & result)
{
	getparenarg (result);
	std::string & str= result.str ();
	std::transform (str.begin (), str.end (), str.begin (), toupper);
}

void RunnerLineImpl::val_LOWER_S (BlResult & result)
{
	getparenarg (result);
	std::string & str= result.str ();
	std::transform (str.begin (), str.end (), str.begin (), tolower);
}

void RunnerLineImpl::val_STR_S (BlResult & result)
{
	gettoken ();
	//valparen (result);
	valbase (result);
	std::ostringstream oss;
	BlNumber n= result.number ();
	if (sysvar::hasFlags1 (sysvar::SpaceStr_s) &&
			n >= zero)
		oss << ' ';
	oss << std::setprecision (10) << n;
	result= oss.str ();
}

void RunnerLineImpl::val_OCT_S (BlResult & result)
{
	expecttoken ('(');
	BlNumber n= expectnum ();
	size_t w= 0;
	if (token.code == ',')
	{
		BlNumber blw= expectnum ();
		w= size_t (blw);
	}
	requiretoken (')');
	gettoken ();
	std::ostringstream oss;
	oss.setf (std::ios::uppercase);
	oss << std::oct <<
		std::setw (w) << std::setfill ('0') <<
		(unsigned long) n;
	result= oss.str ();
}

void RunnerLineImpl::val_BIN_S (BlResult & result)
{
	expecttoken ('(');
	BlNumber bn= expectnum ();
	size_t w= 0;
	if (token.code == ',')
	{
		BlNumber blw= expectnum ();
		w= size_t (blw);
	}
	requiretoken (')');
	gettoken ();
	unsigned long n= (unsigned long) bn;
	std::string str;
	while (n)
	{
		str= ( (n & 1) ? '1' : '0') + str;
		n/= 2;
	}
	if (str.empty () )
		str= std::string (1, '0');
	if (w > 0 && str.size () < w)
		str= std::string (w - str.size (), '0') + str;
	result= str;
}

void RunnerLineImpl::val_INKEY_S (BlResult & result)
{
	gettoken ();
	//result= inkey ();
	// Now we can specify channel used for input.
	BlChannel ch= DefaultChannel;
	if (token.code == '(')
	{
		//gettoken ();
		//if (token.code == '#')
		//	gettoken ();
		//ch= evalchannel ();
		ch= expectrequiredchannel ();
		requiretoken (')');
		gettoken ();
	}
	std::string r= getfile (ch).inkey ();
	if (r == "\n" && sysvar::hasFlags1 (sysvar::ConvertLFCR) )
		r= "\r";
	result= r;
}

void RunnerLineImpl::val_PROGRAMARG_S (BlResult & result)
{
	getparenarg (result);
	result= getprogramarg (size_t (result.number () - 1) );
}

void RunnerLineImpl::val_DATE_S (BlResult & result)
{
	using std::setw;
	using std::setfill;

	gettoken ();
	std::time_t t= time (NULL);
	struct std::tm * ptm= std::localtime (& t);
	std::ostringstream oss;
	oss << setw (2) << setfill ('0') << (ptm->tm_mon + 1) << '-' <<
		setw (2) << setfill ('0') << ptm->tm_mday << '-' <<
		(ptm->tm_year + 1900);
	result= oss.str ();
}

void RunnerLineImpl::val_TIME_S (BlResult & result)
{
	using std::setw;
	using std::setfill;

	gettoken ();
	std::time_t t= time (NULL);
	struct std::tm * ptm= std::localtime (& t);
	std::ostringstream oss;
	oss << setw (2) << setfill ('0') << ptm->tm_hour << ':' <<
		setw (2) << setfill ('0') << ptm->tm_min << ':' <<
		setw (2) << setfill ('0') << ptm->tm_sec;
	result= oss.str ();
}

void RunnerLineImpl::val_INPUT_S (BlResult & result)
{
	expecttoken ('(');
	BlNumber bn= expectnum ();
	BlChannel channel= DefaultChannel;
	switch (token.code)
	{
	case ')':
		break;
	case ',':
		gettoken ();
		//if (token.code == '#')
		//	channel= expectchannel ();
		channel= evaloptionalchannel (channel);
		requiretoken (')');
		break;
	default:
		throw ErrSyntax;
	}
	gettoken ();
	BlFile & in= getfile (channel);
	result= in.read (size_t (bn) );
}

void RunnerLineImpl::val_MKI_S (BlResult & result)
{
	getparenarg (result);
	BlNumber bn= result.number ();
	unsigned short s= (unsigned short) short (bn);
	std::string str;
	str= char (s & 255);
	str+= char (s >> 8);
	result= str;
}

void RunnerLineImpl::val_MKS_S (BlResult & result)
{
	getparenarg (result);
	float f= result.number ();
	std::string str (reinterpret_cast <char *> (& f), sizeof (float) );
	result= str;
}

void RunnerLineImpl::val_MKD_S (BlResult & result)
{
	getparenarg (result);
	double f= result.number ();
	std::string str (reinterpret_cast <char *> (& f), sizeof (double) );
	result= str;
}

void RunnerLineImpl::val_MKL_S (BlResult & result)
{
	getparenarg (result);
	BlNumber bn= result.number ();
	unsigned int s= (unsigned int) int (bn);
	std::string str;
	str= char (s & 255);
	str+= char ( (s >> 8) & 255);
	str+= char ( (s >> 16) & 255);
	str+= char ( (s >> 24) & 255);
	result= str;
}

#if 0
void RunnerLineImpl::val_TRIM (BlResult & result)
{
	using std::string;

	bool tleft= false, tright= false;
	switch (token.code)
	{
	case keyTRIM_S:
		tleft= true; tright= true;
		break;
	case keyLTRIM_S:
		tleft= true;
		break;
	case keyRTRIM_S:
		tright= true;
		break;
	default:
		if (showdebuginfo () )
			cerr << "Erroneous call to valtrim" << endl;
		throw ErrBlassicInternal;
	}
	getparenarg (result);
	string str= result.str ();
	if (tleft)
	{
		string::size_type
			inipos= str.find_first_not_of (' ');
		if (inipos > 0)
			if (inipos == string::npos)
				str= string ();
			else
				//str= str.substr (inipos);
				str.erase (0, inipos);
	}
	if (tright)
	{
		string::size_type
			endpos=  str.find_last_not_of (' ');
		if (endpos != string::npos)
			//str= str.substr (0, endpos + 1);
			str.erase (endpos + 1);
		else str= string ();
	}
	result= str;
}
#endif

void RunnerLineImpl::valtrimbase (BlResult & result,
	bool tleft, bool tright)
{
	using std::string;

	if (! tleft && ! tright)
	{
		if (showdebuginfo () )
			cerr << "Erroneous call to valtrim" << endl;
		throw ErrBlassicInternal;
	}

	getparenarg (result);
	string str= result.str ();
	if (tleft)
	{
		string::size_type
			inipos= str.find_first_not_of (' ');
		if (inipos > 0)
			if (inipos == string::npos)
				str= string ();
			else
				//str= str.substr (inipos);
				str.erase (0, inipos);
	}
	if (tright)
	{
		string::size_type
			endpos=  str.find_last_not_of (' ');
		if (endpos != string::npos)
			//str= str.substr (0, endpos + 1);
			str.erase (endpos + 1);
		else str= string ();
	}
	result= str;
}

void RunnerLineImpl::val_TRIM_S (BlResult & result)
{
	valtrimbase (result, true, true);
}

void RunnerLineImpl::val_LTRIM_S (BlResult & result)
{
	valtrimbase (result, true, false);
}

void RunnerLineImpl::val_RTRIM_S (BlResult & result)
{
	valtrimbase (result, false, true);
}

void RunnerLineImpl::val_FINDFIRST_S (BlResult & result)
{
	getparenarg (result);
	if (! pdirectory)
		pdirectory= new Directory ();
	result= pdirectory->findfirst (result.str () );
}

void RunnerLineImpl::val_FINDNEXT_S (BlResult & result)
{
	if (! pdirectory)
		result= std::string ();
	else
		result= pdirectory->findnext ();
	gettoken ();
}

void RunnerLineImpl::val_COPYCHR_S (BlResult & result)
{
	expecttoken ('(');
	expecttoken ('#');
	BlChannel c= expectchannel ();

	BlChar from= BlChar (0), to= BlChar (255);
	if (token.code == ',')
	{
		from= static_cast <BlChar> (expectnum () );
		if (token.code == ',')
		{
			to= static_cast <BlChar> (expectnum () );
		}
	}
	requiretoken (')');
	gettoken ();

	BlFile & window= getfile (c);
	result= window.copychr (from, to);
}

void RunnerLineImpl::val_STRERR_S (BlResult & result)
{
	getparenarg (result);
	result= ErrStr (static_cast <BlErrNo> (result.integer () ) );
}

void RunnerLineImpl::val_DEC_S (BlResult & result)
{
	expecttoken ('(');
	BlNumber n= expectnum ();
	requiretoken (',');
	std::string format= expectstring ();
	VectorUsing usingf;
	parseusing (format, usingf);
	if (usingf.size () != 1)
		throw ErrFunctionCall;
	UsingNumeric * pun= dynamic_cast <UsingNumeric *> (usingf [0] );
	if (pun == NULL)
		throw ErrFunctionCall;
	BlFileOutString fos;
	pun->putnumeric (fos, n);
	result= fos.str ();
	requiretoken (')');
	gettoken ();
}

void RunnerLineImpl::val_VAL_S (BlResult & result)
{
	//getparenarg (result);
	gettoken ();
	//valparen (result);
	valbase (result);
	std::string str= result.str ();
	if (str.find_first_not_of (" \t")
		== std::string::npos)
	{
		result= std::string ();
	}
	else
	{
		str= std::string ("0 ") + str;

		#if 1

		CodeLine valcodeline;
		valcodeline.scan (str);
		RunnerLineImpl valrunnerline (runner, valcodeline);

		#else

		RunnerLineImpl valrunnerline (runner);
		CodeLine & valcodeline= valrunnerline.getcodeline ();
		valcodeline.scan (str);

		#endif

		valrunnerline.expect (result);
		if (valrunnerline.token.code != keyENDLINE)
			throw ErrSyntax;
		if (result.type () != VarString)
			throw ErrMismatch;
	}
}

void RunnerLineImpl::val_SCREEN_S (BlResult & result)
{
	expecttoken ('(');
	BlInteger y= expectinteger ();
	requiretoken (',');
	BlInteger x= expectinteger ();
	requiretoken (')');
	gettoken ();
	result= graphics::screenchr (x, y);
}

void RunnerLineImpl::val_MKSMBF_S (BlResult & result)
{
	getparenarg (result);
	result= mbf::to_mbf_s (result.number () );
}

void RunnerLineImpl::val_MKDMBF_S (BlResult & result)
{
	getparenarg (result);
	result= mbf::to_mbf_d (result.number () );
}

void RunnerLineImpl::val_REGEXP_REPLACE_S (BlResult & result)
{
	expecttoken ('(');
	std::string searched;
	std::string::size_type init= 0;

	expect (result);
	switch (result.type () )
	{
	case VarString:
		searched= result.str ();
		break;
	case VarNumber:
	case VarInteger:
		init= result.integer ();
		if (init < 1)
			throw ErrFunctionCall;
		--init;
		requiretoken (',');
		searched= expectstring ();
		break;
	default:
		throw ErrBlassicInternal;
	}
	requiretoken (',');
	std::string expr= expectstring ();
	requiretoken (',');
	gettoken ();
	std::string replaceby;
	bool isfunction= false;
	if (token.code == keyFN)
	{
		isfunction= true;
		gettoken ();
		if (token.code != keyIDENTIFIER)
			throw ErrSyntax;
		replaceby= token.str;
		gettoken ();
	}
	else
		replaceby= evalstring ();
	BlInteger flags= 0;
	if (token.code == ',')
	{
		flags= expectinteger ();
	}
	else
	{
		// Default flags value depending on the initial position.
		if (init > 0)
			flags= Regexp::FLAG_NOBEG;
	}
	requiretoken (')');
	gettoken ();

	Regexp regexp (expr, flags);
	if (isfunction)
		result= regexp.replace (searched, init, * this, replaceby);
	else
		result= regexp.replace (searched, init, replaceby);
}

namespace {

class FnErrorShower {
public:
	FnErrorShower (const std::string & fname) :
		finished (false),
		name (fname)
	{ }
	~FnErrorShower ()
	{
		if (! finished && showdebuginfo () )
			cerr << "Error processing FN " << name << endl;
	}
	void finish ()
	{
		finished= true;
	}
private:
	bool finished;
	const std::string & name;
};

class FnLevelIncrementer {
public:
	FnLevelIncrementer (Runner & r) :
		r (r)
	{
		r.inc_fn_level ();
	}
	~FnLevelIncrementer ()
	{
		r.dec_fn_level ();
	}
private:
	Runner & r;
};

} // namespace

void RunnerLineImpl::callfn (Function & f, const std::string & fname,
	LocalLevel & ll, BlResult & result)
{
	switch (f.getdeftype () )
	{
	case Function::DefSingle:
		{
			// Prevents lock on recursive calls.
			if (fInterrupted)
			{
				if (runner.getbreakstate () !=
						onbreak::BreakCont)
					throw BlBreak ();
				else
					fInterrupted= false;
			}

			FnErrorShower check (fname);

			//CodeLine & code= f.getcode ();
			//code.gotochunk (0);
			//RunnerLineImpl fnrunnerline (runner, code);

			//RunnerLineImpl fnrunnerline (runner);
			//CodeLine & fncodeline= fnrunnerline.getcodeline ();

			// Use the heap to decrease the use
			// of the stack in recursive fn calls.
			//auto_ptr <RunnerLineImpl> pfnrunnerline
			//	(new RunnerLineImpl (runner) );
			//CodeLine & fncodeline= pfnrunnerline->getcodeline ();
			//fncodeline= f.getcode ();

			CodeLine fncodeline (f.getcode () );
			auto_ptr <RunnerLineImpl> pfnrunnerline
				(new RunnerLineImpl (runner, fncodeline) );

			FnLevelIncrementer fli (runner);
			//fnrunnerline.expect (result);
			pfnrunnerline->expect (result);
			ll.freelocalvars ();

			check.finish ();
		}
		break;
	case Function::DefMulti:
		{
			ll.addlocalvar (fname);
			//Runner fnrun (program);
			Runner fnrun (runner);
			// We reuse the break position
			// to jump to the fn definition.
			fnrun.set_break (f.getpos () );
			fnrun.gosub_push (ll);
			CodeLine jumpline;
			jumpline.scan ("CONT");
			{
				FnLevelIncrementer fli (runner);
				FnErrorShower check (fname);
				fnrun.runline (jumpline);
				check.finish ();
			}

			size_t stacksize= fnrun.gosub_size ();
			if (stacksize > 1)
			{
				if (showdebuginfo () )
				{
					cerr << "FN END with GOSUB "
						"pending of RETURN" <<
						endl;
				}
				throw ErrGosubWithoutReturn;
			}
			if (stacksize == 0)
			{
				if (showdebuginfo () )
				{
					cerr << "FN control missing" << endl;
				}
				throw ErrBlassicInternal;
			}

			switch (typeofvar (fname) )
			{
			case VarNumber:
				result= evaluatevarnumber (fname);
				break;
			case VarInteger:
				result= evaluatevarinteger (fname);
				break;
			case VarString:
				result= evaluatevarstring (fname);
				break;
			default:
				throw ErrBlassicInternal;
			}
			fnrun.fn_pop ();
		}
		break;
	}
}

void RunnerLineImpl::val_FN (BlResult & result)
{
	// Get FN name.
	expecttoken (keyIDENTIFIER);
	std::string fname= token.str;
	Function f= Function::get (fname);

	// Get parameters.
	gettoken ();
	LocalLevel ll;
	const ParameterList & param= f.getparam ();
	size_t l= param.size ();
	if (l != 0)
	{
		requiretoken ('(');
		for (size_t i= 0; i < l; ++i)
		{
			BlResult aux;
			expect (result);
			//const std::string & var= param [i];
			std::string var= param [i];
			ll.addlocalvar (var);
			VarType type= typeofvar (var);
			switch (type)
			{
			case VarNumber:
				assignvarnumber (var, result.number () );
				break;
			case VarInteger:
				assignvarinteger (var, result.integer () );
				break;
			case VarString:
				assignvarstring (var, result.str () );
				break;
			default:
				throw ErrBlassicInternal;
			}
			if (i < l - 1)
				requiretoken (',');
		}
		requiretoken (')');
		gettoken ();
	}

	// Execute the function.
	callfn (f, fname, ll, result);
}

void RunnerLineImpl::valsubindex (const std::string & varname,
	BlResult & result)
{
	Dimension dims= getdims ();
	switch (typeofvar (varname) )
	{
	case VarNumber:
		result= valuedimnumber (varname, dims);
		break;
	case VarInteger:
		result= valuediminteger (varname, dims);
		break;
	case VarString:
		result= valuedimstring (varname, dims);
		break;
	default:
		throw ErrBlassicInternal;
	}
}

void RunnerLineImpl::val_LET (BlResult & result)
{
	gettoken ();
	VarPointer vp= eval_let ();
	switch (vp.type)
	{
	case VarNumber:
		result= * vp.pnumber;
		break;
	case VarInteger:
		result= * vp.pinteger;
		break;
	case VarString:
		result= * vp.pstring;
		break;
	default:
		throw ErrBlassicInternal;
	}
}

void RunnerLineImpl::val_LABEL (BlResult & result)
{
	gettoken ();
	if (token.code != keyIDENTIFIER)
		throw ErrSyntax;
	result= static_cast <BlNumber> (program.getlabel (token.str) );
	gettoken ();
}

void RunnerLineImpl::val_IDENTIFIER (BlResult & result)
{
	std::string varname (token.str);
	gettoken ();
	if (token.code == '(')
		valsubindex (varname, result);
	else
		switch (typeofvar (varname) )
		{
		case VarNumber:
			result= evaluatevarnumber (varname);
			break;
		case VarInteger:
			result= evaluatevarinteger (varname);
			break;
		case VarString:
			result= evaluatevarstring (varname);
			break;
		default:
			throw ErrBlassicInternal;
		}
}

void RunnerLineImpl::val_STRING (BlResult & result)
{
	result= token.str;
	gettoken ();
}

void RunnerLineImpl::val_NUMBER (BlResult & result)
{
	result= token.number ();
	gettoken ();
}

void RunnerLineImpl::val_INTEGER (BlResult & result)
{
	result= token.integer ();
	gettoken ();
}

void RunnerLineImpl::val_OpenPar (BlResult & result)
{
	gettoken ();
	eval (result);
	if (token.code != keyClosePar)
		throw ErrSyntax;
	gettoken ();
}

void RunnerLineImpl::valbase (BlResult & result)
{
	#ifdef OPERATION_SWITCH

#	define HANDLE_OPER(elem) \
		case key##elem: \
			val_##elem (result); \
			break

#	define HANDLE_OPER2(elem1, elem2) \
		case key##elem1: \
			val_##elem2 (result); \
			break

	switch (token.code)
	{
	HANDLE_OPER (OpenPar);

	HANDLE_OPER (LET);
	HANDLE_OPER (LABEL);
	HANDLE_OPER (FN);

	HANDLE_OPER (MID_S);
	HANDLE_OPER (LEFT_S);
	HANDLE_OPER (RIGHT_S);
	HANDLE_OPER (CHR_S);
	HANDLE_OPER (ENVIRON_S);
	HANDLE_OPER (STRING_S);
	HANDLE_OPER (OSFAMILY_S);
	HANDLE_OPER (HEX_S);
	HANDLE_OPER (SPACE_S);
	HANDLE_OPER (UPPER_S);
	HANDLE_OPER (LOWER_S);
	HANDLE_OPER (STR_S);
	HANDLE_OPER (OCT_S);
	HANDLE_OPER (BIN_S);
	HANDLE_OPER (INKEY_S);
	HANDLE_OPER (PROGRAMARG_S);
	HANDLE_OPER (DATE_S);
	HANDLE_OPER (TIME_S);
	HANDLE_OPER (INPUT_S);
	HANDLE_OPER (MKI_S);
	HANDLE_OPER (MKS_S);
	HANDLE_OPER (MKD_S);
	HANDLE_OPER (MKL_S);
	HANDLE_OPER (TRIM_S);
	HANDLE_OPER (LTRIM_S);
	HANDLE_OPER (RTRIM_S);
	HANDLE_OPER (OSNAME_S);
	HANDLE_OPER (FINDFIRST_S);
	HANDLE_OPER (FINDNEXT_S);
	HANDLE_OPER (COPYCHR_S);
	HANDLE_OPER (STRERR_S);
	HANDLE_OPER (DEC_S);
	HANDLE_OPER (VAL_S);
	HANDLE_OPER (SCREEN_S);
	HANDLE_OPER (MKSMBF_S);
	HANDLE_OPER (MKDMBF_S);
	HANDLE_OPER (REGEXP_REPLACE_S);
	// UCASE$ and LCASE$ are alias for UPPER$ and LOWER$
	HANDLE_OPER2 (UCASE_S, UPPER_S);
	HANDLE_OPER2 (LCASE_S, LOWER_S);

	HANDLE_OPER (ASC);
	HANDLE_OPER (LEN);
	HANDLE_OPER (PEEK);
	HANDLE_OPER (PROGRAMPTR);
	HANDLE_OPER (RND);
	HANDLE_OPER (INT);
	HANDLE_OPER (SIN);
	HANDLE_OPER (COS);
	HANDLE_OPER (PI);
	HANDLE_OPER (TAN);
	HANDLE_OPER (SQR);
	HANDLE_OPER (ASIN);
	HANDLE_OPER (ACOS);
	HANDLE_OPER (INSTR);
	HANDLE_OPER (ATAN);
	HANDLE_OPER (ABS);
	HANDLE_OPER (USR);
	HANDLE_OPER (VAL);
	HANDLE_OPER (EOF);
	HANDLE_OPER (VARPTR);
	HANDLE_OPER (SYSVARPTR);
	HANDLE_OPER (SGN);
	HANDLE_OPER (LOG);
	HANDLE_OPER (LOG10);
	HANDLE_OPER (EXP);
	HANDLE_OPER (TIME);
	HANDLE_OPER (ERR);
	HANDLE_OPER (ERL);
	HANDLE_OPER (CVI);
	HANDLE_OPER (CVS);
	HANDLE_OPER (CVD);
	HANDLE_OPER (CVL);
	HANDLE_OPER (MIN);
	HANDLE_OPER (MAX);
	HANDLE_OPER (CINT);
	HANDLE_OPER (FIX);
	HANDLE_OPER (XMOUSE);
	HANDLE_OPER (YMOUSE);
	HANDLE_OPER (XPOS);
	HANDLE_OPER (YPOS);
	HANDLE_OPER (PEEK16);
	HANDLE_OPER (PEEK32);
	HANDLE_OPER (RINSTR);
	HANDLE_OPER (FIND_FIRST_OF);
	HANDLE_OPER (FIND_LAST_OF);
	HANDLE_OPER (FIND_FIRST_NOT_OF);
	HANDLE_OPER (FIND_LAST_NOT_OF);
	HANDLE_OPER (SINH);
	HANDLE_OPER (COSH);
	HANDLE_OPER (TANH);
	HANDLE_OPER (ASINH);
	HANDLE_OPER (ACOSH);
	HANDLE_OPER (ATANH);
	HANDLE_OPER (ATAN2);
	HANDLE_OPER (TEST);
	HANDLE_OPER (TESTR);
	HANDLE_OPER (POS);
	HANDLE_OPER (VPOS);
	HANDLE_OPER (LOF);
	HANDLE_OPER (FREEFILE);
	HANDLE_OPER (INKEY);
	HANDLE_OPER (ROUND);
	HANDLE_OPER (CVSMBF);
	HANDLE_OPER (CVDMBF);
	HANDLE_OPER (REGEXP_INSTR);
	HANDLE_OPER (ALLOC_MEMORY);
	HANDLE_OPER (LOC);

	HANDLE_OPER (IDENTIFIER);
	HANDLE_OPER (NUMBER);
	HANDLE_OPER (STRING);
	HANDLE_OPER (INTEGER);
	default:
		throw ErrSyntax;
	}

	if (result.type () == VarString)
	{
		if (token.code == '[')
			slice (result);
	}

	#else
	// No OPERATION_SWITCH

	//(this->* array_valfunction [token.code] ) (result);

	(this->* findvalfunc (token.code) ) (result);

	if (result.type () == VarString)
	{
		if (token.code == '[')
			slice (result);
	}

	#endif
	// OPERATION_SWITCH
}

#if 0
void RunnerLineImpl::valparen (BlResult & result)
{
	#if 0
	if (token.code == '(')
	{
		gettoken ();
		eval (result);
		if (token.code != ')')
			throw ErrSyntax;
		gettoken ();
	}
	else
	#endif
		valbase (result);
}
#endif

void RunnerLineImpl::slice (BlResult & result)
{
	using std::string;
	
	string str= result.str ();
	do
	{
		#if 0
		gettoken ();
		string::size_type from= 0;
		string::size_type to= str.size ();
		if (token.code != ']')
		{
			if (token.code != keyTO)
			{
				from= evalstringindex ();
				if (token.code == keyTO)
				{
					gettoken ();
					to= evalstringindex ();
				}
				else
					to= from;
			}
			else
			{
				gettoken ();
				if (token.code != ']')
					to= evalstringindex ();
			}
			requiretoken (']');
		}
		gettoken ();

		if (from > to)
			str.erase ();
		else
		{
			std::string::size_type limit= str.size ();
			if (from >= limit || to >= limit)
				throw ErrBadSubscript;
			str= str.substr (from, to - from + 1);
		}
		#else

		string::size_type from, to;
		evalstringslice (str, from, to);
		if (from + 1 > to)
			str.erase ();
		else
			str= str.substr (from, to - from);

		#endif
	} while (token.code == '[');
	result= str;
}

void RunnerLineImpl::valexponent (BlResult & result)
{
	//valparen (result);
	valbase (result);
	#if 0
	if (result.type () == VarString)
	{
		if (token.code == '[')
			slice (result);
	}
	else
	{
	#endif
		while (token.code == '^')
		{
			gettoken ();
			BlResult guard;
			valunary (guard);
			result= callnumericfunc (pow,
				result.number (), guard.number () );
		}
	//}
}

void RunnerLineImpl::valmod (BlResult & result)
{
	valexponent (result);
	while (token.code == keyMOD)
	{
		gettoken ();
		BlResult guard;
		valexponent (guard);
		result%= guard;
	}
}

void RunnerLineImpl::valunary (BlResult & result)
{
	BlCode op= 0;
	if (token.code == '+' || token.code == '-' || token.code == keyNOT)
	{
		op= token.code;
		gettoken ();
		valunary (result);
	}
	else valmod (result);

	switch (op)
	{
	case 0:
		break;
	case '+':
		if (! result.is_numeric () )
			throw ErrMismatch;
		break;
	case '-':
		result= -result;
		break;
	case keyNOT:
		//result= BlNumber (~ long (result.number () ) );
		{
			sysvar::Flags2Bit f2 (sysvar::getFlags2 () );
			if (f2.has (sysvar::BoolMode) )
			{
				bool r;
				switch (result.type () )
				{
				case VarInteger:
					r= ! result.integer (); break;
				case VarNumber:
					r= ! result.number (); break;
				default:
					throw ErrMismatch;
				}
				result= r ?
					(f2.has (sysvar::TruePositive) ?
						1 :
						-1) :
					0;
			}
			else
				result= ~ result.integer ();
		}
		break;
	default:
		throw ErrBlassicInternal;
	}
}

void RunnerLineImpl::valdivint (BlResult & result)
{
	valunary (result);
	BlCode op= token.code;
	while (op == '\\')
	{
		BlResult guard;
		gettoken ();
		valunary (guard);
		//BlInteger g= guard.integer ();
		//if (g == 0)
		//	throw ErrDivZero;
		//result= result.integer () / g;
		result.integerdivideby (guard);
		op= token.code;
	}
}

void RunnerLineImpl::valmuldiv (BlResult & result)
{
	//valunary (result);
	valdivint (result);
	BlCode op= token.code;
	while (op == '*' || op == '/')
	{
		BlResult guard;
		gettoken ();
		//valunary (guard);
		valdivint (guard);
		switch (op)
		{
		case '*':
			result*= guard;
			break;
		case '/':
			result/= guard;
			break;
		default:
			;
		}
		op= token.code;
	}
}

void RunnerLineImpl::valplusmin (BlResult & result)
{
	valmuldiv (result);
	BlCode op= token.code;
	while (op == '+' || op == '-')
	{
		BlResult guard;
		gettoken ();
		valmuldiv (guard);
		switch (op)
		{
		case '+':
			result+= guard;
			break;
		case '-':
			result-= guard;
			break;
		}
		op= token.code;
	}
}

void RunnerLineImpl::valcomp (BlResult & result)
{
	valplusmin (result);
	BlCode op= token.code;
	if (iscomp (op) )
	{
		bool true_positive= sysvar::hasFlags2 (sysvar::TruePositive);
		BlResult guard;
		bool r;
		do
		{
			gettoken ();
			valplusmin (guard);
			switch (op)
			{
			case '=':
				r= (result == guard);
				break;
			case keyDISTINCT:
				r= (result != guard);
				break;
			case '<':
				r= (result < guard);
				break;
			case keyMINOREQUAL:
				r= (result <= guard);
				break;
			case '>':
				r= (result > guard);
				break;
			case keyGREATEREQUAL:
				r= (result >= guard);
				break;
			default:
				throw ErrBlassicInternal;
			}
			result= r ? (true_positive ? 1 : -1) : 0;
			op= token.code;
		} while (iscomp (op) );
	}
}

void RunnerLineImpl::valorand (BlResult & result)
{
	valcomp (result);
	BlCode op= token.code;
	if (islogical2 (op) )
	{
		bool boolean_mode= sysvar::hasFlags2 (sysvar::BoolMode);
		bool true_positive= sysvar::hasFlags2 (sysvar::TruePositive);
		BlResult guard;
		do
		{
			gettoken ();
			valcomp (guard);
			if (boolean_mode)
			{
				bool r;
				bool r1= result.tobool ();
				bool r2= guard.tobool ();
				switch (op)
				{
				case keyOR:
					r= r1 || r2;
					break;
				case keyAND:
					r= r1 && r2;
					break;
				case keyXOR:
					r= r1 != r2;
					break;
				default:
					throw ErrBlassicInternal;
				}
				result= r ? (true_positive ? 1 : -1) : 0;
			}
			else
			{
				BlInteger n1= result.integer ();
				BlInteger n2= guard.integer ();
				switch (op)
				{
				case keyOR:
					//result= BlNumber (n1 | n2);
					result= n1 | n2;
					break;
				case keyAND:
					//result= BlNumber (n1 & n2);
					result= n1 & n2;
					break;
				case keyXOR:
					//result= BlNumber (n1 ^ n2);
					result= n1 ^ n2;
					break;
				default:
					throw ErrBlassicInternal;
				}
			}
			op= token.code;
		} while (islogical2 (op) );
	}
}

void RunnerLineImpl::eval (BlResult & result)
{
	valorand (result);
}

void RunnerLineImpl::expect (BlResult & result)
{
	gettoken ();
	eval (result);
}

BlLineNumber RunnerLineImpl::evallinenumber ()
{
	BlLineNumber bln;
	switch (token.code)
	{
	case keyNUMBER:
		bln= blassic::result::NumberToInteger (token.number () );
		break;
	case keyINTEGER:
		bln= BlLineNumber (token.integer () );
		break;
	case keyIDENTIFIER:
		bln= program.getlabel (token.str);
		if (bln == LineEndProgram)
			throw ErrNoLabel;
		break;
	default:
		throw ErrSyntax;
	}
	if (bln > BlMaxLineNumber)
	{
		if (showdebuginfo () )
			cerr << "Invalid line number";
		throw ErrSyntax;
	}
	gettoken ();
	return bln;
}

void RunnerLineImpl::evallinerange (BlLineNumber & blnBeg,
	BlLineNumber & blnEnd)
{
	blnBeg= LineBeginProgram;
	blnEnd= LineEndProgram;
	if (! endsentence () && token.code != ',')
	{
		if (token.code != '-')
		{
			blnBeg= evallinenumber ();
			if (token.code == '-')
			{
				gettoken ();
				if (! endsentence () && token.code != ',')
					blnEnd= evallinenumber ();
			}
			else
				blnEnd= blnBeg;
		}
		else
		{
			gettoken ();
			blnEnd= evallinenumber ();
		}
	}
}

Dimension RunnerLineImpl::getdims ()
{
	//TRACEFUNC (tr, "RunnerLineImpl::getdims");
	ASSERT (token.code == '(');

	Dimension dims;
	do
	{
		dims.add (expectinteger () );
	} while (token.code == ',');
	requiretoken (')');
	gettoken ();
	return dims;
}

Dimension RunnerLineImpl::evaldims ()
{
	requiretoken ('(');
	return getdims ();
}

Dimension RunnerLineImpl::expectdims ()
{
	expecttoken ('(');
	return getdims ();
}

void RunnerLineImpl::errorifparam ()
{
	gettoken ();
	require_endsentence ();
}

void RunnerLineImpl::gosub_line (BlLineNumber dest)
{
	//ProgramPos posgosub= runner.getposactual ();
	ProgramPos posgosub (getposactual () );
	posgosub.nextchunk ();
	#if 1
	//if (token.code == keyENDLINE && posgosub.getnum () != 0)
	if (token.code == keyENDLINE &&
		posgosub.getnum () != LineDirectCommand)
	{
		#if 1
		posgosub.nextline ();
		#else
		BlLineNumber num= program.getnextnum (line);
		if (num != 0)
			posgosub= num;
		#endif
	}
	#endif
	runner.gosub_line (dest, posgosub);
}

void RunnerLineImpl::getinkparams ()
{
	if (endsentence () )
		return;
	requiretoken (',');
	gettoken ();
	if (token.code != ',')
	{
		BlInteger ink= evalinteger ();
		graphics::setcolor (ink);
		if (token.code != ',')
		{
			require_endsentence ();
			return;
		}
	}
	gettoken ();
	BlInteger inkmode= evalinteger ();
	require_endsentence ();
	graphics::setdrawmode (inkmode);
}

void RunnerLineImpl::getdrawargs (BlInteger & y)
{
	requiretoken (',');
	y= expectinteger ();
	getinkparams ();
}

void RunnerLineImpl::getdrawargs (BlInteger & x, BlInteger & y)
{
	x= expectinteger ();
	getdrawargs (y);
}

void RunnerLineImpl::make_clear ()
{
	runner.clear ();
	runner.clearerrorgoto ();
	clearvars ();
	Function::clear ();
	graphics::clear_images ();
	definevar (VarNumber, 'A', 'Z');
	runner.trigonometric_default ();
}

bool RunnerLineImpl::syntax_error ()
{
	TRACEFUNC (tr, "RunnerLineImpl::syntax_error");

	throw ErrSyntax;
}

void RunnerLineImpl::valsyntax_error (BlResult &)
{
	TRACEFUNC (tr, "RunnerLineImpl::valsyntax_error");

	throw ErrSyntax;
}

bool RunnerLineImpl::execute_instruction ()
{
	//TRACEFUNC (tr, "RunnerLineImpl::execute_instruction");

	#ifndef INSTRUCTION_SWITCH

	if ( (this->*findfunc (token.code) ) () )
	{
		//TRMESSAGE (tr, "Instruction returned true");
		return true;
	}
	else
		return false;

	#else

#	define HANDLE_ELEM(elem) \
		case key##elem: \
			return do_##elem ()

#	define HANDLE_ELEM2(elem1, elem2) \
		case key##elem1: \
			return do_##elem2 ()

	switch (token.code)
	{
	HANDLE_ELEM (Colon);
	HANDLE_ELEM (END);
	HANDLE_ELEM (LIST);
	HANDLE_ELEM (REM);
	HANDLE_ELEM (LOAD);
	HANDLE_ELEM (SAVE);
	HANDLE_ELEM (NEW);
	HANDLE_ELEM (EXIT);
	HANDLE_ELEM (RUN);
	HANDLE_ELEM (PRINT);
	HANDLE_ELEM (FOR);
	HANDLE_ELEM (NEXT);
	HANDLE_ELEM (IF);
	HANDLE_ELEM (ELSE);
	HANDLE_ELEM (TRON);
	HANDLE_ELEM (TROFF);
	HANDLE_ELEM (LET);
	HANDLE_ELEM (GOTO);
	HANDLE_ELEM (STOP);
	HANDLE_ELEM (CONT);
	HANDLE_ELEM (CLEAR);
	HANDLE_ELEM (GOSUB);
	HANDLE_ELEM (RETURN);
	HANDLE_ELEM (POKE);
	HANDLE_ELEM (DATA);
	HANDLE_ELEM (READ);
	HANDLE_ELEM (RESTORE);
	HANDLE_ELEM (INPUT);
	HANDLE_ELEM (LINE);
	HANDLE_ELEM (RANDOMIZE);
	HANDLE_ELEM (PLEASE);
	HANDLE_ELEM (AUTO);
	HANDLE_ELEM (DIM);
	HANDLE_ELEM (SYSTEM);
	HANDLE_ELEM (ON);
	HANDLE_ELEM (ERROR);
	HANDLE_ELEM (OPEN);
	HANDLE_ELEM (CLOSE);
	HANDLE_ELEM (LOCATE);
	HANDLE_ELEM (CLS);
	HANDLE_ELEM (WRITE);
	HANDLE_ELEM (MODE);
	HANDLE_ELEM (MOVE);
	HANDLE_ELEM (COLOR);
	HANDLE_ELEM (GET);
	HANDLE_ELEM (LABEL);
	HANDLE_ELEM (DELIMITER);
	HANDLE_ELEM (REPEAT);
	HANDLE_ELEM (UNTIL);
	HANDLE_ELEM (WHILE);
	HANDLE_ELEM (WEND);
	HANDLE_ELEM (PLOT);
	HANDLE_ELEM (POPEN);
	HANDLE_ELEM (RESUME);
	HANDLE_ELEM (DELETE);
	HANDLE_ELEM (LOCAL);
	HANDLE_ELEM (PUT);
	HANDLE_ELEM (FIELD);
	HANDLE_ELEM (LSET);

	// Lset and rset use same function.
	HANDLE_ELEM2 (RSET, LSET);

	HANDLE_ELEM (SOCKET);
	HANDLE_ELEM (DRAW);
	HANDLE_ELEM (DEF);
	HANDLE_ELEM (FN);
	HANDLE_ELEM (ERASE);
	HANDLE_ELEM (SWAP);
	HANDLE_ELEM (SYMBOL);
	HANDLE_ELEM (ZONE);
	HANDLE_ELEM (POP);
	HANDLE_ELEM (NAME);
	HANDLE_ELEM (KILL);
	HANDLE_ELEM (FILES);
	HANDLE_ELEM (PAPER);
	HANDLE_ELEM (PEN);
	HANDLE_ELEM (SHELL);
	HANDLE_ELEM (MERGE);
	HANDLE_ELEM (CHDIR);
	HANDLE_ELEM (MKDIR);
	HANDLE_ELEM (RMDIR);
	HANDLE_ELEM (SYNCHRONIZE);
	HANDLE_ELEM (PAUSE);
	HANDLE_ELEM (CHAIN);
	HANDLE_ELEM (ENVIRON);
	HANDLE_ELEM (EDIT);
	HANDLE_ELEM (DRAWR);
	HANDLE_ELEM (PLOTR);
	HANDLE_ELEM (MOVER);
	HANDLE_ELEM (POKE16);
	HANDLE_ELEM (POKE32);
	HANDLE_ELEM (RENUM);
	HANDLE_ELEM (CIRCLE);
	HANDLE_ELEM (MASK);
	HANDLE_ELEM (WINDOW);
	HANDLE_ELEM (GRAPHICS);
	HANDLE_ELEM (BEEP);
	HANDLE_ELEM (DEFINT);

	// DEFINT, DEFSTR, DEFREAL, DEFSNG and DEFDBL use same function.
	HANDLE_ELEM2 (DEFSTR, DEFINT);
	HANDLE_ELEM2 (DEFREAL, DEFINT);
	HANDLE_ELEM2 (DEFSNG, DEFINT);
	HANDLE_ELEM2 (DEFDBL, DEFINT);

	HANDLE_ELEM (INK);
	HANDLE_ELEM (SET_TITLE);
	HANDLE_ELEM (TAG);

	// TAG and TAGOFF use same function.
	HANDLE_ELEM2 (TAGOFF, TAG);

	HANDLE_ELEM (ORIGIN);
	HANDLE_ELEM (DEG);

	// DEG and RAD use same function.
	HANDLE_ELEM2 (RAD, DEG);

	HANDLE_ELEM (INVERSE);
	HANDLE_ELEM (IF_DEBUG);

	// LPRINT and PRINT use same function.
	HANDLE_ELEM2 (LPRINT, PRINT);

	HANDLE_ELEM (LLIST);
	HANDLE_ELEM (WIDTH);
	HANDLE_ELEM (BRIGHT);
	HANDLE_ELEM (DRAWARC);
	HANDLE_ELEM (PULL);
	HANDLE_ELEM (PAINT);
	HANDLE_ELEM (FREE_MEMORY);
	HANDLE_ELEM (SCROLL);
	HANDLE_ELEM (ZX_PLOT);
	HANDLE_ELEM (ZX_UNPLOT);

	HANDLE_ELEM (MID_S);
	HANDLE_ELEM (PROGRAMARG_S);

	HANDLE_ELEM (IDENTIFIER);
	HANDLE_ELEM (NUMBER);
	HANDLE_ELEM2 (INTEGER, NUMBER);
	HANDLE_ELEM (ENDLINE);

	default:
		throw ErrSyntax;
	}

	#endif
}

void RunnerLineImpl::execute ()
{
	TRACEFUNC (tr, "RunnerLineImpl::execute");

	// Cleaned and clarified the code of this function.

	fInElse= false;
	//codprev= 0;
	codprev= codeline.actualcode ();

	for (;;)
	{
	//next_instruction:

		#ifndef BLASSIC_NO_GRAPHICS
		// Allows refreshing of the graphics window.

		#ifndef BLASSIC_USE_WINDOWS
		// In windows a thread takes care of graphics,
		// in others frequent calls to graphics::idle
		// are needed to handle events in the graphics
		// window.

		// Avoid calling idle too many times.
		// Someone wants to tune this?
		if (graphics::ingraphicsmode () )
		{
			static size_t counter= 0;
			if (counter ++ == 100)
			{
				graphics::idle ();
				counter= 0;
			}
		}
		#endif

		#endif


		// Testing little change: control this after executing
		// the instruction.

		#if 0

		// ELSE control. Does not allow a precise syntax
		// checking, but actually works.
		// ELSE can be found in four ways:
		// - When IF look for it.
		// - As the end of a instruction.
		// - After :
		// - As the firsts instruction of one line.
		// Here are treated the two first cases, the
		// others are processed as a normal instruction.

		codprev= codeline.actualcode ();
		if (codprev == keyELSE)
		{
			if (fInElse)
			{
				// Was found by IF, process the rest
				// of the line.
				fInElse= false;
			}
			else
			{
				// Not found by if. Exit the line.
				//break;
				return;
			}
		}

		#endif

		gettoken ();
		actualchunk= codeline.chunk ();

		if (fInterrupted)
		{
			if (runner.getbreakstate () != onbreak::BreakCont)
				throw BlBreak ();
			fInterrupted= false;
		}

		if (runner.channelspolled () )
		{
			BlLineNumber line= runner.getpollnumber ();
			if (line != LineEndProgram)
			{
				runner.gosub_line (line, getposactual (),
					true);
				//break;
				return;
			}
		}

		#if 1

		bool finishline= execute_instruction ();
		if (finishline)
			//break;
			return;

		#else

		#ifndef INSTRUCTION_SWITCH

		if ( (this->*findfunc (token.code) ) () )
		{
			//TRMESSAGE (tr, "Instruction returned true");
			break;
		}

		#endif

		#endif

		// Test.
		//do_func f= findfunc (token.code);
		//bool r= (this->* f) ();
		//if (r)
		//	break;

		// This makes a tiny speed improvement with my test
		// programs, perhaps in other programs can be the
		// contrary? Anyway, the difference is not important,
		// can be supressed and all works.
		if (token.code == keyENDLINE)
		{
			//TRMESSAGE (tr, "End of line");
			//break;
			return;
		}

		#if 1

		codprev= token.code;
		if (codprev == keyELSE)
		{
			//TRMESSAGE (tr, "ELSE after instruction");
			if (fInElse)
			{
				// Was found by IF, process the rest
				// of the line.
				fInElse= false;
			}
			else
			{
				// Not found by if. Exit the line.
				//break;
				return;
			}
		}

		#endif

	//goto next_instruction;

	} // for (;;)
}

// End of runnerline_impl.cpp
