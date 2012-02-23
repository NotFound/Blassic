#ifndef INCLUDE_BLASSIC_CODELINE_H
#define INCLUDE_BLASSIC_CODELINE_H

//	codeline.h
// Revision 11-jul-2004

#include "blassic.h"
#include "keyword.h"

class CodeLine {
	const BlChar * strcontent;
	BlLineNumber linenumber;
	BlLineLength len;
	bool owner;
	BlLineLength pos;
	BlChunk chk;
	BlCode lastcode;
public:
	class Token {
	public:
		BlCode code;
		std::string str;
		BlInteger valueint;
		static BlNumber number (const std::string & str);
		BlNumber number () const;
		BlInteger integer () const { return valueint; }
		inline bool isendsentence () const
		{
			return code == ':' ||
				code == keyENDLINE ||
				code == keyELSE;
		}
	};

	CodeLine ();
	CodeLine (const BlChar * str, BlLineNumber number,
		BlLineLength length);
	CodeLine (const CodeLine & old);
	~CodeLine ();
	void assign (const BlChar * str, BlLineNumber number,
		BlLineLength length);
	CodeLine & operator= (const CodeLine & old);
	bool empty () const { return len == 0; }
	BlLineNumber number () const { return linenumber; }
	void setnumber (BlLineNumber n) { linenumber= n; }
	BlLineLength length () const { return len; }
	BlChunk chunk () const { return chk; }
	//BlChar * content () { return strcontent; }
	const BlChar * content () const { return strcontent; }
	BlCode actualcode () const { return lastcode; }
	Token getdata ();
	//Token gettoken ();
	void gettoken (Token & r);
	void gotochunk (BlChunk chknew);
	void scan (const std::string & line);
};

#endif

// Fin de codeline.h
