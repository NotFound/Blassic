// runnerline_print.cpp
// Revision 7-feb-2005

#include "runnerline_impl.h"

#include "using.h"
#include "sysvar.h"
#include "util.h"
using util::to_string;

#include "trace.h"

#include <cassert>
#define ASSERT assert

namespace sysvar= blassic::sysvar;
using namespace blassic::file;


#define requiretoken(c) if (token.code == c) ; else throw ErrSyntax

#define expecttoken(c) \
	do { \
		gettoken (); \
		if (token.code != c) throw ErrSyntax; \
	} while (0)

void RunnerLineImpl::print_using (BlFile & out)
{
	TRACEFUNC (tr, "RunnerLineImpl::print_using");

	std::string format= expectstring ();
	VectorUsing usingf;
	parseusing (format, usingf);
	if (token.code == ',' || token.code == ';')
		gettoken ();
	const size_t l= usingf.size ();
	size_t ind= 0;
	Using * pf= usingf [ind];
	for (;;)
	{
		if (ind == 0 && pf->isliteral () )
		{
			pf->putliteral (out);
			ind= (ind + 1) % l;
			if (ind == 0)
				throw ErrFunctionCall;
			pf= usingf [ind];
		}
		BlResult result;
		eval (result);
		switch (result.type () )
		{
		case VarNumber:
			pf->putnumeric (out, result.number () );
			break;
		case VarInteger:
			pf->putnumeric (out, result.number () );
			break;
		case VarString:
			pf->putstring (out, result.str () );
			break;
		default:
			ASSERT (false);
			throw ErrBlassicInternal;
		}
		ind= (ind + 1) % l;
		pf= usingf [ind];
		if (ind != 0 && pf->isliteral () )
		{
			pf->putliteral (out);
			ind= (ind + 1) % l;
			// Seen unnecessary, and is erroneous.
			//if (ind == 0)
			//	throw ErrFunctionCall;
			pf= usingf [ind];
		}
		if (endsentence () )
		{
			//out << '\n';
			out.endline ();
			break;
		}
		if (token.code == ';' || token.code == ',')
		{
			gettoken ();
			if (endsentence () )
				break;
		}
	}
}

namespace {

class ChannelSave {
	BlFile & bf;
	bool inversesaved;
	bool inversevalue;
	bool inksaved;
	int inkvalue;
	bool papersaved;
	int papervalue;
	bool brightsaved;
	bool brightvalue;
public:
	ChannelSave (BlFile & bf) :
		bf (bf),
		inversesaved (false),
		inksaved (false),
		papersaved (false),
		brightsaved (false)
	{
		TRACEFUNC (tr, "ChannelSave::ChannelSave");
	}
	~ChannelSave ()
	{
		TRACEFUNC (tr, "ChannelSave::~ChannelSave");

		if (inversesaved)
			bf.inverse (inversevalue);
		if (brightsaved)
			bf.bright (brightvalue);
		if (inksaved)
			bf.setcolor (inkvalue);
		if (papersaved)
			bf.setbackground (papervalue);
	}
	void inverse (bool inv)
	{
		if (! inversesaved)
		{
			inversevalue= bf.getinverse ();
			inversesaved= true;
		}
		bf.inverse (inv);
	}
	void ink (int color)
	{
		if (! inksaved)
		{
			inkvalue= bf.getcolor ();
			inksaved= true;
		}
		bf.setcolor (color);
	}
	void paper (int color)
	{
		if (! papersaved)
		{
			papervalue= bf.getbackground ();
			papersaved= true;
		}
		bf.setbackground (color);
	}
	void bright (bool br)
	{
		if (! brightsaved)
		{
			brightvalue= bf.getbright ();
			brightsaved= true;
		}
		bf.bright (br);
	}
};

} // namespace

bool RunnerLineImpl::do_PRINT ()
{
	TRACEFUNC (tr, "RunnerLineImpl::do_PRINT");

	// Same function used for PRINT and LPRINT, differing only
	// in the default channel.
	BlChannel channel= (token.code == keyLPRINT) ?
		PrinterChannel : DefaultChannel;
	gettoken ();
	if (token.code == '#')
	{
		channel= expectchannel ();
		// Allow ';' for Spectrum compatibility.
		if (token.code == ',' || token.code == ';')
			gettoken ();
		else
			require_endsentence ();
	}

	TRMESSAGE (tr, "Channel: " + to_string (channel) );

	BlFile & out= getfile (channel);

	TRMESSAGE (tr, "Text window: " + to_string (out.istextwindow () ) );
	TRMESSAGE (tr, "File: " + to_string (out.isfile () ) );

	ChannelSave channelsave (out);

	if (token.code == '@')
	{
		BlResult result;
		expect (result);
		BlInteger pos= result.integer ();
		requiretoken (',');
		gettoken ();
		#if 0
		BlInteger row= (pos / 32) + 1;
		BlInteger col= (pos % 32) + 1;
		if (graphics::ingraphicsmode () )
			graphics::locate (row, col);
		else
			locate (row, col);
		#else
		out.gotoxy (pos % 32, pos / 32);
		#endif
	}
	if (endsentence () )
	{
		//out << '\n';
		out.endline ();
		return false;
	}

	bool spacebeforenumber=
		sysvar::hasFlags1 (sysvar::SpaceBefore);
	BlResult result;
	size_t n;
	bool ended= false;
	do {
		switch (token.code) {
		case ',':
		case ';':
			// Separators can be before any statement,
			break;
		case keyUSING:
			print_using (out);
			ended= true;
			break;
		case keyTAB:
			//getparenarg (result);
			// Not required to improve Sinclair ZX compatibility.
			expect (result);
			n= result.integer ();
			if (! sysvar::hasFlags1 (sysvar::TabStyle) )
				n-= 1;
			out.tab (n);
			break;
		case keySPC:
			getparenarg (result);
			n= result.integer ();
			//out << std::string (n, ' ');
			out.putspaces (n);
			break;
		case keyAT:
			out.flush ();
			{
				expect (result);
				//BlInteger row= result.integer () + 1;
				BlInteger row= result.integer ();
				requiretoken (',');
				expect (result);
				//BlInteger col= result.integer () + 1;
				BlInteger col= result.integer ();
				#if 0
				if (graphics::ingraphicsmode () )
					graphics::locate (row, col);
				else
					locate (row, col);
				#else
				out.gotoxy (col, row);
				#endif
			}
			break;
		case keyINK:
			out.flush ();
			{
				BlInteger color= expectinteger ();
				channelsave.ink (color);
			}
			break;
		case keyPAPER:
			out.flush ();
			{
				BlInteger color= expectinteger ();
				channelsave.paper (color);
			}
			break;
		case keyINVERSE:
			out.flush ();
			{
				BlInteger inv= expectinteger ();
				channelsave.inverse (inv);
			}
			break;
		case keyBRIGHT:
			out.flush ();
			{
				BlInteger br= expectinteger ();
				channelsave.bright (br);
			}
			break;
		default:
			eval (result);
			switch (result.type () ) {
			case VarString:
				{
					TRMESSAGE (tr, "string");
					std::string txt= result.str ();
					TRMESSAGE (tr, txt);
					//out << result.str ();
					out << txt;
				}
				break;
			case VarNumber:
				{
					BlNumber n= result.number ();
					if (spacebeforenumber && n >= 0)
						out << ' ';
					out << n;
				}
				break;
			case VarInteger:
				{
					BlInteger n= result.integer ();
					if (spacebeforenumber && n >= 0)
						out << ' ';
					out << n;
				}
				break;
			default:
				ASSERT (false);
				throw ErrBlassicInternal;
			}
		}
		if (ended)
			break;
		if (endsentence () )
		{
			//out << '\n';
			out.endline ();
			break;
		}
		#if 0
		if (token.code != ';' && token.code != ',')
			throw ErrSyntax;
		if (token.code == ',')
			out.tab ();
		gettoken ();
		#else
		// Now separator is not required
		switch (token.code)
		{
		case ',':
			out.tab ();
			gettoken ();
			break;
		case ';':
			gettoken ();
			break;
		default:
			; // Nothing
		}
		#endif
	} while (! endsentence () );
	TRMESSAGE (tr, "Flushing channel");
	out.flush ();
	return false;
}

// End of runnerline_print.cpp
