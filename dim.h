#ifndef INCLUDE_BLASSIC_DIM_H
#define INCLUDE_BLASSIC_DIM_H

// dim.h
// Revision 6-jul-2004

#include <iostream>
#include <cstddef>
#include <vector>

class Dimension {
public:
	void add (size_t n) { dim.push_back (n); }
	size_t size () const { return dim.size (); }
	bool empty () const { return dim.empty (); }
	size_t elements () const;
	size_t operator [] (size_t n) const { return dim [n]; }
	size_t evalpos (const Dimension & d) const;
	bool operator == (const Dimension & d) const
	{ return dim == d.dim; }
private:
	std::vector <size_t> dim;
};

std::ostream & operator << (std::ostream & os, const Dimension & d);

#endif

// Fin de dim.h
