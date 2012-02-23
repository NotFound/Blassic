// function.cpp
// Revision 10-jul-2004

#include "function.h"
#include "error.h"

#include <vector>
#include <map>

class ParameterList::Internal {
public:
	static Internal * getnew ();
	void addref ();
	void delref ();
	void push_back (const std::string & name);
	size_t size () const;
	//const std::string & get (size_t n) const;
	std::string get (size_t n) const;
private:
	Internal ();
	Internal (const Internal &); // Forbidden
	~Internal () { }
	void operator= (const Internal &); // Forbidden
	size_t counter;
	std::vector <std::string> element;
};

ParameterList::Internal::Internal () :
	counter (1)
{
}

void ParameterList::Internal::addref ()
{
	++counter;
}

void ParameterList::Internal::delref ()
{
	if (--counter == 0)
		delete this;
}

ParameterList::Internal * ParameterList::Internal::getnew ()
{
	return new Internal;
}

void ParameterList::Internal::push_back (const std::string & name)
{
	element.push_back (name);
}

size_t ParameterList::Internal::size () const
{
	return element.size ();
}

//const std::string & ParameterList::Internal::get (size_t n) const
std::string ParameterList::Internal::get (size_t n) const
{
	if (n >= element.size () )
		throw ErrBlassicInternal;
	return element [n];
}

ParameterList::ParameterList () :
	pin (Internal::getnew () )
{
}

ParameterList::ParameterList (const ParameterList & pl) :
	pin (pl.pin)
{
	pin->addref ();
}

ParameterList::~ParameterList ()
{
	pin->delref ();
}

ParameterList & ParameterList::operator= (const ParameterList & pl)
{
	pl.pin->addref ();
	pin->delref ();
	pin= pl.pin;
	return * this;
}

void ParameterList::push_back (const std::string & name)
{
	pin->push_back (name);
}

size_t ParameterList::size () const
{
	return pin->size ();
}

//const std::string & ParameterList::operator [] (size_t n) const
std::string ParameterList::operator [] (size_t n) const
{
	return pin->get (n);
}

class Function::Internal {
public:
	void addref ();
	void delref ();
	static Internal * getnew
		(const std::string & strdef, const ParameterList & param)
	{
		return new Internal (strdef, param);
	}
	static Internal * getnew
		(ProgramPos posfn, const ParameterList & param)
	{
		return new Internal (posfn, param);
	}
	DefType getdeftype () const { return deftype; }
	CodeLine & getcode () { return code; }
	ProgramPos getpos () const { return pos; }
	const ParameterList & getparam () { return param; }
protected:
	// Protected to avoid a warning on certain versions of gcc.
	Internal (const std::string & strdef, const ParameterList & param);
	~Internal () { }
private:
	Internal (ProgramPos posfn, const ParameterList & param);
	Internal (const Internal &); // Forbidden
	void operator = (const Internal &); // Forbidden
	size_t counter;
	DefType deftype;
	CodeLine code;
	ProgramPos pos;
	ParameterList param;
};

Function::Internal::Internal
		(const std::string & strdef, const ParameterList & param) :
	counter (1),
	deftype (DefSingle),
	param (param)
{
	code.scan (strdef);
}

Function::Internal::Internal
		(ProgramPos posfn, const ParameterList & param) :
	counter (1),
	deftype (DefMulti),
	pos (posfn),
	param (param)
{
}

void Function::Internal::addref ()
{
	++counter;
}

void Function::Internal::delref ()
{
	if (--counter == 0)
		delete this;
}

Function::Function (const std::string & strdef, const ParameterList & param) :
	pin (Internal::getnew (strdef, param) )
{
}

Function::Function (ProgramPos posfn, const ParameterList & param) :
	pin (Internal::getnew (posfn, param) )
{
}

Function::Function (const Function & f) :
	pin (f.pin)
{
	pin->addref ();
}

Function::~Function ()
{
	pin->delref ();
}

Function & Function::operator= (const Function & f)
{
	f.pin->addref ();
	pin->delref ();
	pin= f.pin;
	return * this;
}

Function::DefType Function::getdeftype () const
{
	return pin->getdeftype ();
}

CodeLine & Function::getcode ()
{
	return pin->getcode ();
}

ProgramPos Function::getpos () const
{
	return pin->getpos ();
}

const ParameterList & Function::getparam ()
{
	return pin->getparam ();
}

namespace {

typedef std::map <std::string, Function> mapfunction_t;
mapfunction_t mapfunction;

} // namespace

void Function::clear ()
{
	mapfunction.clear ();
}

void Function::insert (const std::string & name)
{
	mapfunction_t::iterator it=
		mapfunction.find (name);
	if (it != mapfunction.end () )
		it->second= * this;
	else
		mapfunction.insert (std::make_pair (name, * this) );
}

Function & Function::get (const std::string & name)
{
	mapfunction_t::iterator it=
		mapfunction.find (name);
	if (it != mapfunction.end () )
		return it->second;
	throw ErrFunctionNoDefined;
}

// Fin de function.cpp
