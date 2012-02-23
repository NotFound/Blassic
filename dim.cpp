// dim.cpp
// Revision 14-oct-2003

#include "dim.h"
#include "error.h"

#include <algorithm>

class dim_calc {
public:
	dim_calc () : r (1) { }
	void operator () (const size_t n)
	{ r*= n + 1; }
	size_t value () { return r; }
private:
	size_t r;
};

size_t Dimension::elements () const
{
	dim_calc r= std::for_each (dim.begin (), dim.end (), dim_calc () );
	return r.value ();
}

size_t Dimension::evalpos (const Dimension & d) const
{
	size_t n= size ();
	if (d.size () != n)
		throw ErrBadSubscript;
	size_t pos= d [0];
	if (pos > dim [0])
		throw ErrBadSubscript;
	for (size_t i= 1; i < n; ++i)
	{
		if (d [i] > dim [i] )
			throw ErrBadSubscript;
		pos*= dim [i] + 1;
		pos+= d [i];
	}
	return pos;
}

// Only for debug.

std::ostream & operator << (std::ostream & os, const Dimension & d)
{
	size_t s= d.size ();
	if (s == 0)
		os << "(empty)";
	else
	{
		os << '(';
		for (size_t i= 0; i < s; ++i)
		{
			os << d [i];
			if (i < s -1)
				os << ", ";
		}
		os << ')';
	}
	return os;
}

// Fin de dim.cpp
