// runnerline_instructions.cpp
// Revision 24-apr-2009

#include "runnerline_impl.h"

#include "error.h"
#include "runner.h"
#include "program.h"
#include "sysvar.h"
#include "graphics.h"
#include "util.h"
using util::to_string;
#include "memory.h"
#include "directory.h"
#include "charset.h"

#include "trace.h"

#include <memory>
using std::auto_ptr;

#include <iostream>
using std::cerr;
using std::endl;
using std::flush;

#include <string.h>
#include <time.h>

#include <cassert>
#define ASSERT assert

namespace sysvar= blassic::sysvar;
namespace onbreak= blassic::onbreak;

using namespace blassic::file;

#define requiretoken(c) if (token.code == c) ; else throw ErrSyntax

#define expecttoken(c) \
	do { \
		gettoken (); \
		if (token.code != c) throw ErrSyntax; \
	} while (false)


bool RunnerLineImpl::do_Colon ()
{
	return false;
}

bool RunnerLineImpl::do_ENDLINE ()
{
	return true;
}

bool RunnerLineImpl::do_INTEGER ()
{
	// goto omitido

	if (codprev != keyELSE && codprev != keyTHEN)
	{
		if (codprev != keyGOTO && codprev != keyGOSUB)
			throw ErrSyntax;
		if (! sysvar::hasFlags1 (sysvar::ThenOmitted) )
			throw ErrBlassicInternal;
			// Not ErrSyntax, this is not supposed to happen.
	}

	BlLineNumber bln= evallinenumber ();
	require_endsentence ();

	if (codprev != keyGOSUB)
		runner.goto_to (bln);
	else
		gosub_line (bln);

	return true;
}

bool RunnerLineImpl::do_NUMBER ()
{
	return do_INTEGER ();
}

bool RunnerLineImpl::do_END ()
{
	errorifparam ();

	#if 1

	runner.close_all ();
	runner.setstatus (Runner::Ended);
	return true;

	#else

	throw blassic::ProgramEnd ();

	#endif
}

void RunnerLineImpl::do_list (BlChannel nfile)
{
	// Same function used for LIST and LLIST, differing only
	// in the default channel.

	//BlChannel nfile= (token.code == keyLLIST) ?
	//	PrinterChannel : DefaultChannel;
	gettoken ();
	if (token.code == '#')
	{
		nfile= expectchannel ();
		if (!endsentence () )
		{
			requiretoken (',');
			gettoken ();
		}
	}

	BlLineNumber iniline, endline;
	evallinerange (iniline, endline);

	require_endsentence ();

	program.list (iniline, endline, getfile (nfile) );
}

bool RunnerLineImpl::do_LIST ()
{
	do_list (DefaultChannel);
	return false;
}

bool RunnerLineImpl::do_LLIST ()
{
	do_list (PrinterChannel);
	return false;
}

bool RunnerLineImpl::do_REM ()
{
	return true;
}

bool RunnerLineImpl::do_LOAD ()
{
	using std::ifstream;
	using std::ios;

	std::string progname= expectstring ();
	if (endsentence () )
	{
		program.load (progname);
		runner.setstatus (Runner::Ended);
		return true;
	}
	requiretoken (',');
	gettoken ();
	if (token.code == keyIDENTIFIER &&
			typeofvar (token.str) == VarString)
	{
		std::string varname= token.str;
		gettoken ();
		require_endsentence ();

		ifstream is (progname.c_str (), ios::binary | ios::in);
		// Without explicit in read doesn't work on hp-ux,
		// I don't know why.

		if (! is.is_open () )
			throw ErrFileNotFound;
		is.seekg (0, std::ios::end);
		size_t size= is.tellg ();
		is.seekg (0, std::ios::beg);
		std::string result;
		// Opening a block to avoid an error on Borland C++ that
		// calls ~auto_buffer when throwing ErrFileNotFound
		{
			util::auto_buffer <char> aux (size);
			is.read (aux, size);
			result.assign (aux, size);
		}
		assignvarstring (varname, result);
	}
	else
	{
		char * init;
		{
			BlNumber bn= evalnum ();
			init= reinterpret_cast <char *> (size_t (bn) );
		}
		size_t len= 0;
		if (token.code == ',')
		{
			BlNumber bn= expectnum ();
			len= size_t (bn);
		}
		require_endsentence ();

		ifstream is (progname.c_str (), std::ios::binary);
		if (! is.is_open () )
			throw ErrFileNotFound;
		if (len == 0)
		{
			is.seekg (0, std::ios::end);
			len= is.tellg ();
			is.seekg (0, std::ios::beg);
		}
		is.read (init, len);
	}
	return false;
}

bool RunnerLineImpl::do_SAVE ()
{
	using std::ofstream;

	std::string progname= expectstring ();
	if (endsentence () )
		program.save (progname);
	else
	{
		requiretoken (',');
		expecttoken (keyIDENTIFIER);
		if (token.str.size () != 1)
			throw ErrSyntax;
		char c= char (toupper (token.str [0] ) );
		switch (c)
		{
		case 'A':
			gettoken ();
			require_endsentence ();
			{
				//BlFileRegular fout (progname, BlFile::Output);
				//program.list (LineBeginProgram,
				//	LineEndProgram, fout);
				auto_ptr <BlFile> pfout
					(newBlFileRegular (progname, Output) );
				program.list (LineBeginProgram,
					LineEndProgram, * pfout);
			}
			break;
		case 'B':
			gettoken ();
			if (token.code != ',')
				throw ErrSyntax;
			gettoken ();
			{
			const char * init;
			size_t len;
			if (token.code == keyIDENTIFIER &&
				typeofvar (token.str) == VarString)
			{
				std::string * pstr= addrvarstring (token.str);
				init= pstr->data ();
				len= pstr->size ();
				gettoken ();
			}
			else
			{
				BlNumber bn= evalnum ();
				init= reinterpret_cast <const char *>
					(size_t (bn) );
				if (token.code != ',')
					throw ErrSyntax;
				bn= expectnum ();
				len= size_t (bn);
			}
			require_endsentence ();
			ofstream os (progname.c_str (), std::ios::binary);
			if (! os.is_open () )
				throw ErrFileNotFound;
			os.write (init, len);
			}
			break;
		default:
			throw ErrSyntax;
		}
	}
	return false;
}

bool RunnerLineImpl::do_NEW ()
{
	errorifparam ();
	program.renew ();
	make_clear ();
	return true;
}

bool RunnerLineImpl::do_EXIT ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_EXIT");

	gettoken ();
	int r= 0;
	if (! endsentence () )
	{
		BlNumber n= evalnum ();
		if (! endsentence () )
			throw ErrSyntax;
		r= int (n);
	}
	throw Exit (r);
}

bool RunnerLineImpl::do_RUN ()
{
	gettoken ();
	BlLineNumber dest= LineBeginProgram;
	if (token.code == keyNUMBER || token.code == keyINTEGER)
	{
		dest= evallinenumber ();
		require_endsentence ();
	}
	else if (! endsentence () )
	{
		std::string progname= evalstring ();
		require_endsentence ();
		program.renew ();
		program.load (progname);
	}
	make_clear ();
	runner.clearreadline ();
	runner.run_to (dest);
	return true;
}

bool RunnerLineImpl::do_FOR ()
{
	const BlInteger one= 1;
	ProgramPos posfor (getposactual () );
	expecttoken (keyIDENTIFIER);
	std::string varname= token.str;
	expecttoken ('=');

	std::auto_ptr <ForElement> pelement;
	switch (typeofvar (varname) )
	{
	case VarNumber:
		{
			BlNumber initial= expectnum ();
			requiretoken (keyTO);
			BlNumber final= expectnum ();
			BlNumber step= (token.code == keySTEP) ?
				expectnum () : 1.0;
			pelement.reset (newForElementNumber (varname, posfor,
				initial, final, step) );
		}
		break;
	case VarInteger:
		{
			BlInteger initial= expectinteger ();
			requiretoken (keyTO);
			BlInteger final= expectinteger ();
			BlInteger step= (token.code == keySTEP) ?
				expectinteger () : one;
			pelement.reset (newForElementInteger (varname, posfor,
				initial, final, step) );
		}
		break;
	default:
		throw ErrMismatch;
	}

	switch (token.code)
	{
	case ':':
		pelement->nextchunk ();
		break;
	case keyENDLINE:
		if (posfor.getnum () != LineDirectCommand)
			pelement->nextline ();
		else
			pelement->nextchunk ();
		break;
	default:
		throw ErrSyntax;
	}
	runner.push_for (pelement.get () );
	pelement.release ();
	return false;
}

bool RunnerLineImpl::do_NEXT ()
{
	gettoken ();
	//std::string varname;
	if (endsentence () )
	{
		#if 0
		//if (runner.for_empty () )
		//	throw ErrNextWithoutFor;
		ForElement & fe= runner.for_top ();
		if (fe.next () )
		{
			runner.jump_to (fe.getpos () );
			return true;
		}
		runner.for_pop ();
		return false;
		#else

		return runner.next ();

		#endif
	}

	for (;;)
	{
		requiretoken (keyIDENTIFIER);
		//varname= token.str;
		//gettoken ();

		#if 0
		//if (runner.for_empty () )
		//	throw ErrNextWithoutFor;
		ForElement * pfe= & runner.for_top ();
		//if (! varname.empty () && ! fe.isvar (varname) )
		if (! pfe->isvar (token.str) )
		{
			if (sysvar::get (sysvar::TypeOfNextCheck) == 0)
			{
				if (showdebuginfo () )
					cerr << "Processing NEXT " <<
						token.str <<
						" but current FOR is " <<
						pfe->var () <<
						endl;
				throw ErrNextWithoutFor;
			}
			else
			{
				// In ZX style NEXT can be omitted.
				do {
					runner.for_pop ();
					pfe= & runner.for_top ();
				} while (! pfe->isvar (token.str) );
			}
		}
		if (pfe->next () )
		{
			runner.jump_to (pfe->getpos () );
			return true;
		}
		runner.for_pop ();
		#else

		if (runner.next (token.str) )
			return true;

		#endif

		gettoken ();
		if (endsentence () )
			break;
		requiretoken (',');
		//expecttoken (keyIDENTIFIER);
		//varname= token.str;
		gettoken ();
	}
	return false;
}

bool RunnerLineImpl::do_IF ()
{
	BlNumber result= expectnum ();

	if (token.code != keyTHEN)
	{
		// First check if THEN can be omitted.
		if (! sysvar::hasFlags1 (sysvar::ThenOmitted) )
			throw ErrSyntax;
		// If that case, check the valid instructions.
		if (token.code != keyGOTO && token.code != keyGOSUB)
			throw ErrSyntax;
	}

	if (result != 0)
		return false;
	else
	{
		// Search ELSE, with possible nested IFs.
		unsigned int iflevel= 1;
		do
		{
			gettoken ();
			switch (token.code)
			{
			case keyIF:
				++iflevel; break;
			case keyELSE:
				--iflevel; break;
			default:
				break;
			}
		} while (iflevel > 0 && token.code != keyENDLINE);
		if (token.code == keyELSE)
		{
			// Continue processing the line in the position
			// after the ELSE. The flag marks the ELSE must
			// be entered, otherwise is supposed that has
			// encountered as the end of other instruction.
			fInElse= true;
			return false;
		}
		else
		{
			// Only can be ENDLINE.
			return true;
		}
	}
}

bool RunnerLineImpl::do_TRON ()
{
	gettoken ();
	bool fLine= false;
	if (token.code == keyLINE)
	{
		fLine= true;
		gettoken ();
	}
	//BlChannel channel= DefaultChannel;
	//if (token.code == '#')
	//	channel= expectchannel ();
	BlChannel channel= evaloptionalchannel ();

	require_endsentence ();
	runner.tron (fLine, channel);
	return false;
}

bool RunnerLineImpl::do_TROFF ()
{
	errorifparam ();
	runner.troff ();
	return false;
}

#if 0
void RunnerLineImpl::letsubindex (const std::string & varname)
{
	Dimension dims= expectdims ();
	requiretoken ('=');
	BlResult result;
	expect (result);
	require_endsentence ();
	switch (typeofvar (varname) )
	{
	case VarNumber:
		assigndimnumber (varname, dims, result.number () );
		break;
	case VarInteger:
		assigndiminteger (varname, dims, result.integer () );
		break;
	case VarString:
		assigndimstring (varname, dims, result.str () );
		break;
	default:
		throw ErrBlassicInternal;
	}
}
#endif

bool RunnerLineImpl::do_IDENTIFIER ()
{
	#if 0

	requiretoken (keyIDENTIFIER);
	std::string varname= token.str;
	gettoken ();
	if (token.code == '(')
	{
		letsubindex (varname);
		return false;
	}
	//requiretoken ('=');
	// Crash on hp-ux when failed. Compiler fault?
	// Workaround:
	if (token.code != '=')
		throw ErrSyntax;

	BlResult result;
	expect (result);
	require_endsentence ();
	switch (typeofvar (varname) )
	{
	case VarNumber:
		assignvarnumber (varname, result.number () );
		break;
	case VarInteger:
		assignvarinteger (varname, result.integer () );
		break;
	case VarString:
		assignvarstring (varname, result.str () );
		break;
	default:
		throw ErrBlassicInternal;
	}

	#else

	eval_let ();
	require_endsentence ();

	#endif

	return false;
}

bool RunnerLineImpl::do_LET ()
{
	gettoken ();
	return do_IDENTIFIER ();
}

bool RunnerLineImpl::do_GOTO ()
{
	gettoken ();
	BlLineNumber dest= evallinenumber ();
	require_endsentence ();
	runner.goto_to (dest);
	return true;
}

bool RunnerLineImpl::do_GOSUB ()
{
	gettoken ();
	BlLineNumber dest= evallinenumber ();
	require_endsentence ();

	gosub_line (dest);
	return true;
}

bool RunnerLineImpl::do_STOP ()
{
	errorifparam ();

	#if 1

	BlFile & f= getfile0 ();
	f << "**Stopped**";

	//if (line.number () != 0)
	//	f << " in " << line.number ();
	//if (pline->number () != 0)
	//	f << " in " << pline->number ();
	if (codeline.number () != LineDirectCommand)
		f << " in " << codeline.number ();

	//f << '\n';
	f.endline ();
	//ProgramPos posbreak (runner.getposactual () );
	ProgramPos posbreak (getposactual () );
	posbreak.nextchunk ();
	runner.set_break (posbreak);
	runner.setstatus (Runner::Stopped);
	return true;

	#else

	throw blassic::ProgramStop ();

	#endif
}

bool RunnerLineImpl::do_CONT ()
{
	errorifparam ();
	runner.jump_break ();
	return true;
}

bool RunnerLineImpl::do_CLEAR ()
{
	gettoken ();
	switch (token.code)
	{
	case keyINK:
		gettoken ();
		require_endsentence ();
		graphics::clearink ();
		break;
	case keyINPUT:
		gettoken ();
		require_endsentence ();
		runner.clean_input ();
		break;
	default:
		if (! endsentence () )
			throw ErrSyntax;
		make_clear ();
		break;
	}
	return false;
}

bool RunnerLineImpl::do_RETURN ()
{
	gettoken ();
	BlLineNumber line= LineEndProgram;
	if (! endsentence () )
	{
		line= evallinenumber ();
		require_endsentence ();
	}
	ProgramPos pos;
	runner.gosub_pop (pos);
	if (line != LineEndProgram)
		runner.goto_to (line);
	else
		runner.jump_to (pos);
	return true;
}

bool RunnerLineImpl::do_POKE ()
{
	BlNumber bnAddr= expectnum ();
	requiretoken (',');
	//BlChar * addr= (BlChar *) (unsigned int) bnAddr;
	BlChar * addr= (BlChar *) (size_t) bnAddr;
	BlNumber bnValue= expectnum ();
	require_endsentence ();
	BlChar value= (BlChar) (unsigned int) bnValue;
	* addr= value;
	return false;
}

bool RunnerLineImpl::do_READ ()
{
	// Yes, I know this function has many gotos and is poorly
	// structured. Maybe someday I rewrite it. In the
	// meantime, if someone write a good replacement and send
	// it to me, I will put his/her name here in upper case ;)

	//TRACEFUNC (tr, "RunnerLineImpl::do_READ");

	ListVarPointer lvp;
	gettoken ();
	evalmultivarpointer (lvp);
	require_endsentence ();
	if (lvp.empty () )
	{
		if (showdebuginfo () )
			cerr << "Empty READ var list" << endl;
		throw ErrSyntax;
	}
	ListVarPointer::iterator itvp= lvp.begin ();
	const ListVarPointer::iterator itvpend= lvp.end ();

	BlLineNumber & datanumline= runner.getdatanumline ();
	BlChunk & datachunk= runner.getdatachunk ();
	unsigned short & dataelem= runner.getdataelem ();

	//CodeLine dataline= program.getline (datanumline);
	CodeLine dataline;

	program.getline (datanumline, dataline);

	CodeLine::Token datatok;
otra:
	if (dataline.number () == LineEndProgram)
		throw ErrDataExhausted;
	if (dataline.number () > datanumline)
	{
		datachunk= 0;
		dataelem= 0;
	}
	//datatok= dataline.gettoken ();
	dataline.gettoken (datatok);
	if (dataline.chunk () < datachunk)
	{
		while (dataline.chunk () < datachunk)
		{
			//datatok= dataline.gettoken ();
			dataline.gettoken (datatok);
			if (datatok.code == keyENDLINE)
				break;
		}
		if (datatok.code != keyENDLINE)
			//datatok= dataline.gettoken ();
			dataline.gettoken (datatok);
	}
	if (datatok.code == keyENDLINE)
	{
		//cerr << "Read searching next line" << endl;
		//dataline= program.getnextline (dataline);
		program.getnextline (dataline);
		goto otra;
	}
otra2:
	while (datatok.code != keyDATA)
	{
		dataelem= 0;
		datachunk= dataline.chunk ();
		do {
			//datatok= dataline.gettoken ();
			dataline.gettoken (datatok);
		} while (datatok.code != keyENDLINE &&
			dataline.chunk () == datachunk);
		if (datatok.code == keyENDLINE)
		{
			//cerr << "Read searching next line" << endl;
			//dataline= program.getnextline (dataline);
			program.getnextline (dataline);
			if (dataline.number () == LineEndProgram)
				throw ErrDataExhausted;
			//datatok= dataline.gettoken ();
			dataline.gettoken (datatok);
		}
		else
			//datatok= dataline.gettoken ();
			dataline.gettoken (datatok);
	}
	datatok= dataline.getdata ();
	unsigned short elem= 0;
otra3:
	while (elem < dataelem)
	{
		//datatok= dataline.gettoken ();
		dataline.gettoken (datatok);
		if (datatok.code == ':')
		{
			//datatok= dataline.gettoken ();
			dataline.gettoken (datatok);
			dataelem= 0;
			goto otra2;
		}
		if (datatok.code == keyENDLINE)
		{
			//cerr << "Read searching next line" << endl;
			//dataline= program.getnextline (dataline);
			program.getnextline (dataline);
			goto otra;
		}
		if (datatok.code != ',')
		{
			if (showdebuginfo () )
				cerr << "DATA invalid on line " <<
					dataline.number () << endl;
			throw ErrSyntax;
		}
		datatok= dataline.getdata ();

		// Supressed this check to allow a comma at end of data.
		//if (datatok.isendsentence () )
		//	throw ErrSyntax;

		++elem;
	}

	//cerr << "(Data en linea " << dataline.number () <<
	//	" '" << datatok.str << "' " <<
	//	datatok.integer () << ')' << flush;

	datanumline= dataline.number ();
	datachunk= dataline.chunk ();
	dataelem= (unsigned short) (elem + 1);

	switch (itvp->type)
	{
	case VarNumber:
		* itvp->pnumber= datatok.number ();
		break;
	case VarInteger:
		switch (datatok.code)
		{
		case keyINTEGER:
			* itvp->pinteger= datatok.valueint;
			break;
		case keyENDLINE:
			* itvp->pinteger= 0;
			break;
		case keySTRING:
			{
				
				BlResult r= datatok.number ();
				* itvp->pinteger= r.integer ();
			}
			break;
		default:
			if (showdebuginfo () )
				cerr << "DATA mismatch on line " <<
					dataline.number () << endl;
			throw ErrMismatch;
		}
		break;
	#if 0
	case VarString:
		switch (datatok.code)
		{
		case keySTRING:
			* itvp->pstring= datatok.str;
			break;
		case keyINTEGER:
			* itvp->pstring= to_string (datatok.valueint);
			break;
		case keyENDLINE:
			* itvp->pstring= std::string ();
			break;
		default:
			if (showdebuginfo () )
				cerr << "Unexpected token in DATA in line" <<
					datanumline << endl;
			throw ErrBlassicInternal;
		}
		break;
	#endif
	case VarString:
	case VarStringSlice:
		{
			std::string value;
			switch (datatok.code)
			{
			case keySTRING:
				value= datatok.str;
				break;
			case keyINTEGER:
				value= to_string (datatok.valueint);
				break;
			case keyENDLINE:
				value= std::string ();
				break;
			default:
				if (showdebuginfo () )
					cerr << "Unexpected token in DATA "
						"in line" <<
						datanumline << endl;
				throw ErrBlassicInternal;
			}
			if (itvp->type == VarString)
				* itvp->pstring= value;
			else
				assignslice (* itvp, BlResult (value) );
		}
		break;
	default:
		if (showdebuginfo () )
			cerr << "Unexpected var type in READ "
				"of DATA in line" << datanumline << endl;
		throw ErrBlassicInternal;
	}
	if (++itvp != itvpend)
		goto otra3;

	return false;
}

bool RunnerLineImpl::do_DATA ()
{
	do
	{
		gettoken ();
	} while (! endsentence () );
	return false;
}

bool RunnerLineImpl::do_RESTORE ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_restore");

	gettoken ();
	if (! endsentence () )
	{
		BlLineNumber bln= evallinenumber ();
		require_endsentence ();
		TRMESSAGE (tr,
			std::string ("Restoring to ") + to_string (bln) );
		runner.setreadline (bln);
	}
	else
	{
		TRMESSAGE (tr, "Restoring to begin");
		runner.clearreadline ();
	}
	return false;
}

namespace {

struct clearvar {
	void operator () (VarPointer & vt) const
	{
		switch (vt.type)
		{
		case VarNumber:
			* vt.pnumber= 0;
			break;
		case VarInteger:
			* vt.pinteger= 0;
			break;
		case VarString:
			vt.pstring->erase ();
			break;
		default:
			throw ErrBlassicInternal;
		}
	}
};

bool isdelimdiscardingwhite (std::istream & is, char delim)
{
	char c;
	while (is.get (c) )
	{
		if (c == delim)
		{
			is.putback (c);
			return true;
		}
		if (! isspace (c) )
		{
			is.putback (c);
			return false;
		}
	}
	return true; // We count eof as delimiter
}

void parsestring (std::string & result, std::istringstream & iss,
	char quote, char delimiter)
{
	std::string str;
	char c;
	bool atend= isdelimdiscardingwhite
		(iss, delimiter);
	if (! atend)
	{
		iss >> c;
		if (! iss)
			atend= true;
	}
	if (! atend)
	{
		if (c == quote)
		{
			#if 0
			while (iss >> c && c != quote)
			{
				str+= c;
			}
			#else
			while (iss >> c)
			{
				if (c != quote)
					str+= c;
				else
				{
					iss >> c;
					if (! iss)
						break;
					if (c == quote)
						str+= c;
					else
					{
						iss.unget ();
						break;
					}
				}
			}
			#endif
		}
		else
		{
			while (iss && c != delimiter)
			{
				str+= c;
				iss >> c;
			}
			if (iss)
				iss.unget ();
		}
	}
	//* vp.pstring= str;
	result= str;
}

void parsenumber (BlNumber & result, std::istringstream & iss,
	char /* quote*/, char delimiter)
{
	BlNumber n;
	if (isdelimdiscardingwhite (iss, delimiter) )
		n= 0;
	else
	{
		iss >> n;
		if (! iss)
			throw ErrMismatch;
	}
	//* vp.pnumber= n;
	result= n;
}

void parseinteger (BlInteger & result, std::istringstream & iss,
	char /* quote*/, char delimiter)
{
	BlInteger n;
	if (isdelimdiscardingwhite (iss, delimiter) )
		n= 0;
	else
	{
		iss >> n;
		if (! iss)
			throw ErrMismatch;
	}
	//* vp.pinteger= n;
	result= n;
}

} // namespace

bool RunnerLineImpl::do_INPUT ()
{
	gettoken ();
	BlChannel channel= DefaultChannel;
	if (token.code == '#')
	{
		channel= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	std::string prompt;
	bool lineend= true;
	if (token.code == ';')
	{
		lineend= false;
		gettoken ();
	}
	switch (token.code)
	{
	case keySTRING:
		prompt= token.str;
		gettoken ();
		switch (token.code)
		{
		// This was reversed, corected on 3-jun-2003
		case ';':
			prompt+= "? ";
			break;
		case ',':
			break;
		default:
			throw ErrSyntax;
		}
		std::cout << flush;
		gettoken ();
		break;
	default:
		prompt= "? ";
	}

	// Parse the list of variables.

	ListVarPointer inputvars;
	evalmultivarpointer (inputvars);
	require_endsentence ();
	if (inputvars.empty () )
		throw ErrSyntax;

	// Prepare input channel

	std::string input;
	BlFile & in= getfile (channel);
	char quote= in.quote ();
	char delimiter= in.delimiter ();

	// Accept line from input

	for (;;)
	{
		if (channel == DefaultChannel)
		{
			runner.clean_input ();
			//cursorvisible ();
		}
		if (! prompt.empty () )
		{
			if (channel == DefaultChannel || in.istextwindow () )
			{
				in << prompt;
				in.flush ();
			}
		}
		in.getline (input, lineend);

		if (fInterrupted)
		{
			std::cin.clear ();
			if (runner.getbreakstate () != onbreak::BreakCont)
				// This throw causes an error in windows,
				// we return and let the caller catch
				// the interrupted condition instead.
				//throw BlBreak ();
				return false;
			else
			{
				fInterrupted= false;
				//in << '\n';
				in.endline ();
				continue;
			}
		}
		break;
	}

	// Process the line entered

	// We must take into account that an whitespace can be a delimiter
	// (tipically a tab).

	std::istringstream iss (input);
	iss.unsetf (std::ios::skipws);

	ListVarPointer::iterator itvp= inputvars.begin ();
	const ListVarPointer::iterator itvpend= inputvars.end ();

	for ( ; itvp != itvpend; ++itvp)
	{
		VarPointer & vp= * itvp;
		switch (vp.type)
		{
		case VarNumber:
			parsenumber (* vp.pnumber, iss, quote, delimiter);
			break;
		case VarInteger:
			parseinteger (* vp.pinteger, iss, quote, delimiter);
			break;
		case VarString:
			parsestring (* vp.pstring, iss, quote, delimiter);
			break;
		case VarStringSlice:
			{
				std::string str;
				parsestring (str, iss, quote, delimiter);
				assignslice (vp, BlResult (str) );
			}
			break;
		default:
			throw ErrBlassicInternal;
		}
		if (isdelimdiscardingwhite (iss, delimiter) )
		{
			if (iss.eof () )
			{
				//++i;
				++itvp;
				break;
			}
			else
				iss.get ();
		}
		else
			throw ErrMismatch;
	}

	// If not enough data entered, clear remaining vars.

	std::for_each (itvp, itvpend, clearvar () );

	return false;
}

void RunnerLineImpl::do_line_input ()
{
	#if 0

	gettoken ();
	BlChannel channel= DefaultChannel;
	if (token.code == '#')
	{
		channel= expectchannel ();
		requiretoken (',');
		gettoken ();
	}

	#else

	gettoken ();
	BlChannel channel= DefaultChannel;
	if (token.code == '#')
	{
		channel= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	std::string prompt;
	bool lineend= true;
	if (token.code == ';')
	{
		lineend= false;
		gettoken ();
	}
	switch (token.code)
	{
	case keySTRING:
		prompt= token.str;
		gettoken ();
		switch (token.code)
		{
		// This was reversed, corected on 3-jun-2003
		case ';':
			prompt+= "? ";
			break;
		case ',':
			break;
		default:
			throw ErrSyntax;
		}
		std::cout << flush;
		gettoken ();
		break;
	default:
		prompt= "? ";
	}

	#endif

	requiretoken (keyIDENTIFIER);
	std::string varname= token.str;
	gettoken ();
	require_endsentence ();
	if (typeofvar (varname) != VarString)
		throw ErrMismatch;

	std::string value;
	BlFile & in= getfile (channel);
	if (channel == DefaultChannel)
	{
		runner.clean_input ();
		//cursorvisible ();
	}

	for (;;)
	{
		if (! prompt.empty () )
		{
			if (channel == DefaultChannel || in.istextwindow () )
			{
				in << prompt;
				in.flush ();
			}
		}
		in.getline (value, lineend);
		if (fInterrupted)
		{
			std::cin.clear ();
			if (runner.getbreakstate () != onbreak::BreakCont)
				return;
			else
			{
				fInterrupted= false;
				//in << '\n';
				in.endline ();
				continue;
			}
		}
		break;
	}

	assignvarstring (varname, value);
}

bool RunnerLineImpl::do_LINE ()
{
	gettoken ();
	if (token.code == keyINPUT)
	{
		do_line_input ();
		return false;
	}
	graphics::Point from;
	if (token.code == '-')
		from= graphics::getlast ();
	else
	{
		requiretoken ('(');
		BlInteger x= expectinteger ();
		requiretoken (',');
		BlInteger y= expectinteger ();
		requiretoken (')');
		expecttoken ('-');
		from= graphics::Point (x, y);
	}
	expecttoken ('(');
	BlInteger x= expectinteger ();
	requiretoken (',');
	BlInteger y= expectinteger ();
	requiretoken (')');
	gettoken ();
	enum Type { TypeLine, TypeRect, TypeFillRect };
	Type type= TypeLine;
	if (! endsentence () )
	{
		requiretoken (',');
		gettoken ();
		if (token.code != ',')
		{
			BlInteger color= evalinteger ();
			graphics::setcolor (color);
		}
		if (token.code == ',')
		{
			gettoken ();
			requiretoken (keyIDENTIFIER);
			if (token.str == "B")
				type= TypeRect;
			else if (token.str == "BF")
				type= TypeFillRect;
			else throw ErrSyntax;
			gettoken ();
		}
		require_endsentence ();
	}

	switch (type)
	{
	case TypeLine:
		graphics::move (from.x, from.y);
		graphics::line (x, y);
		break;
	case TypeRect:
		graphics::rectangle (from, graphics::Point (x, y) );
		break;
	case TypeFillRect:
		graphics::rectanglefilled (from, graphics::Point (x, y) );
		break;
	}

	return false;
}

bool RunnerLineImpl::do_RANDOMIZE ()
{
	gettoken ();
	unsigned int seedvalue;
	if (endsentence () )
		seedvalue= time (NULL);
	else
	{
		BlNumber result= evalnum ();
		//errorifparam ();
		require_endsentence ();
		//seedvalue= static_cast <unsigned int> (result);
		seedvalue= util::checked_cast <unsigned int>
			(result, ErrBadSubscript);
	}

	#if 0
	randgen.seed (seedvalue);
	#else
	runner.seedrandom (seedvalue);
	#endif

	return false;
}

bool RunnerLineImpl::do_AUTO ()
{
	if (codeline.number () != LineDirectCommand)
		throw ErrInvalidCommand;
	BlLineNumber
		auto_ini= sysvar::get32 (sysvar::AutoInit),
		auto_inc= sysvar::get32 (sysvar::AutoInc);
	gettoken ();
	if (! endsentence () )
	{
		if (token.code != ',')
		{
			switch (token.code)
			{
			case keyNUMBER:
				auto_ini= (BlLineNumber) token.number ();
				break;
			case keyINTEGER:
				auto_ini= token.integer ();
				break;
			default:
				throw ErrSyntax;
			}
			gettoken ();
		}
		if (token.code == ',')
		{
			gettoken ();
			switch (token.code)
			{
			case keyNUMBER:
				auto_inc= (BlLineNumber) token.number ();
				break;
			case keyINTEGER:
				auto_inc= token.integer ();
				break;
			default:
				throw ErrSyntax;
			}
			gettoken ();
		}
		require_endsentence ();
	}
	#if 0
	blnAuto= auto_ini;
	blnAutoInc= auto_inc;
	#else
	runner.setauto (auto_ini, auto_inc);
	#endif
	return false;
}

bool RunnerLineImpl::do_DIM ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_DIM");

	do {
		expecttoken (keyIDENTIFIER);
		std::string varname= token.str;
		Dimension dims= expectdims ();
		switch (typeofvar (varname) )
		{
		case VarNumber:
			dimvarnumber (varname, dims);
			break;
		case VarInteger:
			dimvarinteger (varname, dims);
			break;
		case VarString:
			dimvarstring (varname, dims);
			break;
		default:
			throw ErrBlassicInternal;
		}
	} while (token.code == ',');
	require_endsentence ();
	return false;
}

bool RunnerLineImpl::do_SYSTEM ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_SYSTEM");

	errorifparam ();
	throw Exit ();
}

bool RunnerLineImpl::do_ON ()
{
	enum TypeOn { OnNoValid, OnGoto, OnGosub };
	gettoken ();
	switch (token.code)
	{
	case keyERROR:
		{
			expecttoken (keyGOTO);
			gettoken ();
			// Allow use line 0 if using a label.
			bool islabel= token.code == keyIDENTIFIER;
			BlLineNumber bln= evallinenumber ();
			require_endsentence ();
			if (bln != 0 || islabel)
				runner.seterrorgoto (bln, codeline.number () );
			else
				runner.clearerrorgoto ();
		}
		return false;
	case keyBREAK:
		gettoken ();
		switch (token.code)
		{
		case keySTOP:
			runner.setbreakstate (onbreak::BreakStop);
			break;
		case keyCONT:
			runner.setbreakstate (onbreak::BreakCont);
			break;
		case keyGOSUB:
			{
				gettoken ();
				BlLineNumber bln= evallinenumber ();
				errorifparam ();
				runner.setbreakgosub (bln);
			}
			break;
		default:
			throw ErrSyntax;
		}
		return false;
	case keySharp:
		{
			BlChannel ch= expectchannel ();
			requiretoken (keyGOSUB);
			gettoken ();
			BlLineNumber line= evallinenumber ();
			require_endsentence ();
			runner.pollchannel (ch, line);
		}
		return false;
	default:
		break;
	}
	BlNumber bn= evalnum ();
	size_t n= (size_t) bn;

	TypeOn type= OnNoValid;
	switch (token.code)
	{
	case keyGOTO:
		type= OnGoto;
		break;
	case keyGOSUB:
		type= OnGosub;
		break;
	default:
		throw ErrSyntax;
	}
	BlLineNumber bln;
	std::vector <BlLineNumber> line;
	do
	{
		gettoken ();
		bln= evallinenumber ();
		line.push_back (bln);
	} while (token.code == ',');
	require_endsentence ();
	size_t l= line.size ();
	if (n > 0 && n <= l)
	{
		switch (type)
		{
		case OnGoto:
			runner.goto_to (line [n - 1] );
			break;
		case OnGosub:
			gosub_line (line [n - 1] );
			break;
		default:
			; // Avoid a warning
		}
		return true;
	}
	return false;
}

bool RunnerLineImpl::do_ERROR ()
{
	BlNumber blCod= expectnum ();
	require_endsentence ();
	throw BlErrNo (blCod);
}

bool RunnerLineImpl::do_OPEN ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_OPEN");

	BlCode op= token.code;
	BlChannel channel= DefaultChannel;
	std::string filename;
	OpenMode mode= Input;
	size_t record_len= 128;
	bool binary= false;
	// Defined here to avoid an error in C++ Builder
	auto_ptr <BlFile> pbf;

	gettoken ();
	if (token.code == keyERROR)
	{
		if (op != keyOPEN)
			throw ErrSyntax;
		op= keyERROR;
		expecttoken (keyAS);
		//gettoken ();
		//if (token.code == '#')
		//	gettoken ();
		//channel= evalchannel ();
		channel= expectrequiredchannel ();
	}
	else
	{
		BlResult result;
		eval (result);
		std::string firstarg= result.str ();
		switch (token.code)
		{
		case keyFOR:
		// Newer syntax.
			filename= firstarg;
			gettoken ();
			if (token.code == keyBINARY)
			{
				binary= true;
				gettoken ();
			}
			switch (token.code)
			{
			case keyINPUT:
				mode= Input;
				break;
			case keyOUTPUT:
				mode= Output;
				break;
			case keyAPPEND:
				mode= Append;
				break;
			case keyRANDOM:
				mode= Random;
				break;
			case keyLPRINT:
				if (op != keyOPEN)
					throw ErrSyntax;
				mode= Append;
				channel= PrinterChannel;
				gettoken ();
				break;
			default:
				throw ErrSyntax;
			}
			if (channel == DefaultChannel)
			{
				expecttoken (keyAS);
				//gettoken ();
				//if (token.code == '#')
				//	gettoken ();
				//channel= evalchannel ();
				channel= expectrequiredchannel ();

				if (token.code == keyLEN)
				{
					expecttoken ('=');
					gettoken ();
					BlNumber bn= evalnum ();
					record_len= size_t (bn);
					if (mode != Random)
						throw ErrFileMode;
				}
			}
			break;
		case keyAS:
			filename= firstarg;
			if (op == keyPOPEN)
				mode= InOut;
			//gettoken ();
			//if (token.code == '#')
			//	gettoken ();
			//channel= evalchannel ();
			channel= expectrequiredchannel ();
			break;
		case ',':
		// Older syntax.
			if (firstarg == "I" || firstarg == "i")
				mode= Input;
			else if (firstarg == "O" || firstarg == "o")
				mode= Output;
			else if (firstarg == "A" || firstarg == "a")
				mode= Append;
			else if (firstarg == "R" || firstarg == "r")
				mode= Random;
			else
				throw ErrFileMode;
			//gettoken ();
			//if (token.code == '#')
			//	gettoken ();
			//channel= evalchannel ();
			channel= expectrequiredchannel ();
			requiretoken (',');
			filename= expectstring ();
			if (token.code == ',')
			{
				if (mode != Random)
					throw ErrFileMode;
				BlNumber bn= expectnum ();
				record_len= size_t (bn);
			}
			break;
		default:
			throw ErrSyntax;
		} // switch
	} // if

	require_endsentence ();
	if (channel == DefaultChannel)
		throw ErrFileNumber;

	// BINARY is ignored except with regular file.
	//auto_ptr <BlFile> pbf;
	switch (op)
	{
	case keyOPEN:
		if (mode == Random)
			pbf. reset (newBlFileRandom (filename, record_len) );
		else
		{
			if (binary)
				mode= OpenMode (mode | Binary);
			pbf.reset (newBlFileRegular (filename, mode) );
		}
		break;
	case keyPOPEN:
		if (mode != Input && mode != Output && mode != InOut)
			throw ErrFileMode;
		pbf.reset (newBlFilePopen (filename, mode) );
		break;
	case keyERROR:
		pbf.reset (newBlFileOutput (cerr) );
		break;
	default:
		throw ErrBlassicInternal;
	}

	runner.setfile (channel, pbf.get () );
	pbf.release ();
	return false;
}

bool RunnerLineImpl::do_POPEN ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_POPEN");

	OpenMode mode= InOut;

	std::string filename= expectstring ();
	bool witherror= false;
	if (token.code == keyERROR)
	{
		witherror= true;
		gettoken ();
	}
	if (token.code == keyFOR)
	{
		gettoken ();
		switch (token.code)
		{
		case keyINPUT:
			mode= Input;
			break;
		case keyOUTPUT:
			mode= Output;
			break;
		default:
			throw ErrSyntax;
		}
		gettoken ();
	}
	requiretoken (keyAS);
	BlChannel  channel= expectrequiredchannel ();
	require_endsentence ();

	if (witherror)
		mode= OpenMode (mode | WithErr);
	auto_ptr <BlFile> pbf (newBlFilePopen (filename, mode) );
	runner.setfile (channel, pbf.get () );
	pbf.release ();

	return false;
}

bool RunnerLineImpl::do_CLOSE ()
{
	gettoken ();
	if (endsentence () )
	{
		runner.close_all ();
		return false;
	}
	if (token.code == keyINPUT || token.code == keyOUTPUT)
	{
		bool isinput= token.code == keyINPUT;
		for (;;)
		{
			BlChannel channel= expectrequiredchannel ();
			BlFile & f= getfile (channel);
			if (isinput)
				f.closein ();
			else
				f.closeout ();
			if (token.code != ',')
				break;
				
		}
		require_endsentence ();
		return false;
	}
	for (;;)
	{
		BlChannel channel;
		if (token.code == keyLPRINT)
		{
			channel= PrinterChannel;
			gettoken ();
		}
		else
		{
			if (token.code == '#')
				gettoken ();
			channel= evalchannel ();
		}
		if (channel == DefaultChannel)
			throw ErrFileNumber;
		runner.closechannel (channel);
		if (token.code != ',')
		{
			if (endsentence () )
				break;
			throw ErrSyntax;
		}
		gettoken ();
	}
	return false;
}

bool RunnerLineImpl::do_LOCATE ()
{
	gettoken ();
	BlChannel ch= DefaultChannel;
	if (token.code == '#')
	{
		ch= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	BlInteger row= evalinteger ();
	requiretoken (',');
	BlInteger col= expectinteger ();
	require_endsentence ();
	BlFile & out= getfile (ch);
	if (sysvar::hasFlags1 (sysvar::LocateStyle) )
	{
		// LOCATE style Amstrad CPC: col, row
		std::swap (row, col);
	}
	out.gotoxy (col - 1, row - 1);
	return false;
}

bool RunnerLineImpl::do_CLS ()
{
	//gettoken ();
	//BlChannel ch= DefaultChannel;
	//if (token.code == '#')
	//{
	//	ch= expectchannel ();
	//}
	BlChannel ch= expectoptionalchannel ();
	require_endsentence ();
	BlFile & out= getfile (ch);
	out.cls ();
	return false;
}

bool RunnerLineImpl::do_WRITE ()
{
	gettoken ();
	BlChannel channel= DefaultChannel;
	if (token.code == '#')
	{
		channel= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	BlFile & out= getfile (channel);
	BlResult result;
	char quote= out.quote ();
	for (;;)
	{
		if (token.code != ',')
		{
			eval (result);
			switch (result.type () )
			{
			case VarNumber:
				out << result.number ();
				break;
			case VarInteger:
				out<< result.integer ();
				break;
			case VarString:
				if (quote) out << quote;
				out << result.str ();
				if (quote) out << quote;
				break;
			default:
				throw ErrBlassicInternal;
			}
		}
		if (token.code == ',')
		{
			out << out.delimiter ();
			gettoken ();
		}
		else break;
	}
	require_endsentence ();
	//out << '\n';
	out.endline ();
	return false;
}

bool RunnerLineImpl::do_MODE ()
{
	//BlNumber mode= expectnum ();
	BlResult result;
	expect (result);

	bool spectrummode= false;
	//BlInteger mode;
	if (result.type () == VarString)
	{
		require_endsentence ();
		const std::string & strmode= result.str ();
		graphics::setmode (strmode);
		//mode= 1;
		if (strmode == "spectrum")
			spectrummode= true;
	}
	else
	{
		BlInteger mode= result.integer ();
		if (endsentence () )
			graphics::setmode (mode);
		else
		{
			requiretoken (',');
			BlInteger height= expectinteger ();
			bool inverty= false;
			BlInteger zoomx= 1, zoomy= 1;
			if (token.code == ',')
			{
				gettoken ();
				if (token.code != ',')
					inverty= evalinteger ();
				if (token.code == ',')
				{
					gettoken ();
					if (token.code != ',')
						zoomx= evalinteger ();
					if (token.code == ',')
						zoomy= expectinteger ();
				}
			}
			require_endsentence ();
			graphics::setmode (mode, height, inverty,
				zoomx, zoomy);
		}
	}
	runner.destroy_windows ();
	runner.resetfile0 ();
	if (spectrummode)
		runner.spectrumwindows ();
	return false;
}

bool RunnerLineImpl::do_MOVE ()
{
	#if 0
	BlNumber x= expectnum ();
	requiretoken (',');
	BlNumber y= expectnum ();
	require_endsentence ();
	graphics::move (int (x), int (y) );
	#else
	BlInteger x, y;
	getdrawargs (x, y);
	graphics::move (x, y);
	#endif
	return false;
}

bool RunnerLineImpl::do_COLOR ()
{
	gettoken ();
	BlFile & out= getfile0 ();
	if (token.code != ',')
	{
		BlInteger color= evalinteger ();
		out.setcolor (color);
		if (graphics::ingraphicsmode () )
			graphics::setcolor (color);
		if (endsentence () )
			return false;
		requiretoken (',');
	}
	gettoken ();
	if (token.code != ',')
	{
		BlInteger back= evalinteger ();
		out.setbackground (back);
		if (graphics::ingraphicsmode () )
			graphics::setbackground (back);
		if (endsentence () )
			return false;
		requiretoken (',');
	}
	gettoken ();
	// Border color in Gwbasic. Ignored.
	evalinteger ();
	require_endsentence ();
	return false;
}

void RunnerLineImpl::do_get_image ()
{
	BlInteger x1= expectinteger ();
	requiretoken (',');
	BlInteger y1= expectinteger ();
	requiretoken (')');
	expecttoken ('-');
	expecttoken ('(');
	BlInteger x2= expectinteger ();
	requiretoken (',');
	BlInteger y2= expectinteger ();
	requiretoken (')');
	expecttoken (',');
	gettoken ();
	requiretoken (keyIDENTIFIER);
	std::string name= token.str;
	gettoken ();
	require_endsentence ();
	graphics::get_image (x1, y1, x2, y2, name);
}

bool RunnerLineImpl::do_GET ()
{
	gettoken ();
	if (token.code == '(')
	{
		do_get_image ();
		return false;
	}

	if (token.code != keyIDENTIFIER ||
		typeofvar (token.str) != VarString)
	{
		// GET #
		if (token.code == '#')
			gettoken ();
		BlChannel channel= evalchannel ();
		size_t pos= 0;
		if (token.code == ',')
		{
			BlNumber bnPos= expectnum ();
			pos= size_t (bnPos);
			if (pos == 0)
				throw ErrBadRecord;
		}
		require_endsentence ();
		BlFile & out= getfile (channel);
		out.get (pos);
		return false;
	}
	// GET var$
	std::string varname= token.str;
	gettoken ();
	require_endsentence ();

	#if 0
	std::string r= graphics::ingraphicsmode () ?
		graphics::getkey () :
		cursor::getkey ();
	#else
	std::string r= getfile0 ().getkey ();
	#endif

	if (r == "\n" && sysvar::hasFlags1 (sysvar::ConvertLFCR) )
		r= "\r";
	assignvarstring (varname, r);
	return false;
}

bool RunnerLineImpl::do_LABEL ()
{
	expecttoken (keyIDENTIFIER);
	//std::string label= token.str;
	gettoken ();
	require_endsentence ();
	return false;
}

bool RunnerLineImpl::do_DELIMITER ()
{
	gettoken ();
	BlChannel channel= DefaultChannel;
	if (token.code == '#')
	{
		channel= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	BlFile & in= getfile (channel);
	std::string str= evalstring ();
	char delim= 0, quote= 0, escape= 0;
	if (! str.empty () )
		delim= str [0];
	if (! endsentence () )
	{
		requiretoken (',');
		str= expectstring ();
		if (! str.empty () )
			quote= str [0];
		if (! endsentence () )
		{
			requiretoken(',');
			str= expectstring ();
			if (! str.empty () )
				escape= str [0];
			require_endsentence ();
		}
	}
	in.delimiter (delim);
	in.quote (quote);
	in.escape (escape);
	return false;
}

bool RunnerLineImpl::do_REPEAT ()
{
	errorifparam ();
	ProgramPos posrepeat (getposactual () );
	if (token.code == keyENDLINE)
	{
		if (posrepeat.getnum () != LineDirectCommand)
			posrepeat.nextline ();
		else
			throw ErrRepeatWithoutUntil;
	}
	else
		posrepeat.nextchunk ();
	runner.repeat_push (RepeatElement (posrepeat) );
	return false;
}

bool RunnerLineImpl::do_UNTIL ()
{
	if (runner.repeat_empty () )
		throw ErrUntilWithoutRepeat;
	BlResult br;
	expect (br);
	bool cond= br.tobool ();
	require_endsentence ();
	if (cond)
	{
		runner.repeat_pop ();
		return false;
	}
	RepeatElement re= runner.repeat_top ();
	runner.jump_to (re.getpos () );
	return true;
}

bool RunnerLineImpl::do_WHILE ()
{
	//ProgramPos poswhile (runner.getposactual () );
	ProgramPos poswhile (getposactual () );
	BlResult br;
	expect (br);
	bool cond= br.tobool ();
	require_endsentence ();

	if (cond)
	{
		if (! runner.in_wend () )
			runner.while_push (WhileElement (poswhile) );
		else
			runner.in_wend (false);
		return false;
	}
	if (runner.in_wend () )
	{
		runner.while_pop ();
		runner.in_wend (false);
	}
	bool sameline= true;

	// Find the WEND.
	for (size_t level= 1; level > 0; )
	{
		getnextchunk ();
		if (token.code == keyENDLINE)
		{
			if (codeline.number () == LineDirectCommand)
				throw BlError (ErrWhileWithoutWend, poswhile);
			program.getnextline (codeline);
			if (codeline.number () == LineEndProgram)
				throw BlError (ErrWhileWithoutWend, poswhile);
			sameline= false;
			gettoken ();
		}
		switch (token.code)
		{
		case keyWHILE: ++level; break;
		case keyWEND: --level; break;
		default: ;
		}
	}
	// At the WEND, check the syntax.
	gettoken ();
	require_endsentence ();

	if (sameline)
		return false;

	ProgramPos pos (codeline.number (), codeline.chunk () );
	if (token.code == keyENDLINE)
	{
		// If direct command, is sameline.
		pos.nextline ();
	}
	runner.jump_to (pos);
	return true;
}

bool RunnerLineImpl::do_WEND ()
{
	errorifparam ();
	if (runner.while_empty () )
		throw ErrWendWithoutWhile;
	WhileElement w= runner.while_top ();
	runner.jump_to (w.getpos () );
	runner.in_wend (true);
	return true;
}

bool RunnerLineImpl::do_PLOT ()
{
	BlInteger x, y;
	std::vector <graphics::Point> points;
	gettoken ();
	if (token.code == keyTO)
	{
		points.push_back (graphics::getlast () );
	}
	else
	{
		x= evalinteger ();
		requiretoken (',');
		y= expectinteger ();
		if (token.code != keyTO)
		{
			getinkparams ();
			graphics::plot (x, y);
			return false;
		}
		points.push_back (graphics::Point (int (x), int (y) ) );
	}
	for (;;)
	{
		x= expectinteger ();
		requiretoken (',');
		y= expectinteger ();
		points.push_back (graphics::Point (x, y) );
		if (endsentence () )
			break;
		requiretoken (keyTO);
	}
	graphics::plot (points);
	return false;
}

bool RunnerLineImpl::do_RESUME ()
{
	ProgramPos posresume= runner.geterrpos ();
	BlLineNumber errline= posresume.getnum ();
	if (errline == LineDirectCommand || errline == LineEndProgram)
		throw ErrCannotResume;

	gettoken ();
	if (token.code == keyNEXT)
	{
		gettoken ();
		require_endsentence ();
		runner.resume_next (posresume);
	}
	else if (endsentence () )
	{
		runner.jump_to (posresume);
	}
	else
	{
		BlLineNumber line= evallinenumber ();
		require_endsentence ();
		runner.goto_to (line);
	}
	runner.clearerror ();
	return true;
}

bool RunnerLineImpl::do_DELETE ()
{
	gettoken ();
	BlLineNumber iniline, endline;
	evallinerange (iniline, endline);
	require_endsentence ();
	program.deletelines (iniline, endline);
	runner.setstatus (Runner::Ended);
	return true;
}

bool RunnerLineImpl::do_LOCAL ()
{
	do
	{
		expecttoken (keyIDENTIFIER);
		runner.gosub_addlocalvar (token.str);
		gettoken ();
	} while (token.code == ',');
	require_endsentence ();
	return false;
}

void RunnerLineImpl::do_put_image ()
{
	BlInteger x= expectinteger ();
	requiretoken (',');
	BlInteger y= expectinteger ();
	requiretoken (')');
	expecttoken (',');
	gettoken ();
	requiretoken (keyIDENTIFIER);
	std::string name= token.str;
	gettoken ();

	// Mode used.
	int mode= 0;
	if (! endsentence () )
	{
		requiretoken (',');
		gettoken ();
		switch (token.code)
		{
		case keyXOR:
			mode= 1; break;
		case keyAND:
			mode= 2; break;
		case keyOR:
			mode= 3; break;
		case keyNOT:
			mode= 4; break;
		default:
			throw ErrSyntax;
		}
		gettoken ();
		require_endsentence ();
	}
	graphics::put_image (x, y, name, mode);
}

bool RunnerLineImpl::do_PUT ()
{
	gettoken ();
	if (token.code == '(')
	{
		do_put_image ();
		return false;
	}

	if (token.code == '#')
		gettoken ();
	BlChannel channel= evalchannel ();
	size_t pos= 0;
	if (token.code == ',')
	{
		BlNumber bnPos= expectnum ();
		pos= size_t (bnPos);
	}
	require_endsentence ();

	BlFile & out= getfile (channel);
	out.put (pos);

	return false;
}

bool RunnerLineImpl::do_FIELD ()
{
	gettoken ();
	enum FieldType { FieldSimple, FieldClear, FieldAppend };
	FieldType type= FieldSimple;
	switch (token.code)
	{
	case keyCLEAR:
		type= FieldClear;
		gettoken ();
		break;
	case keyAPPEND:
		type= FieldAppend;
		gettoken ();
		break;
	default:
		// Nothing to do.
		;
	}
	if (token.code == '#')
		gettoken ();
	BlChannel channel= evalchannel ();
	BlFile & out= getfile (channel);
	if (type == FieldClear)
	{
		require_endsentence ();
		out.field_clear ();
		return false;
	}
	std::vector <BlFile::field_element> fe;
	do
	{
		requiretoken (',');
		BlNumber bnSize= expectnum ();
		size_t size= size_t (bnSize);
		requiretoken (keyAS);
		expecttoken (keyIDENTIFIER);
		#if 1
		// Now matrix elements are accepted as field vars.
		gettoken ();
		Dimension dim;
		if (token.code == '(')
			dim= getdims ();
		fe.push_back (BlFile::field_element (size, token.str, dim) );
		#else
		fe.push_back ( BlFile::field_element (size, token.str) );
		gettoken ();
		#endif
	} while (! endsentence () );
	switch (type)
	{
	case FieldSimple:
		out.field (fe);
		break;
	case FieldAppend:
		out.field_append (fe);
		break;
	default:
		throw ErrBlassicInternal;
	}
	return false;
}

bool RunnerLineImpl::do_LSET ()
{
	BlFile::Align align;
	switch (token.code)
	{
	case keyLSET: align= BlFile::AlignLeft; break;
	case keyRSET: align= BlFile::AlignRight; break;
	default: throw ErrBlassicInternal;
	}

	// Get the variable to L/RSET.
	gettoken ();
	if (token.code != keyIDENTIFIER)
		throw ErrSyntax;
	std::string var= token.str;
	if (typeofvar (var) != VarString)
		throw ErrMismatch;
	//expecttoken ('=');
	//bool hasindex= false;
	Dimension dim;
	//std::string * pstr;
	gettoken ();
	if (token.code == '(')
	{
		// It's a matrix element.
		//hasindex= true;
		dim= getdims ();
		//pstr= addrdimstring (var, dim);
	}
	//else
	//	pstr= addrvarstring (var);

	requiretoken ('=');
	const std::string value= expectstring ();
	require_endsentence ();

	// First try to assign to a field var.

	#if 0
	// Blassic does not accept a matrix element as field.
	if (! hasindex)
		if (runner.assign_channel_var (var, value, align) )
			return false;
	#else
	// Now accepts.
	if (runner.assign_channel_var (var, dim, value, align) )
		return false;
	#endif

	// If a field with that name exists, the variable already
	// has been assigned, then we can return.

	// If there is not file field with such name, do the work
	// here with the previous string length.
	std::string * pstr= dim.empty () ?
		addrvarstring (var) :
		addrdimstring (var, dim);
	const std::string::size_type l= pstr->size ();
	if (l == 0)
		return false;
	#if 0
	const std::string::size_type lvalue= value.size ();
	if (lvalue < l)
	{
		if (align == BlFile::AlignLeft)
		{
			* pstr= value + std::string (l - lvalue, ' ');
		}
		else
		{
			* pstr= std::string (l - lvalue, ' ') + value;
		}
	}
	else if (lvalue > l)
	{
		* pstr= value.substr (0, l);
	}
	else
		* pstr= value;
	#else
	if (align == BlFile::AlignLeft)
		* pstr= util::stringlset (value, l);
	else
		* pstr= util::stringrset (value, l);
	#endif
	ASSERT (pstr->size () == l);
	return false;
}

bool RunnerLineImpl::do_SOCKET ()
{
	std::string host= expectstring ();
	requiretoken (',');
	BlNumber bn= expectnum ();
	requiretoken (keyAS);
	//gettoken ();
	//if (token.code == '#')
	//	gettoken ();
	//BlChannel channel= evalchannel ();
	BlChannel channel= expectrequiredchannel ();
	require_endsentence ();

	auto_ptr <BlFile> pbf (newBlFileSocket (host, short (bn) ) );
	runner.setfile (channel, pbf.get () );
	pbf.release ();

	return false;
}

bool RunnerLineImpl::do_MID_S ()
{
	expecttoken ('(');
	expecttoken (keyIDENTIFIER);
	std::string varname= token.str;
	if (typeofvar (varname) != VarString)
		throw ErrMismatch;

	Dimension dim;
	//std::string * presult;
	//expecttoken (',');
	gettoken ();
	if (token.code == '(')
	{
		dim= getdims ();
		//presult= addrdimstring (varname, dim);
	}
	//else
	//{
	//	presult= addrvarstring (varname);
	//}
	requiretoken (',');

	size_t inipos;
	{
		BlNumber bnInipos= expectnum ();
		inipos= util::checked_cast <size_t> (bnInipos, ErrMismatch);
	}
	if (inipos == 0)
		throw ErrMismatch;
	--inipos;
	//size_t len= 0; // Initialized to avoid warning.
	std::string::size_type len= 0; // Initialized to avoid warning.
	bool fLen= false;
	if (token.code == ',')
	{
		BlNumber bnLen= expectnum ();
		len= util::checked_cast <std::string::size_type>
			(bnLen, ErrMismatch);
		fLen= true;
	}
	requiretoken (')');
	expecttoken ('=');
	std::string value= expectstring ();
	require_endsentence ();

	if (! fLen)
		len= std::string::npos;

	// First try to assign to a field var.

	if (runner.assign_mid_channel_var (varname, dim, value, inipos, len) )
		return false;

	// If there is no file field, do the work here.

	std::string * pstr= dim.empty () ?
		addrvarstring (varname) :
		addrdimstring (varname, dim);
	size_t l= value.size ();
	if (! fLen || len > l)
		len= l;

	l= pstr->size ();
	if (inipos >= l)
		return false;
	if (inipos + len > l)
		len= l - inipos;
	std::copy (value.begin (), value.begin () + len,
		pstr->begin () + inipos);
	return false;
}

bool RunnerLineImpl::do_DRAW ()
{
	//BlNumber x= expectnum ();
	BlResult r;
	expect (r);
	if (r.type () == VarString)
		graphics::draw (r.str () );
	else
	{
		BlInteger x= r.integer ();
		#if 0
		requiretoken (',');
		//BlNumber y= expectnum ();
		expect (r);
		BlInteger y= r.integer ();

		require_endsentence ();
		#else
		BlInteger y;
		getdrawargs (y);
		#endif
		//graphics::line (int (x), int (y) );
		graphics::line (x, y);
	}
	return false;
}

namespace {

std::string quoteescape (const std::string & str)
{
	std::string result;
	for (std::string::size_type i= 0, l= str.size (); i < l; ++i)
	{
		char c= str [i];
		if (c == '"')
			result+= "\"\"";
		else
			result+= c;
	}
	return result;
}

} // namespace

bool RunnerLineImpl::do_def_fn ()
{
	// Get function name.

	expecttoken (keyIDENTIFIER);
	std::string fnname= token.str;
	gettoken ();

	// Get parameters.

	ParameterList param;
	switch (token.code)
	{
	case '=':
		// Single sentence function without parameters
		break;
	case '(':
		// Function with parameters
		do {
			expecttoken (keyIDENTIFIER);
			param.push_back (token.str);
			gettoken ();
		} while (token.code == ',');
		requiretoken (')');
		gettoken ();
		if (token.code != '=' && ! endsentence () )
			throw ErrSyntax;
		break;
	default:
		if (endsentence () )
			break; // Multi sentence function without parameters
		throw ErrSyntax;
	}

	// Get function body.

	bool retval= false;
	auto_ptr <Function> pf;
	if (endsentence () )
	{
		// Multi sentence function. Search for FN END.

		ProgramPos posfn (codeline.number (), codeline.chunk () );
		if (token.code == keyENDLINE)
			posfn.nextline ();
		bool sameline= true;
		for (bool finding= true; finding;)
		{
			getnextchunk ();
			if (token.code == keyENDLINE)
			{
				if (codeline.number () == LineDirectCommand)
					throw ErrInvalidDirect;
				program.getnextline (codeline);
				if (codeline.number () == LineEndProgram)
					throw ErrIncompleteDef;
				sameline= false;
				gettoken ();
			}
			switch (token.code)
			{
			case keyFN:
				gettoken ();
				if (token.code == keyEND)
					finding= false;
				//else
				//	throw ErrSyntax;
				break;
			}
		}
		gettoken ();
		require_endsentence ();

		// Prepare to skip function body.

		if (! sameline)
		{
			ProgramPos pos (codeline.number (),
				codeline.chunk () );
			if (token.code == keyENDLINE)
			{
				// Can't be direct command.
				pos.nextline ();
			}
			runner.jump_to (pos);
			retval= true;
		}
		pf.reset (new Function (posfn, param) );
	}
	else
	{
		// Single sentence funcion

		gettoken ();
		std::string fndef ("0 ");
		// The "0 " is the fake line number.
		for ( ; ! endsentence (); gettoken () )
		{
			if (token.code < 256)
				fndef+= token.code;
			else
				switch (token.code)
				{
				case keyIDENTIFIER:
				case keyNUMBER:
					fndef+= token.str;
					break;
				case keyINTEGER:
					fndef+= to_string
						(token.integer () );
					break;
				case keySTRING:
					fndef+= '"';
					fndef+= quoteescape (token.str);
					fndef+= '"';
					break;
				default:
					fndef+= decodekeyword (token.code);
				}
			fndef+= ' ';
		}
		pf.reset (new Function (fndef, param) );
	}

	// Put in table of functions.

	pf->insert (fnname);
	// No need to release pf, insert does a copy.

	return retval;
}

namespace {

inline char get_letter (const std::string & str)
{
	if (str.size () != 1)
		throw ErrSyntax;
	char c= str [0];
	if (! isalpha (c) )
		throw ErrSyntax;
	return c;
}

} // namespace

void RunnerLineImpl::definevars (VarType type)
{
	do
	{
		expecttoken (keyIDENTIFIER);
		char c= get_letter (token.str);
		gettoken ();
		if (token.code == '-')
		{
			expecttoken (keyIDENTIFIER);
			char c2= get_letter (token.str);
			definevar (type, c, c2);
			gettoken ();
		}
		else
			definevar (type, c);
	} while (token.code == ',');
	require_endsentence ();
}

bool RunnerLineImpl::do_DEF ()
{
	gettoken ();
	VarType type;
	switch (token.code)
	{
	case keyFN:
		return do_def_fn ();
	case keySTR:
	case keyINT:
	case keyREAL:
		type= token.code == keySTR ? VarString :
			token.code == keyINT ? VarInteger : VarNumber;
		#if 0
		do {
			expecttoken (keyIDENTIFIER);
			char c= get_letter (token.str);
			gettoken ();
			if (token.code == '-')
			{
				expecttoken (keyIDENTIFIER);
				char c2= get_letter (token.str);
				definevar (type, c, c2);
				gettoken ();
			}
			else
				definevar (type, c);
		} while (token.code == ',');
		require_endsentence ();
		#else
		definevars (type);
		#endif
		return false;
	default:
		throw ErrSyntax;
	}
}

bool RunnerLineImpl::do_FN ()
{
	gettoken ();
	bool isend= true;
	switch (token.code)
	{
	case keyEND:
		break;
	case keyRETURN:
		isend= false;
		break;
	default:
		throw ErrSyntax;
	}
	errorifparam ();

	//runner.setstatus (Runner::Ended);
	//return true;

	if (! isend)
	{
		if (runner.fn_level () == 0)
			throw ErrUnexpectedFnEnd;
		while (runner.gosub_size () > 1)
		{
			ProgramPos unused;
			runner.gosub_pop (unused);
		}
	}

	//throw blassic::ProgramFnEnd ();

	runner.setstatus (Runner::FnEnded);
	return true;
}

bool RunnerLineImpl::do_PROGRAMARG_S ()
{
	std::vector <std::string> args;
	gettoken ();
	if (! endsentence () )
	{
		for (;;)
		{
			std::string par= evalstring ();
			args.push_back (par);
			if (endsentence () )
				break;
			requiretoken (',');
			gettoken ();
		}
	}
	setprogramargs (args);
	return false;
}

bool RunnerLineImpl::do_ERASE ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_ERASE");

	do
	{
		expecttoken (keyIDENTIFIER);
		std::string str (token.str);
		switch (typeofvar (str) )
		{
		case VarNumber:
			erasevarnumber (str);
			break;
		case VarInteger:
			erasevarinteger (str);
			break;
		case VarString:
			erasevarstring (str);
			break;
		default:
			throw ErrBlassicInternal;
		}
		gettoken ();
	} while (token.code == ',');
	require_endsentence ();
	return false;
}

bool RunnerLineImpl::do_SWAP ()
{
	expecttoken (keyIDENTIFIER);
	std::string strvar1= token.str;
	gettoken ();
	Dimension dim1;
	bool isarray1= false;
	if (token.code == '(')
	{
		dim1= getdims ();
		isarray1= true;
	}
	requiretoken (',');
	gettoken ();
	requiretoken (keyIDENTIFIER);
	std::string strvar2= token.str;
	gettoken ();
	Dimension dim2;
	bool isarray2= false;
	if (token.code == '(')
	{
		dim2= getdims ();
		isarray2= true;
	}
	require_endsentence ();
	VarType type= typeofvar (strvar1);
	if (type != typeofvar (strvar2) )
		throw ErrMismatch;
	switch (type)
	{
	case VarNumber:
		{
			BlNumber * pbn1= isarray1 ?
				addrdimnumber (strvar1, dim1) :
				addrvarnumber (strvar1);
			BlNumber * pbn2= isarray2 ?
				addrdimnumber (strvar2, dim2) :
				addrvarnumber (strvar2);
			std::swap (* pbn1, * pbn2);
		}
		break;
	case VarInteger:
		{
			BlInteger * pbi1= isarray1 ?
				addrdiminteger (strvar1, dim1) :
				addrvarinteger (strvar1);
			BlInteger * pbi2= isarray2 ?
				addrdiminteger (strvar2, dim2) :
				addrvarinteger (strvar2);
			std::swap ( * pbi1, * pbi2);
		}
		break;
	case VarString:
		{
			std::string * pstr1= isarray1 ?
				addrdimstring (strvar1, dim1) :
				addrvarstring (strvar1);
			std::string * pstr2= isarray2 ?
				addrdimstring (strvar2, dim2) :
				addrvarstring (strvar2);
			std::swap (* pstr1, * pstr2);
		}
		break;
	default:
		throw ErrBlassicInternal;
	}
	return false;
}

bool RunnerLineImpl::do_SYMBOL ()
{
	gettoken ();
	if (token.code == keyAFTER)
	{
		BlInteger n= expectinteger ();

		// Support for change of character set.

		if (token.code == keyAS)
		{
			using namespace charset;
			std::string model= expectstring ();
			if (model == "default")
				default_charset= & default_data;
			else if (model == "cpc")
				default_charset= & cpc_data;
			else if (model == "spectrum")
				default_charset= & spectrum_data;
			else
				throw ErrImproperArgument;
		}

		require_endsentence ();
		graphics::symbolafter (static_cast <int> (n) );
		return false;
	}
	BlNumber bnSymbol= evalnum ();

	unsigned char byte [8];
	for (int i= 0; i < 8; ++i)
		byte [i]= 0;

	for (int i= 0; i < 8; ++i)
	{
		if (token.code != ',')
		{
			// Parameters can be omitted
			break;
		}
		BlNumber bnByte= expectnum ();
		byte [i]= static_cast <unsigned char> (bnByte);
	}
	require_endsentence ();
	graphics::definesymbol ( int (bnSymbol), byte);
	return false;
}

bool RunnerLineImpl::do_ZONE ()
{
	BlInteger z= expectinteger ();
	require_endsentence ();
	sysvar::set16 (sysvar::Zone, static_cast <short> (z) );
	return false;
}

bool RunnerLineImpl::do_POP ()
{
	errorifparam ();
	ProgramPos notused;
	runner.gosub_pop (notused);
	return false;
}

bool RunnerLineImpl::do_NAME ()
{
	std::string strOrig= expectstring ();
	requiretoken (keyAS);
	std::string strDest= expectstring ();
	require_endsentence ();
	rename_file (strOrig, strDest);
	return false;
}

bool RunnerLineImpl::do_KILL ()
{
	std::string strFile= expectstring ();
	require_endsentence ();
	remove_file (strFile);
	return false;
}

bool RunnerLineImpl::do_FILES ()
{
	std::string param;
	gettoken ();
	BlChannel ch= DefaultChannel;
	if (token.code == '#')
	{
		ch= expectchannel ();
		if (! endsentence () )
		{
			requiretoken (',');
			param= expectstring ();
		}
	}
	else
	{
		if (! endsentence () )
			param= evalstring ();
	}
	require_endsentence ();
	if (param.empty () )
		param= "*";

	std::vector <std::string> file;

	// Populate the vector with the files searched.
	Directory d;
	for (std::string r= d.findfirst (param.c_str () ); ! r.empty ();
			r= d.findnext () )
		file.push_back (r);

	const size_t l= file.size ();

	// GwBasic do this, I mimic it.
	if (l == 0)
		throw ErrFileNotFound;

	size_t maxlength= 0;
	for (size_t i= 0; i < l; ++i)
		maxlength= std::max (maxlength, file [i].size () );
	++maxlength;
	BlFile & bf= getfile (ch);
	size_t width= 0;
	try
	{
		width= bf.getwidth ();
	}
	catch (...)
	{
	}
	size_t cols= width / maxlength;
	if (cols <= 1)
	{
		for (size_t i= 0; i < l; ++i)
		{
			//bf << file [i] << '\n';
			bf << file [i];
			bf.endline ();
		}
	}
	else
	{
		const size_t widthcol= width / cols;
		for (size_t i= 0; i < l; ++i)
		{
			const std::string & str= file [i];
			bf << file [i];
			if (i % cols == cols - 1)
				//bf << '\n';
				bf.endline ();
			else
				bf << std::string
					(widthcol - str.size (), ' ');
		}
		if ( l > 0 && l % cols != 0)
			//bf << '\n';
			bf.endline ();
	}
	return false;
}

bool RunnerLineImpl::do_PAPER ()
{
	gettoken ();
	BlChannel ch= DefaultChannel;
	if (token.code == '#')
	{
		ch= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	BlInteger color= evalinteger ();
	require_endsentence ();
	#if 0
	if (graphics::ingraphicsmode () )
		graphics::setbackground (color);
	else
		textbackground (color);
	#else
	BlFile & out= getfile (ch);
	out.setbackground (color);
	#endif
	return false;
}

bool RunnerLineImpl::do_PEN ()
{
	gettoken ();
	BlChannel ch= DefaultChannel;
	if (token.code == '#')
	{
		ch= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	BlFile & out= getfile (ch);
	if (token.code != ',')
	{
		// All parameters can't be omitted
		BlInteger color= evalinteger ();
		#if 0
		if (graphics::ingraphicsmode () )
			graphics::setcolor (int (color) );
		else
			textcolor (int (color) );
		#else
		out.setcolor (color);
		#endif
		if (token.code != ',')
		{
			require_endsentence ();
			return false;
		}
	}
	gettoken ();
	if (token.code != ',')
	{
		BlInteger bgmode= evalinteger ();
		graphics::settransparent (bgmode);
		if (token.code != ',')
		{
			require_endsentence ();
			return false;
		}
	}
	BlInteger mode= expectinteger ();
	graphics::setdrawmode (mode);
	require_endsentence ();
	return false;
}

bool RunnerLineImpl::do_SHELL ()
{
	gettoken ();
	if (endsentence () )
		throw ErrNotImplemented;
	std::string command= evalstring ();
	require_endsentence ();
	int r= system (command.c_str () );
	//std::cerr << "Result: " << r << endl;
	if (r == -1)
		throw ErrOperatingSystem;
	sysvar::set (sysvar::ShellResult, char (r >> 8) );
	return false;
}

bool RunnerLineImpl::do_MERGE ()
{
	std::string progname= expectstring ();
	require_endsentence ();
	program.merge (progname);
	runner.setstatus (Runner::Ended);
	return true;
}

bool RunnerLineImpl::do_CHDIR ()
{
	std::string dirname= expectstring ();
	require_endsentence ();
	change_dir (dirname);
	return false;
}

bool RunnerLineImpl::do_MKDIR ()
{
	std::string dirname= expectstring ();
	require_endsentence ();
	make_dir (dirname);
	return false;
}

bool RunnerLineImpl::do_RMDIR ()
{
	std::string dirname= expectstring ();
	require_endsentence ();
	remove_dir (dirname);
	return false;
}

bool RunnerLineImpl::do_SYNCHRONIZE ()
{
	gettoken ();
	if (endsentence () )
		graphics::synchronize ();
	else
	{
		BlNumber n= evalnum ();
		graphics::synchronize (n != 0);
	}
	return false;
}

bool RunnerLineImpl::do_PAUSE ()
{
	TRACEFUNC (tr, "do_pause");

	BlNumber bln= expectnum ();
	require_endsentence ();
	unsigned long n= static_cast <unsigned long> (bln);

	// Allow pending writes in graphic window before the pause.
	graphics::idle ();

	sleep_milisec (n);

	return false;
}

bool RunnerLineImpl::do_CHAIN ()
{
	bool merging= false;
	BlLineNumber inidel= LineNoDelete;
	BlLineNumber enddel= LineNoDelete;

	gettoken ();
	if (token.code == keyMERGE)
	{
		merging= true;
		gettoken ();
	}
	std::string progname= evalstring ();
	BlLineNumber iniline= LineBeginProgram;
	if (! endsentence () )
	{
		requiretoken (',');
		gettoken ();
		if (token.code != keyDELETE)
		{
			iniline= evallinenumber ();
			if (! endsentence () )
			{
				requiretoken (',');
				expecttoken (keyDELETE);
			}
		}
		if (token.code == keyDELETE)
		{
			if (! merging)
				throw ErrSyntax;
			gettoken ();
			evallinerange (inidel, enddel);
		}
		require_endsentence ();
	}

	if (merging)
		program.merge (progname, inidel, enddel);
	else
		program.load (progname);

	runner.run_to (iniline);
	return true;
}

bool RunnerLineImpl::do_ENVIRON ()
{
	BlResult result;
	expect (result);
	const std::string & str= result.str ();
	size_t l= str.size ();
	// We use an auto alloc, then in case of error the memory is freed.
	util::auto_alloc <char> envstr (l + 1);
	memcpy (envstr, str.data (), l);
	envstr [l]= 0;
	if (putenv (envstr) != 0)
		throw ErrFunctionCall;
	// Do not free the string now, is part of the environment.
	envstr.release ();
	return false;
}

bool RunnerLineImpl::do_EDIT ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_edit");

	gettoken ();
	if (token.code != keyNUMBER && token.code != keyINTEGER)
		throw ErrSyntax;
	BlLineNumber dest= evallinenumber ();
	require_endsentence ();

	#if 0

	std::string buffer;
	{
		BlFileOutString bfos;
		program.list (dest, dest, bfos);
		buffer= bfos.str ();
		if (buffer.empty () )
		{
			//bfos << dest << " \n";
			bfos << dest;
			bfos.endline ();
			buffer= bfos.str ();
		}
	}
	buffer.erase (buffer.size () - 1);
	static const std::string number ("01234567890");
	size_t inipos= buffer.find_first_of (number);
	ASSERT (inipos != std::string::npos);
	inipos= buffer.find_first_not_of (number, inipos);
	ASSERT (inipos != std::string::npos);
	++inipos;

	if (editline (getfile0 (), buffer, inipos) )
	{
		CodeLine code;
		code.scan (buffer);
		BlLineNumber nline= code.number ();
		if (nline == 0)
			throw ErrBlassicInternal;
		else
		{
			if (code.empty () )
				program.deletelines (nline, nline);
			else
				program.insert (code);
		}
	}

	#else

	//editline (getfile0 (), program, dest);
	std::string line;
	if (runner.editline (dest, line) )
	{
		CodeLine codeline;
		codeline.scan (line);
		BlLineNumber nline= codeline.number ();
		if (nline == LineDirectCommand)
		{
			// Need revision.
			runner.runline (codeline);
		}
		else
		{
			program.insert (codeline);
		}
	}

	#endif

	return false;
}

bool RunnerLineImpl::do_DRAWR ()
{
	#if 0
	BlNumber x= expectnum ();
	requiretoken (',');
	BlNumber y= expectnum ();
	require_endsentence ();
	graphics::liner (int (x), int (y) );
	#else
	BlInteger x, y;
	getdrawargs (x, y);
	graphics::liner (x, y);
	#endif
	return false;
}

bool RunnerLineImpl::do_PLOTR ()
{
	#if 0
	BlNumber x= expectnum ();
	requiretoken (',');
	BlNumber y= expectnum ();
	require_endsentence ();
	graphics::plotr (int (x), int (y) );
	#else
	BlInteger x, y;
	getdrawargs (x, y);
	graphics::plotr (x, y);
	#endif
	return false;
}

bool RunnerLineImpl::do_MOVER ()
{
	#if 0
	BlNumber x= expectnum ();
	requiretoken (',');
	BlNumber y= expectnum ();
	require_endsentence ();
	graphics::mover (int (x), int (y) );
	#else
	BlInteger x, y;
	getdrawargs (x, y);
	graphics::mover (x, y);
	#endif
	return false;
}

bool RunnerLineImpl::do_POKE16 ()
{
	BlNumber bnAddr= expectnum ();
	requiretoken (',');
	BlChar * addr= (BlChar *) (size_t) bnAddr;
	BlNumber bnValue= expectnum ();
	require_endsentence ();
	unsigned short value= (unsigned short) bnValue;
	poke16 (addr, value);
	return false;
}

bool RunnerLineImpl::do_POKE32 ()
{
	BlNumber bnAddr= expectnum ();
	requiretoken (',');
	BlChar * addr= (BlChar *) (size_t) bnAddr;
	BlNumber bnValue= expectnum ();
	require_endsentence ();
	BlInteger value= BlInteger (bnValue);
	poke32 (addr, value);
	return false;
}

bool RunnerLineImpl::do_RENUM ()
{
	gettoken ();
	BlLineNumber newnumber= 10;
	BlLineNumber oldnumber= LineBeginProgram;
	BlLineNumber increment= 10;
	BlLineNumber stop= LineEndProgram;
	if (! endsentence () )
	{
		if (token.code != ',')
			newnumber= evallinenumber ();
		if (! endsentence () )
		{
			requiretoken (',');
			gettoken ();
			if (token.code != ',')
				oldnumber= evallinenumber ();
			if (! endsentence () )
			{
				requiretoken (',');
				gettoken ();
				if (token.code != ',')
					increment= evallinenumber ();
				if (! endsentence () )
				{
					requiretoken (',');
					gettoken ();
					stop= evallinenumber ();
					require_endsentence ();
				}
			}
		}
	}
	program.renum (newnumber, oldnumber, increment, stop);
	runner.setstatus (Runner::Ended);
	return true;
}

bool RunnerLineImpl::do_CIRCLE ()
{
	gettoken ();
	//requiretoken ('(');
	if (token.code != '(')
	{
		// Spectrum syntax.
		BlInteger x= evalinteger ();
		requiretoken (',');
		BlInteger y= expectinteger ();
		requiretoken (',');
		BlInteger radius= expectinteger ();
		require_endsentence ();
		graphics::circle (x, y, radius);
		return false;
	}
	BlResult r;
	expect (r);
	BlInteger x= r.integer ();
	requiretoken (',');
	expect (r);
	BlInteger y= r.integer ();
	requiretoken (')');
	expecttoken (',');
	expect (r);
	BlInteger radius= r.integer ();
	BlNumber arcbeg= 0, arcend= 0;
	bool fArc= false;
	bool fElliptic= false;
	BlNumber elliptic= 0; // initialized to avoid a warning.
	if (endsentence () )
		goto do_it;
	requiretoken (',');
	gettoken ();
	if (token.code != ',')
	{
		BlInteger color= evalinteger ();
		graphics::setcolor (color);
		if (endsentence () )
			goto do_it;
		requiretoken (',');
	}
	gettoken ();
	if (token.code != ',')
	{
		arcbeg= evalnum ();
		fArc= true;
		if (endsentence () )
			goto do_it;
		requiretoken (',');
	}
	gettoken ();
	if (token.code != ',')
	{
		arcend= evalnum ();
		fArc= true;
		if (endsentence () )
			goto do_it;
		requiretoken (',');
	}
	gettoken ();
	elliptic= evalnum ();
	fElliptic= true;
	require_endsentence ();
do_it:
	if (! fElliptic)
	{
		if (! fArc)
			graphics::circle (x, y, radius);
		else
			graphics::arccircle (x, y, radius, arcbeg, arcend);
	}
	else
	{
		int rx, ry;
		if (elliptic > 1)
		{
			rx= static_cast <int> (radius / elliptic);
			ry= radius;
		}
		else
		{
			rx= radius;
			ry= static_cast <int> (radius * elliptic);
		}
		if (! fArc)
			graphics::ellipse (x, y, rx, ry);
		else
			graphics::arcellipse (x, y, rx, ry, arcbeg, arcend);
	}
	return false;
}

bool RunnerLineImpl::do_MASK ()
{
	BlResult r;
	gettoken ();
	if (token.code != ',')
	{
		eval (r);
		graphics::mask (r.integer () );
	}
	if (! endsentence () )
	{
		requiretoken (',');
		expect (r);
		graphics::maskdrawfirstpoint (r.integer () );
	}
	require_endsentence ();
	return false;
}

bool RunnerLineImpl::do_WINDOW ()
{
	gettoken ();
	if (token.code == keySWAP)
	{
		BlChannel ch1= expectchannel ();
		requiretoken (',');
		BlChannel ch2= expectchannel ();
		require_endsentence ();
		runner.windowswap (ch1, ch2);
		return false;
	}

	BlChannel ch= DefaultChannel;
	if (token.code == '#')
	{
		ch= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	BlInteger x1= evalinteger ();
	requiretoken (',');
	BlInteger x2= expectinteger ();
	requiretoken (',');
	BlInteger y1= expectinteger ();
	requiretoken (',');
	BlInteger y2= expectinteger ();
	require_endsentence ();
	if (runner.isfileopen (ch) )
	{
		BlFile & bf= runner.getfile (ch);
		if (bf.istextwindow () )
		{
			bf.reset (x1, x2, y1, y2);
			return false;
		}
	}

	auto_ptr <BlFile> pbf (newBlFileWindow (ch, x1, x2, y1, y2) );
	runner.setfile (ch, pbf.get () );
	pbf.release ();

	return false;
}

void RunnerLineImpl::do_graphics_pen ()
{
	gettoken ();
	if (token.code != ',')
	{
		BlInteger ink= evalinteger ();
		graphics::setcolor (ink);
		if (endsentence () )
			return;
		requiretoken (',');
	}
	BlInteger transpmode= expectinteger ();
	graphics::settransparent (transpmode);
	require_endsentence ();
}

void RunnerLineImpl::do_graphics_paper ()
{
	BlInteger ink= expectinteger ();
	require_endsentence ();
	graphics::setbackground (ink);
}

void RunnerLineImpl::do_graphics_cls ()
{
	gettoken ();
	if (! endsentence () )
	{
		BlInteger ink= evalinteger ();
		require_endsentence ();
		graphics::setbackground (ink);
	}
	graphics::cls ();
}

bool RunnerLineImpl::do_GRAPHICS ()
{
	gettoken ();
	switch (token.code)
	{
	case keyPEN:
		do_graphics_pen ();
		break;
	case keyPAPER:
		do_graphics_paper ();
		break;
	case keyCLS:
		do_graphics_cls ();
		break;
	default:
		throw ErrSyntax;
	}
	return false;
}

bool RunnerLineImpl::do_BEEP ()
{
	gettoken ();
	require_endsentence ();

	#if 0
	if (graphics::ingraphicsmode () )
		graphics::ring ();
	else
		cursor::ring ();
	#else
	runner.ring ();
	#endif

	return false;
}

bool RunnerLineImpl::do_DEFINT ()
{
	VarType type;
	switch (token.code)
	{
	case keyDEFINT:
		type= VarInteger; break;
	case keyDEFSTR:
		type= VarString; break;
	case keyDEFREAL: case keyDEFSNG: case keyDEFDBL:
		type= VarNumber; break;
	default:
		throw ErrBlassicInternal;
	}
	definevars (type);
	return false;
}

bool RunnerLineImpl::do_INK ()
{
	int inknum= expectinteger ();
	if (endsentence () )
	{
		// INK Spectrum style (set pen color).
		getfile0 ().setcolor (inknum);
		return false;
	}
	requiretoken (',');
	int r= expectinteger ();
	if (endsentence () )
	{
		graphics::ink (inknum, r);
		return false;
	}
	requiretoken (',');
	int g= expectinteger ();
	if (endsentence () )
	{
		// Flashing ink in Amstrad CPC,
		// just ignore second parameter.
		graphics::ink (inknum, r);
		return false;
	}
	requiretoken (',');
	int b= expectinteger ();
	require_endsentence ();
	graphics::ink (inknum, r, g, b);
	return false;
}

bool RunnerLineImpl::do_SET_TITLE ()
{
	std::string str= expectstring ();
	require_endsentence ();

	#if 0
	if (graphics::ingraphicsmode () )
		graphics::set_title (str);
	else
		cursor::set_title (str);
	#else
	runner.set_title (str);
	#endif

	return false;
}

bool RunnerLineImpl::do_TAG ()
{
	BlCode code= token.code;
	ASSERT (code == keyTAG || code == keyTAGOFF);

	//BlChannel ch= DefaultChannel;
	//gettoken ();
	//if (token.code == '#')
	//	ch= expectchannel ();
	BlChannel ch= expectoptionalchannel ();

	require_endsentence ();
	BlFile & f= runner.getfile (ch);
	if (code == keyTAG)
		f.tag ();
	else
		f.tagoff ();
	return false;
}

bool RunnerLineImpl::do_ORIGIN ()
{
	BlInteger x= expectinteger ();
	requiretoken (',');
	BlInteger y= expectinteger ();
	if (! endsentence () )
	{
		requiretoken (',');
		BlInteger minx= expectinteger ();
		requiretoken (',');
		BlInteger maxx= expectinteger ();
		requiretoken (',');
		BlInteger miny= expectinteger ();
		requiretoken (',');
		BlInteger maxy= expectinteger ();
		require_endsentence ();
		graphics::limits (minx, maxx, miny, maxy);
	}
	graphics::origin (x, y);
	return false;
}

bool RunnerLineImpl::do_DEG ()
{
	ASSERT (token.code == keyDEG || token.code == keyRAD);
	TrigonometricMode newmode= token.code == keyRAD ?
		TrigonometricRad : TrigonometricDeg;
	gettoken ();
	require_endsentence ();
	runner.trigonometric_mode (newmode);
	return false;
}

bool RunnerLineImpl::do_INVERSE ()
{
	BlChannel ch= DefaultChannel;
	gettoken ();
	if (token.code == '#')
	{
		ch= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	BlInteger inv= evalinteger ();
	runner.getfile (ch).inverse (inv % 2);
	return false;
}

bool RunnerLineImpl::do_IF_DEBUG ()
{
	BlInteger n= expectinteger ();
	require_endsentence ();
	// If the parameter is lower than the current debug level,
	// ignore the rest of the line.
	return n > sysvar::get16 (sysvar::DebugLevel);
}

bool RunnerLineImpl::do_WIDTH ()
{
	expecttoken (keyLPRINT);
	BlFile & printer= getfile (PrinterChannel);
	gettoken ();
	if (token.code != ',')
	{
		BlInteger w= evalinteger ();
		printer.setwidth (w);
		if (endsentence () )
			return false;
	}
	requiretoken (',');
	BlInteger m= expectinteger ();
	require_endsentence ();
	printer.setmargin (m);
	return false;
}

bool RunnerLineImpl::do_BRIGHT ()
{
	BlChannel ch= DefaultChannel;
	gettoken ();
	if (token.code == '#')
	{
		ch= expectchannel ();
		requiretoken (',');
		gettoken ();
	}
	BlInteger br= evalinteger ();
	runner.getfile (ch).bright (br % 2);
	return false;
}

bool RunnerLineImpl::do_PLEASE ()
{
	gettoken ();
	if (endsentence () )
		throw ErrPolite;
	if (token.code == keyPLEASE)
		throw ErrNoTeDejo;

	#if 1

	return execute_instruction ();

	#else

	#if 0

	mapfunc_t::const_iterator it=
		mapfunc.find (token.code);
	if (it == mapend)
		throw ErrSyntax;
	return (this->*it->second) ();

	#else

	return (this->*findfunc (token.code) ) ();

	#endif

	#endif
}

bool RunnerLineImpl::do_DRAWARC ()
{
	BlInteger x= expectinteger ();
	requiretoken (',');
	BlInteger y= expectinteger ();
	BlNumber g= 0;
	if (token.code == ',')
		g= expectnum ();
	require_endsentence ();
	graphics::drawarc (x, y, g);
	return false;
}

bool RunnerLineImpl::do_PULL ()
{
	enum PullType { PullRepeat, PullFor, PullWhile, PullGosub };
	gettoken ();
	PullType type= PullRepeat;
	switch (token.code)
	{
	case keyREPEAT:
		gettoken ();
		break;
	case keyFOR:
		type= PullFor;
		gettoken ();
		break;
	case keyWHILE:
		type= PullWhile;
		gettoken ();
		break;
	case keyGOSUB:
		type= PullGosub;
		gettoken ();
		break;
	}
	require_endsentence ();
	switch (type)
	{
	case PullRepeat:
		if (runner.repeat_empty () )
			throw ErrUntilWithoutRepeat;
		runner.repeat_pop ();
		break;
	case PullFor:
		runner.for_top (); // Simple way to do a for stack check.
		runner.for_pop ();
		break;
	case PullWhile:
		if (runner.while_empty () )
			throw ErrWendWithoutWhile;
		runner.while_pop ();
		break;
	case PullGosub:
		// Same as POP
		{
			ProgramPos pos;
			runner.gosub_pop (pos);
		}
		break;
	}
	return false;
}

bool RunnerLineImpl::do_PAINT ()
{
	expecttoken ('(');
	BlInteger x= expectinteger ();
	requiretoken (',');
	BlInteger y= expectinteger ();
	requiretoken (')');

	int actual= graphics::getcolor ();
	int paintattr= actual, borderattr= actual;
	gettoken ();
	if (token.code == ',')
	{
		gettoken ();
		if (token.code != ',')
		{
			paintattr= evalinteger ();
			borderattr= paintattr;
		}
		if (! endsentence () )
		{
			requiretoken (',');
			borderattr= expectinteger ();
		}

	}
	require_endsentence ();

	graphics::paint (x, y, paintattr, borderattr);
	return false;
}

bool RunnerLineImpl::do_FREE_MEMORY ()
{
	gettoken ();
	if (endsentence () )
	{
		blassic::memory::dyn_freeall ();
		return false;
	}
	BlInteger mempos= evalinteger ();
	require_endsentence ();
	blassic::memory::dyn_free (mempos);
	return false;
}

bool RunnerLineImpl::do_SCROLL ()
{
	BlChannel ch= DefaultChannel;
	int nlines= 1;
	gettoken ();
	if (! endsentence () )
	{
		if (token.code == '#')
		{
			ch= expectchannel ();
			if (token.code == ',')
				nlines= expectinteger ();
		}
		else
			nlines= evalinteger ();
		require_endsentence ();
	}
	runner.getfile (ch).scroll (nlines);
	return false;
}

bool RunnerLineImpl::do_ZX_PLOT ()
{
	BlInteger x= expectinteger ();
	requiretoken (',');
	BlInteger y= expectinteger ();
	require_endsentence ();
	graphics::zxplot (graphics::Point (x, y) );
	return false;
}

bool RunnerLineImpl::do_ZX_UNPLOT ()
{
	BlInteger x= expectinteger ();
	requiretoken (',');
	BlInteger y= expectinteger ();
	require_endsentence ();
	graphics::zxplot (graphics::Point (x, y) );
	return false;
}

bool RunnerLineImpl::do_ELSE ()
{
	// This is not the best possible way of treating ELSE,
	// but can't diagnose an error of ELSE misplaced with
	// the available info without reexploring the line,
	// and that will be slow.
	// Anyway, several old Basic I have tested do the same
	// thing, and after all the goal is to be similar to
	// these.
	return true;
}

// End of runnerline_instructions.cpp
