// blassic.cpp
// Revision 24-apr-2009

#include "blassic.h"

#include "keyword.h"

#include "program.h"
#include "runner.h"
#include "cursor.h"

#include "graphics.h"
#include "charset.h"
#include "sysvar.h"
#include "trace.h"
#include "util.h"
#include "error.h"

#include <string>

#include <iostream>
#include <cctype>
#include <fstream>
//#include <iomanip>
#include <sstream>
#include <memory>

#include <stdio.h>
#include <string.h>
#include <signal.h>


#ifdef BLASSIC_USE_WINDOWS

#include <windows.h>

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#endif

#if defined __BORLANDC__ && defined _Windows

#include <condefs.h>

#else

#define USEUNIT(a)

#endif

USEUNIT("codeline.cpp");
USEUNIT("error.cpp");
USEUNIT("keyword.cpp");
USEUNIT("program.cpp");
USEUNIT("runner.cpp");
USEUNIT("token.cpp");
USEUNIT("var.cpp");
USEUNIT("dim.cpp");
USEUNIT("file.cpp");
USEUNIT("cursor.cpp");
USEUNIT("graphics.cpp");
USEUNIT("sysvar.cpp");
USEUNIT("version.cpp");
USEUNIT("trace.cpp");
USEUNIT("socket.cpp");
USEUNIT("runnerline.cpp");
USEUNIT("function.cpp");
USEUNIT("key.cpp");
USEUNIT("edit.cpp");
USEUNIT("directory.cpp");
USEUNIT("using.cpp");
USEUNIT("regexp.cpp");
USEUNIT("dynamic.cpp");
USEUNIT("mbf.cpp");
USEUNIT("memory.cpp");
USEUNIT("fileconsole.cpp");
USEUNIT("filepopen.cpp");
USEUNIT("fileprinter.cpp");
USEUNIT("filewindow.cpp");
USEUNIT("runnerline_instructions.cpp");
USEUNIT("showerror.cpp");
USEUNIT("runnerline_impl.cpp");
USEUNIT("runnerline_print.cpp");
USEUNIT("filesocket.cpp");
USEUNIT("charset_spectrum.cpp");
USEUNIT("charset_default.cpp");
USEUNIT("charset_cpc.cpp");
USEUNIT("element.cpp");
//---------------------------------------------------------------------------
bool fInterrupted= false;

const std::string strPrompt ("Ok");

namespace sysvar= blassic::sysvar;

//************************************************
//	Local functions and classes
//************************************************

namespace {

void set_title (const std::string & str, Runner & runner)
{
	// C++ Builder don't like this.
	//std::string title ( str.empty () ? std::string ("Blassic") :
	//	(str == "-") ? std::string ("Blassic (stdin)") : str);
	std::string title;
	if (str.empty () )
		title= "Blassic";
	else if (str == "-")
		title= "Blassic (stdin)";
	else
		title= str;

	graphics::set_default_title (title);
	runner.set_title (title);
}

// Workaround to problem in cygwin:
#ifndef SIGBREAK
const int SIGBREAK= 21;
#endif

void handle_sigint (int)
{
	fInterrupted= true;
	#ifdef BLASSIC_USE_WINDOWS
	signal (SIGINT, handle_sigint);
	signal (SIGBREAK, handle_sigint);
	#endif
}

#ifndef BLASSIC_USE_WINDOWS

void handle_SIGSEGV (int, siginfo_t * info, void *)
{
	fprintf (stderr, "Segmentation fault with si_addr %p"
		#ifdef HAVE_SIGINFO_T_SI_PTR
		" si_ptr %p"
		#endif
		" and si_code %i\n",
		info->si_addr,
		#ifdef HAVE_SIGINFO_T_SI_PTR
		info->si_ptr,
		#endif
		info->si_code);
	abort ();
}

#endif

void init_signal_handlers ()
{
	TRACEFUNC (tr, "init_signal_handlers");

	#ifdef BLASSIC_USE_WINDOWS

	signal (SIGINT, handle_sigint);
	signal (SIGBREAK, handle_sigint);

	#else

	struct sigaction act;
	act.sa_handler= handle_sigint;
	act.sa_flags= 0;
	sigaction (SIGINT, & act, 0);

	signal (SIGPIPE, SIG_IGN);

	#ifndef NDEBUG
	signal (SIGUSR1, TraceFunc::show);
	#endif

	// This helps debugging machine code.
	#ifndef NDEBUG
	act.sa_handler= NULL;
	act.sa_sigaction= & handle_SIGSEGV;
	act.sa_flags= SA_SIGINFO;
	sigaction (SIGSEGV, & act, 0);
	#endif

	#endif
}

class Initializer {
public:
	Initializer (const char * progname, bool detached) :
		detached_text (detached),
		detached_graphics (false)
	{
		TRACEFUNC (tr, "Initializer::Initializer");

		if (! detached_text)
			cursor::initconsole ();
		graphics::initialize (progname);
	}
	~Initializer ()
	{
		TRACEFUNC (tr, "Initializer::~Initializer");

		if (! detached_graphics)
			graphics::uninitialize ();
		if (! detached_text)
			cursor::quitconsole ();
	}
	void detachgraphics () { detached_graphics= true; }
	void detachtext () { detached_text= true; }
private:
	bool detached_text;
	bool detached_graphics;
};

std::vector <std::string> args;

void setprogramargs (char * * argv, size_t n)
{
	TRACEFUNC (tr, "setprogramargs");

	sysvar::set16 (sysvar::NumArgs, short (n) );
	args.clear ();
	std::copy (argv, argv + n, std::back_inserter (args) );
}

void blassic_runprogram (const std::string & name,
	Runner & runner)
{
	set_title (name, runner);
	runner.run ();
}

} // namespace

// These are globals.

void setprogramargs (const std::vector <std::string> & nargs)
{
	TRACEFUNC (tr, "setprogramargs");

	sysvar::set16 (sysvar::NumArgs, short (nargs.size () ) );
	args= nargs;
}

std::string getprogramarg (size_t n)
{
	//if (n >= num_args)
	if (n >= args.size () )
		return std::string ();
	//return std::string (args [n]);
	return args [n];
}

namespace {

class ProtectCerrBuf {
public:
	ProtectCerrBuf () :
		buf (std::cerr.rdbuf () )
	{
	}
	~ProtectCerrBuf ()
	{
		std::cerr.rdbuf (buf);
	}
private:
	std::streambuf * buf;
};

//************************************************
//		Options processing.
//************************************************

class Options {
public:
	Options (int argc, char * * argv);
	bool must_run () const;

	// Public, to read it directly instead of provide accesor.
	bool detach;
	bool tron;
	bool tronline;
	bool errout;
	std::string exec_command;
	std::string print_args;
	std::string progname;
	std::string mode;

private:
	bool norun;
	const int argc;
	char * * const argv;

	typedef bool (Options::*handle_option_t) (int & n);
	typedef std::map <std::string, handle_option_t> mapoption_t;
	static mapoption_t mapoption;
	static bool initmapoption ();
	static bool mapoption_inited;
	static const mapoption_t::const_iterator no_option;

	static const char IncompatibleExecPrint [];

	handle_option_t gethandle (const char * str);

	bool handle_option_exec (int & n);
	bool handle_option_print (int & n);
	bool handle_option_mode (int & n);
	bool handle_option_detach (int & n);
	bool handle_option_auto (int & n);
	bool handle_option_exclude (int & n);
	bool handle_option_debug (int & n);
	bool handle_option_info (int & n);
	bool handle_option_cpc (int & n);
	bool handle_option_spectrum (int & n);
	bool handle_option_msx (int & n);
	bool handle_option_gwbasic (int & n);
	bool handle_option_appleII (int & n);
	bool handle_option_allflags (int & n);
	bool handle_option_rotate (int & n);
	bool handle_option_lfcr (int & n);
	bool handle_option_norun (int & n);
	bool handle_option_tron (int & n);
	bool handle_option_tronline (int & n);
	bool handle_option_errout (int & n);
	bool handle_option_comblank (int & n);
	bool handle_option_double_dash (int & n);
	bool handle_option_dash (int & n);
	bool handle_option_default (int & n);
};

Options::Options (int argc, char * * argv) :
	detach (false),
	tron (false),
	tronline (false),
	errout (false),
	norun (false),
	argc (argc),
	argv (argv)
{
	TRACEFUNC (tr, "Options::Options");

	int n= 1;
	for ( ; n < argc; ++n)
	{
		if ( (this->*gethandle (argv [n]) ) (n) )
			break;
	}

	if (n >= argc)
		return;

	progname= argv [n];
	++n;

	if (n < argc)
	{
		int narg= argc - n;
		setprogramargs (argv + n, narg);
	}
}

bool Options::must_run () const
{
	return ! norun && ! progname.empty ();
}

Options::mapoption_t Options::mapoption;

bool Options::initmapoption ()
{
	mapoption ["-e"]= & Options::handle_option_exec;
	mapoption ["-p"]= & Options::handle_option_print;
	mapoption ["-m"]= & Options::handle_option_mode;
	mapoption ["-d"]= & Options::handle_option_detach;
	mapoption ["-a"]= & Options::handle_option_auto;
	mapoption ["-x"]= & Options::handle_option_exclude;
	mapoption ["--debug"]= & Options::handle_option_debug;
	mapoption ["--info"]= & Options::handle_option_info;
	mapoption ["--cpc"]= & Options::handle_option_cpc;
	mapoption ["--spectrum"]= & Options::handle_option_spectrum;
	mapoption ["--msx"]= & Options::handle_option_msx;
	mapoption ["--gwbasic"]= & Options::handle_option_gwbasic;
	mapoption ["--appleII"]= & Options::handle_option_appleII;
	mapoption ["--allflags"]= & Options::handle_option_allflags;
	mapoption ["--rotate"]= & Options::handle_option_rotate;
	mapoption ["--lfcr"]= & Options::handle_option_lfcr;
	mapoption ["--norun"]= & Options::handle_option_norun;
	mapoption ["--tron"]= & Options::handle_option_tron;
	mapoption ["--tronline"]= & Options::handle_option_tronline;
	mapoption ["--errout"]= & Options::handle_option_errout;
	mapoption ["--comblank"]= & Options::handle_option_comblank;
	mapoption ["--"]= & Options::handle_option_double_dash;
	mapoption ["-"]= & Options::handle_option_dash;

	return true;
}

bool Options::mapoption_inited= Options::initmapoption ();

const Options::mapoption_t::const_iterator
	Options::no_option= mapoption.end ();

const char Options::IncompatibleExecPrint []=
	"Options -e and -p are incompatibles";

Options::handle_option_t Options::gethandle (const char * str)
{
	mapoption_t::const_iterator it (mapoption.find (str) );
	if (it != no_option)
		return it->second;
	else
	{
		if (str [0] == '-')
			throw "Invalid option";
		else
			return & Options::handle_option_default;
	}
}

bool Options::handle_option_exec (int & n)
{
	if (++n == argc)
		throw "Option -e needs argument";
	if (! exec_command.empty () )
		throw "Option -e can only be used once";
	if (! print_args.empty () )
		throw IncompatibleExecPrint;
	exec_command= argv [n];
	return false;
}

bool Options::handle_option_print (int & n)
{
	static const char PrintNeedsArgument []=
		"Option -p needs at least one argument";

	if (++n == argc)
		throw PrintNeedsArgument;
	if (! exec_command.empty () )
		throw IncompatibleExecPrint;

	bool empty= true;
	while (n < argc)
	{
		if (strcmp (argv [n], "--") == 0)
			break;

		if (! print_args.empty () )
			print_args+= ": ";
		print_args+= "PRINT ";
		print_args+= argv [n];

		++n;
		empty= false;
	}
	if (empty)
		throw PrintNeedsArgument;
	return false;
}

bool Options::handle_option_mode (int & n)
{
	if (++n == argc)
		throw "Option -m needs argument";
	if (!mode.empty () )
		throw "Option -m can only be used once";
	mode= argv [n];
	if (mode.empty () )
		throw "Invalid empty string mode";
	return false;
}

bool Options::handle_option_detach (int & /*n*/)
{
	detach= true;
	return false;
}

bool Options::handle_option_auto (int & n)
{
	if (++n == argc)
		throw "Option -a needs argument";
	std::istringstream iss (argv [n]);
	BlLineNumber ini;
	iss >> ini;
	char c= char (iss.get () );
	if (! iss.eof () )
	{
		if (c != ',')
			throw "Bad parameter";
		BlLineNumber inc;
		iss >> inc;
		if (! iss)
			throw "Bad parameter";
		iss >> c;
		if (! iss.eof () )
			throw "Bad parameter";
		sysvar::set32 (sysvar::AutoInc, inc);
	}
	sysvar::set32 (sysvar::AutoInit, ini);
	return false;
}

bool Options::handle_option_exclude (int & n)
{
	if (++n == argc)
		throw "Option -x needs argument";
	excludekeyword (argv [n] );
	return false;
}

bool Options::handle_option_debug (int & n)
{
	if (++n == argc)
		throw "Option --debug needs argument";
	sysvar::set32 (sysvar::DebugLevel, atoi (argv [n] ) );
	return false;
}

bool Options::handle_option_info (int & /* n */)
{
	sysvar::setFlags1 (sysvar::ShowDebugInfo);
	return false;
}

bool Options::handle_option_cpc (int & /* n */)
{
	TRACEFUNC (tr, "handle_option_cpc");

	sysvar::setFlags1 (sysvar::LocateStyle | sysvar::ThenOmitted |
		sysvar::SpaceBefore | sysvar::SpaceStr_s);
	sysvar::set16 (sysvar::Zone, 13);

	charset::default_charset= & charset::cpc_data;

	return false;
}

bool Options::handle_option_spectrum (int & /* n */)
{
	TRACEFUNC (tr, "handle_option_spectrum");

	sysvar::set (sysvar::TypeOfVal, 1);
	sysvar::set (sysvar::TypeOfNextCheck, 1);
	sysvar::set (sysvar::TypeOfDimCheck, 1);
	sysvar::setFlags1 (sysvar::TabStyle | sysvar::RelaxedGoto);
	sysvar::setFlags2 (sysvar::SeparatedGoto |
		sysvar::TruePositive | sysvar::BoolMode);
	sysvar::set16 (sysvar::Zone, 16);

	charset::default_charset= & charset::spectrum_data;

	return false;
}

bool Options::handle_option_msx (int & /* n */)
{
	TRACEFUNC (tr, "handle_option_msx");

	sysvar::setFlags1 (sysvar::ThenOmitted |
		sysvar::SpaceBefore | sysvar::SpaceStr_s);
	sysvar::set16 (sysvar::Zone, 14);

	charset::default_charset= & charset::msx_data;

	return false;
}

bool Options::handle_option_gwbasic (int & /* n */)
{
	TRACEFUNC (tr, "handle_option_gwbasic");

	sysvar::setFlags1 (sysvar::ThenOmitted |
		sysvar::SpaceBefore | sysvar::SpaceStr_s);
	sysvar::set16 (sysvar::Zone, 14);
	return false;
}

bool Options::handle_option_appleII (int & /* n */)
{
	TRACEFUNC (tr, "handle_option_appleII");

	// Pending of determine Flags 1 values.
	//sysvar::setFlags1 (sysvar::LocateStyle | sysvar::ThenOmitted |
	//	sysvar::SpaceBefore | sysvar::SpaceStr_s);
	sysvar::setFlags2 (sysvar::TruePositive | sysvar::BoolMode);
	// Pending of stablish Zone value.
	sysvar::set16 (sysvar::Zone, 13);
	return false;
}

bool Options::handle_option_allflags (int & /* n */)
{
	sysvar::setFlags1 (sysvar::Flags1Full);
	sysvar::setFlags2 (sysvar::Flags2Full);
	return false;
}

bool Options::handle_option_rotate (int & /* n */)
{
	sysvar::set (sysvar::GraphRotate, 1);
	return false;
}

bool Options::handle_option_lfcr (int & /* n */)
{
	sysvar::setFlags1 (sysvar::ConvertLFCR);
	return false;
}

bool Options::handle_option_norun (int & /* n */)
{
	norun= true;
	return false;
}

bool Options::handle_option_tron (int & /* n */)
{
	tron= true;
	return false;
}

bool Options::handle_option_tronline (int & /* n */)
{
	tron= true;
	tronline= true;
	return false;
}

bool Options::handle_option_errout (int & /* n */)
{
	errout= true;
	std::cerr.rdbuf (std::cout.rdbuf () );
	return false;
}

bool Options::handle_option_comblank (int & /* n */)
{
	sysvar::setFlags2 (sysvar::BlankComment);
	return false;
}

bool Options::handle_option_double_dash (int & n)
{
	++n;
	return true;
}

bool Options::handle_option_dash (int & /* n */)
{
	return true;
}

bool Options::handle_option_default (int & /*n*/)
{
	return true;
}

class BlBuf : public std::streambuf
{
public:
	BlBuf (GlobalRunner & globalrunner) :
		globalrunner (globalrunner)
	{
	}
private:
	void sendchar (blassic::file::BlFile & f, char c)
	{
		if (c == '\n')
			f.endline ();
		else
			f << c;
	}
	int overflow (int ch)
	{
		sync ();
		blassic::file::BlFile & f=
			globalrunner.getfile (DefaultChannel);
		sendchar (f, static_cast <char>
				(static_cast <unsigned char> (ch) ) );
		return 0;
	}
	int sync ()
	{
		blassic::file::BlFile & f=
			globalrunner.getfile (DefaultChannel);
		std::streamsize n= pptr () - pbase ();
		for (std::streamsize i= 0; i < n; ++i)
			sendchar (f, * (pbase () + i) );
		pbump (-n);
		gbump (egptr () - gptr () );
		return 0;
	}
	GlobalRunner & globalrunner;
};

int blassic_main (int argc, char * * argv)
{
	using std::cerr;
	using std::endl;

	TRACEFUNC (tr, "blassic_main");

	init_signal_handlers ();
	sysvar::init ();

	Options options (argc, argv);

	//Program program;
	std::auto_ptr <Program> pprogram (newProgram () );
	Program & program= * pprogram.get ();

	// Load program before detaching, or error messages are lost.
	// Can be to run it or for use with options print or execute.
	// Seems not very useful with print, but does not hurt and
	// somebody can find an application.

	if (! options.progname.empty () )
	{
		if (options.progname == "-")
			program.load (std::cin);
		else
			program.load (options.progname);
	}

	// Detach is done before initializing console and graphics.

	if (options.detach)
	{
		#ifdef BLASSIC_USE_WINDOWS

		FreeConsole ();

		#else

		switch (fork () )
		{
		case pid_t (-1):
			throw "fork failed";
		case pid_t (0):
			// Child. Detach and continue.
			{
				bool showdebug= showdebuginfo ();
				int newstd= open ("/dev/null", O_RDWR);
				if (newstd == -1)
				{
					close (STDIN_FILENO);
					close (STDOUT_FILENO);
					if (! showdebug)
						close (STDERR_FILENO);
				}
				else
				{
					dup2 (newstd, STDIN_FILENO);
					dup2 (newstd, STDOUT_FILENO);
					if (! showdebug)
						dup2 (newstd, STDERR_FILENO);
					close (newstd);
				}
			}
			break;
		default:
			// Parent: exit inmediately.
			return 0;
		}

		#endif
	}

	// Iniittialize console if not detached and graphics.

	Initializer initializer (argv [0], options.detach);

	// Execute mode options.

	bool spectrummode= false;

	if (! options.mode.empty () )
	{
		if (options.mode.size () > 0 && isdigit (options.mode [0] ) )
		{
			int mode= atoi (options.mode.c_str () );
			std::string::size_type x=
				options.mode.find ('x');
			if (x != std::string::npos)
			{
				int mode2= atoi
					(options.mode.c_str () + x + 1);
				graphics::setmode
					(mode, mode2, false);
			}
			else
				if (mode != 0)
					graphics::setmode (mode);
		}
		else
		{
			graphics::setmode (options.mode);
			if (options.mode == "spectrum")
				spectrummode= true;
		}
	}

	// Initialize runners.

	GlobalRunner globalrunner (program);
	Runner runner (globalrunner);

	if (spectrummode)
		runner.spectrumwindows ();

	// Save state of cerr now, after possible redirection to cout
	// but before redirection to program output, to keep it
	// usable in main.
	ProtectCerrBuf protectcerrbuf;

	if (options.errout)
	{
		static BlBuf buf (globalrunner);
		std::cerr.rdbuf (& buf);
	}

	// Tron options.

	if (options.tron)
		globalrunner.tron (options.tronline, 0);

	// Execution options.

	static const char EXECUTING []= "Executing line: ";

	if (! options.exec_command.empty () )
	{
		if (showdebuginfo () )
			cerr << EXECUTING << options.exec_command <<
				endl;
		CodeLine codeline;
		codeline.scan (options.exec_command);
		if (codeline.number () != LineDirectCommand)
		{
			program.insert (codeline);
			runner.run ();
		}
		else
			runner.runline (codeline);
		return 0;
	}

	if (! options.print_args.empty () )
	{
		if (showdebuginfo () )
			cerr << EXECUTING << options.print_args <<
				endl;
		CodeLine codeline;
		codeline.scan (options.print_args);
		runner.runline (codeline);
		return 0;
	}

	//if (! options.progname.empty () )
	if (options.must_run () )
	{
		blassic_runprogram (options.progname, runner);
		return 0;
	}

	// And if nothing of these...

	set_title (options.progname, runner);
	runner.interactive ();

	return 0;
}

} // namespace

//************************************************
//		class Exit
//************************************************

Exit::Exit (int ncode) :
	exitcode (ncode)
{ }

int Exit::code () const
{
	return exitcode;
}

//************************************************
//		main
//************************************************

int main (int argc, char * * argv)
{
	using std::cerr;
	using std::endl;

	// This can make some speed gain.
	//std::ios::sync_with_stdio (false);
	std::cout.sync_with_stdio (false);

	TRACEFUNC (tr, "main");
	int r;

	try
	{
		r= blassic_main (argc, argv);
		#ifndef NDEBUG
		std::ostringstream oss;
		oss << "Returning " << r << " without exception.";
		TRMESSAGE (tr, oss.str () );
		#endif
	}
	catch (BlErrNo ben)
	{
		cerr << ErrStr (ben) << endl;
		TRMESSAGE (tr, ErrStr (ben) );
		r= 127;
	}
	catch (BlError & be)
	{
		cerr << be << endl;
		TRMESSAGE (tr, util::to_string (be) );
		r= 127;
	}
	catch (BlBreakInPos & bbip)
	{
		cerr << bbip << endl;
		TRMESSAGE (tr, util::to_string (bbip) );
		r= 127;
	}
	catch (std::exception & e)
	{
		cerr << e.what () << endl;
		TRMESSAGE (tr, e.what () );
		r= 127;
	}
	catch (Exit & e)
	{
		r= e.code ();
		TRMESSAGE (tr, "Exit " + util::to_string (r) );
	}
	catch (const char * str)
	{
		cerr << str << endl;
		TRMESSAGE (tr, str);
		r= 127;
	}
	catch (...)
	{
		cerr << "Unexpected error." << endl;
		TRMESSAGE (tr, "Unexpected error.");
		r= 127;
	}

	return r;
}

// End of blassic.cpp
