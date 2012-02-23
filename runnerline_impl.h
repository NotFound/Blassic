#ifndef INCLUDE_BLASSIC_RUNNERLINE_IMPL_H
#define INCLUDE_BLASSIC_RUNNERLINE_IMPL_H

// runnerline_impl.h
// Revision 23-jan-2005

#include "runnerline.h"

#include "blassic.h"
#include "error.h"
#include "result.h"
#include "codeline.h"
#include "file.h"
#include "function.h"
#include "var.h"

#include <list>
#include <algorithm>

class Runner;
class Program;
class LocalLevel;
class Directory;

//#define INSTRUCTION_SWITCH
//#define OPERATION_SWITCH

#define BRUTAL_MODE
#define ONE_TABLE

class RunnerLineImpl : public RunnerLine {
public:
	typedef blassic::result::BlResult BlResult;

	RunnerLineImpl (Runner & runner);
	RunnerLineImpl (Runner & runner, const CodeLine & newcodeline);
	~RunnerLineImpl ();

	void setcodeline (const CodeLine & newcodeline)
	{
		codeline= newcodeline;
	}
	CodeLine & getcodeline () { return codeline; }

	bool execute_instruction ();
	void execute ();

	BlLineNumber number () const { return codeline.number (); }

	ProgramPos getposactual () const
	{
		//return ProgramPos (pline->number (), actualchunk);
		return ProgramPos (codeline.number (), actualchunk);
	}
private:
	Program & program;
	CodeLine::Token token;
	BlCode codprev;

	BlChunk actualchunk;
	Directory * pdirectory;

	// Instrucion execution.

	bool fInElse;

	// ************ Dispatch functions ****************

	#ifndef INSTRUCTION_SWITCH
	typedef bool (RunnerLineImpl::* do_func) ();
	#endif

	#ifndef OPERATION_SWITCH
	typedef void (RunnerLineImpl::* do_valfunction) (BlResult &);
	#endif

	#ifdef ONE_TABLE

	#ifdef INSTRUCTION_SWITCH
	#error Incompatible options
	#endif

	struct functions_t {
		do_func inst_func;
		do_valfunction val_func;
	};
	struct tfunctions_t {
		BlCode code;
		functions_t f;
		bool operator < (const tfunctions_t & t) const
		{
			return code < t.code;
		}
	};
	static const tfunctions_t tfunctions [];
	static const tfunctions_t * tfunctionsend;

	#ifdef BRUTAL_MODE

	static functions_t array_functions [];

	#endif

	#else
	// No ONE_TABLE

	// ************ Instruction functions *************

	#ifndef INSTRUCTION_SWITCH

	struct tfunc_t {
		BlCode code;
		do_func f;
		bool operator < (const tfunc_t & t) const
		{
			return code < t.code;
		}
	};

	static const tfunc_t tfunc [];
	static const tfunc_t * tfuncend;

	#endif

	// ************ Val functions *****************

	#ifndef OPERATION_SWITCH

	struct valfunction_t {
		BlCode code;
		do_valfunction f;
		bool operator < (const valfunction_t & t) const
		{
			return code < t.code;
		}
	};

	static const valfunction_t valfunction [];
	static const valfunction_t * valfunctionend;

	#endif

	#ifdef BRUTAL_MODE

	static do_valfunction array_valfunction [];

	static do_func array_func [];

	#endif

	#endif
	// ONE_TABLE

	#ifdef BRUTAL_MODE

	static bool init_array_func ();
	static bool array_func_inited;

	#endif

	#ifndef NDEBUG
	static bool checktfunc ();
	static const bool tfuncchecked;
	#endif

	#ifndef INSTRUCTION_SWITCH
	do_func findfunc (BlCode code);
	#endif

	#ifndef OPERATION_SWITCH
	do_valfunction findvalfunc (BlCode code);
	#endif


	//*****************************************************


	bool syntax_error ();
	void valsyntax_error (BlResult &);

	void getnextchunk ();

	blassic::file::BlFile & getfile (BlChannel channel) const;
	blassic::file::BlFile & getfile0 () const;

	void gettoken () { codeline.gettoken (token); }

	bool endsentence () { return token.isendsentence (); }
	void require_endsentence () const // throw (BlErrNo)
	{
		if (! token.isendsentence () )
			throw ErrSyntax;
	}

	#if 0
	// Use macros instead to avoid strange errors on hp-ux.
	void requiretoken (BlCode code) const throw (BlErrNo);
	void expecttoken (BlCode code) throw (BlErrNo);
	#endif

	BlNumber evalnum ();
	BlNumber expectnum ();
	BlInteger evalinteger ();
	BlInteger expectinteger ();
	std::string evalstring ();
	std::string expectstring ();

	BlChannel evalchannel ();
	BlChannel expectchannel ();
	BlChannel evaloptionalchannel (BlChannel defchan= DefaultChannel);
	BlChannel expectoptionalchannel (BlChannel defchan= DefaultChannel);
	BlChannel evalrequiredchannel ();
	BlChannel expectrequiredchannel ();

	void parenarg (BlResult & result);
	void getparenarg (BlResult & result);
	void getparenarg (BlResult & result, BlResult & result2);
	blassic::file::BlFile & getparenfile ();

	void valnumericfunc (double (* f) (double), BlResult & result);
	void valnumericfunc2 (double (* f) (double, double),
		BlResult & result);
	void valtrigonometricfunc
		(double (* f) (double), BlResult & result);
	void valtrigonometricinvfunc
		(double (* f) (double), BlResult & result);

	void val_ASC (BlResult & result);
	void val_LEN (BlResult & result);
	void val_PEEK (BlResult & result);
	void val_PEEK16 (BlResult & result);
	void val_PEEK32 (BlResult & result);
	void val_PROGRAMPTR (BlResult & result);
	void val_SYSVARPTR (BlResult & result);
	void val_RND (BlResult & result);
	void val_INT (BlResult & result);
	void val_SIN (BlResult & result);
	void val_COS (BlResult & result);
	void val_PI (BlResult & result);
	void val_TAN (BlResult & result);
	void val_SQR (BlResult & result);
	void val_ASIN (BlResult & result);
	void val_ACOS (BlResult & result);
	void val_ATAN (BlResult & result);
	void val_ABS (BlResult & result);
	void val_LOG (BlResult & result);
	void val_LOG10 (BlResult & result);
	void val_EXP (BlResult & result);
	void val_TIME (BlResult & result);
	void val_ERR (BlResult & result);
	void val_ERL (BlResult & result);
	void val_FIX (BlResult & result);
	void val_XMOUSE (BlResult & result);
	void val_YMOUSE (BlResult & result);
	void val_XPOS (BlResult & result);
	void val_YPOS (BlResult & result);
	void val_SINH (BlResult & result);
	void val_COSH (BlResult & result);
	void val_TANH (BlResult & result);
	void val_ASINH (BlResult & result);
	void val_ACOSH (BlResult & result);
	void val_ATANH (BlResult & result);
	void val_ATAN2 (BlResult & result);

	void valinstrbase (BlResult & result, bool reverse);
	void val_INSTR (BlResult & result);
	void val_RINSTR (BlResult & result);

	void valfindfirstlast (BlResult & result, bool first, bool yesno);
	void val_FIND_FIRST_OF (BlResult & result);
	void val_FIND_LAST_OF (BlResult & result);
	void val_FIND_FIRST_NOT_OF (BlResult & result);
	void val_FIND_LAST_NOT_OF (BlResult & result);

	void val_USR (BlResult & result);
	void val_VAL (BlResult & result);
	void val_EOF (BlResult & result);
	void val_VARPTR (BlResult & result);
	void val_SGN (BlResult & result);
	void val_CVI (BlResult & result);
	void val_CVS (BlResult & result);
	void val_CVD (BlResult & result);
	void val_CVL (BlResult & result);
	void val_MIN (BlResult & result);
	void val_MAX (BlResult & result);
	void val_CINT (BlResult & result);
	void val_TEST (BlResult & result);
	void val_TESTR (BlResult & result);
	void val_POS (BlResult & result);
	void val_VPOS (BlResult & result);
	void val_LOF (BlResult & result);
	void val_FREEFILE (BlResult & result);
	void val_INKEY (BlResult & result);
	void val_ROUND (BlResult & result);
	void val_CVSMBF (BlResult & result);
	void val_CVDMBF (BlResult & result);
	void val_REGEXP_INSTR (BlResult & result);
	void val_ALLOC_MEMORY (BlResult & result);
	void val_LOC (BlResult & result);

	void val_MID_S (BlResult & result);
	void val_LEFT_S (BlResult & result);
	void val_RIGHT_S (BlResult & result);
	void val_CHR_S (BlResult & result);
	void val_ENVIRON_S (BlResult & result);
	void val_STRING_S (BlResult & result);
	void val_OSFAMILY_S (BlResult & result);
	void val_OSNAME_S (BlResult & result);
	void val_HEX_S (BlResult & result);
	void val_SPACE_S (BlResult & result);
	void val_UPPER_S (BlResult & result);
	void val_LOWER_S (BlResult & result);
	void val_STR_S (BlResult & result);
	void val_OCT_S (BlResult & result);
	void val_BIN_S (BlResult & result);
	void val_INKEY_S (BlResult & result);
	void val_PROGRAMARG_S (BlResult & result);
	void val_DATE_S (BlResult & result);
	void val_TIME_S (BlResult & result);
	void val_INPUT_S (BlResult & result);
	void val_MKI_S (BlResult & result);
	void val_MKS_S (BlResult & result);
	void val_MKD_S (BlResult & result);
	void val_MKL_S (BlResult & result);
	void valtrimbase (BlResult & result, bool tleft, bool trigth);
	void val_TRIM_S (BlResult & result);
	void val_LTRIM_S (BlResult & result);
	void val_RTRIM_S (BlResult & result);
	void val_FINDFIRST_S (BlResult & result);
	void val_FINDNEXT_S (BlResult & result);
	void val_COPYCHR_S (BlResult & result);
	void val_STRERR_S (BlResult & result);
	void val_DEC_S (BlResult & result);
	void val_VAL_S (BlResult & result);
	void val_SCREEN_S (BlResult & result);
	void val_MKSMBF_S (BlResult & result);
	void val_MKDMBF_S (BlResult & result);
	void val_REGEXP_REPLACE_S (BlResult & result);

	void val_LET (BlResult & result);
	void val_LABEL (BlResult & result);
	void val_IDENTIFIER (BlResult & result);
	void val_NUMBER (BlResult & result);
	void val_STRING (BlResult & result);
	void val_INTEGER (BlResult & result);
	void val_OpenPar (BlResult & result);

public:
	void callfn (Function & f, const std::string & fname,
		LocalLevel & ll, BlResult & result);
private:
	void val_FN (BlResult & result);

	void valsubindex (const std::string & varname, BlResult & result);
	void valbase (BlResult & result);
	//void valparen (BlResult & result);

	void slice (BlResult & result);

	void valexponent (BlResult & result);
	void valmod (BlResult & result);
	void valunary (BlResult & result);
	void valdivint (BlResult & result);
	void valmuldiv (BlResult & result);
	void valplusmin (BlResult & result);
	void valcomp (BlResult & result);
	void valorand (BlResult & result);

	void eval (BlResult & result);
	void expect (BlResult & result);

	std::string::size_type evalstringindex ();
	void evalstringslice (const std::string & str,
		std::string::size_type & from, std::string::size_type & to);
	void assignslice (VarPointer & vp, const BlResult & result);

	VarPointer evalvarpointer ();
	typedef std::list <VarPointer> ListVarPointer;
	void evalmultivarpointer (ListVarPointer & lvp);
	VarPointer eval_let ();

	BlLineNumber evallinenumber ();
	void evallinerange (BlLineNumber & blnBeg, BlLineNumber & blnEnd);
	Dimension getdims ();
	Dimension evaldims ();
	Dimension expectdims ();

	void errorifparam ();
	void gosub_line (BlLineNumber dest);

	void getinkparams ();
	void getdrawargs (BlInteger & y);
	void getdrawargs (BlInteger & x, BlInteger & y);

	void make_clear ();

	void print_using (blassic::file::BlFile & out);
	void letsubindex (const std::string &varname);
	void do_line_input ();
	void do_get_image ();
	void do_put_image ();
	void definevars (VarType type);
	void do_graphics_pen ();
	void do_graphics_paper ();
	void do_graphics_cls ();
	bool do_def_fn ();
	void do_list (BlChannel nfile);

	bool do_Colon ();
	bool do_ENDLINE ();
	bool do_INTEGER ();
	bool do_NUMBER ();
	bool do_END ();
	bool do_LIST ();
	bool do_LLIST ();
	bool do_REM ();
	bool do_LOAD ();
	bool do_SAVE ();
	bool do_NEW ();
	bool do_EXIT ();
	bool do_RUN ();
	bool do_PRINT ();
	bool do_FOR ();
	bool do_NEXT ();
	bool do_IF ();
	bool do_TRON ();
	bool do_TROFF ();
	bool do_IDENTIFIER ();
	bool do_LET ();
	bool do_GOTO ();
	bool do_GOSUB ();
	bool do_STOP ();
	bool do_CONT ();
	bool do_CLEAR ();
	bool do_RETURN ();
	bool do_POKE ();
	bool do_READ ();
	bool do_DATA ();
	bool do_RESTORE ();
	bool do_INPUT ();
	bool do_LINE ();
	bool do_RANDOMIZE ();
	bool do_AUTO ();
	bool do_DIM ();
	bool do_SYSTEM ();
	bool do_ON ();
	bool do_ERROR ();
	bool do_OPEN ();
	bool do_POPEN ();
	bool do_CLOSE ();
	bool do_LOCATE ();
	bool do_CLS ();
	bool do_WRITE ();
	bool do_MODE ();
	bool do_MOVE ();
	bool do_COLOR ();
	bool do_GET ();
	bool do_LABEL ();
	bool do_DELIMITER ();
	bool do_REPEAT ();
	bool do_UNTIL ();
	bool do_WHILE ();
	bool do_WEND ();
	bool do_PLOT ();
	bool do_RESUME ();
	bool do_DELETE ();
	bool do_LOCAL ();
	bool do_PUT ();
	bool do_FIELD ();
	bool do_LSET ();
	bool do_SOCKET ();
	bool do_MID_S ();
	bool do_DRAW ();
	bool do_DEF ();
	bool do_FN ();
	bool do_PROGRAMARG_S ();
	bool do_ERASE ();
	bool do_SWAP ();
	bool do_SYMBOL ();
	bool do_ZONE ();
	bool do_POP ();
	bool do_NAME ();
	bool do_KILL ();
	bool do_FILES ();
	bool do_PAPER ();
	bool do_PEN ();
	bool do_SHELL ();
	bool do_MERGE ();
	bool do_CHDIR ();
	bool do_MKDIR ();
	bool do_RMDIR ();
	bool do_SYNCHRONIZE ();
	bool do_PAUSE ();
	bool do_CHAIN ();
	bool do_ENVIRON ();
	bool do_EDIT ();
	bool do_DRAWR ();
	bool do_PLOTR ();
	bool do_MOVER ();
	bool do_POKE16 ();
	bool do_POKE32 ();
	bool do_RENUM ();
	bool do_CIRCLE ();
	bool do_MASK ();
	bool do_WINDOW ();
	bool do_GRAPHICS ();
	bool do_BEEP ();
	bool do_DEFINT ();
	bool do_INK ();
	bool do_SET_TITLE ();
	bool do_TAG ();
	bool do_ORIGIN ();
	bool do_DEG ();
	bool do_INVERSE ();
	bool do_IF_DEBUG ();
	bool do_WIDTH ();
	bool do_BRIGHT ();
	bool do_PLEASE ();
	bool do_DRAWARC ();
	bool do_PULL ();
	bool do_PAINT ();
	bool do_FREE_MEMORY ();
	bool do_SCROLL ();
	bool do_ZX_PLOT ();
	bool do_ZX_UNPLOT ();
	bool do_ELSE ();
};

#endif

// End of runnerline_impl.h
