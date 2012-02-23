// sysvar.cpp
// Revision 6-feb-2005

#include "sysvar.h"

#include "charset.h"
#include "trace.h"

// For debugging.
#include <sstream>


namespace {

BlChar system_vars [blassic::sysvar::EndSysVar];

} // namespace


size_t blassic::sysvar::address ()
{
	return reinterpret_cast <size_t> (system_vars);
}

void blassic::sysvar::set (size_t var, BlChar value)
{
	system_vars [var]= value;
}

void blassic::sysvar::set16 (size_t var, short value)
{
	poke16 (system_vars + var, value);
}

void blassic::sysvar::set32 (size_t var, BlInteger value)
{
	poke32 (system_vars + var, value);
}

BlChar blassic::sysvar::get (size_t var)
{
	return system_vars [var];
}

unsigned short blassic::sysvar::get16 (size_t var)
{
	return static_cast <unsigned short> (peek16 (system_vars + var) );
}

unsigned long blassic::sysvar::get32 (size_t var)
{
	return peek32 (system_vars + var);
}

// Flags operations.

blassic::sysvar::Flags1Bit blassic::sysvar::getFlags1 ()
{
	return static_cast <Flags1Bit> (system_vars [Flags1] );
}

blassic::sysvar::Flags2Bit blassic::sysvar::getFlags2 ()
{
	return static_cast <Flags2Bit> (system_vars [Flags2] );
}

bool blassic::sysvar::hasFlags1 (Flags1Bit f)
{
	//return (getFlags1 () & f) != Flags1Clean;
	return getFlags1 ().has (f);
}

bool blassic::sysvar::hasFlags2 (Flags2Bit f)
{
	//return (getFlags2 () & f) != Flags2Clean;
	return getFlags2 ().has (f);
}

void blassic::sysvar::setFlags1 (Flags1Bit f)
{
	BlChar * const pflag= system_vars + Flags1;
	*pflag|= f.get ();
}

void blassic::sysvar::setFlags2 (Flags2Bit f)
{
	BlChar * const pflag= system_vars + Flags2;
	*pflag|= f.get ();
}

// Initialization

void blassic::sysvar::init ()
{
	TRACEFUNC (tr, "blassic::sysvar::init");
	#ifndef NDEBUG
	{
		std::ostringstream oss;
		oss << "System vars address: " <<
			static_cast <void *> (system_vars);
		TRMESSAGE (tr, oss.str () );
	}
	#endif

	set16 (GraphicsWidth, 0);
	set16 (GraphicsHeight, 0);
	set16 (NumArgs, 0);
	set16 (VersionMajor, version::Major);
	set16 (VersionMinor, version::Minor);
	set16 (VersionRelease, version::Release);
	set32 (AutoInit, 10);
	set32 (AutoInc, 10);
	set32 (CharGen, reinterpret_cast <size_t> (& charset::data) );
	set (ShellResult, 0);
	set (TypeOfVal, 0); // VAL simple, number only.
	set (TypeOfNextCheck, 0); // Strict next check
	set (TypeOfDimCheck, 0); // Need erase before dim already dimensioned
	set16 (MaxHistory, 100);
	setFlags1 (Flags1Clean);
			// Bit 0: LOCATE style Microsoft (row, col),
			// Bit 1: TAB style normal.
			// Bit 2: THEN omitted not accepted.
			// Bit 3: Without space before number in PRINT.
			// Bit 4: Without initial space in STR$.
			// Bit 5: Without convert LF to CR.
			// Bit 6: Do not show debug info.
			// Bit 7: Strict GOTO mode.

	#ifdef BLASSIC_USE_WINDOWS
	const BlChar defaultprinterline= 1;
	#else
	const BlChar defaultprinterline= 0;
	#endif
	set (PrinterLine, defaultprinterline);

	// Changed this, the processor stack overflows easily
	// in windows, at least in W98 with C++ Builder.
	//set32 (MaxFnLevel, 1000); // Seems a good limit.
	set32 (MaxFnLevel, 50);

	set16 (DebugLevel, 0);
	set16 (Zone, 8);
	set (GraphRotate, 0);
	setFlags2 (Flags2Clean);
			// Bit 0: GOTO and GOSUB listed as one word.
			// Bit 1: true is -1
			// Bit 2: logical ops are binary
			// Bit 3: blank lines as comments.
	set16 (TronChannel, 0);
	set (TronFlags, 0);
}

// Fin de sysvar.cpp
