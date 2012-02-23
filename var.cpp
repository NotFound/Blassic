// var.cpp
// Revision 7-feb-2005

//#define KEEP_IT_SIMPLE


#include "var.h"
#include "sysvar.h"
#include "error.h"
#include "util.h"

#include <vector>
#include <algorithm>
#include <stdexcept>

//#include <cctype>
//using std::toupper;

#include <iostream>
using std::cerr;
using std::endl;

#ifdef __BORLANDC__
#pragma warn -inl
#if __BORLANDC__ >= 0x0560
#pragma warn -8091
#endif
#endif

#ifndef USE_HASH_MAP

#include <map>
#define MAP std::map

#else

#if __GNUC__ < 3
#include <hash_map>
#define N_MAP std
#else
#include <ext/hash_map>
#define N_MAP __gnu_cxx
#endif

#define MAP N_MAP::hash_map

namespace N_MAP {

template <> struct hash <std::string>
{
	hash () : hashstr (hash <const char *> () ) { }
	size_t operator () (const std::string & str) const
	{ return hashstr (str.c_str () ); }
private:
	hash <const char *> hashstr;
};

} // namespace N_MAP

#endif

namespace sysvar= blassic::sysvar;


namespace {

VarType tabletype [27];

bool inittabletype ()
{
	std::fill_n (tabletype, util::dim_array (tabletype), VarNumber);
	return true;
}

bool table_inited= inittabletype ();

}

VarType typeofvar (const std::string & name)
{
	switch (name [name.size () - 1] )
	{
	case '#':
		// There is no single or double distiction.
		return VarNumber;
	case '!':
		return VarNumber;
	case '%':
		return VarInteger;
	case '$':
		return VarString;
	default:
		//return tabletype [toupper (name [0]) - 'A'];
		return tabletype [name [0] - 'A'];
	}
}

void definevar (VarType type, char c)
{
	//tabletype [toupper (c) - 'A']= type;
	tabletype [c - 'A']= type;
}

void definevar (VarType type, char cfrom, char cto)
{
	//size_t from= toupper (cfrom) - 'A';
	//size_t to= toupper (cto) - 'A';
	size_t from= cfrom - 'A';
	size_t to= cto - 'A';
	std::fill (tabletype + from, tabletype + to + 1, type);
}

namespace {

template <char c>
inline std::string stripvar (const std::string n)
{
	const std::string::size_type l= n.size () - 1;
	if (n [l] == c)
		return n.substr (0, l);
	return n;
}

inline std::string stripvarnumber (const std::string & n)
{
	//return stripvar <'!'> (n);
	const std::string::size_type l= n.size () - 1;
	if (n [l] == '!' || n [l] == '#')
		return n.substr (0, l);
	return n;
}

inline std::string stripvarinteger (const std::string & n)
{
	return stripvar <'%'> (n);
}

inline std::string stripvarstring (const std::string & n)
{
	return stripvar <'$'> (n);
}

template <class C>
inline void initnewvar (C &)
{ }

template <>
inline void initnewvar (BlNumber & n)
{ n= BlNumber (); }
template <>
inline void initnewvar (BlInteger & n)
{ n= BlInteger (); }

#ifndef KEEP_IT_SIMPLE

template <class C>
class Table {
	static const size_t chunk_size= 512;
	std::vector <C *> vc;
	size_t n;
	static void clearchunk (C * pc);
public:
	Table ();
	~Table ();
	void clear ();
	C * newvar ();
};

template <class C>
Table<C>::Table () :
	n (0)
{ }

template <class C>
Table<C>::~Table ()
{
	clear ();
}

template <class C>
void Table<C>::clearchunk (C * pc)
{
	delete [] pc;
}

template <class C>
void Table<C>::clear ()
{
	std::for_each (vc.begin (), vc.end (), clearchunk);
	vc.clear ();
	n= 0;
}

template <class C>
C * Table<C>::newvar ()
{
	size_t nelem= n % chunk_size;
	if (nelem == 0)
	{
		C * pnc= new C [chunk_size];
		vc.push_back (pnc);
	}
	size_t nchunk= n / chunk_size;
	++n;
	return & vc [nchunk] [nelem];
}

Table <BlNumber> tablenumber;
Table <BlInteger> tableinteger;
Table <std::string> tablestring;

template <class C>
inline Table <C> & table ();

template <>
inline Table <BlNumber> & table <BlNumber> ()
{ return tablenumber; }
template <>
inline Table <BlInteger> & table <BlInteger> ()
{ return tableinteger; }
template <>
inline Table <std::string> & table <std::string> ()
{ return tablestring; }

MAP <std::string, BlNumber *> numvar;
MAP <std::string, BlInteger *> integervar;
MAP <std::string, std::string *> stringvar;

template <class C>
inline MAP <std::string, C *> & mapvar ();

template <>
inline MAP <std::string, BlNumber *> & mapvar <BlNumber> ()
{ return numvar; }
template <>
inline MAP <std::string, BlInteger *> & mapvar <BlInteger> ()
{ return integervar; }
template <>
inline MAP <std::string, std::string *> & mapvar <std::string> ()
{ return stringvar; }

template <class C>
inline C * getaddr (const std::string name)
{
	C * addr= mapvar <C> () [name];
	if (! addr)
	{
		addr= table <C> ().newvar ();
		mapvar <C> () [name]= addr;
		initnewvar (* addr);
	}
	return addr;
}

template <class C>
inline void assignvar (const std::string & n, const C & value)
{
	C * pc= getaddr <C> (n);
	* pc= value;
}

template <class C>
inline C evaluatevar (const std::string & n)
{
	C * pc= getaddr <C> (n);
	return * pc;
}

#else
// Keep it Simple.

MAP <std::string, BlNumber> varnumber;
MAP <std::string, BlInteger> varinteger;
MAP <std::string, std::string> varstring;

#endif


} // namespace

void assignvarnumber (const std::string & name, BlNumber value)
{
	#ifndef KEEP_IT_SIMPLE
	//assignvar (name, value);
	// Ops, I forget to strip here.
	assignvar (stripvarnumber (name), value);
	#else
	varnumber [stripvarnumber (name) ]= value;
	#endif
}

void assignvarinteger (const std::string & name, BlInteger value)
{
	#ifndef KEEP_IT_SIMPLE
	assignvar (stripvarinteger (name), value);
	#else
	varinteger [stripvarinteger (name) ]= value;
	#endif
}

void assignvarstring (const std::string & name, const std::string & value)
{
	#ifndef KEEP_IT_SIMPLE
	assignvar (stripvarstring (name), value);
	#else
	varstring [stripvarstring (name) ]= value;
	#endif
}

BlNumber evaluatevarnumber (const std::string & name)
{
	#ifndef KEEP_IT_SIMPLE
	//return evaluatevar <BlNumber> (name);
	// Ops, I forget to strip here.
	return evaluatevar <BlNumber> (stripvarnumber (name) );
	#else
	return varnumber [stripvarnumber (name) ];
	#endif
}

BlInteger evaluatevarinteger (const std::string & name)
{
	#ifndef KEEP_IT_SIMPLE
	return evaluatevar <BlInteger> (stripvarinteger (name) );
	#else
	return varinteger [stripvarinteger (name) ];
	#endif
}

std::string evaluatevarstring (const std::string & name)
{
	#ifndef KEEP_IT_SIMPLE
	return evaluatevar <std::string> (stripvarstring (name) );
	#else
	return varstring [stripvarstring (name) ];
	#endif
}

BlNumber * addrvarnumber (const std::string & name)
{
	#ifndef KEEP_IT_SIMPLE
	//return getaddr <BlNumber> (name);
	// Ops, I forget to strip here.
	return getaddr <BlNumber> (stripvarnumber (name) );
	#else
	return & varnumber [stripvarnumber (name) ];
	#endif
}

BlInteger * addrvarinteger (const std::string & name)
{
	#ifndef KEEP_IT_SIMPLE
	return getaddr <BlInteger> (stripvarinteger (name) );
	#else
	return & varinteger [stripvarinteger (name) ];
	#endif
}

std::string * addrvarstring (const std::string & name)
{
	#ifndef KEEP_IT_SIMPLE
	return getaddr <std::string> (stripvarstring (name) );
	#else
	return & varstring [stripvarstring (name) ];
	#endif
}

//*********************************************************
//                    ARRAYS
//*********************************************************

namespace {

template <class C>
class Array {
public:
	Array (const Dimension & nd);
	Array (); // Default constructor required for map []
	Array (const Array & a);
	~Array ();
	void operator = (const Array & a);
	const Dimension & dim () const { return d; }
	C * getvalue (size_t n) { return value + n; }
private:
	Dimension d;
	size_t * pcount;
	C * value;
	void addref ();
	void delref ();
};

template <class C>
Array <C>::Array (const Dimension & nd) :
	d (nd),
	pcount (new size_t),
	value (new C [nd.elements () ] )
{
	if (value == NULL)
		throw ErrOutMemory;
	* pcount= 1;
	std::for_each (value, value + nd.elements (),
		initnewvar <C> );
}

template <class C>
Array <C>::Array () : // Default constructor required for map []
	pcount (new size_t),
	value (new C [0] )
{
	* pcount= 1;
}

template <class C>
Array <C>::Array (const Array & a) :
	d (a.d),
	pcount (a.pcount),
	value (a.value)
{
	addref ();
}

template <class C>
Array <C>::~Array ()
{
	delref ();
}

template <class C>
void Array <C>::operator = (const Array & a)
{
	if (this != & a)
	{
		delref ();
		d= a.d;
		pcount= a.pcount;
		value= a.value;
		addref ();
	}
}

template <class C>
void Array <C>::addref ()
{
	++ (* pcount);
}

template <class C>
void Array <C>::delref ()
{
	if (-- (* pcount) == 0)
	{
		//cerr << "Array of dim " << d << " deleted" << endl;
		delete [] value;
		delete pcount;
	}
}

#if 0
template <class C>
inline Array <C> makeArray (const Dimension & nd, C * nvalue)
{
	return Array <C> (nd, nvalue);
}
#endif

template <class C>
struct ArrayVar {
	typedef MAP <std::string, Array <C> > map;
	typedef typename map::iterator iterator;
	typedef typename map::const_iterator const_iterator;
	typedef typename map::value_type value_type;
};

MAP <std::string, Array <BlNumber> > arrayvarnumber;
MAP <std::string, Array <BlInteger> > arrayvarinteger;
MAP <std::string, Array <std::string> > arrayvarstring;

template <class C>
inline typename ArrayVar <C>::map & arrayvar ();
// Se necesita como plantilla para usarlo en otras plantillas.
// Lo definimos solamente para los tipos usados.

template <>
inline ArrayVar <BlNumber>::map &
	arrayvar <BlNumber> ()
{ return arrayvarnumber; }
template <>
inline ArrayVar <BlInteger>::map &
	arrayvar <BlInteger> ()
{ return arrayvarinteger; }
template <>
inline ArrayVar<std::string>::map &
	arrayvar <std::string> ()
{ return arrayvarstring; }

template <class C>
inline void dimvar (const std::string & name, const Dimension & d)
{
	typename ArrayVar <C>::iterator it= arrayvar <C> ().find (name),
		end= arrayvar <C> ().end ();
	//if (arrayvar <C> ().find (name) != arrayvar <C> ().end () )
	//	throw ErrAlreadyDim;
	if (it != end && sysvar::get (sysvar::TypeOfDimCheck) == 0)
		throw ErrAlreadyDim;
	#if 0
	size_t n= d.elements ();
	util::auto_buffer <C> value (n);
	arrayvar <C> () [name]= makeArray (d, value.data () );
	#else
	//Array <C> a (d);
	//arrayvar <C> () [name]= a;
	arrayvar <C> () [name]= Array <C> (d);
	#endif
	//std::for_each (value.begin (), value.end (), initnewvar <C> );
	//value.release ();
}

template <class C>
inline void erasevar (const std::string & name)
{
	typename ArrayVar <C>::const_iterator it=
		arrayvar <C> ().find (name);
	if (it == arrayvar <C> ().end () )
		throw ErrFunctionCall;
	arrayvar <C> ().erase (name);
	//arrayvar <C> ().erase (* it);
}

Dimension defaultdimension (const Dimension & d)
{
	Dimension n;
	for (size_t i= 0, l= d.size (); i < l; ++i)
		n.add (10);
	return n;
}

template <class C>
void createdefault (const std::string & name, const Dimension & d)
{
	Dimension n= defaultdimension (d);
	dimvar<C> (name, n);
}

template <class C>
inline C * addrdim (const std::string & name, const Dimension & d)
{
	typename ArrayVar <C>::iterator it=
		arrayvar <C> ().find (name);
	if (it == arrayvar <C> ().end () )
	{
		createdefault <C> (name, d);
		it= arrayvar <C> ().find (name);
		if (it == arrayvar <C> ().end () )
		{
			if (showdebuginfo () )
				cerr << "Default creation of array failed" <<
					endl;
			throw ErrBlassicInternal;
		}
	}
	size_t n= it->second.dim ().evalpos (d);
	//return it->second.value + n;
	return it->second.getvalue (n);
}

template <class C>
inline C valuedim (const std::string & name, const Dimension & d)
{
	return * addrdim <C> (name, d);
}

template <class C>
inline void assigndim (const std::string & name,
	const Dimension & d, const C & result)
{
	* addrdim <C> (name, d)= result;
}

} // namespace

void dimvarnumber (const std::string & name, const Dimension & d)
{
	dimvar <BlNumber> (stripvarnumber (name), d);
}

void dimvarinteger (const std::string & name, const Dimension & d)
{
	dimvar <BlInteger> (stripvarinteger (name), d);
}

void dimvarstring (const std::string & name, const Dimension & d)
{
	dimvar <std::string> (stripvarstring (name), d);
}

void erasevarnumber (const std::string & name)
{
	erasevar <BlNumber> (stripvarnumber (name) );
}

void erasevarinteger (const std::string & name)
{
	erasevar <BlInteger> (stripvarinteger (name) );
}

void erasevarstring (const std::string & name)
{
	erasevar <std::string> (stripvarstring (name) );
}

BlNumber valuedimnumber (const std::string & name, const Dimension & d)
{
	return valuedim <BlNumber> (stripvarnumber (name), d);
}

BlInteger valuediminteger (const std::string & name, const Dimension & d)
{
	return valuedim <BlInteger> (stripvarinteger (name), d);
}

std::string valuedimstring (const std::string & name, const Dimension & d)
{
	return valuedim <std::string> (stripvarstring (name), d);
}

void assigndimnumber (const std::string & name, const Dimension & d,
	BlNumber result)
{
	assigndim (stripvarnumber (name), d, result);
}

void assigndiminteger (const std::string & name, const Dimension & d,
	BlInteger result)
{
	assigndim (stripvarinteger (name), d, result);
}

void assigndimstring (const std::string & name, const Dimension & d,
	const std::string & result)
{
	assigndim (stripvarstring (name), d, result);
}

BlNumber * addrdimnumber (const std::string & name, const Dimension & d)
{
	return addrdim <BlNumber> (stripvarnumber (name), d);
}

BlInteger * addrdiminteger (const std::string & name, const Dimension & d)
{
	return addrdim <BlInteger> (stripvarinteger (name), d);
}

std::string * addrdimstring (const std::string & name, const Dimension & d)
{
	return addrdim <std::string> (stripvarstring (name), d);
}

//**********************************************************
//		Borrado de variables
//**********************************************************

namespace {

#if 0
template <class C>
class FreeArray {
public:
	void operator ()
		(const typename ArrayVar <C>::value_type & var)
	{
		delete [] var.second.value;
	}
};
#endif

template <class C>
inline void cleararray ()
{
	//std::for_each (arrayvar <C> ().begin (), arrayvar <C> ().end (),
	//	FreeArray<C> () );
	arrayvar <C> ().clear ();
}

#ifndef KEEP_IT_SIMPLE

template <class C>
void clear ()
{
	table <C> ().clear ();
	mapvar <C> ().clear ();
}

#endif

} // namespace

void clearvars ()
{
	#ifndef KEEP_IT_SIMPLE
	clear <BlNumber> ();
	clear <BlInteger> ();
	clear <std::string> ();
	#else
	varnumber.clear ();
	varinteger.clear ();
	varstring.clear ();
	#endif

	cleararray <BlNumber> ();
	cleararray <BlInteger> ();
	cleararray <std::string> ();
}

// Fin de var.cpp
