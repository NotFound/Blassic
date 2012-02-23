#ifndef INCLUDE_BLASSIC_RESULT_H
#define INCLUDE_BLASSIC_RESULT_H

// result.h
// Revision 8-jan-2005

#ifdef __BORLANDC__
#pragma warn -inl
#endif

#include "blassic.h"

#include "error.h"

#include <math.h>

// Now the aritmetic operations between integers are converted
// to floating point, to avoid truncations on integer overflow.
// Comment the next #define to get the old behaviour.

#define CONVERTNUMBER

namespace blassic {

namespace result {

inline double round (double n)
{
	double intpart;
	double fracpart= modf (n, & intpart);
	if (fracpart >= 0.5)
		return intpart + 1;
	else if (fracpart <= -0.5)
		return intpart - 1;
	else
		return intpart;
}

inline BlInteger NumberToInteger (BlNumber n)
{
	n= round (n);
	if (n > BlIntegerMax || n < BlIntegerMin)
		throw ErrOverflow;
	return static_cast <BlInteger> (n);
}

class BlResult {
	VarType vartype;
	std::string varstr;
	union N {
		BlNumber varnumber;
		BlInteger varinteger;
		N () { }
		N (BlNumber num)
		{
			varnumber= num;
		}
		N (BlInteger inum)
		{
			varinteger= inum;
		}
	} n;
	void integertonumber ()
	{
		vartype= VarNumber;
		n.varnumber= n.varinteger;
	}
	void numbertointeger ()
	{
		vartype= VarInteger;
		//n.varinteger= BlInteger (n.varnumber);
		n.varinteger= NumberToInteger (n.varnumber);
	}
public:
	BlResult () :
		vartype (VarUndef)
	{ }
	BlResult (BlNumber num) :
		vartype (VarNumber),
		n (num)
	{ }
	BlResult (BlInteger inum) :
		vartype (VarInteger),
		n (inum)
	{ }
	BlResult (const std::string & str) :
		vartype (VarString),
		varstr (str)
	{ }
	BlResult (const BlResult & br) :
		vartype (br.vartype)
	{
		switch (vartype)
		{
		case VarString:
			varstr= br.varstr;
			break;
		case VarNumber:
			n.varnumber= br.n.varnumber;
			break;
		case VarInteger:
			n.varinteger= br.n.varinteger;
			break;
		default:
			;
		}
	}
	void assign (BlNumber num)
	{
		vartype= VarNumber;
		n.varnumber= num;
	}
	void assign (BlInteger inum)
	{
		vartype= VarInteger;
		n.varinteger= inum;
	}
	void assign (const std::string & str)
	{
		vartype= VarString;
		varstr= str;
	}
	void operator = (const BlResult & br)
	{
		vartype= br.vartype;
		switch (vartype)
		{
		case VarString:
			varstr= br.varstr;
			break;
		case VarNumber:
			n.varnumber= br.n.varnumber;
			break;
		case VarInteger:
			n.varinteger= br.n.varinteger;
			break;
		default:
			;
		}
	}
	VarType type () const { return vartype; }
	bool is_numeric () const { return is_numeric_type (vartype); }
	const std::string & str () const
	{
		if (vartype != VarString)
			throw ErrMismatch;
		return varstr;
	}
	std::string & str ()
	{
		if (vartype != VarString)
			throw ErrMismatch;
		return varstr;
	}
	BlNumber number () const
	{
		switch (vartype)
		{
		case VarNumber:
			return n.varnumber;
		case VarInteger:
			return n.varinteger;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
	}
	BlInteger integer () const
	{
		switch (vartype)
		{
		case VarNumber:
			//return BlInteger (n.varnumber);
			return NumberToInteger (n.varnumber);
		case VarInteger:
			return n.varinteger;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
	}
	bool tobool () const
	{
		switch (vartype)
		{
		case VarNumber:
			return n.varnumber != 0.0;
		case VarInteger:
			return n.varinteger != 0;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
	}
	BlNumber numberdenom () const
	{
		switch (vartype)
		{
		case VarNumber:
			if (! n.varnumber)
				throw ErrDivZero;
			return n.varnumber;
		case VarInteger:
			if (! n.varinteger)
				throw ErrDivZero;
			return n.varinteger;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
	}
	BlInteger integerdenom () const
	{
		switch (vartype)
		{
		case VarNumber:
			{
				BlInteger inum= NumberToInteger (n.varnumber);
				if (! inum)
					throw ErrDivZero;
				return inum;
			}
		case VarInteger:
			if (! n.varinteger)
				throw ErrDivZero;
			return n.varinteger;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
	}
	/*BlResult & */ void operator = (const std::string & nstr)
	{
		vartype= VarString;
		varstr= nstr;
		//return * this;
	}
	/*BlResult & */ void operator = (BlNumber num)
	{
		vartype= VarNumber;
		n.varnumber= num;
		//varstr.erase ();
		//return * this;
	}
	// Do not define operator = for BlInteger,
	// it will be a redefinition for one of
	// long, int or short.
	/*BlResult & */ void operator = (long inum)
	{
		vartype= VarInteger;
		n.varinteger= inum;
		//varstr.erase ();
		//return * this;
	}
	void operator = (unsigned long inum)
	{
		vartype= VarInteger;
		n.varinteger= inum;
	}
	/*BlResult & */ void operator = (int inum)
	{
		vartype= VarInteger;
		n.varinteger= inum;
		//varstr.erase ();
		//return * this;
	}
	/*BlResult & */ void operator = (unsigned int inum)
	{
		vartype= VarInteger;
		n.varinteger= inum;
		//varstr.erase ();
		//return * this;
	}
	/*BlResult & */ void operator = (short inum)
	{
		vartype= VarInteger;
		n.varinteger= inum;
	}
	/*BlResult & */ void operator = (unsigned short inum)
	{
		vartype= VarInteger;
		n.varinteger= inum;
	}
	#if 0
	/*BlResult & */ void operator = (size_t inum)
	{
		vartype= VarInteger;
		n.varinteger= inum;
		//varstr.erase ();
		//return * this;
	}
	#endif
	/*BlResult & */ void operator += (const BlResult & br)
	{
		switch (vartype)
		{
		case VarString:
			varstr+= br.str ();
			break;
		case VarNumber:
			n.varnumber+= br.number ();
			break;
		case VarInteger:
			#ifndef CONVERTNUMBER
			switch (br.vartype)
			{
			case VarInteger:
				n.varinteger+= br.integer ();
				break;
			default:
				integertonumber ();
				n.varnumber+= br.number ();
			}
			#else
			assign (n.varinteger + br.number () );
			#endif
			break;
		default:
			throw ErrBlassicInternal;
		}
		//return * this;
	}
	/*BlResult & */ void operator -= (const BlResult & br)
	{
		switch (vartype)
		{
		case VarNumber:
			n.varnumber-= br.number ();
			break;
		case VarInteger:
			#ifndef CONVERTNUMBER
			switch (br.vartype)
			{
			case VarInteger:
				n.varinteger-= br.integer ();
				break;
			default:
				vartype= VarNumber;
				n.varnumber= n.varinteger;
				n.varnumber-= br.number ();
			}
			#else
			assign (n.varinteger - br.number () );
			#endif
			break;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
		//return * this;
	}
	/*BlResult & */ void operator *= (const BlResult & br)
	{
		switch (vartype)
		{
		case VarNumber:
			n.varnumber*= br.number ();
			break;
		case VarInteger:
			#ifndef CONVERTNUMBER
			switch (br.vartype)
			{
			case VarInteger:
				n.varinteger*= br.integer ();
				break;
			default:
				integertonumber ();
				n.varnumber*= br.number ();
			}
			#else
			assign (n.varinteger * br.number () );
			#endif
			break;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
		//return * this;
	}
	/*BlResult & */ void operator /= (const BlResult & br)
	{
		switch (vartype)
		{
		case VarInteger:
			#ifndef CONVERTNUMBER
			integertonumber ();
			#else
			assign (n.varinteger / br.numberdenom () );
			break;
			#endif
		case VarNumber:
			{
				BlNumber num= br.number ();
				if (num == 0)
					throw ErrDivZero;
				n.varnumber/= num;
			}
			break;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
		//return * this;
	}
	/*BlResult & */ void operator %= (const BlResult & br)
	{
		switch (vartype)
		{
		case VarInteger:
			n.varinteger= n.varinteger % br.integerdenom ();
			break;
		case VarNumber:
			assign (NumberToInteger (n.varnumber) %
				br.integerdenom () );
			break;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
		//return * this;
	}
	void integerdivideby (const BlResult & br)
	{
		switch (vartype)
		{
		case VarInteger:
			n.varinteger/= br.integerdenom ();
			break;
		case VarNumber:
			assign (NumberToInteger (n.varnumber) /
				br.integerdenom () );
			break;
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
	}
	BlResult operator - ()
	{
		switch (vartype)
		{
		case VarNumber:
			return BlResult (-n.varnumber);
		case VarInteger:
			#ifndef CONVERTNUMBER
			return BlResult (-n.varinteger);
			#else
			return BlResult (-number () );
			#endif
		case VarString:
			throw ErrMismatch;
		default:
			throw ErrBlassicInternal;
		}
	}
	bool operator == (const BlResult & br)
	{
		switch (vartype)
		{
		case VarString:
			return varstr == br.str ();
		case VarNumber:
			return n.varnumber == br.number ();
		case VarInteger:
			switch (br.vartype)
			{
			case VarInteger:
				return n.varinteger == br.n.varinteger;
			case VarNumber:
				return n.varinteger == br.n.varnumber;
			case VarString:
				throw ErrMismatch;
			default:
				throw ErrBlassicInternal;
			}
		default:
			throw ErrBlassicInternal;
		}
	}
	bool operator != (const BlResult & br)
	{
		return ! operator == (br);
	}
	bool operator < (const BlResult & br)
	{
		switch (vartype)
		{
		case VarString:
			return varstr < br.str ();
		case VarNumber:
			return n.varnumber < br.number ();
		case VarInteger:
			switch (br.vartype)
			{
			case VarInteger:
				return n.varinteger < br.n.varinteger;
			case VarNumber:
				return n.varinteger < br.n.varnumber;
			case VarString:
				throw ErrMismatch;
			default:
				throw ErrBlassicInternal;
			}
		default:
			throw ErrBlassicInternal;
		}
	}
	bool operator <= (const BlResult & br)
	{
		switch (vartype)
		{
		case VarString:
			return varstr <= br.str ();
		case VarNumber:
			return n.varnumber <= br.number ();
		case VarInteger:
			switch (br.vartype)
			{
			case VarInteger:
				return n.varinteger <= br.n.varinteger;
			case VarNumber:
				return n.varinteger <= br.n.varnumber;
			case VarString:
				throw ErrMismatch;
			default:
				throw ErrBlassicInternal;
			}
		default:
			throw ErrBlassicInternal;
		}
	}
	bool operator > (const BlResult & br)
	{
		return ! operator <= (br);
	}
	bool operator >= (const BlResult & br)
	{
		return ! operator < (br);
	}
};

} // namespace result

} // namespace blassic

#endif

// Fin de result.h
