#ifndef INCLUDE_BLASSIC_RUNNER_H
#define INCLUDE_BLASSIC_RUNNER_H

// runner.h
// Revision 6-feb-2005

//#include "program.h"

class Program;

#include "blassic.h"

#include "error.h"
#include "file.h"
#include "codeline.h"
#include "var.h"
#include "element.h"

//#include "runnerline.h"

#include <iostream>
#include <stack>
#include <vector>
#include <map>


// ********************* GlobalRunner **********************


namespace blassic {


namespace onbreak {

enum BreakState { BreakStop, BreakCont, BreakGosub };

} // namespace onbreak


} // namespace blassic


enum TrigonometricMode { TrigonometricRad, TrigonometricDeg };

// We encapsulate the random generator in a class to have all code
// related to it in a place, and to be able to change it easily.

// random in some systems may not use the RAND_MAX value,
// then we use it only in linux.

class RandomGenerator {
public:
	BlNumber operator () ()
	{
		#if defined __linux__
		return BlNumber (random () ) / (RAND_MAX + 1.0);
		#else
		return BlNumber (rand () ) / (RAND_MAX + 1.0);
		#endif
	}
	void seed (unsigned int value)
	{
		#if defined __linux__
		srandom (value);
		#else
		srand (value);
		#endif
	}
};

class GlobalRunner {
public:
	GlobalRunner (Program & prog);
	~GlobalRunner ();

	// Program
private:
	Program & program;
public:
	Program & getprogram () { return program; }

	// TRON stuff
private:
	bool fTron, fTronLine;
	BlChannel blcTron;
	void do_tronline (const CodeLine & line);
public:
	void tron (bool fLine, BlChannel blc);
	void troff ();
	void tronline (const CodeLine & codeline)
	{
		if (fTron)
			do_tronline (codeline);
	}

	// GlobalRunner Channel stuff
public:
	typedef std::map <BlChannel, blassic::file::BlFile *> ChanFile;
	typedef std::map <BlChannel, BlLineNumber> ChanPolled;
private:
	ChanFile chanfile;
	ChanPolled chanpolled;
	bool alreadypolled;
	bool clearingpolled;
public:
	bool assign_channel_var
		(const std::string & var, const Dimension & dim,
			const std::string & value,
			blassic::file::BlFile::Align align);
	bool assign_mid_channel_var
		(const std::string & var, const Dimension & dim,
			const std::string & value,
			size_t inipos, std::string::size_type len);
	bool isfileopen (BlChannel channel) const
	{ return chanfile.find (channel) != chanfile.end (); }
	BlChannel freefile () const;
	blassic::file::BlFile & getfile (BlChannel channel) const;
	void setfile (BlChannel channel, blassic::file::BlFile * npfile);
	void resetfile0 ();
	void resetfileprinter ();
	void close_all ();
	void destroy_windows ();
	void closechannel (BlChannel channel);
	void windowswap (BlChannel ch1, BlChannel ch2);

	void pollchannel (BlChannel ch, BlLineNumber line);
	bool channelspolled ();
	BlLineNumber getpollnumber ();
	void setpolled ();
	void clearpolled ();

	// GlobalRunner DATA / READ / RESTORE stuff
private:
	BlLineNumber datanumline;
	BlChunk datachunk;
	unsigned short dataelem;
public:
	BlLineNumber & getdatanumline () { return datanumline; }
	BlChunk & getdatachunk () { return datachunk; }
	unsigned short & getdataelem () { return dataelem; }
	void setreadline (BlLineNumber bln)
	{
		datanumline= bln;
		datachunk= 0;
		dataelem= 0;
	}
	void clearreadline ()
	{
		datanumline= LineBeginProgram;
		datachunk= 0;
		dataelem= 0;
	}

	// GlobalRunner ON ERROR GOTO stuff.
private:
	BlLineNumber blnErrorGoto;
	BlLineNumber blnErrorGotoSource;
public:
	void clearerrorgoto ();
	void seterrorgoto (BlLineNumber line, BlLineNumber source);
	BlLineNumber geterrorgoto () const
	{ return blnErrorGoto; }
	BlLineNumber geterrorgotosource () const
	{ return blnErrorGotoSource; }

	// GlobalRunner ON BREAK
private:
	blassic::onbreak::BreakState breakstate;
	BlLineNumber breakgosubline;
public:
	void setbreakstate (blassic::onbreak::BreakState newstate)
	{
		breakstate= newstate;
	}
	blassic::onbreak::BreakState getbreakstate () const
	{ return breakstate; }
	void setbreakgosub (BlLineNumber bln)
	{
		breakstate= blassic::onbreak::BreakGosub;
		breakgosubline= bln;
	}
	BlLineNumber getbreakgosub () { return breakgosubline; }

	// Control of depth of fn calls.
private:
	size_t fn_current_level;
public:
	size_t fn_level () const { return fn_current_level; }
	void inc_fn_level ();
	void dec_fn_level ();

	// Trigonometric mode.
private:
	TrigonometricMode trigmode;
public:
	TrigonometricMode trigonometric_mode () const { return trigmode; }
	void trigonometric_default () { trigmode= TrigonometricRad; }
	void trigonometric_mode (TrigonometricMode trigmode_n)
	{ trigmode= trigmode_n; }

	// Random number generator.
private:
	RandomGenerator randgen;
public:
	BlNumber getrandom () { return randgen (); }
	void seedrandom (unsigned int value) { randgen.seed (value); }
};


// ********************* Runner **********************


class GosubStack {
private:
	GlobalRunner & globalrunner;
	typedef std::stack <GosubElement> st_t;
	st_t st;
public:
	typedef st_t::size_type size_type;
public:
	GosubStack (GlobalRunner & globalrunner);	
	~GosubStack ();
	bool empty () const
	{
		return st.empty ();
	}
	size_type size () const
	{
		return st.size ();
	}
	void check_fn () const
	{
		if (st.empty () )
			throw ErrUnexpectedFnEnd;
		if (st.top ().isgosub () )
			throw ErrGosubWithoutReturn;
	}
	void erase ();
	void push (ProgramPos pos, bool is_polled)
	{
		st.push (GosubElement (pos, is_polled) );
		if (is_polled)
			globalrunner.setpolled ();
	}
	void push (LocalLevel & ll)
	{
		st.push (GosubElement (ll) );
	}
	void pop (ProgramPos & ppos)
	{
		if (st.empty () )
			throw ErrReturnWithoutGosub;
		GosubElement & go= st.top ();
		if (! go.isgosub () )
			throw ErrReturnWithoutGosub;
		ppos= go.getpos ();
		go.freelocalvars ();
		if (go.ispolled () )
			globalrunner.clearpolled ();
		st.pop ();
	}
	void popfn ()
	{
		if (st.empty () )
			throw ErrReturnWithoutGosub;
		GosubElement & go= st.top ();
		if (go.isgosub () )
			throw ErrGosubWithoutReturn;
		go.freelocalvars ();
		st.pop ();
	}
	void addlocalvar (const std::string & name)
	{
		if (st.empty () )
			throw ErrMisplacedLocal;
		GosubElement go= st.top ();
		go.addlocalvar (name);
	}
};

class Runner {
public:
	enum RunnerStatus {
		Ended,
		FnEnded,
		ReadyToRun,
		Running,
		Stopped,
		Jump,
		Goto,
		//InitingCommand,
		//Command,
		JumpResumeNext,
		LastStatus= JumpResumeNext,
	};
private:
	enum StatusRun { KeepRunning, StopNow };

	typedef StatusRun (Runner::* checkstatusfunc)
		(CodeLine &, const CodeLine &);
	static checkstatusfunc checkstatus [LastStatus + 1];

	StatusRun checkstatusEnded
		(CodeLine & line, const CodeLine & line0);
	StatusRun checkstatusFnEnded
		(CodeLine & line, const CodeLine & line0);
	StatusRun checkstatusReadyToRun
		(CodeLine & line, const CodeLine & line0);
	StatusRun checkstatusRunning
		(CodeLine & line, const CodeLine & line0);
	StatusRun checkstatusStopped
		(CodeLine & line, const CodeLine & line0);
	StatusRun checkstatusJump
		(CodeLine & line, const CodeLine & line0);
	StatusRun checkstatusGoto
		(CodeLine & line, const CodeLine & line0);
	//StatusRun checkstatusInitingCommand
	//	(CodeLine & line, const CodeLine & line0);
	//StatusRun checkstatusCommand
	//	(CodeLine & line, const CodeLine & line0);
	StatusRun checkstatusJumpResumeNext
		(CodeLine & line, const CodeLine & line0);
	bool is_run_status (CodeLine & line, const CodeLine & line0);


	GlobalRunner & globalrunner;
	Program & program;
	RunnerStatus status;
	bool fInElse;
	bool fInWend;
public:
	//Runner (Program & prog);
	Runner (GlobalRunner & gr);
	Runner (const Runner & runner);
	~Runner ();

	Program & getprogram () { return program; }
	void clear ();
	void getline (std::string & line);
	void runline_catch (const CodeLine & codeline, ProgramPos pos,
		BlError & berr, bool & endloop, bool & dobreak);
	void runline (const CodeLine & codeline0);
	void run ();
	bool editline (BlLineNumber bln, std::string & str);
	void interactive ();

	// Runner *********** Errors **************
private:
	BlCode codprev;
	BlError berrLast;
public:
	BlErrNo geterr () const;
	ProgramPos geterrpos () const;
	BlLineNumber geterrline () const;
	void clearerror ();
	BlError geterror () const;
	void seterror (const BlError & er);

	// Runner *********** Flow control **************
private:
	ProgramPos posgoto;
public:
	void setstatus (RunnerStatus stat) { status= stat; }
	void run_to (BlLineNumber line)
	{
		posgoto= line;
		status= ReadyToRun;
	}
	void run_to (ProgramPos pos)
	{
		posgoto= pos;
		status= ReadyToRun;
	}
	void jump_to (BlLineNumber line)
	{
		posgoto= line;
		status= Jump;
	}
	void jump_to (ProgramPos pos)
	{
		posgoto= pos;
		status= Jump;
	}
	void goto_to (BlLineNumber line)
	{
		posgoto= line;
		status= Goto;
	}
	void goto_to (ProgramPos pos)
	{
		posgoto= pos;
		status= Goto;
	}
	void resume_next (ProgramPos pos)
	{
		posgoto= pos;
		status= JumpResumeNext;
	}

	// Runner *********** FOR / NEXT **************
private:
	std::stack <ForElement *, std::vector <ForElement *> > forstack;
public:
	void push_for (ForElement * pfe)
	{
		forstack.push (pfe);
	}
	//bool for_empty () const { return forstack.empty (); }
	ForElement & for_top ()
	{
		if (forstack.empty () )
			throw ErrNextWithoutFor;
		return * forstack.top ();
	}
	void for_pop () { delete forstack.top (); forstack.pop (); }
	// New version, supposed faster.
	bool next ();
	bool next (const std::string & varname);

	// Runner *********** GOSUB / RETURN / FN **************
private:
	GosubStack gosubstack;
public:
	bool gosub_empty () const
	{ return gosubstack.empty (); }
	size_t gosub_size () const
	{ return gosubstack.size (); }
	void gosub_check_fn () const
	{ gosubstack.check_fn (); }
	void gosub_pop (ProgramPos & pos)
	{ gosubstack.pop (pos); }
	void fn_pop () { gosubstack.popfn (); }
	void gosub_addlocalvar (const std::string & str)
	{
		gosubstack.addlocalvar (str);
	}
	void gosub_push (LocalLevel & ll)
	{
		gosubstack.push (ll);
	}
	void gosub_line (BlLineNumber dest, ProgramPos posgosub,
		bool is_polled= false)
	{
		gosubstack.push (posgosub, is_polled);
		goto_to (dest);
	}
	size_t fn_level () const { return globalrunner.fn_level (); }
	void inc_fn_level ()
	{ globalrunner.inc_fn_level (); }
	void dec_fn_level ()
	{ globalrunner.dec_fn_level (); }

	// Runner *********** REPEAT / UNTIL **************
private:
	std::stack <RepeatElement> repeatstack;
public:
	bool repeat_empty () const { return repeatstack.empty (); }
	void repeat_pop () { repeatstack.pop (); }
	RepeatElement & repeat_top () { return repeatstack.top (); }
	void repeat_push (const RepeatElement & re) { repeatstack.push (re); }

	// Runner *********** WHILE / WEND **************
private:
	std::stack <WhileElement> whilestack;
public:
	bool in_wend () { return fInWend; }
	void in_wend (bool f) { fInWend= f; }
	bool while_empty () { return whilestack.empty (); }
	void while_pop () { whilestack.pop (); }
	WhileElement & while_top () { return whilestack.top (); }
	void while_push (const WhileElement & we) { whilestack.push (we); }

	// Runner *********** TRON **************

	void tron (bool fLine, BlChannel blc);
	void troff ();
	void tronline (const CodeLine & line)
	{ globalrunner.tronline (line); }

	// Runner *********** BREAK / CONT **************
private:
	ProgramPos posbreak;
public:
	void jump_break ()
	{
		//if (! posbreak)
		if (posbreak.getnum ()  == LineEndProgram ||
				posbreak.getnum () == LineDirectCommand)
			throw ErrNoContinue;
		posgoto= posbreak;
		//posbreak= 0;
		posbreak= LineEndProgram;
		status= Jump;
	}
	void set_break (ProgramPos pos) { posbreak= pos; }

	#if 0
	void setbreakstate (BreakState newstate)
	{
		breakstate= newstate;
	}
	BreakState getbreakstate () { return breakstate; }
	void setbreakgosub (BlLineNumber bln)
	{
		breakstate= BreakGosub;
		breakgosubline= bln;
	}
	BlLineNumber getbreakgosub () { return breakgosubline; }
	#else

	void setbreakstate (blassic::onbreak::BreakState newstate)
	{
		globalrunner.setbreakstate (newstate);
	}
	blassic::onbreak::BreakState getbreakstate () const
	{
		return globalrunner.getbreakstate ();
	}
	void setbreakgosub (BlLineNumber bln)
	{
		globalrunner.setbreakgosub (bln);
	}
	BlLineNumber getbreakgosub ()
	{
		return globalrunner.getbreakgosub ();
	}

	#endif


	// Runner *********** DATA / READ / RESTORE **************

	BlLineNumber & getdatanumline ()
	{ return globalrunner.getdatanumline (); }
	BlChunk & getdatachunk ()
	{ return globalrunner.getdatachunk (); }
	unsigned short & getdataelem ()
	{ return globalrunner.getdataelem (); }

	// Runner *********** ON ERROR GOTO **************

	void showfailerrorgoto () const;
	void clearerrorgoto ()
	{ globalrunner.clearerrorgoto (); }
	void seterrorgoto (BlLineNumber line, BlLineNumber source)
	{ globalrunner.seterrorgoto (line, source); }
	BlLineNumber geterrorgoto () const
	{ return globalrunner.geterrorgoto (); }
	BlLineNumber geterrorgotosource () const
	{ return globalrunner.geterrorgotosource (); }

	// Runner *********** Channels **************

	#if 0
	void assign_channel_var
		(const std::string & var, const std::string & value,
			blassic::file::BlFile::Align align)
	{
		for (ChanFile::iterator it= chanfile.begin ();
			it != chanfile.end ();
			++it)
		{
			it->second->assign (var, value, align);
		}
	}
	#else
	bool assign_channel_var
		(const std::string & var, const Dimension & dim,
			const std::string & value,
			blassic::file::BlFile::Align align)
	{ return globalrunner.assign_channel_var (var, dim, value, align); }
	#endif
	bool assign_mid_channel_var
		(const std::string & var, const Dimension & dim,
			const std::string & value,
			size_t inipos, std::string::size_type len)
	{
		return globalrunner.assign_mid_channel_var
			(var, dim, value, inipos, len);
	}
	//bool isfileopen (BlChannel channel) const
	//{ return chanfile.find (channel) != chanfile.end (); }
	bool isfileopen (BlChannel channel) const
	{ return globalrunner.isfileopen (channel); }
	BlChannel freefile () const
	{ return globalrunner.freefile (); }
	blassic::file::BlFile & getfile (BlChannel channel) const
	{ return globalrunner.getfile (channel); }
	blassic::file::BlFile & getfile0 () const
	{ return globalrunner.getfile (DefaultChannel); }
	void setfile (BlChannel channel, blassic::file::BlFile * npfile)
	{ globalrunner.setfile (channel, npfile); }
	void resetfile0 ()
	{ globalrunner.resetfile0 (); }
	void close_all ()
	{ globalrunner.close_all (); }
	void destroy_windows ()
	{ globalrunner.destroy_windows (); }
	void closechannel (BlChannel channel)
	{ globalrunner.closechannel (channel); }
	void windowswap (BlChannel ch1, BlChannel ch2)
	{ globalrunner.windowswap (ch1, ch2); }

	void pollchannel (BlChannel ch, BlLineNumber line)
	{ globalrunner.pollchannel (ch, line); }
	bool channelspolled ()
	{ return globalrunner.channelspolled (); }
	BlLineNumber getpollnumber ()
	{ return globalrunner.getpollnumber (); }

	void spectrumwindows ();

	// Runner *********** DATA / READ / RESTORE **************

	void setreadline (BlLineNumber bln)
	{ globalrunner.setreadline (bln); }
	void clearreadline ()
	{ globalrunner.clearreadline (); }

	// Runner *********** AUTO **************
private:
	BlLineNumber blnAuto, blnAutoInc;
public:
	void setauto (BlLineNumber line, BlLineNumber inc)
	{ blnAuto= line; blnAutoInc= inc; }

	// Runner *********** Trigonometric mode **************
	TrigonometricMode trigonometric_mode ()
	{ return globalrunner.trigonometric_mode (); }
	void trigonometric_default ()
	{ globalrunner.trigonometric_default (); }
	void trigonometric_mode (TrigonometricMode trigmode_n)
	{ globalrunner.trigonometric_mode (trigmode_n); }

private:

	bool processline (const std::string & line);

public:
	// Runner *********** Random number generator **************

	BlNumber getrandom ()
	{ return globalrunner.getrandom (); }
	void seedrandom (unsigned int value)
	{ globalrunner.seedrandom (value); }

	// Runner *********** Auxiliars **************

	void clean_input ();
	void ring ();
	void set_title (const std::string & str);
};

std::ostream & operator << (std::ostream & os, Runner::RunnerStatus status);

#endif

// Fin de runner.h
