// mbf.cpp
// Revision 9-jul-2004

#include "mbf.h"
#include "error.h"

#include <math.h>

// For debugging:
#include <iostream>
#include <iomanip>

double mbf::mbf_s (const std::string & s)
{
	if (s.size () != 4)
		throw ErrFunctionCall;
	unsigned char b0= s [0], b1= s [1], b2= s [2], b3= s [3];
	double n= (b2 & 0x7F) | 0x80;
	n= 256 * n + b1;
	n= 256 * n + b0;
	short e= static_cast <short> (b3 - 128 - 24);
	if (e > 0)
		for ( ; e > 0; --e)
			n*= 2;
	else
		for ( ; e < 0; ++e)
			n/= 2;
	if (b2 & 0x80)
		n= -n;
	return n;
}

double mbf::mbf_d (const std::string & s)
{
	if (s.size () != 8)
		throw ErrFunctionCall;
	unsigned char b0= s [0], b1= s [1], b2= s [2], b3= s [3],
		b4= s [4], b5= s [5], b6= s [6], b7= s [7];
	double n= (b6 & 0x7F) | 0x80;
	n= 256 * n + b5;
	n= 256 * n + b4;
	n= 256 * n + b3;
	n= 256 * n + b2;
	n= 256 * n + b1;
	n= 256 * n + b0;
	short e= static_cast <short> (b7 - 128 - 56);
	if (e > 0)
		for ( ; e > 0; --e)
			n*= 2;
	else
		for ( ; e < 0; ++e)
			n/= 2;
	if (b6 & 0x80)
		n= -n;
	return n;
}

namespace {

double zero= 0.0;

} // namespace

std::string mbf::to_mbf_s (double v)
{
	bool negative= v < 0;
	if (negative)
		v= -v;
	int e= static_cast <int> (log (v) / log (2) );
	v/= pow (2, e - 23);
	unsigned long l= static_cast <unsigned long> (v);

	#if 0
	std::cerr << "e= " << std::dec << e <<
		" l= " << std::hex <<
		std::setw (8) << std::setfill ('0') <<
		l << std::endl;
	#endif

	unsigned char b3= static_cast <unsigned char> (e + 128 + 1);
	unsigned char b2= static_cast <unsigned char> (l / (256 * 256) );
	unsigned char b1= static_cast <unsigned char> ((l / 256) % 256);
	unsigned char b0= static_cast <unsigned char> (l % 256);
	b2&= 0x7F;
	if (negative)
		b2|= 0x80;
	#if 0
	std::cerr << std::setw (2) << int (b0) << int (b1) <<
		int (b2) << int (b3) << std::endl;
	#endif

	return std::string (1, char (b0) ) + char (b1) + char (b2) + char (b3);
}

std::string mbf::to_mbf_d (double v)
{
	bool negative= v < 0;
	if (negative)
		v= -v;
	int e= static_cast <int> (log (v) / log (2) );
	v/= pow (2, e - 55);
	//unsigned long l= static_cast <unsigned long> (v);
	double l;
	modf (v, & l);

	#if 0
	std::cerr << "e= " << std::dec << e <<
		" l= " << std::hex <<
		std::setw (8) << std::setfill ('0') <<
		l << std::endl;
	#endif

	unsigned char b7= static_cast <unsigned char> (e + 128 + 1);

	#if 0
	unsigned char b6= l / (256.0 * 256 * 256 * 256 * 256 * 256);
	unsigned char b5= (l / (256.0 * 256 * 256 * 256 * 256) ) % 256.0;
	unsigned char b4= (l / (256.0 * 256 * 256 * 256) ) % 256.0;
	unsigned char b3= (l / (256UL * 256 * 256) ) % 256.0;
	unsigned char b2= (l / (256 * 256) ) % 256.0;
	unsigned char b1= (l / 256) % 256.0;
	unsigned char b0= l % 256.0;
	#else
	double ll;
	modf (l / 256, & ll);
	unsigned char b0= static_cast <unsigned char> (l - 256 * ll);
	l= ll;
	modf (l / 256, & ll);
	unsigned char b1= static_cast <unsigned char> (l - 256 * ll);
	l= ll;
	modf (l / 256, & ll);
	unsigned char b2= static_cast <unsigned char> (l - 256 * ll);
	l= ll;
	modf (l / 256, & ll);
	unsigned char b3= static_cast <unsigned char> (l - 256 * ll);
	l= ll;
	modf (l / 256, & ll);
	unsigned char b4= static_cast <unsigned char> (l - 256 * ll);
	l= ll;
	modf (l / 256, & ll);
	unsigned char b5= static_cast <unsigned char> (l - 256 * ll);
	l= ll;
	unsigned char b6= static_cast <unsigned char> (l);
	#endif
	b6&= 0x7F;
	if (negative)
		b6|= 0x80;

	#if 0
	std::cerr << std::setw (2) << int (b0) << int (b1) <<
		int (b2) << int (b3) << std::endl;
	#endif

	return std::string (1, char (b0) ) + char (b1) + char (b2) +
		char (b3) + char (b4) + char (b5) + char (b6) + char (b7);
}

// End of mbf.cpp
