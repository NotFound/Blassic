#ifndef INCLUDE_UTIL_H
#define INCLUDE_UTIL_H

// util.h
// Revision 24-apr-2009

#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include <stdlib.h>

#ifdef __BORLANDC__
#pragma warn -8027
#endif

namespace util {

inline std::string stringlset (const std::string & value,
	std::string::size_type l)
{
	const std::string::size_type lvalue= value.size ();
	if (lvalue < l)
		return value + std::string (l - lvalue, ' ');
	else if (lvalue > l)
		return value.substr (0, l);
	else
		return value;
}

inline std::string stringrset (const std::string & value,
	std::string::size_type l)
{
	const std::string::size_type lvalue= value.size ();
	if (lvalue < l)
		return std::string (l - lvalue, ' ') + value;
	else if (lvalue > l)
		return value.substr (0, l);
	else
		return value;
}

template <class DEST, class ORG, class EX>
DEST checked_cast (ORG org, EX ex)
{
	DEST dest= static_cast <DEST> (org);
	if (static_cast <ORG> (dest) != org)
		throw ex;
	return dest;
}

template <class C, size_t N>
size_t dim_array (C (&) [N])
{ return N; }

template <class C, size_t N>
size_t dim_array (const C (&) [N])
{ return N; }

template <class C>
class auto_buffer
{
public:
	auto_buffer (size_t ns) :
		p (new C [ns]),
		s (ns)
	{ }
	auto_buffer () : p (0) { }
	~auto_buffer ()
	{ delete [] p; }
	void release () { p= 0; }
	void alloc (size_t ns)
	{
		delete [] p;
		p= new C [ns];
		s= ns;
	}
	operator C * () { return p; }
	typedef C * iterator;
	iterator begin () { return p; }
	iterator end () { return p + s; }
	// We define data for commodity, and better legibility
	// than begin where not used as iterator.
	C * data () { return p; }
private:
	auto_buffer (const auto_buffer &); // Forbidden
	auto_buffer & operator = (const auto_buffer &); // Forbidden
	C * p;
	size_t s;
};

template <class C>
class auto_alloc
{
public:
	auto_alloc (size_t size) :
		p (static_cast <C *> (malloc (size * sizeof (C) ) ) )
	{
		if (p == 0)
			throw std::bad_alloc ();
	}
	~auto_alloc ()
	{ free (p); }
	void release () { p= 0; }
	operator C * () { return p; }
	C * data () { return p; } // Commodity when cast is needed
private:
	auto_alloc (const auto_alloc &); // Forbidden
	auto_alloc & operator = (const auto_alloc &); // Forbidden
	C * p;
};

// This template is intended for use as a pointer to a
// private implementation that translates the constness of
// the operations in the main class to the implementation.

template <class C>
class pimpl_ptr
{
public:
	pimpl_ptr (C * ptr) :
		p (ptr)
	{ }
	~pimpl_ptr ()
	{
		delete p;
	}

	// Bypass the acces control
	C * get () const
	{ return p; }

	// The dereference operator that preserves constness.
	C & operator * ()
	{ return * p; }
	const C & operator * () const
	{ return * p; }
	C * operator -> ()
	{ return p; }
	const C * operator -> () const
	{
		return p;
	}
private:
	pimpl_ptr (const pimpl_ptr &); // Forbidden
	pimpl_ptr & operator = (const pimpl_ptr &); // Forbidden
	C * const p;
};

template <class C>
std::string to_string (const C & c)
{
	std::ostringstream oss;
	oss << c;
	return oss.str ();
}

// Functions used to avoid warnings about unused parameters.

template <class T>
inline void touch (const T & t)
{
	(void) t;
}
template <class T1, class T2>
inline void touch (const T1 & t1, const T2 & t2)
{
	(void) t1; (void) t2;
}
template <class T1, class T2, class T3>
inline void touch (const T1 & t1, const T2 & t2, const T3 & t3)
{
	(void) t1; (void) t2; (void) t3;
}
template <class T1, class T2, class T3, class T4>
inline void touch (const T1 & t1, const T2 & t2, const T3 & t3,
	const T4 & t4)
{
	(void) t1; (void) t2; (void) t3;
	(void) t4;
}
template <class T1, class T2, class T3, class T4, class T5>
inline void touch (const T1 & t1, const T2 & t2, const T3 & t3,
	const T4 & t4, const T5 & t5)
{
	(void) t1; (void) t2; (void) t3;
	(void) t4; (void) t5;
}
template <class T1, class T2, class T3, class T4, class T5, class T6>
inline void touch (const T1 & t1, const T2 & t2, const T3 & t3,
	const T4 & t4, const T5 & t5, const T6 & t6)
{
	(void) t1; (void) t2; (void) t3;
	(void) t4; (void) t5; (void) t6;
}

} // namespace

#endif

// End of util.h
