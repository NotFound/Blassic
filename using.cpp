// using.cpp
// Revision 9-jan-2005

#include "using.h"
#include "util.h"
#include "trace.h"

#include <sstream>
#include <iomanip>
#include <memory>
using std::auto_ptr;
#include <cstdio>
using std::sprintf;
#include <cstdlib>
#include <cmath>
#include <stdexcept>
#include <algorithm>

#include <cassert>
#define ASSERT assert

using namespace blassic::file;

//**************************************
//	Auxiliary functions
//**************************************

namespace {

const char poundsign= '\xA3';
#ifdef _Windows
const char eurosign= '\x80';
#else
const char eurosign= '\xA4';
#endif

bool charformat [256]= { false };

bool initformat ()
{
	charformat [static_cast <unsigned char> ('#') ]= true;
	charformat [static_cast <unsigned char> ('.') ]= true;
	charformat [static_cast <unsigned char> ('+') ]= true;
	charformat [static_cast <unsigned char> ('*') ]= true;
	charformat [static_cast <unsigned char> ('$') ]= true;
	charformat [static_cast <unsigned char> (poundsign) ]= true;
	charformat [static_cast <unsigned char> (eurosign) ]= true;
	charformat [static_cast <unsigned char> ('\\') ]= true;
	charformat [static_cast <unsigned char> ('&') ]= true;
	charformat [static_cast <unsigned char> ('!') ]= true;
	return true;
}

bool initedformat= initformat ();

bool ischarformat (char c)
{
	return charformat [static_cast <unsigned char> (c) ];
}

}

//**************************************
//		Using
//**************************************

Using::~Using ()
{ }

bool Using::isliteral () const
{ return false; }

void Using::putliteral (BlFile &) const
{ throw ErrMismatch; }

void Using::putnumeric (BlFile &, BlNumber) const
{ throw ErrMismatch; }

void Using::putstring (BlFile &, const std::string &) const
{ throw ErrMismatch; }

//**************************************
//		UsingLiteral
//**************************************

UsingLiteral::UsingLiteral (std::istream & is)
{
	TRACEFUNC (tr, "UsingLiteral::UsingLiteral");

	init (is);
}

UsingLiteral::UsingLiteral (std::istream & is,
		const std::string & strinit) :
	s (strinit)
{
	TRACEFUNC (tr, "UsingLiteral::UsingLiteral");

	init (is);
}

UsingLiteral::UsingLiteral (const std::string & strinit) :
	s (strinit)
{
	TRACEFUNC (tr, "UsingLiteral::UsingLiteral");
}

void UsingLiteral::init (std::istream & is)
{
	TRACEFUNC (tr, "UsingLiteral::init");

	char c;
	while (is >> c && ! ischarformat (c) )
	{
		s+= c;
	}
	ASSERT (! s.empty () );
	TRMESSAGE (tr, std::string ("Literal is: \"") + s + '"');
	if (is)
		is.unget ();
}

bool UsingLiteral::isliteral () const
{ return true; }

void UsingLiteral::addstr (const std::string & stradd)
{
	s+= stradd;
}

void UsingLiteral::putliteral (BlFile & out) const
{
	out << s;
}

void UsingLiteral::putnumeric (BlFile &, BlNumber) const
{ ASSERT (false); throw ErrBlassicInternal; }

void UsingLiteral::putstring (BlFile &, const std::string &) const
{ ASSERT (false); throw ErrBlassicInternal; }

//**************************************
//		UsingNumeric
//**************************************

UsingNumeric::UsingNumeric (std::istream & is, std::string & tail) :
	invalid (true),
	digit (0),
	decimal (0),
	scientific (0),
	milliards (false),
	putsign (false),
	signatend (false),
	blankpositive (false),
	asterisk (false),
	dollar (false),
	pound (false),
	euro (false)
{
	TRACEFUNC (tr, "UsingNumeric::UsingNumeric");

	char c;
	is >> c;
	ASSERT (is);
	if (c == '+')
	{
		putsign= true;
		s+= c;
		is >> c;
	}

	if (is)
	{
		switch (c)
		{
		case '*':
			s+= c;
			if (! (is >> c) )
				return;
			if (c != '*')
			{
				TRMESSAGE (tr, std::string (1, c) +
					" following *");
				is.unget ();
				return;
			}
			asterisk= true;
			digit+= 2;
			if (is >> c)
			{
				switch (c)
				{
				case '$':
					dollar= true;
					s+= c;
					is >> c;
					break;
				case poundsign:
					pound= true;
					s+= c;
					is >> c;
					break;
				case eurosign:
					euro= true;
					s+= c;
					is >> c;
					break;
				}
			}
			break;
		case '$':
		case poundsign:
		case eurosign:
			s+= c;
			{
				char sign= c;
				if (! (is >> c) )
					return;
				if (c != sign)
				{
					is.unget ();
					return;
				}
				++digit;
				switch (sign)
				{
				case '$': dollar= true; break;
				case poundsign: pound= true; break;
				case eurosign: euro= true; break;
				}
				s+= c;
			}
			is >> c;
			break;
		}
	}

	while (is && c == '#')
	{
		invalid= false;
		++digit;
		s+= c;
		is >> c;
	}
	if (is)
	{
		if (c == '.')
		{
			s+= c;
			while (is >> c && c == '#')
			{
				invalid= false;
				++decimal;
				s+= c;
			}
		}
	}
	if (is)
	{
		if (c == '^')
		{
			++scientific;
			s+= c;
			if (invalid)
				return;
			while (scientific < 5 && is >> c && c == '^')
				++scientific;
			if (scientific == 5)
				is >> c;
			if (scientific < 4)
			{
				tail= std::string (scientific, '^');
				scientific= 0;
			}
			else
				scientific-= 2;
		}
	}
	if (is && (c == '+' || c == '-') )
	{
		s+= c;
		if (invalid)
			return;
		putsign= true;
		signatend= true;
		if (c == '-')
			blankpositive= true;
		is >> c;
	}
	if (is)
		is.unget ();
}

namespace {

class UsingOverflow { };

}

namespace {
// Workaround for a problem in gcc
double zero= 0.0;
}

void UsingNumeric::putliteral (BlFile &) const
{ ASSERT (false); throw ErrBlassicInternal; }

void UsingNumeric::putnumeric (BlFile & out, BlNumber n) const
{
	try
	{
		int negative;
		if (scientific > 0)
		{
			size_t d= digit;
			if (n < zero && ! putsign)
				--d;
			#if 0
			int prec= d + decimal;
			int dec;
			char * aux= ecvt (n, prec, & dec, & negative);
			int e= dec - static_cast <int> (d);
			//if (scientific == 0 && e != 0)
			//	throw UsingOverflow ();
			std::string stre= util::to_string (abs (e) );
			int ezeroes= scientific - stre.size ();
			if (ezeroes < 0)
				throw UsingOverflow ();

			if (putsign && ! signatend)
			{
				out << (negative ? '-' : '+');
			}
			if (! putsign && negative)
				out << '-';
			out << (std::string (aux, d) + '.' +
				std::string (aux + d) ) <<
				'E' << (e < 0 ? '-' : '+') <<
				std::string (ezeroes, '0') <<
				stre;
			#else
			int prec= d + decimal - 1;
			//std::cout << '(' << d << ',' << prec << ')' <<
			//	std::flush;
			char buffer [64];
			sprintf (buffer, "%+.*e", prec, n);
			//std::cout << '[' << buffer << ']' << std::flush;
			negative= buffer [0] == '-';
			std::string aux= std::string (1, buffer [1]) +
				(buffer + 3);
			std::string::size_type pose= aux.find ('e');
			if (pose != std::string::npos)
				aux.erase (pose);
			int e= static_cast <int>
				(std::floor (std::log10 (fabs (n) ) ) ) +
				- static_cast <int> (d) + 1;
			std::string stre= util::to_string (abs (e) );
			int ezeroes= scientific - stre.size ();
			if (ezeroes < 0)
				throw UsingOverflow ();
			if (putsign && ! signatend)
			{
				out << (negative ? '-' : '+');
			}
			if (! putsign && negative)
				out << '-';
			out << (aux.substr (0, d) + '.' +
				aux.substr (d) ) <<
				'E' << (e < 0 ? '-' : '+') <<
				std::string (ezeroes, '0') <<
				stre;
			#endif
		}
		else
		{
			int numdig= 0;
			negative= n < 0;
			if (negative)
				n= -n;
			if (n != 0)
				numdig= int (std::log10 (n * 10 ) );
			if (numdig < 0)
				numdig= 0;
			size_t w= numdig;
			if (digit > w)
				w= digit;
			size_t maxw= digit;
			if (decimal > 0)
			{
				w+= decimal + 1;
				maxw+= decimal + 1;
			}
			if (negative && ! putsign)
			{
				--w;
				--maxw;
			}
			std::ostringstream oss;
			if (decimal > 0)
				oss.setf (std::ios::showpoint);

			// Some implementations lack the fixed manipulator.
			oss.setf (std::ios::fixed, std::ios::floatfield);
			oss << std::setw (w) <<
				//std::setprecision (decimal + numdig) <<
				std::setprecision (decimal) <<
				//std::fixed <<
				n;
			std::string strn= oss.str ();
			if (digit == 0 && strn [0] == '0')
				strn.erase (0, 1);
			if (strn.size () > maxw)
				throw UsingOverflow ();
			if (dollar || pound || euro)
			{
				char sign= dollar ? '$' :
					pound ? poundsign : eurosign;
				strn.insert (strn.find_first_not_of (" "),
					1, sign);
			}
			if ( (putsign && ! signatend) ||
				(! putsign && negative) )
			{
				std::string::size_type possign=
					strn.find_first_not_of (" ");
				if (possign == std::string::npos)
					possign= 0;
				strn.insert (possign, 1,
					negative ? '-' : '+');
			}
			if (asterisk)
			{
				for (size_t i= 0,
					l= strn.find_first_not_of (" ");
					i < l; ++i)
				{
					strn [i]= '*';
				}
			}
			out << strn;
		}
		if (signatend)
		{
			ASSERT (putsign);
			out << (negative ? '-' :
				blankpositive ? ' ' : '+' );
		}
	}
	catch (UsingOverflow &)
	{
		out << '%' << n;
	}
}

//**************************************
//		UsingString
//**************************************

UsingString::UsingString (std::istream & is) :
	n (0)
{
	char c;
	is >> c;
	ASSERT (is);
	switch (c)
	{
	case '\\':
		n= 1;
		while (is >> c && c == ' ')
			++n;
		if (is)
		{
			if (c == '\\')
				++n;
			else
				is.unget ();
		}
		break;
	case '&':
		// Nothing in particular
		break;
	case '!':
		n= 1;
		break;
	default:
		throw ErrBlassicInternal;
	}
}

void UsingString::putliteral (BlFile &) const
{ ASSERT (false); throw ErrBlassicInternal; }

void UsingString::putstring (BlFile & out, const std::string & str) const
{
	if (n > 0)
		out << str.substr (0, n);
	else
		out << str;
}

//**************************************
//		VectorUsing
//**************************************

VectorUsing::VectorUsing ()
{
}

VectorUsing::~VectorUsing ()
{
	std::for_each (v.begin (), v.end (), delete_it);
}

void VectorUsing::push_back (const Using & u)
{
	UsingLiteral * prev;
	if (! v.empty () && u.isliteral () &&
		(prev= dynamic_cast <UsingLiteral *> (v.back () ) ) != NULL)
	{
		prev->addstr (dynamic_cast <const UsingLiteral &> (u).str () );
		return;
	}
	v.push_back (u.clone () );
}

//**************************************
//		parseusing
//**************************************

void parseusing (const std::string & str, VectorUsing & vu)
{
	TRACEFUNC (tr, "parseusing");

	std::istringstream is (str);
	is.unsetf (std::ios::skipws);
	int c;
	while ( (c= is.peek () ) != EOF)
	{
		switch (static_cast <char> (c) )
		{
		case '#':
		case '.':
		case '+':
		case '*':
		case '$':
		case poundsign:
		case eurosign:
			{
				std::string tail;
				UsingNumeric un (is, tail);
				if (un.isvalid () )
					vu.push_back (un);
				else
				{
					UsingLiteral ul (un.str () );
					vu.push_back (ul);
				}
				if (! tail.empty () )
				{
					UsingLiteral ul (is, tail);
					vu.push_back (ul);
				}
			}
			break;
		case '\\':
		case '&':
		case '!':
			{
				UsingString us (is);
				vu.push_back (us);
			}
			break;
		default:
			{
				UsingLiteral ul (is);
				vu.push_back (ul);
			}
		}
	}
}

// End of using.cpp
