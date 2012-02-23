#ifndef INCLUDE_BLASSIC_TOKEN_H
#define INCLUDE_BLASSIC_TOKEN_H

// token.h
// Revision 7-feb-2005


#include "blassic.h"

#include <string>


class Tokenizer {
public:
	enum Type { Empty, EndLine, Blank, Plain, Literal, Integer };
	Tokenizer (const std::string & source);

	// Members of token are public accessible for simplicity.
	// Use with care.
	class Token {
	public:
		Token () :
			type (Empty)
		{ }
		Token (Type t) :
			type (t)
		{ }
		Token (Type t, const std::string & str) :
			type (t), str (str)
		{ }
		Token (BlInteger n) :
			type (Integer), n (n)
		{ }
		Type type;
		std::string str;
		BlInteger n;
	};
	Token get ();
	std::string getrest ();
	unsigned char peek ();
private:
	unsigned char peeknospace ();
	unsigned char nextchar ();
	unsigned char nextcharnospace ();
	void ungetchar ();
	std::string str;
	std::string::size_type pos;
	const std::string::size_type limit;
};

#endif

// Fin de token.h
