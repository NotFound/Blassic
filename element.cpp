// element.cpp
// Revision 8-jan-2005

#include "element.h"

#include "var.h"
#include "trace.h"

ForElementNumber::ForElementNumber (const std::string & nvar,
		ProgramPos pos,
		BlNumber initial, BlNumber nmax, BlNumber nstep) :
	ForElement (nvar, pos),
	max (nmax),
	step (nstep)
{
	varaddr= addrvarnumber (nvar);
	* varaddr= initial;
}

ForElementNumberInc::ForElementNumberInc (const std::string & var,
		ProgramPos pos,
		BlNumber initial, BlNumber max, BlNumber step) :
	ForElementNumber (var, pos, initial, max, step)
{
	//TRACEFUNC (tr, "ForElementNumberInc::ForElementNumberInc");
}

bool ForElementNumberInc::next ()
{
	* varaddr+= step;
	return * varaddr <= max;
}

ForElementNumberDec::ForElementNumberDec (const std::string & var,
		ProgramPos pos,
		BlNumber initial, BlNumber max, BlNumber step) :
	ForElementNumber (var, pos, initial, max, step)
{
	//TRACEFUNC (tr, "ForElementNumberDec::ForElementNumberDec");
}

bool ForElementNumberDec::next ()
{
	* varaddr+= step;
	return * varaddr >= max;
}

ForElementInteger::ForElementInteger (const std::string & nvar,
		ProgramPos pos,
		BlInteger initial, BlInteger nmax, BlInteger nstep) :
	ForElement (nvar, pos),
	max (nmax),
	step (nstep)
{
	varaddr= addrvarinteger (nvar);
	* varaddr= initial;
}

ForElementIntegerInc::ForElementIntegerInc (const std::string & var,
		ProgramPos pos,
		BlInteger initial, BlInteger max, BlInteger step) :
	ForElementInteger (var, pos, initial, max, step)
{
	//TRACEFUNC (tr, "ForElementIntegerInc::ForElementIntegerInc");
}

bool ForElementIntegerInc::next ()
{
	* varaddr+= step;
	return * varaddr <= max;
}

ForElementIntegerDec::ForElementIntegerDec (const std::string & var,
		ProgramPos pos,
		BlInteger initial, BlInteger max, BlInteger step) :
	ForElementInteger (var, pos, initial, max, step)
{
	//TRACEFUNC (tr, "ForElementIntegerDec::ForElementIntegerDec");
}

bool ForElementIntegerDec::next ()
{
	* varaddr+= step;
	return * varaddr >= max;
}

// End of element.cpp
