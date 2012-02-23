// token.cpp
// Revision 25-jul-2004

#include "token.h"
#include <cctype>
#include <sstream>

using std::isalpha;
using std::isdigit;
using std::isxdigit;
using std::isspace;

Tokenizer::Tokenizer (const std::string & source) :
	str (source), pos (0), limit (source.size () )
{
}

unsigned char Tokenizer::peek ()
{
	if (pos < limit)
		return str [pos];
	else
		return '\0';
}

unsigned char Tokenizer::peeknospace ()
{
	unsigned char c= '\0';
	std::string::size_type p= pos;
	while (p < limit && (c= str [p] ) == ' ')
		++p;
	if (p < limit)
		return c;
	else
		return '\0';
}

unsigned char Tokenizer::nextchar ()
{
	if (pos < limit)
		return str [pos++];
	else
		return '\0';
}

unsigned char Tokenizer::nextcharnospace ()
{
	unsigned char c= '\0';
	while (pos < limit && (c= str [pos] ) == ' ')
		++pos;
	if (pos < limit)
	{
		++pos;
		return c;
	}
	else
		return '\0';
}

void Tokenizer::ungetchar ()
{
	if (pos > 0)
		--pos;
}

//namespace {

inline bool isident (char c)
{
	return isalpha (c) || isdigit (c) || c == '_';
}

//}

Tokenizer::Token Tokenizer::get ()
{
	unsigned char c= nextchar ();
	if (c == '\0')
		return Token (EndLine);

	std::string str;
	if (isspace (c) )
	{
		do {
			str+= c;
		} while (isspace (c= nextchar () ) );
		if (c != '\0')
			ungetchar ();
		return Token (Blank, str);
	}

	switch (c)
	{
	case '"':
		//str+= c;
		while ( (c= nextchar () ) != '\0')
		{
			if (c == '"')
			{
				c= peek ();
				if (c != '"')
					break;
				nextchar ();
			}
			str+= c;
		}
		return Token (Literal, str);
	case '&':
		// Hex, octal or binary number
		str= '&';
		c= nextchar ();
		if (c == 'x' || c == 'X') // Binary
		{
			str+= c;
			while ( (c= nextchar () ) == '0' || c == '1')
				str+= c;
			if (c != 0)
				ungetchar ();
		}
		else if (c == 'o' || c == 'O') // Octal
		{
			str+= c;
			while ( (c= nextchar () ) >= '0' && c <= '7')
				str+= c;
			if (c != 0)
				ungetchar ();
		}
		else // Hexadecimal
		{
			if (c == 'h' || c == 'H')
			{
				str+= c;
				c= nextchar ();
			}
			while (isxdigit (c) )
			{
				str+= c;
				c= nextchar ();
			}
			if (c != 0)
				ungetchar ();
		}
		return Token (Plain, str);
	case '=':
		{
			str= c;
			c= peeknospace ();
			if (c == '>' || c == '<')
			{
				str+= c;
				(void) nextcharnospace ();
			}
		}
		return Token (Plain, str);
	case '<':
		str= c;
		c= peeknospace ();
		if (c == '=' || c == '>')
		{
			str+= c;
			(void) nextcharnospace ();
		}
		return Token (Plain, str);
	case '>':
		str= c;
		c= peeknospace ();
		if (c == '=' || c == '<')
		{
			str+= c;
			(void) nextcharnospace ();
		}
		return Token (Plain, str);
	default:
		; // Later.
	}

	if (isalpha (c) )
	{
		do
		{
			str+= c;
		} while ( isident (c= nextchar () ) );
		if (c == '$')
			str+= c;
		else
			if (c != '\0')
				ungetchar ();
		return Token (Plain, str);
	}

	if (isdigit (c) || c == '.')
	{
		bool nofloat= true;
		while (isdigit (c) )
		{
			str+= c;
			c= nextchar ();
		}
		if (c == '.')
		{
			str+= '.';
			nofloat= false;
			while ( isdigit (c= nextchar () ) )
				str+= c;
		}
		if (c == 'e' || c == 'E')
		{
			str+= c;
			nofloat= false;
			c= nextchar ();
			if (! isdigit (c) && c != '+' && c != '-')
			{
				// Data such as 1E
				while (isident (c) )
				{
					str+= c;
					c= nextchar ();
				}
				if (c != 0)
					ungetchar ();
				return Token (Plain, str);
			}
			if (c == '+' || c == '-')
			{
				str+= c;
				c= nextchar ();
			}
			while (isdigit (c) )
			{
				str+= c;
				c= nextchar ();
			}
		}
		if (c != 0)
		{
			if (isident (c) )
			{
				do {
					str+= c;
					c= nextchar ();
				} while (isident (c) );
				if (c != 0)
					ungetchar ();
				return Token (Plain, str);
			}
			ungetchar ();
		}
		if (nofloat)
		{
			#if 0
			std::istringstream iss (str);
			BlInteger n;
			iss >> n;
			if (iss)
			{
				iss.get ();
				//if (! iss)
				if (iss.eof () )
					return Token (n);
			}
			#else
			BlInteger n= 0;
			std::string::size_type i;
			const std::string::size_type l= str.size ();
			for (i= 0; i < l; ++i)
			{
				BlInteger digit= str [i] - '0';
				if (n > (BlIntegerMax / 10) )
					break;
				n*= 10;
				if (n > BlIntegerMax - digit)
					break;
				n+= digit;
			}
			if (i == l)
				return Token (n);
			#endif
		}
		return Token (Plain, str);
	}

	// If nothing else:
	str= c;
	return Token (Plain, str);
}

std::string Tokenizer::getrest ()
{
	std::string r;
	char c;
	while ( (c= nextchar () ) != 0)
	{
		if (c == '"')
		{
			r+= c;
			while ( (c= nextchar () ) != '\0')
			{
				if (c == '"')
				{
					c= peek ();
					if (c != '"')
						break;
					nextchar ();
				}
				r+= c;
			}
			r+= '\0';
		}
		else
			r+= c;
	}
	return r;
}

// Fin de token.cpp
