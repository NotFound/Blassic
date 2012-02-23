#ifndef INCLUDE_ELEMENT_H
#define INCLUDE_ELEMENT_H

// element.h
// Revision 1-jan-2005

#include "blassic.h"

#include <string>

class Element {
public:
	Element (ProgramPos ppos) :
		ppos (ppos)
	{ }
	void nextchunk () { ppos.nextchunk (); }
	void nextline () { ppos.nextline (); }
	ProgramPos getpos () const { return ppos; }
private:
	ProgramPos ppos;
};

class ForElement : public Element {
public:
	ForElement (const std::string & nvar, ProgramPos pos) :
		Element (pos),
		varname (nvar)
	{ }
	virtual ~ForElement () { }
	virtual bool next ()= 0;
	const std::string var () const
	{ return varname; }
	bool isvar (const std::string & nvar) const
	{ return varname == nvar; }
private:
	ForElement (const ForElement &); // Forbidden
	ForElement & operator = (const ForElement &); // Forbidden
	const std::string varname;
};

class ForElementNumber : public ForElement {
public:
	ForElementNumber (const std::string & nvar,
			ProgramPos pos,
			BlNumber initial, BlNumber nmax, BlNumber nstep);
protected:
	BlNumber * varaddr, max, step;
};

class ForElementNumberInc : public ForElementNumber {
public:
	ForElementNumberInc (const std::string & var, ProgramPos pos,
			BlNumber initial, BlNumber max, BlNumber step);
	bool next ();
};

class ForElementNumberDec : public ForElementNumber {
public:
	ForElementNumberDec (const std::string & var, ProgramPos pos,
			BlNumber initial, BlNumber max, BlNumber step);
	bool next ();
};

class ForElementInteger : public ForElement {
public:
	ForElementInteger (const std::string & nvar,
			ProgramPos pos,
			BlInteger initial, BlInteger nmax, BlInteger nstep);
protected:
	BlInteger * varaddr, max, step;
};

class ForElementIntegerInc : public ForElementInteger {
public:
	ForElementIntegerInc (const std::string & var, ProgramPos pos,
			BlInteger initial, BlInteger max, BlInteger step);
	bool next ();
};

class ForElementIntegerDec : public ForElementInteger {
public:
	ForElementIntegerDec (const std::string & var, ProgramPos pos,
			BlInteger initial, BlInteger max, BlInteger step);
	bool next ();
};

inline ForElementNumber * newForElementNumber (const std::string & var,
	ProgramPos pos, BlNumber initial, BlNumber max, BlNumber step)
{
	if (step >= 0.0)
		return new ForElementNumberInc (var, pos, initial, max, step);
	else
		return new ForElementNumberDec (var, pos, initial, max, step);
}

inline ForElementInteger * newForElementInteger (const std::string & var,
	ProgramPos pos, BlInteger initial, BlInteger max, BlInteger step)
{
	if (step >= 0)
		return new ForElementIntegerInc (var, pos, initial, max, step);
	else
		return new ForElementIntegerDec (var, pos, initial, max, step);
}

class RepeatElement : public Element {
public:
	RepeatElement (ProgramPos pos) :
		Element (pos)
	{ }
};

class WhileElement : public Element {
public:
	WhileElement (ProgramPos pos) :
		Element (pos)
	{ }
};

class LocalLevel {
public:
	LocalLevel ();
	LocalLevel (const LocalLevel & ll);
	~LocalLevel ();
	LocalLevel & operator= (const LocalLevel & ll);
	void addlocalvar (const std::string & name);
	void freelocalvars ();
private:
	class Internal;
	Internal * pi;
};

class GosubElement : public Element, public LocalLevel {
public:
	GosubElement (ProgramPos pos, bool is_polled) :
		Element (pos),
		is_gosub (true),
		is_polled (is_polled)
	{ }
	GosubElement (LocalLevel & ll) :
		Element (0),
		LocalLevel (ll),
		is_gosub (false),
		is_polled (false)
	{ }
	bool isgosub () const { return is_gosub; }
	bool ispolled () const { return is_polled; }
private:
	bool is_gosub;
	bool is_polled;
};


#endif

// End of element.h
