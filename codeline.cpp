// codeline.cpp
// Revision 10-feb-2005

#include "codeline.h"

#include "keyword.h"
#include "token.h"
#include "error.h"
#include "util.h"

#include "trace.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

#include <math.h>

using std::cerr;
using std::endl;
using std::flush;
using std::istringstream;
using std::fill_n;
using std::transform;
using std::isxdigit;


namespace {

inline bool is_space (char c)
{
	return isspace (static_cast <unsigned char> (c) );
}

inline bool is_bindigit (char c)
{
	return c == '0' || c == '1';
}

inline bool is_octdigit (char c)
{
	return c >= '0' && c <= '7';
}

inline bool is_decdigit (char c)
{
	return isdigit (static_cast <unsigned char> (c) );
}


inline bool is_hexdigit (char c)
{
	return isxdigit (static_cast <unsigned char> (c) );
}

inline bool is_base (char c, int base)
{
	switch (base)
	{
	case 2:
		return is_bindigit (c);
	case 8:
		return is_octdigit (c);
	case 16:
		return is_hexdigit (c);
	default:
		return false;
	}
}

inline int valdigit (unsigned char c)
{
	if (islower (c) )
		c= static_cast <unsigned char> (c + '9' - 'a' + 1);
	else if (isupper (c) )
		c= static_cast <unsigned char> (c + '9' - 'A' + 1);
	return c - '0';
}

inline bool is_beginidentifier (char c)
{
	return isalpha (static_cast <unsigned char> (c) ) /* || c == '_' */;
}

inline bool is_identifier (char c)
{
	return isalnum (static_cast <unsigned char> (c) ) || c == '_';
}

} // namespace

BlNumber CodeLine::Token::number (const std::string & str)
{
	const size_t l= str.size ();
	if (l == 0)
		return 0;
	size_t i= 0;
	BlNumber n= 0;
	unsigned char c;
	if (str [0] == '&')
	{
		if (l > 1)
		{
			long num= 0;
			c= str [1];
			if (c == 'X' || c == 'x')
			{
			// Binary
				i= 1;
				while (++i < l && is_bindigit (c= str [i]) )
				{
					num*= 2;
					num+= c- '0';
				}
			}
			else if (c == 'O' || c == 'o')
			{
			// Octal
				i= 1;
				while (++i < l && is_octdigit (c= str [i]) )
				{
					num*= 8;
					num+= c- '0';
				}
			}
			else
			{
			// Hexadecimal
				if (c == 'H' || c == 'h')
					i= 1;
				while (++i < l && is_hexdigit (c= str [i] ) )
				{
					num*= 16;
					if (c >= 'a' && c <= 'f')
						c= (unsigned char)
							(c + '9' - 'a' + 1);
					else if (c >= 'A' && c <= 'F')
						c= (unsigned char)
							(c + '9' - 'A' + 1);
					num+= c - '0';
				}
			}
			n= num;
		}
	}
	else {
		// Decimal
		#if 0
		while (i < l && is_digit (c= str [i]) )
		{
			n*= 10;
			n+= c - '0';
			++i;
		}
		if (i < l && str [i] == '.')
		{
			++i;
			BlNumber mult= 0.1;
			while (i < l && is_digit (c= str [i] ) )
			{
				n+= (c - '0') * mult;
				mult/= 10;
				++i;
			}
		}
		if (i < l && (str [i] == 'E' || str [i] == 'e') )
		{
			++i;
			BlNumber e= 0;
			bool neg= false;
			if (str [i] == '-')
			{
				neg= true;
				++i;
			}
			while (i < l && (is_digit (c= str [i] ) ) )
			{
				e*= 10;
				e+= c - '0';
				++i;
			}
			if (neg) e= -e;
			n*= pow (10, e);
		}
		#else
		istringstream iss (str);
		iss >> n;
		#endif
	}
	return n;
}

BlNumber CodeLine::Token::number () const
{
	switch (code)
	{
	case keyNUMBER:
	case keySTRING:
		return number (str);
	case keyINTEGER:
		return valueint;
	case keyENDLINE:
		return 0.0;
	default:
		if (showdebuginfo () )
			cerr << "Codeline::Token::number called but code= " <<
				code << " is not valid." << endl;
		throw ErrBlassicInternal;
	}
}

CodeLine::CodeLine () :
	strcontent (0),
	linenumber (LineEndProgram),
	len (0),
	owner (false),
	pos (0),
	chk (0),
	lastcode (0)
{
}

CodeLine::CodeLine (const BlChar * str, BlLineNumber number,
		BlLineLength length) :
	strcontent (str),
	linenumber (number),
	len (length),
	owner (false),
	pos (0),
	chk (0),
	lastcode (0)
{
}

CodeLine::CodeLine (const CodeLine & old) :
	strcontent (old.strcontent),
	linenumber (old.linenumber),
	len (old.len),
	owner (false),
	pos (0),
	chk (0),
	lastcode (0)
{
}

CodeLine::~CodeLine ()
{
	TRACEFUNC (tr, "CodeLine::~CodeLine");

	if (owner && strcontent)
		delete [] strcontent;
}

void CodeLine::assign (const BlChar * str, BlLineNumber number,
	BlLineLength length)
{
	if (owner)
		delete strcontent;
	strcontent= str;
	linenumber= number;
	len= length;
	owner= false;
	pos= 0;
	chk= 0;
	lastcode= 0;
}

CodeLine & CodeLine::operator= (const CodeLine & old)
{
	if (& old != this)
	{
		if (owner && strcontent)
			delete [] strcontent;
		strcontent= old.strcontent;
		linenumber= old.linenumber;
		len= old.len;
		owner= false;
		pos= 0;
		chk= 0;
		lastcode= 0;
	}
	return * this;
}

CodeLine::Token CodeLine::getdata ()
{
	using std::string;

	while (pos < len && is_space (strcontent [pos] ) )
		++pos;
	Token r;
	if (pos >= len)
	{
		r.code= lastcode= keyENDLINE;
		return r;
	}
	char c= strcontent [pos];
	if (c == '"')
	{
		++pos;
		while ( (c= strcontent [pos++]) != '\0')
			r.str+= c;
	}
	else if (c == INTEGER_PREFIX)
	{
		//BlInteger n;
		//n= * (BlInteger *) (strcontent + pos + 1);
		//r.valueint= n;
		r.valueint= peek32 (strcontent + pos + 1);
		r.code= keyINTEGER;
		pos+= 5;
		return r;
	}
	else
	{
		while (pos < len &&
			(c= strcontent [pos]) != ',' && c != ':' && c != '\'')
		{
			if (c == INTEGER_PREFIX)
			{
				r.str+= util::to_string
					(peek32 (strcontent + pos + 1) );
				pos+= 5;
			}
			else
			{
				if (iskey (c) )
				{
					BlCode s= c;
					s<<= 8;
					s|= strcontent [pos + 1];
					r.str+= decodekeyword (s);
					pos+= 2;
				}
				else
				{
					r.str+= c;
					++pos;
				}
			}
		}
		string::size_type last= r.str.find_last_not_of (" ");
		if (last != string::npos)
			r.str.erase (last + 1);
	}
	r.code= keySTRING;
	return r;
}

namespace {

BlChar validinitident [256];
BlChar validident [256];

bool inittables ()
{
	fill_n (& validident [0], 256, 0);
	fill_n (& validinitident [0], 256, 0);

	// cast to avoid a warning.
	validident [static_cast <unsigned char> ('_')]= '_';

	for (BlChar i= '0'; i <= '9'; ++i)
	{
		validident [i]= i;
	}

	for (BlChar i= 'A'; i <= 'Z'; ++i)
	{
		validident [i]= i;
		validinitident [i]= i;
	}
	for (BlChar i= 'a'; i <= 'z'; ++i)
	{
		validident [i]= BlChar (i - 'a' + 'A');
		validinitident [i]= BlChar (i - 'a' + 'A');
	}
	return true;
}

bool initiated= inittables ();

} // namespace

void CodeLine::gettoken (Token & r)
{
	using std::string;

	while (pos < len && is_space (strcontent [pos] ) )
		++pos;
	if (pos >= len)
	{
		r.code= lastcode= keyENDLINE;
		++chk;
		return;
	}
	BlChar c= strcontent [pos];
	BlChar c2;
	if (c == '\0')
	{
		r.code= lastcode= keyENDLINE;
		++chk;
		return;
	}
	else if (iskey (c) )
	{
		BlCode code= c;
		++pos;
		code<<= 8;
		code|= strcontent [pos++];
		switch (code)
		{
		case keyTHEN:
		case keyELSE:
			++chk;
			break;
		case keyEQUALMINOR:
			code= keyMINOREQUAL;
			break;
		case keyEQUALGREATER:
			code= keyGREATEREQUAL;
			break;
		case keyGREATERMINOR:
			code= keyDISTINCT;
			break;
		default:
			; // Nothing in particular.
		}
		r.code= lastcode= code;
		return;
	}
	else if ( (c2= validinitident [c]) != 0)
	{
		r.code= lastcode= keyIDENTIFIER;
		r.str= c2;
		while ( ++pos < len &&
			(c2= validident [ (c= strcontent [pos] ) ] ) != 0)
		{
			r.str+= c2;
		}
		if (pos < len &&
			(c == '$' || c == '%' || c == '!' || c == '#') )
		{
			++pos;
			r.str+= c;
		}
		return;
	}
	else if (is_decdigit (c) || c == '.')
	{
		r.code= lastcode= keyNUMBER;
		{
			string strnum;
			while (pos < len &&
				(is_decdigit (c= strcontent [pos]) ) )
			{
				strnum+= c;
				++pos;
			}
			if (pos < len && (c= strcontent [pos] ) == '.')
			{
				strnum+= '.';
				++pos;
				while (pos < len &&
					(is_decdigit (c= strcontent [pos]) ) )
				{
					strnum+= c;
					++pos;
				}
			}
			if (pos < len && (c == 'E' || c == 'e') )
			{
				strnum+= c;
				++pos;
				if ( (c= strcontent [pos]) == '-' || c == '+')
				{
					strnum+= c;
					++pos;
				}
				while (pos < len &&
					(is_decdigit (c= strcontent [pos] ) ) )
				{
					strnum+= c;
					++pos;
				}
			}
			r.str= strnum;
		}
		return;
	}
	else if (c == '&')
	{
		#if 0
		// Hexadecimal, octal or binary number.
		r.code= keyNUMBER;
		r.str= '&';
		++pos;
		if (pos < len)
		{
			c= strcontent [pos];
			if (c == 'X' || c == 'x')
			{
				// Binary
				++pos;
				r.str+= c;
				while (pos < len &&
					is_bindigit (strcontent [pos] ) )
				{
					r.str+= strcontent [pos];
					++pos;
				}
			}
			else if (c == 'O' || c == 'o')
			{
				// Octal
				++pos;
				r.str+= c;
				while (pos < len &&
					is_octdigit (strcontent [pos] ) )
				{
					r.str+= strcontent [pos];
					++pos;
				}
			}
			else
			{
				// Hexadecimal
				if (c == 'H' || c == 'h')
				{
					++pos;
					r.str+= c;
				}
				while (pos < len &&
					is_xdigit (strcontent [pos] ) )
				{
					r.str+= strcontent [pos];
					++pos;
				}
			}
		}
		#else

		// Doing it another way, to return the values
		// as integers.
		++pos;
		if (pos >= len)
			r.code= '&';
		else
		{
			int base= 0;
			BlLineLength oldpos= pos;
			c= strcontent [pos];
			switch (c)
			{
			case 'X': case 'x':
				// Binary
				base= 2; ++pos; break;
			case 'O': case 'o':
				// Octal
				base= 8; ++pos; break;
			case 'H': case 'h':
				// Hexadecimal
				base= 16; ++pos; break;
			default:
				if (is_hexdigit (c) )
					base= 16;
			}
			switch (base)
			{
			case 2:
			case 8:
			case 16:
				if (pos >= len || ! is_base
					( (c= strcontent [pos] ), base) )
				{
					pos= oldpos;
					r.code= '&';
				}
				else
				{
					r.code= lastcode= keyINTEGER;
					r.valueint= 0;
					do
					{
						r.valueint*= base;
						r.valueint+= valdigit (c);
						++pos;
					} while (pos < len && is_base
						( (c= strcontent [pos] ),
							base) );
				}
				return;
				//break;
			default:
				pos= oldpos;
				r.code= lastcode= '&';
				return;
			}
		}

		#endif
	}
	else
	{
		++pos;
		switch (c)
		{
		case INTEGER_PREFIX:
			r.code= lastcode= keyINTEGER;
			{
				r.valueint= peek32 (strcontent + pos);
				pos+= 4;
			}
			return;
			//break;
		case '"':
			r.code= lastcode= keySTRING;
			r.str.erase ();
			while ( (c= strcontent [pos++]) != '\0')
				r.str+= c;
			return;
			//break;
		case '<':
			if (pos < len)
			{
				switch (strcontent [pos] )
				{
				case '>':
					r.code= keyDISTINCT;
					++pos;
					break;
				case '=':
					r.code= keyMINOREQUAL;
					++pos;
					break;
				default:
					r.code= '<';
				}
			}
			else r.code= '<';
			lastcode= r.code;
			return;
			//break;
		case '>':
			if (pos < len)
			{
				switch (strcontent [pos] )
				{
				case '<':
					r.code= keyDISTINCT;
					++pos;
					break;
				case '=':
					r.code= keyGREATEREQUAL;
					++pos;
					break;
				default:
					r.code= '>';
				}
			}
			else r.code= '>';
			lastcode= r.code;
			return;
			//break;
		case '=':
			if (pos < len)
			{
				switch (strcontent [pos] )
				{
				case '<':
					r.code= keyMINOREQUAL;
					++pos;
					break;
				case '>':
					r.code= keyGREATEREQUAL;
					++pos;
					break;
				default:
					r.code= '=';
				}
			}
			else r.code= '=';
			lastcode= r.code;
			return;
			//break;
		case '\'':
			r.code= lastcode= keyENDLINE;
			++chk;
			pos= len;
			return;
			//break;
		case ':':
			r.code= lastcode= ':';
			++chk;
			return;
			//break;
		default:
			r.code= lastcode= c;
			return;
		}
	}
}

void CodeLine::gotochunk (BlChunk chknew)
{
	pos= 0; chk= 0;
	if (chknew == 0)
	{
		lastcode= 0;
		return;
	}
	while (chk < chknew)
	{
		if (pos >= len)
		{
			lastcode= keyENDLINE;
			return;
		}
		char c= strcontent [pos];
		if (c == INTEGER_PREFIX)
		{
			pos+= 5;
		}
		else if (iskey (c) )
		{
			BlCode code;
			code= strcontent [pos++];
			code<<= 8;
			code|= strcontent [pos++];
			if (code == keyTHEN || code == keyELSE)
			{
				lastcode= code;
				++chk;
			}
			else if (code == keyREM)
			{
				lastcode= code;
				pos= len;
			}
		}
		else
		{
			if (c == '\'')
			{
				lastcode= '\'';
				pos= len;
			}
			if (c == '"')
			{
				++pos;
				while (strcontent [pos] != '\0')
					++pos;
				++pos;
			}
			else
			{
				if (c == ':')
				{
					lastcode= ':';
					++chk;
				}
				++pos;
			}
		}
	}
}

namespace {

using std::string;

inline string stringupper (const string & str)
{
	string u (str.size (), 0);
	transform (str.begin (), str.end (), u.begin (), toupper);
	return u;
}

inline bool is_word (const Tokenizer::Token & token, const char * str)
{
	if (token.type != Tokenizer::Plain)
		return false;
	return stringupper (token.str) == str;
}

inline bool isGO (const Tokenizer::Token & token)
{
	return is_word (token, "GO");
}

inline bool isSUB (const Tokenizer::Token & token)
{
	return is_word (token, "SUB");
}

const string strprefinteger (1, INTEGER_PREFIX);

inline string codeinteger (BlInteger n)
{
	#ifdef BLASSIC_INTEL

	return strprefinteger +
		string (reinterpret_cast <const char *> (& n), 4);

	#else

	return strprefinteger +
		char (n & 0xFF) +
		char ( (n >> 8) & 0xFF) +
		char ( (n >> 16) & 0xFF) +
		char (n >> 24);

	#endif
}

} // namespace

void CodeLine::scan (const std::string & line)
{
	using std::string;

	//linenumber= 0;
	//len= 0;
	//pos= 0;
	//BlLineNumber newlinenumber= 0;
	BlLineNumber newlinenumber= LineDirectCommand;
	string newcontent;
	static const char INVALID []= "Line number invalid";

	if (! line.empty () )
	{
		int i= 0, l= line.size ();
		while (i < l && is_space (line [i] ) )
			++i;
		if (i < l && is_decdigit (line [i] ) )
		{
			newlinenumber= 0;
			while (i < l && is_decdigit (line [i] ) )
			{
				if (newlinenumber > BlMaxLineNumber / 10)
				{
					if (showdebuginfo () )
						cerr << INVALID << endl;
					throw ErrSyntax;
				}
				newlinenumber*= 10;
				BlLineNumber n= static_cast <BlLineNumber>
					(line [i] - '0');
				if (newlinenumber > BlMaxLineNumber - n)
				{
					if (showdebuginfo () )
						cerr << INVALID << endl;
					throw ErrSyntax;
				}
				newlinenumber+= n;
				++i;
			}
			if (i < l && line [i] == ' ') ++i;
		}
		else i= 0;

		Tokenizer t (line.substr (i) );
		//string str;
		Tokenizer::Token token;
		Tokenizer::Token prevtoken;
		BlCode code;
		bool incomment= false;
		bool addspace;
		string::size_type prevsize= string::npos;
		string::size_type lastsize= 0;

		while (! incomment &&
			(token= t.get () ).type != Tokenizer::EndLine)
		{
			//cerr << '(' << str << ')' << flush;
			switch (token.type)
			{
			case Tokenizer::Blank:
				newcontent+= token.str;
				break;
			case Tokenizer::Literal:
				newcontent+= '"';
				newcontent+= token.str;
				newcontent+= '\0';
				break;
			case Tokenizer::Integer:
				newcontent+= codeinteger (token.n);
				break;
			case Tokenizer::Plain:
				//code= 0;
				addspace= false;
				if (token.str == "?")
				{
					code= keyPRINT;
					char c= t.peek ();
					if (c != '\0' && !
							is_space (c) )
						addspace= true;
				}
				else
					code= keyword (token.str);

				// GO TO
				if (code == keyTO)
				{
					if (isGO (prevtoken) )
					{
						code= keyGOTO;
						newcontent.erase
							(prevsize);
					}
				}
				// GO SUB
				if (code == 0 && isSUB (token) )
				{
					if (isGO (prevtoken) )
					{
						code= keyGOSUB;
						newcontent.erase
							(prevsize);
					}
				}
				if (code == 0)
				{
					newcontent+= token.str;
					if (token.str == "'")
						incomment= true;
				}
				else
				{
					newcontent+= char (code >> 8);
					newcontent+= char (code & 0xFF);
					if (code == keyREM)
						incomment= true;
				}
				if (addspace)
					newcontent+= ' ';
				break;
			default:
				throw ErrBlassicInternal;
			}
			if (token.type != Tokenizer::Blank)
			{
				prevtoken= token;
				prevsize= lastsize;
			}
			lastsize= newcontent.size ();
		}
		if (incomment)
		{
			newcontent+= t.getrest ();
		}

		//if (owner && strcontent)
		//	delete [] strcontent;

	}

	BlLineLength newlen= newcontent.size ();
	if (newlen > 0)
	{
		//strcontent= new unsigned char [newlen];
		//newcontent.copy ( (char *) strcontent, newlen);
		unsigned char * auxcontent= new unsigned char [newlen];
		// No need to protect auxcontent, copy and delete
		// must not throw.
		newcontent.copy ( (char *) auxcontent, newlen);
		if (owner && strcontent)
			delete [] strcontent;
		strcontent= auxcontent;
		owner= true;
	}
	else
	{
		if (owner && strcontent)
			delete [] strcontent;
		owner= false;
		strcontent= NULL;
	}

	len= newlen;
	linenumber= newlinenumber;
	pos= 0;
	chk= 0;
	lastcode= 0;
}

// Fin de codeline.cpp
