#ifndef INCLUDE_BLASSIC_FUNCTION_H
#define INCLUDE_BLASSIC_FUNCTION_H

// function.h
// Revision 7-feb-2005


#include "codeline.h"

#include <string>


class ParameterList {
public:
	ParameterList ();
	ParameterList (const ParameterList & pl);
	~ParameterList ();
	ParameterList & operator= (const ParameterList & pl);
	void push_back (const std::string & name);
	size_t size () const;
	//const std::string & operator [] (size_t n) const;
	std::string operator [] (size_t n) const;
private:
	class Internal;
	Internal * pin;
};

class Function {
public:
	enum DefType { DefSingle, DefMulti };
	Function (const std::string & strdef, const ParameterList & param);
	Function (ProgramPos posfn, const ParameterList & param);
	Function (const Function & f);
	~Function ();
	Function & operator= (const Function &);
	DefType getdeftype () const;
	CodeLine & getcode ();
	ProgramPos getpos () const;
	const ParameterList & getparam ();
	static void clear ();
	void insert (const std::string & name);
	static Function & get (const std::string & name);
private:
	class Internal;
	Internal * pin;
};

#endif

// Fin de function.h
