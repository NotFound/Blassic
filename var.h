#ifndef INCLUDE_BLASSIC_VAR_H
#define INCLUDE_BLASSIC_VAR_H

// var.h
// Revision 8-jan-2005

#include "blassic.h"
#include "dim.h"

// Now defined in blassic.h to reduce dependencies.

//enum VarType { VarUndef, VarNumber, VarString, VarInteger };

//inline bool is_numeric_type (VarType v)
//{ return v == VarNumber || v == VarInteger; }


struct VarPointer {
	VarType type;
	union {
		BlNumber * pnumber;
		BlInteger * pinteger;
		std::string * pstring;
	};
	std::string::size_type from, to;
};

VarType typeofvar (const std::string & name);

void definevar (VarType type, char c);
void definevar (VarType type, char cfrom, char cto);

void clearvars ();
void assignvarnumber (const std::string & name, BlNumber value);
void assignvarinteger (const std::string & name, BlInteger value);
void assignvarstring (const std::string & name, const std::string & value);

BlNumber evaluatevarnumber (const std::string & name);
BlInteger evaluatevarinteger (const std::string & name);
std::string evaluatevarstring (const std::string & name);

BlNumber * addrvarnumber (const std::string & name);
BlInteger * addrvarinteger (const std::string & name);
std::string * addrvarstring (const std::string & name);

void dimvarnumber (const std::string & name, const Dimension & d);
void dimvarinteger (const std::string & name, const Dimension & d);
void dimvarstring (const std::string & name, const Dimension & d);

void erasevarnumber (const std::string & name);
void erasevarinteger (const std::string & name);
void erasevarstring (const std::string & name);

BlNumber valuedimnumber (const std::string & name, const Dimension & d);
BlInteger valuediminteger (const std::string & name, const Dimension & d);
std::string valuedimstring (const std::string & name, const Dimension & d);

void assigndimnumber (const std::string & name, const Dimension & d,
	BlNumber value);
void assigndiminteger (const std::string & name, const Dimension & d,
	BlInteger value);
void assigndimstring (const std::string & name, const Dimension & d,
	const std::string & value);

BlNumber * addrdimnumber (const std::string & name, const Dimension & d);
BlInteger * addrdiminteger (const std::string & name, const Dimension & d);
std::string * addrdimstring (const std::string & name, const Dimension & d);

#endif

// Fin de var.h
