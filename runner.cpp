// runner.cpp
// Revision 24-apr-2009

#include "runner.h"

#include "result.h"

// Testing
#include "runnerline.h"
//#include "runnerline_impl.h"

#include "program.h"
#include "keyword.h"
#include "var.h"
#include "codeline.h"
#include "dim.h"
#include "cursor.h"
#include "graphics.h"
#include "sysvar.h"

#include "util.h"
using util::to_string;
using util::touch;

#include "trace.h"
#include "edit.h"
#include "socket.h"
#include "memory.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <vector>
#include <algorithm>
#include <memory>
#include <typeinfo>

#include <math.h>

#include <cassert>
#define ASSERT assert

#if __BORLANDC__ >= 0x0560
#pragma warn -8091
#endif

using std::cerr;
using std::endl;
using std::for_each;
using std::auto_ptr;

namespace sysvar= blassic::sysvar;
namespace onbreak= blassic::onbreak;

using namespace blassic::file;

//************************************************
//		Auxiliar
//************************************************

namespace {

const std::string strbreak ("**+BREAK+**");

void deletefile (GlobalRunner::ChanFile::value_type & chf)
{
	delete chf.second;
}

} // namespace

//************************************************
//		GlobalRunner
//************************************************

GlobalRunner::GlobalRunner (Program & prog) :
	program (prog),
	fTron (false),
	alreadypolled (false),
	breakstate (onbreak::BreakStop),
	fn_current_level (0)
{
	TRACEFUNC (tr, "GlobalRunner::GlobalRunner");

	clearerrorgoto ();
	clearreadline ();
	resetfile0 ();
	resetfileprinter ();
	trigonometric_default ();
}

GlobalRunner::~GlobalRunner ()
{
	TRACEFUNC (tr, "GlobalRunner::~GlobalRunner");

	for_each (chanfile.begin (), chanfile.end (), deletefile);
	blassic::memory::dyn_freeall ();
}

void GlobalRunner::tron (bool fLine, BlChannel blc)
{
	fTron= true;
	fTronLine= fLine;
	BlChar flags= static_cast <BlChar> (fLine ? 3 : 1);
	BlChar oldflags= sysvar::get (sysvar::TronFlags);
	flags= flags | (oldflags & static_cast <BlChar> (~3) );
	sysvar::set (sysvar::TronFlags, flags);
	blcTron= blc;
	sysvar::set16 (sysvar::TronChannel, blc);
}

void GlobalRunner::troff ()
{
	fTron= false;
	fTronLine= false;
	BlChar oldflags= sysvar::get (sysvar::TronFlags);
	sysvar::set (sysvar::TronFlags, oldflags & static_cast <BlChar> (~3) );
	sysvar::set16 (sysvar::TronChannel, 0);
}

void GlobalRunner::do_tronline (const CodeLine & codeline)
{
	if (codeline.number () == LineDirectCommand)
		return;

	BlFile & file= getfile (blcTron);
	if (fTronLine)
		program.listline (codeline, file);
	else
	{
		//file << '[' << BlNumber (codeline.number () ) << ']';
		file << '[' <<
			static_cast <BlInteger> (codeline.number () ) <<
			']';
		file.flush ();
	}
}

void GlobalRunner::clearerrorgoto ()
{
	blnErrorGoto= LineEndProgram;
	blnErrorGotoSource= LineEndProgram;
}

void GlobalRunner::seterrorgoto (BlLineNumber line, BlLineNumber source)
{
	// Line must be a valid program line except 0, because 0 mean
	// deactivate, source can be a valid program line or a direct
	// command.
	if (line > BlMaxLineNumber ||
		(source > BlMaxLineNumber &&  source != LineDirectCommand ) )
	{
		ASSERT (false);
		throw ErrBlassicInternal;
	}
	blnErrorGoto= line;
	blnErrorGotoSource= source;
}

bool GlobalRunner::assign_channel_var
	(const std::string & var, const Dimension & dim,
		const std::string & value, BlFile::Align align)
{
	bool r= false;
	for (ChanFile::iterator it= chanfile.begin ();
		it != chanfile.end ();
		++it)
	// Changed behaviour: instead to assign to all possible
	// fields with the same name in all files, assign only
	// to the first.
	// Retesting the initial behaviour.
	#if 1
	{
		if (it->second->assign (var, dim, value, align) )
			r= true;
	}
	return r;
	#else
	{
		if (it->second->assign (var, dim, value, align) )
			return true;
	}
	return false;
	#endif
}

bool GlobalRunner::assign_mid_channel_var
	(const std::string & var, const Dimension & dim,
		const std::string & value,
		size_t inipos, std::string::size_type len)
{
	bool r= false;
	for (ChanFile::iterator it= chanfile.begin ();
		it != chanfile.end ();
		++it)
	{
		if (it->second->assign_mid (var, dim, value, inipos, len) )
			r= true;
	}
	return r;
}

BlChannel GlobalRunner::freefile () const
{
	for (BlChannel channel= 1; channel < PrinterChannel; ++channel)
		if (chanfile.find (channel) == chanfile.end () )
			return channel;
	return 0;
}

BlFile & GlobalRunner::getfile (BlChannel channel) const
{
	ChanFile::const_iterator it= chanfile.find (channel);
	if (it == chanfile.end () )
	{
		if (showdebuginfo () )
			cerr << "Channel " << channel <<
				" is not opened" << endl;
		throw ErrFileNumber;
	}
	return * it->second;
}

void GlobalRunner::setfile (BlChannel channel, BlFile * npfile)
{
	TRACEFUNC (tr, "GlobalRunner::setfile");

	std::pair <ChanFile::iterator, bool> r
		(chanfile.insert (ChanFile::value_type (channel, npfile) ) );
	if (! r.second)
	{
		TRMESSAGE (tr, "Exist, delete and substitute");
		delete r.first->second;
		r.first->second= npfile;
	}
	else
	{
		TRMESSAGE (tr, "Inserted new");
	}
}

void GlobalRunner::resetfile0 ()
{
	TRACEFUNC (tr, "GlobalRunner::resetfile0");

	auto_ptr <BlFile> pf (graphics::ingraphicsmode () ?
		newBlFileWindow (DefaultChannel) :
		newBlFileConsole () );
	setfile (DefaultChannel, pf.get () );
	pf.release ();
}

void GlobalRunner::resetfileprinter ()
{
	auto_ptr <BlFile> pf (newBlFilePrinter () );
	setfile (PrinterChannel, pf.get () );
	pf.release ();
}

void GlobalRunner::close_all ()
{
	std::vector <BlChannel> w;
	for (ChanFile::iterator it= chanfile.begin ();
		it != chanfile.end (); ++it)
	{
		BlFile * f= it->second;
		if (f->isfile () )
		{
			delete f;
			w.push_back (it->first);
		}
	}
	for (size_t i= 0, l= w.size (); i < l; ++i)
	{
		size_t d= chanfile.erase (w [i] );
		ASSERT (d == 1);
		(void) d; // Avoid warning about unused.
	}
}

void GlobalRunner::destroy_windows ()
{
	TRACEFUNC (tr, "GlobalRunner::destroy_windows");

	std::vector <BlChannel> w;
	for (ChanFile::iterator it= chanfile.begin ();
		it != chanfile.end (); ++it)
	{
		BlFile * f= it->second;
		//if (typeid (* f) == typeid (BlFileWindow) )
		if (f->istextwindow () )
		{
			delete f;
			w.push_back (it->first);
		}
	}
	for (size_t i= 0, l= w.size (); i < l; ++i)
	{
		TRMESSAGE (tr, "Destroying window " + to_string (w [i] ) );
		size_t d= chanfile.erase (w [i] );
		ASSERT (d == 1);
		touch (d);
	}
}

void GlobalRunner::closechannel (BlChannel channel)
{
	if (channel == PrinterChannel)
		resetfileprinter ();
	else
	{
		ChanFile::iterator it= chanfile.find (channel);
		if (it != chanfile.end () )
		{
			delete it->second;
			chanfile.erase (it);
		}
	}
}

void GlobalRunner::windowswap (BlChannel ch1, BlChannel ch2)
{
	bool showdebug= showdebuginfo ();
	ChanFile::iterator it1= chanfile.find (ch1);
	ChanFile::iterator it2= chanfile.find (ch2);
	if (it1 == chanfile.end () || it2 == chanfile.end () )
	{
		if (showdebug)
		{
			if (it1 == chanfile.end () )
				cerr << "Channel " << ch1 <<
					" is not opened" << endl;
			if (it2 == chanfile.end () )
				cerr << "Channel " << ch2 <<
					" is not opened" << endl;
		}
		throw ErrFileNumber;
	}

	//bool fail1= typeid (* it1->second) != typeid (BlFileWindow);
	//bool fail2= typeid (* it2->second) != typeid (BlFileWindow);
	bool fail1= ! it1->second->istextwindow ();
	bool fail2= ! it2->second->istextwindow ();
	if (fail1 || fail2)
	{
		if (showdebug)
		{
			if (fail1)
				cerr << "Channel " << ch1 <<
					" is not a window" << endl;
			if (fail2)
				cerr << "Channel " << ch2 <<
					" is not a window" << endl;
		}
		throw ErrFileMode;
	}

	std::swap (it1->second, it2->second);
}

void GlobalRunner::pollchannel (BlChannel ch, BlLineNumber line)
{
	TRACEFUNC (tr, "GlobalRunner::pollchannel");

	if (! isfileopen (ch) )
		throw ErrFileNumber;
	if (line == 0)
		chanpolled.erase (ch);
	else
		chanpolled [ch]= line;
}

bool GlobalRunner::channelspolled ()
{
	// This is to let the program execute one instruction
	// before doing other poll check.

	if (chanpolled.empty () )
		return false;
	else
	{
		if (alreadypolled)
		{
			if (clearingpolled)
				alreadypolled= false;
			return false;
		}
		else
			return true;
	}
}

void GlobalRunner::setpolled ()
{
	alreadypolled= true;
	clearingpolled= false;
}

void GlobalRunner::clearpolled ()
{
	clearingpolled= true;
}

namespace {

class Polled {
	GlobalRunner & gr;
public:
	Polled (GlobalRunner & gr) :
		gr (gr)
	{ }
	bool operator () (const GlobalRunner::ChanPolled::value_type & cp)
	{
		BlFile & f= gr.getfile (cp.first);
		return f.poll ();
	}
};

} // namespace

BlLineNumber GlobalRunner::getpollnumber ()
{
	TRACEFUNC (tr, "GlobalRunner::getpollnumber");

	ChanPolled::iterator it=
		std::find_if (chanpolled.begin (), chanpolled.end (),
			Polled (* this) );
	if (it != chanpolled.end () )
		return it->second;
	else
		return LineEndProgram;
}

void GlobalRunner::inc_fn_level ()
{
	const unsigned long maxlevel= sysvar::get32 (sysvar::MaxFnLevel);
	if (fn_current_level >= maxlevel)
	{
		if (showdebuginfo () )
			cerr << "Maximum level of FN recursion of "  <<
				maxlevel << " has been reached" << endl;
		throw ErrFnRecursion;
	}

	++fn_current_level;
}

void GlobalRunner::dec_fn_level ()
{
	if (fn_current_level == 0)
	{
		if (showdebuginfo () )
			cerr << "Internal problem handling FN exit" <<
				endl;
		throw ErrBlassicInternal;
	}
	--fn_current_level;
}

//************************************************
//		LocalLevel
//************************************************

class LocalLevel::Internal {
	typedef blassic::result::BlResult BlResult;
	friend class LocalLevel; // Only to avoid a warning in gcc.
public:
	Internal () : nref (1) { }
	void addref () { ++nref; }
	void delref ()
	{
		if (--nref == 0)
			delete this;
	}
	void addlocalvar (const std::string & name);
	void freelocalvars ();
private:
	~Internal () { }
	size_t nref;
	typedef std::map <std::string, BlResult> maploc_t;
	maploc_t maploc;
};

void LocalLevel::Internal::addlocalvar (const std::string & name)
{
	TRACEFUNC (tr, "LocalLevel::Internal::addlocalvar");

	// Changing this to make it exception safe.
	#if 0
	if (maploc.find (name) != maploc.end () )
		return;
	BlResult result;

	switch (typeofvar (name) )
	{
	case VarNumber:
		{
			BlNumber n= 0;
			std::swap (n, * addrvarnumber (name) );
			result= n;
			TRMESSAGE (tr, std::string ("Saving ") + name +
				" " + to_string (n) );
		}
		break;
	case VarInteger:
		{
			BlInteger n= 0;
			std::swap (n, * addrvarinteger (name) );
			result= n;
			TRMESSAGE (tr, std::string ("Saving ") + name +
				" " + to_string (n) );
		}
		break;
	case VarString:
		{
			std::string str;
			swap (str, * addrvarstring (name) );
			result= str;
			TRMESSAGE (tr, std::string ("Saving ") + name +
				" " + str);
		}
		break;
	default:
		if (showdebuginfo () )
			cerr << "Type of local variable '" <<
				name << "'unknown" << endl;
		throw ErrBlassicInternal;
	}
	maploc [name]= result;
	#else

	BlResult result;
	VarType vtype= typeofvar (name);
	VarPointer vp;
	switch (vtype)
	{
	case VarInteger:
		vp.pinteger= addrvarinteger (name);
		result= * vp.pinteger;
		TRMESSAGE (tr, std::string ("Saving ") + name +
			" " + to_string (result.integer () ) );
		break;
	case VarNumber:
		vp.pnumber= addrvarnumber (name);
		result= * vp.pnumber;
		TRMESSAGE (tr, std::string ("Saving ") + name +
			" " + to_string (result.number () ) );
		break;
	case VarString:
		vp.pstring= addrvarstring (name);
		result= * vp.pstring;
		TRMESSAGE (tr, std::string ("Saving ") + name +
			" " + result.str () );
		break;
	default:
		if (showdebuginfo () )
			cerr << "Type of local variable '" <<
				name << "'unknown" << endl;
		throw ErrBlassicInternal;
	}
	std::pair <maploc_t::iterator, bool> r= maploc.insert
		(maploc_t::value_type (name, result) );

	// If already declared as local, does nothing.
	if (! r.second)
	{
		TRMESSAGE (tr, name + " was already saved");
		return;
	}

	switch (vtype)
	{
	case VarInteger:
		* vp.pinteger= BlInteger ();
		break;
	case VarNumber:
		* vp.pnumber= BlNumber ();
		break;
	case VarString:
		vp.pstring->erase ();
		break;
	default:
		throw ErrBlassicInternal;
	}

	#endif
}

namespace {

using blassic::result::BlResult;

void freelocalvar (const std::pair <std::string, BlResult> & p)
{
	TRACEFUNC (tr, "freelocalvar");

	switch (p.second.type () )
	{
	case VarNumber:
		assignvarnumber (p.first, p.second.number () );
		TRMESSAGE (tr, std::string ("Restoring ") + p.first +
			" to " + to_string (p.second.number () ) );
		break;
	case VarInteger:
		assignvarinteger (p.first, p.second.integer () );
		TRMESSAGE (tr, std::string ("Restoring ") + p.first +
			" to " + to_string (p.second.integer () ) );
		break;
	case VarString:
		assignvarstring (p.first, p.second.str () );
		TRMESSAGE (tr, std::string ("Restoring ") + p.first +
			" to " + p.second.str () );
		break;
	default:
		if (showdebuginfo () )
			cerr << "Freeing local variable of unknown type" <<
				endl;
		throw ErrBlassicInternal;
	}
}

} // namespace

void LocalLevel::Internal::freelocalvars ()
{
	for_each (maploc.begin (), maploc.end (), freelocalvar);
}

LocalLevel::LocalLevel () :
	pi (new Internal)
{
}

LocalLevel::LocalLevel (const LocalLevel & ll) :
	//Element (ge),
	pi (ll.pi)
{
	pi->addref ();
}

LocalLevel::~LocalLevel ()
{
	pi->delref ();
}

LocalLevel & LocalLevel::operator = (const LocalLevel & ll)
{
	ll.pi->addref ();
	pi->delref ();
	pi= ll.pi;
	return * this;
}

void LocalLevel::freelocalvars ()
{
	pi->freelocalvars ();
}

void LocalLevel::addlocalvar (const std::string & name)
{
	pi->addlocalvar (name);
}

//************************************************
//		class GosubStack
//************************************************

GosubStack::GosubStack (GlobalRunner & globalrunner) :
	globalrunner (globalrunner)
{
}

GosubStack::~GosubStack ()
{
	// This is to ensure that on exiting from a multiline
	// DEF FN the local variables are correctly restored,
	// even on exceptions.

	TRACEFUNC (tr, "GosubStack::~GosubStack");

	const bool inexcept= std::uncaught_exception ();

	try
	{
		while (! st.empty () )
		{
			GosubElement & ge= st.top ();
			ge.freelocalvars ();
			if (ge.ispolled () )
				globalrunner.clearpolled ();
			st.pop ();
		}
	}
	catch (...)
	{
		// Bad thing. Perhaps restoring a local variable
		// has run out of memory?
		// Try to inform the user without aborting.
		if (! inexcept)
			throw;
		else
		{
			cerr << "**ERROR** "
				"Failed to restore local variables" <<
				endl;
		}
	}
}

void GosubStack::erase ()
{
	// Unnecessary restore local vars here, when is called
	// vars will be cleared after.
	while (! st.empty () )
		st.pop ();
}

//************************************************
//	Auxiliar functions of Runner
//************************************************

namespace {

const char * statusname (Runner::RunnerStatus status)
{
	const char * str= "unknown";
	switch (status)
	{
	case Runner::Ended:
		str= "Ended"; break;
	case Runner::FnEnded:
		str= "FnEnded"; break;
	case Runner::ReadyToRun:
		str= "ReadyToRun"; break;
	case Runner::Running:
		str= "Runnig"; break;
	case Runner::Stopped:
		str= "Stopped"; break;
	case Runner::Jump:
		str= "Jump"; break;
	case Runner::Goto:
		str= "Goto"; break;
	//case Runner::InitingCommand:
	//	str= "InitingCommand"; break;
	//case Runner::Command:
	//	str= "Command"; break;
	case Runner::JumpResumeNext:
		str= "JumpResumeNext"; break;
	}
	return str;
}

} // namespace

std::ostream & operator << (std::ostream & os, Runner::RunnerStatus status)
{
	os << "RunnerStatus is: " << statusname (status) << std::endl;
	return os;
}

//************************************************
//		class Runner
//************************************************

Runner::Runner (GlobalRunner & gr) :
	globalrunner (gr),
	program (gr.getprogram () ),
	status (Ended),
	fInElse (false),
	fInWend (false),
	gosubstack (globalrunner),
	posbreak (LineEndProgram),
	blnAuto (LineEndProgram)
{
	TRACEFUNC (tr, "Runner::Runner");

	clearerror ();
}

Runner::Runner (const Runner & runner) :
	globalrunner (runner.globalrunner),
	program (runner.program),
	status (Ended),
	fInElse (false),
	fInWend (false),
	gosubstack (globalrunner),
	posbreak (LineEndProgram),
	blnAuto (LineEndProgram)
{
	TRACEFUNC (tr, "Runner::Runner (copy)");
}

Runner::~Runner ()
{
}

void Runner::clear ()
{
	close_all ();

	// Clear loops stacks.
	while (! forstack.empty () )
		for_pop ();
	gosubstack.erase ();
	while (! repeatstack.empty () )
		repeatstack.pop ();
	while (! whilestack.empty () )
		whilestack.pop ();

	// Do RESTORE
	clearreadline ();

	// Errors
	clearerror ();
	globalrunner.clearerrorgoto ();
}

//	Runner ************ errors ***************

BlErrNo Runner::geterr () const
{
	return berrLast.geterr ();
}

ProgramPos Runner::geterrpos () const
{
	return berrLast.getpos ();
}

BlLineNumber Runner::geterrline () const
{
	return berrLast.getpos ().getnum ();
}

void Runner::clearerror ()
{
	//berrLast= BlError (0, LineEndProgram);
	berrLast.clear ();
}

BlError Runner::geterror () const
{
	return berrLast;
}

void Runner::seterror (const BlError & er)
{
	berrLast= er;
}

void Runner::spectrumwindows ()
{
	// Only can be called in graphics mode, default channel must
	// be a window.
	ASSERT (graphics::ingraphicsmode () );
	ASSERT (getfile0 ().istextwindow () );

	// Adjust main window to the normal print area.
	getfile0 ().reset (1, 32, 1, 22);

	// Set channel 1, if free or already a window, to the window
	// of the INPUT area. Leave untouched if is not a window.
	if (! isfileopen (1) )
	{
		auto_ptr <BlFile> pf (newBlFileWindow (1, 1, 32, 23, 24) );
		setfile (1, pf.get () );
		pf.release ();
	}
	else
	{
		BlFile & f1= getfile (1);
		if (f1.istextwindow () )
			f1.reset (1, 32, 23, 24);
	}
}

void Runner::getline (std::string & line)
{
	clean_input ();
	BlFile & file= getfile0 ();
	file.getline (line);
}

void Runner::run ()
{
	// Una chapuza por ahora.
	std::string str ("RUN");
	CodeLine code;
	code.scan (str);
	runline (code);
}

bool Runner::next ()
{
	//TRACEFUNC (tr, "Runner::next ()");

	if (forstack.empty () )
		throw ErrNextWithoutFor;
	ForElement * pfe= forstack.top ();
	if (pfe->next () )
	{
		//TRMESSAGE (tr, "NEXT -> " +
		//	to_string (pfe->getpos ().getnum () ) + ":" +
		//	to_string (pfe->getpos ().getchunk () ) );
		jump_to (pfe->getpos () );
		return true;
	}
	else
	{
		//TRMESSAGE (tr, "End FOR");
		forstack.pop ();
		return false;
	}
}

bool Runner::next (const std::string & varname)
{
	if (forstack.empty () )
		throw ErrNextWithoutFor;
	ForElement * pfe= forstack.top ();
	if (! pfe->isvar (varname) )
	{
		if (sysvar::get (sysvar::TypeOfNextCheck) == 0)
		{
			if (showdebuginfo () )
				cerr << "Processing NEXT " <<
					varname <<
					" but current FOR is " <<
					pfe->var () <<
					" and mode is strict" <<
					endl;
			throw ErrNextWithoutFor;
		}
		else
		{
			// In ZX style NEXT can be omitted.
			do
			{
				forstack.pop ();
				if (forstack.empty () )
					throw ErrNextWithoutFor;
				pfe= forstack.top ();
			} while (! pfe->isvar (varname) );
		}
	}
	if (pfe->next () )
	{
		jump_to (pfe->getpos () );
		return true;
	}
	else
	{
		forstack.pop ();
		return false;
	}
}

void Runner::tron (bool fLine, BlChannel blc)
{
	globalrunner.tron (fLine, blc);
}

void Runner::troff ()
{
	globalrunner.troff ();
}

namespace {

inline bool goto_relaxed ()
{
	return sysvar::hasFlags1 (sysvar::RelaxedGoto);
}

} // namespace

inline Runner::StatusRun Runner::checkstatusEnded
	(CodeLine &, const CodeLine &)
{
	return StopNow;
}

inline Runner::StatusRun Runner::checkstatusFnEnded
	(CodeLine &, const CodeLine &)
{
	TRACEFUNC (tr, "Runner::checkstatusFnEnded");

	if (fn_level () == 0)
		throw ErrUnexpectedFnEnd;
	gosub_check_fn ();

	return StopNow;
}

inline Runner::StatusRun Runner::checkstatusReadyToRun
	(CodeLine & codeline, const CodeLine &)
{
	BlLineNumber gline= posgoto.getnum ();
	if (gline == LineBeginProgram)
		codeline= program.getfirstline ();

	else
	{
		CodeLine aux;
		program.getline (gline, aux);
		if (aux.number () != gline)
		{
			if (showdebuginfo () )
				cerr << "Line " << gline <<
					" not exist" << endl;
			throw ErrLineNotExist;
		}
		codeline= aux;
	}
	//if (codeline.number () == 0)
	if (codeline.number () == LineEndProgram)
	{
		//setstatus (Ended);
		//throw blassic::ProgramPassedLastLine ();

		setstatus (Ended);
		return StopNow;
	}
	else
	{
		setstatus (Running);
		return KeepRunning;
	}
}

inline Runner::StatusRun Runner::checkstatusRunning
	(CodeLine & codeline, const CodeLine &)
{
	if (codeline.number () == LineDirectCommand)
	{
		setstatus (Ended);
		return StopNow;
	}
	else
	{
		program.getnextline (codeline);
		if (codeline.number () == LineEndProgram)
		{
			#if 1

			if (fn_level () > 0)
			{
				//throw blassic::ProgramPassedLastLine ();
				throw ErrNoFnEnd;
			}
			setstatus (Ended);
			return StopNow;

			#else
				throw blassic::ProgramPassedLastLine ();
			#endif
		}
		else
			return KeepRunning;
	}
}

inline Runner::StatusRun Runner::checkstatusStopped
	(CodeLine &, const CodeLine &)
{
	return StopNow;
}

inline Runner::StatusRun Runner::checkstatusJump
	(CodeLine & codeline, const CodeLine & codeline0)
{
	TRACEFUNC (tr, "Runner::checkstatusJump");

	const BlLineNumber gotoline= posgoto.getnum ();
	if (gotoline != LineDirectCommand)
	{
		if (codeline.number () != gotoline)
		{
			program.getline (posgoto, codeline);
			if (codeline.number () == LineEndProgram)
			{
				//throw blassic::ProgramPassedLastLine ();
				if (fn_level () > 0)
					throw ErrNoFnEnd;
				setstatus (Ended);
				return StopNow;
			}
		}
		else
		{
			TRMESSAGE (tr, "Same line");
			codeline.gotochunk (posgoto.getchunk () );
		}
	}
	else
	{
		if (codeline.number () != LineDirectCommand)
			codeline= codeline0;
		codeline.gotochunk (posgoto.getchunk () );
	}
	setstatus (Running);
	return KeepRunning;
}

inline Runner::StatusRun Runner::checkstatusGoto
	(CodeLine & codeline, const CodeLine &)
{
	const BlLineNumber gotoline= posgoto.getnum ();
	if (codeline.number () != gotoline)
	{
		CodeLine aux;
		program.getline (gotoline, aux);
		BlLineNumber l= aux.number ();
		//if (l == 0 ||
		//	(l != gotoline && ! goto_relaxed () ) )
		if (l != gotoline && ! goto_relaxed () )
		{
			if (showdebuginfo () )
				cerr << "Line " << gotoline <<
					" not exist" << endl;
			throw ErrLineNotExist;
		}
		codeline= aux;
	}
	else
		codeline.gotochunk (posgoto.getchunk () );
	setstatus (Running);
	return KeepRunning;
}

#if 0
inline Runner::StatusRun Runner::checkstatusInitingCommand
	(CodeLine &, const CodeLine &)
{
	setstatus (Command);
	return KeepRunning;
}

inline Runner::StatusRun Runner::checkstatusCommand
	(CodeLine &, const CodeLine &)
{
	return StopNow;
	//return KeepRunning;
}
#endif

inline Runner::StatusRun Runner::checkstatusJumpResumeNext
	(CodeLine & codeline, const CodeLine & codeline0)
{
	const BlLineNumber gotoline= posgoto.getnum ();
	if (gotoline != LineDirectCommand)
	{
		if (codeline.number () != gotoline)
		{
			program.getline (posgoto, codeline);
			if (codeline.number () == LineEndProgram)
			{
				//throw blassic::ProgramPassedLastLine ();
				if (fn_level () > 0)
					throw ErrNoFnEnd;
				setstatus (Ended);
				return StopNow;
			}
		}
		else
			codeline.gotochunk
				(posgoto.getchunk () );
	}
	else
	{
		if (codeline.number () != LineDirectCommand)
			codeline= codeline0;
		codeline.gotochunk (posgoto.getchunk () );
	}
	
	BlLineNumber n= codeline.number ();
	if (n == gotoline)
	{
		CodeLine::Token tok;
		codeline.gettoken (tok);
		if (tok.code == keyIF)
		{
			if (n == LineDirectCommand)
			{
				setstatus (Ended);
				return StopNow;
			}
			program.getnextline (codeline);
			if (codeline.number () == LineEndProgram)
			{
				//throw blassic::ProgramPassedLastLine ();
				if (fn_level () > 0)
					throw ErrNoFnEnd;
				setstatus (Ended);
				return StopNow;
			}
		}
		else
		{
			while (! tok.isendsentence () )
				codeline.gettoken (tok);
		}
	}
	setstatus (Running);
	return KeepRunning;
}

Runner::checkstatusfunc Runner::checkstatus [Runner::LastStatus + 1]=
{
	&Runner::checkstatusEnded,
	&Runner::checkstatusFnEnded,
	&Runner::checkstatusReadyToRun,
	&Runner::checkstatusRunning,
	&Runner::checkstatusStopped,
	&Runner::checkstatusJump,
	&Runner::checkstatusGoto,
	//&Runner::checkstatusInitingCommand,
	//&Runner::checkstatusCommand,
	&Runner::checkstatusJumpResumeNext,
};

#if 0
inline bool Runner::checkstatus
	(CodeLine & codeline, const CodeLine & codeline0)
{
	switch (status)
	{
	case ReadyToRun:
		{
			BlLineNumber gline= posgoto.getnum ();
			if (gline == LineBeginProgram)
				codeline= program.getfirstline ();

			else
			{
				CodeLine aux;
				program.getline (gline, aux);
				if (aux.number () != gline)
				{
					if (showdebuginfo () )
						cerr << "Line " << gline <<
							" not exist" << endl;
					throw ErrLineNotExist;
				}
				codeline= aux;
			}
			//if (codeline.number () == 0)
			if (codeline.number () == LineEndProgram)
				//setstatus (Ended);
				throw blassic::ProgramPassedLastLine ();
			else
			{
				setstatus (Running);
				return true;
			}
		}
		//break;
	case Jump:
		{
			const BlLineNumber gotoline= posgoto.getnum ();
			if (gotoline != LineDirectCommand)
			{
				if (codeline.number () != gotoline)
				{
					program.getline (posgoto, codeline);
					if (codeline.number () == LineEndProgram)
						throw blassic::ProgramPassedLastLine ();
				}
				else
					codeline.gotochunk
						(posgoto.getchunk () );
			}
			else
			{
				if (codeline.number () != LineDirectCommand)
					codeline= codeline0;
				codeline.gotochunk (posgoto.getchunk () );
			}
		}
		setstatus (Running);
		return true;
	case Goto:
		{
			const BlLineNumber gotoline= posgoto.getnum ();
			if (codeline.number () != gotoline)
			{
				CodeLine aux;
				program.getline (gotoline, aux);
				BlLineNumber l= aux.number ();
				//if (l == 0 ||
				//	(l != gotoline && ! goto_relaxed () ) )
				if (l != gotoline && ! goto_relaxed () )
				{
					if (showdebuginfo () )
						cerr << "Line " << gotoline <<
							" not exist" << endl;
					throw ErrLineNotExist;
				}
				codeline= aux;
			}
			else
				codeline.gotochunk (posgoto.getchunk () );
		}
		setstatus (Running);
		return true;
	case Running:
		//if (codeline.number () != 0)
		//	program.getnextline (codeline);
		//if (codeline.number () == 0)
		//{
		//	status= Ended;
		//	if (fn_level () > 0)
		//		throw blassic::ProgramPassedLastLine ();
		//}
		//else
		//	return true;
		if (codeline.number () == LineDirectCommand)
		{
			setstatus (Ended);
			return false;
		}
		else
		{
			program.getnextline (codeline);
			if (codeline.number () == LineEndProgram)
			#if 0
			{
				setstatus (Ended);
				if (fn_level () > 0)
					throw blassic::ProgramPassedLastLine
						();
				return false;
			}
			#else
				throw blassic::ProgramPassedLastLine ();
			#endif
			else
				return true;
		}
	case InitingCommand:
		setstatus (Command);
		return true;
	default:
		;
	}
	return false;
}

#else

inline bool Runner::is_run_status
	(CodeLine & codeline, const CodeLine & codeline0)
{
	return (this->*(checkstatus [status]) ) (codeline, codeline0)
		== KeepRunning;
}

#endif

void Runner::showfailerrorgoto () const
{
	if (! showdebuginfo () )
		return;
	BlLineNumber source= geterrorgotosource ();

	cerr << "Line " << geterrorgoto () <<
		" specified in ON ERROR GOTO on ";
	if (source == LineDirectCommand)
		cerr << "a direct command";
	else
		cerr << "line " << source;
	cerr <<	" does not exist" <<
		endl;
}

namespace {

bool handle_error (Runner & runner, BlError & berr)
{
	BlLineNumber errorgoto= runner.geterrorgoto ();
	if (errorgoto == LineEndProgram)
	{
		if (runner.fn_level () > 0)
		{
			// The position of the error
			// will be defined by the
			// first fn caller.
			if (showdebuginfo () )
				cerr << "Error " << berr.geterr () <<
					" inside DEF FN on line " <<
					berr.getpos ().getnum () <<
					endl;
		}
		return true;
	}
	else
	{
		CodeLine aux;
		runner.getprogram ().getline (errorgoto, aux);
		if (aux.number () != errorgoto)
		{
			runner.showfailerrorgoto ();
			berr.seterr (ErrLineNotExist);
			return true;
		}
		runner.seterror (berr);
		// Clear berr, or it will be still active
		// next time something is throwed.
		berr.clear ();
		runner.jump_to (errorgoto);
		return false;
	}
}

} // namespace

#if 0
void Runner::runline_catch (const CodeLine & codeline, ProgramPos pos,
	BlError & berr, bool & endloop, bool & dobreak)
{
	TRACEFUNC (tr, "Runner::runline_catch");

	//bool fnend= false;

	try
	{
		throw; // Relaunch pending exception to catch it.
	}
	catch (std::bad_alloc &)
	{
		berr= BlError (ErrOutMemory,
			//runnerline.getposactual () );
			pos);
		endloop= handle_error (* this, berr);
		//if (endloop)
		//	break;
	}
	catch (SocketError & se)
	{
		if (showdebuginfo () )
			cerr << se.what () << endl;
		berr= BlError (ErrSocket,
			//runnerline.getposactual () );
			pos);
		endloop= handle_error (* this, berr);
		//if (endloop)
		//	break;
	}
	catch (BlErrNo e)
	{
		berr= BlError (e,
			//runnerline.getposactual () );
			pos);
		endloop= handle_error (* this, berr);
		//if (endloop)
		//	break;
	}
	catch (BlError & newberr)
	{
		berr= newberr;
		endloop= handle_error (* this, berr);
		//if (endloop)
		//	break;
	}
	catch (BlBreak &)
	{
		fInterrupted= false;
		//ProgramPos actual= runnerline.getposactual ();
		ProgramPos actual= pos;
		switch (getbreakstate () )
		{
		case onbreak::BreakStop:
			if (fn_level () > 0)
				throw;
			endloop= true;
			dobreak= true;
			break;
		case onbreak::BreakCont:
			throw BlError (ErrBlassicInternal, actual);
		case onbreak::BreakGosub:
			gosub_line (getbreakgosub (), actual);
			break;
		}
		//if (endloop)
		//	break;
	}
	#if 0
	catch (blassic::ProgramEnd &)
	{
		if (fn_level () > 0)
		{
			if (showdebuginfo () )
				cerr << "END inside DEF FN in line " <<
					codeline.number () <<
					endl;
			throw;
		}
		close_all ();
		setstatus (Ended);
		//break;
		endloop= true; // New
	}
	catch (blassic::ProgramPassedLastLine &)
	{
		if (fn_level () > 0)
		{
			if (showdebuginfo () )
				cerr << "End of program reached "
					"inside DEF FN" <<
					endl;
			throw;
		}
		//break;
		endloop= true; // New
	}
	catch (blassic::ProgramStop &)
	{
		if (fn_level () > 0)
		{
			if (showdebuginfo () )
				cerr << "STOP inside DEF FN "
					"in line "
					 <<
					codeline.number () <<
					endl;
			throw;
		}
		BlFile & f= getfile0 ();
		f << "**Stopped**";
		if (codeline.number () != LineDirectCommand)
			f << " in " << codeline.number ();
		f.endline ();
		//ProgramPos posbreak (runnerline.getposactual () );
		ProgramPos posbreak (pos);
		posbreak.nextchunk ();
		set_break (posbreak);
		setstatus (Stopped);
		//break;
		endloop= true; // New
	}
	catch (blassic::ProgramFnEnd &)
	{
		#if 0
		TRMESSAGE (tr, "FnEnd");
		if (fn_level () == 0)
		{
			berr= BlError (ErrUnexpectedFnEnd, pos);
			endloop= handle_error (* this, berr);
		}
		try
		{
			gosub_check_fn ();
			setstatus (Ended);
			endloop= true;
		}
		catch (BlErrNo e)
		{
			berr.set (e, pos);
			endloop= handle_error (* this, berr);
		}
		#else

		fnend= true;

		#endif
	}
	if (fnend)
	{
		TRMESSAGE (tr, "FnEnd");
		if (fn_level () == 0)
		{
			berr.set (ErrUnexpectedFnEnd, pos);
			endloop= handle_error (* this, berr);
		}
		try
		{
			gosub_check_fn ();
			setstatus (Ended);
			endloop= true;
		}
		catch (BlErrNo e)
		{
			berr.set (e, pos);
			endloop= handle_error (* this, berr);
		}
	}
	#endif
}
#endif

#if 0

void Runner::runline (const CodeLine & codeline0)
{
	TRACEFUNC (tr, "Runner::runline");

	if (codeline0.number () != LineDirectCommand)
	{
		ASSERT (false);
		throw ErrBlassicInternal;
	}

	auto_ptr <RunnerLine> prunnerline
		(newRunnerLine (* this, codeline0) );
	RunnerLine & runnerline= * prunnerline.get ();

	CodeLine & codeline= runnerline.getcodeline ();
	bool endloop= false;
	bool dobreak= false;
	BlError berr;

	setstatus (InitingCommand);

	// Test:
	otra:

	//for (;;)
	//{
		try
		{
			TRMESSAGE  (tr, statusname (status) );
			if (! is_run_status (codeline, codeline0) )
			//if ( (this->*(checkstatus [status]) )
			//		(codeline, codeline0) == StopNow)
			{
				// Test:
				//break;
				goto atloopend;
			}

			//do {
				//TRMESSAGE  (tr, statusname (status) );

				//runnerline.setline (codeline);

				// Do tron if active.
				tronline (codeline);

				//runnerline.execute ();
				prunnerline->execute ();
			//} while (checkstatus (codeline, codeline0) );
		}
		catch (std::bad_alloc &)
		{
			berr.set (ErrOutMemory, runnerline.getposactual () );
			endloop= handle_error (* this, berr);
			//if (endloop)
			//	break;
		}
		catch (SocketError & se)
		{
			if (showdebuginfo () )
				cerr << se.what () << endl;
			berr.set (ErrSocket, runnerline.getposactual () );
			endloop= handle_error (* this, berr);
			//if (endloop)
			//	break;
		}
		catch (BlErrNo e)
		{
			berr.set (e, runnerline.getposactual () );
			endloop= handle_error (* this, berr);
			//if (endloop)
			//	break;
		}
		catch (BlError & newberr)
		{
			berr= newberr;
			endloop= handle_error (* this, berr);
			//if (endloop)
			//	break;
		}
		catch (BlBreak &)
		{
			fInterrupted= false;
			ProgramPos actual= runnerline.getposactual ();
			//ProgramPos actual= pos;
			switch (getbreakstate () )
			{
			case onbreak::BreakStop:
				if (fn_level () > 0)
					throw;
				endloop= true;
				dobreak= true;
				break;
			case onbreak::BreakCont:
				throw BlError (ErrBlassicInternal, actual);
			case onbreak::BreakGosub:
				gosub_line (getbreakgosub (), actual);
				break;
			}
			//if (endloop)
			//	break;
		}
		#if 0
		catch (blassic::ProgramEnd &)
		{
			if (fn_level () > 0)
			{
				if (showdebuginfo () )
					cerr << "END inside DEF FN in line " <<
						codeline.number () <<
						endl;
				throw;
			}
			close_all ();
			setstatus (Ended);
			//break;
			endloop= true; // New
		}
		catch (blassic::ProgramPassedLastLine &)
		{
			if (fn_level () > 0)
			{
				if (showdebuginfo () )
					cerr << "End of program reached "
						"inside DEF FN" <<
						endl;
				throw;
			}
			setstatus (Ended);
			//break;
			endloop= true; // New
		}
		catch (blassic::ProgramStop &)
		{
			if (fn_level () > 0)
			{
				if (showdebuginfo () )
					cerr << "STOP inside DEF FN "
						"in line "
						 <<
						codeline.number () <<
						endl;
				throw;
			}
			BlFile & f= getfile0 ();
			std::ostringstream oss;
			oss << "**Stopped**";
			if (codeline.number () != LineDirectCommand)
				oss << " in " << codeline.number ();
			f << oss.str ();
			f.endline ();
			ProgramPos posbreak (runnerline.getposactual () );
			//ProgramPos posbreak (pos);
			posbreak.nextchunk ();
			set_break (posbreak);
			setstatus (Stopped);
			//break;
			endloop= true; // New
		}
		catch (blassic::ProgramFnEnd &)
		{
			#if 0
			TRMESSAGE (tr, "FnEnd");
			if (fn_level () == 0)
			{
				berr= BlError (ErrUnexpectedFnEnd, pos);
				endloop= handle_error
					(* this, berr);
			}
			try
			{
				gosub_check_fn ();
				setstatus (Ended);
				endloop= true;
			}
			catch (BlErrNo e)
			{
				berr= BlError (e, pos);
				endloop= handle_error
					(* this, berr);
			}
			#else

			if (fn_level () == 0)
			{
				berr= BlError (ErrUnexpectedFnEnd,
					//pos);
					runnerline.getposactual () );
			}
			else
			{
				gosub_check_fn ();
				setstatus (Ended);
			}
			endloop= true;

			#endif
		}
		#endif
		catch (...)
		{
			TRMESSAGE (tr, "catched ...");
			throw;
		}

		#if 0
		catch (...)
		{
			runline_catch (codeline, runnerline.getposactual (),
				berr, endloop, dobreak);
		}
		#endif

		// Test
		#if 0

		if (endloop)
			break;
		#else

			if (! endloop)
				goto otra;
		#endif

		#if 0
		check_again:
		try
		{
			if (! checkstatus (codeline, codeline0) )
				//break;
				endloop= true;
		}
		catch (...)
		{
			runline_catch (codeline, runnerline.getposactual (),
				berr, endloop, dobreak);
			if (endloop)
			{
				//break;
			}
			else
				goto check_again;
		}
		if (endloop)
			break;
		#endif

	// Test:
	//} // for (;;)
	atloopend:

	if (endloop)
	{
		if (dobreak)
			throw BlBreakInPos (runnerline.getposactual () );
		else if (berr.geterr () != 0)
		{
			seterror (berr);
			throw berr;
		}
	}
}

#else

void Runner::runline (const CodeLine & codeline0)
{
	TRACEFUNC (tr, "Runner::runline");

	if (codeline0.number () != LineDirectCommand)
	{
		ASSERT (false);
		throw ErrBlassicInternal;
	}

	#if 1

	auto_ptr <RunnerLine> prunnerline
		(newRunnerLine (* this, codeline0) );
	RunnerLine & runnerline= * prunnerline.get ();

	#else

	RunnerLineImpl runnerline (* this, codeline0);

	#endif

	CodeLine & codeline= runnerline.getcodeline ();
	bool endloop= false;
	bool dobreak= false;
	BlError berr;

	//setstatus (InitingCommand);
	setstatus (Running);

	// {
	// TRACEFUNC (traux, "runline auxiliar block");

	while (! endloop)
	{
		try
		{
			do {
				//TRMESSAGE  (tr, statusname (status) );

				// Do tron if active.
				tronline (codeline);

				//prunnerline->execute ();
				runnerline.execute ();

				TRMESSAGE (tr, "Line finished");
			} while (is_run_status (codeline, codeline0) );
			endloop= true;
		}
		catch (std::bad_alloc &)
		{
			berr.set (ErrOutMemory, runnerline.getposactual () );
			endloop= handle_error (* this, berr);
		}
		catch (SocketError & se)
		{
			if (showdebuginfo () )
				cerr << se.what () << endl;
			berr.set (ErrSocket, runnerline.getposactual () );
			endloop= handle_error (* this, berr);
		}
		catch (BlErrNo e)
		{
			berr.set (e, runnerline.getposactual () );
			endloop= handle_error (* this, berr);
		}
		catch (BlError & newberr)
		{
			berr= newberr;
			endloop= handle_error (* this, berr);
		}
		catch (BlBreak &)
		{
			fInterrupted= false;
			ProgramPos actual= runnerline.getposactual ();
			switch (getbreakstate () )
			{
			case onbreak::BreakStop:
				if (fn_level () > 0)
					throw;
				endloop= true;
				dobreak= true;
				break;
			case onbreak::BreakCont:
				throw BlError (ErrBlassicInternal, actual);
			case onbreak::BreakGosub:
				gosub_line (getbreakgosub (), actual);
				break;
			}
		}
		//catch (Exit & e)
		//{
		//	//TRMESSAGE (traux, "Exitting");
		//	throw (Exit (e.code () ) );
		//}

	} // while

	// } // Auxiliar block

	//if (endloop)
	//{
		if (dobreak)
			throw BlBreakInPos (runnerline.getposactual () );
		else if (berr.geterr () != 0)
		{
			seterror (berr);
			throw berr;
		}
	//}
}

#endif

bool Runner::processline (const std::string & line)
{
	TRACEFUNC (tr, "Runner::processline");

	CodeLine codeline;
	codeline.scan (line);
	BlLineNumber nline= codeline.number ();
	//if (nline == 0)
	if (nline == LineDirectCommand)
	{
		if (blnAuto != LineEndProgram)
		{
			// This probably must be changed.
			if (codeline.empty () )
				program.deletelines (blnAuto, blnAuto);
			else
			{
				codeline.setnumber (blnAuto);
				program.insert (codeline);
			}
			if (blnAuto > BlMaxLineNumber - blnAutoInc)
			{
				blnAuto= LineEndProgram;
				throw BlError (ErrLineExhausted,
					LineDirectCommand);
			}
			else
				blnAuto+= blnAutoInc;
		}
		else
		{
			if (codeline.empty () )
				return false;
			runline (codeline);
			return true;
		}
	}
	else
	{
		if (nline > BlMaxLineNumber)
			throw BlError (ErrLineExhausted);
		if (codeline.empty () )
			program.deletelines (nline, nline);
		else
			program.insert (codeline);
		if (blnAuto != LineEndProgram)
		{
			blnAuto= codeline.number ();
			if (blnAuto > BlMaxLineNumber - blnAutoInc)
			{
				blnAuto= LineEndProgram;
				throw BlError (ErrLineExhausted,
					LineDirectCommand);
			}
			else
				blnAuto+= blnAutoInc;
		}
	}
	return false;
}

namespace {

void welcome (BlFile & f)
{
	f.endline ();
	f << "Blassic " << version::Major << '.' <<
		version::Minor << '.' <<
		version::Release;
	f.endline ();
	f << "(C) 2001-2009 Julian Albo";
	f.endline ();
	f.endline ();
}

} // namespace

bool Runner::editline (BlLineNumber bln, std::string & str)
{
	TRACEFUNC (tr, "editline - line number->string");

	std::string buffer;
	{
		BlFileOutString bfos;
		program.list (bln, bln, bfos);
		buffer= bfos.str ();
		if (buffer.empty () )
		{
			bfos << bln << ' ';
			bfos.endline ();
			buffer= bfos.str ();
		}
	}
	buffer.erase (buffer.size () - 1);
	TRMESSAGE  (tr, std::string (1, '\'') + buffer + '\'');

	static const std::string number ("01234567890");
	size_t inipos= buffer.find_first_of (number);
	ASSERT (inipos != std::string::npos);
	inipos= buffer.find_first_not_of (number, inipos);
	ASSERT (inipos != std::string::npos);
	++inipos;

	BlFile & bf= getfile0 ();
	bool r;
	if ( (r= blassic::edit::editline (bf, buffer, inipos) ) == true)
		str= buffer;
	return r;
}

void Runner::interactive ()
{
	TRACEFUNC (tr, "Runner::interactive");

	// Now the title is stablished in blassic.cpp
	//set_title ("blassic");

	welcome (getfile0 () );

	bool showprompt= true;
	for (;;)
	{
		std::string line;
		if (blnAuto != LineEndProgram)
		{
			if (! editline (blnAuto, line) )
				fInterrupted= true;
		}
		else
		{
			if (showprompt)
			{
				BlFile & f= getfile0 ();
				f << strPrompt;
				f.endline ();
			}
			//cout << "] " << flush;

			getfile0 ().getline (line, true);
		}

		if (fInterrupted)
		{
			fInterrupted= false;
			BlFile & bf= getfile0 ();
			bf << strbreak;
			bf.endline ();
			std::cin.clear ();
			blnAuto= LineEndProgram;
			continue;
		}
		if (! std::cin)
			break;

		#ifndef _Windows

		if (! line.empty () && line [line.size () - 1] == '\r')
			line= line.substr (0, line.size () - 1);

		#endif

		try
		{
			showprompt= processline (line);
		}
		catch (BlErrNo ben)
		{
			BlFile & f= getfile0 ();
			f << ErrStr (ben);
			f.endline ();
			setstatus (Stopped);
		}
		catch (BlError & be)
		{
			BlFile & f= getfile0 ();
			f << to_string (be);
			f.endline ();
			setstatus (Stopped);
		}
		catch (BlBreakInPos & bbip)
		{
			BlFile & f= getfile0 ();
			f << to_string (bbip);
			f.endline ();
			set_break (bbip.getpos () );
			setstatus (Stopped);
		}
	} // for (;;)
}

void Runner::clean_input ()
{
	if (graphics::ingraphicsmode () )
		graphics::clean_input ();
	else
		cursor::clean_input ();
}

void Runner::ring ()
{
	if (graphics::ingraphicsmode () )
		graphics::ring ();
	else
		cursor::ring ();
}

void Runner::set_title (const std::string & str)
{
	if (graphics::ingraphicsmode () )
		graphics::set_title (str);
	else
		cursor::set_title (str);
}

// End of runner.cpp
