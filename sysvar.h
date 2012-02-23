#ifndef INCLUDE_BLASSIC_SYSVAR_H
#define INCLUDE_BLASSIC_SYSVAR_H

// sysvar.h
// Revision 6-feb-2005

#if defined __BORLANDC__
#pragma warn -8058
#endif

#include "blassic.h"


namespace blassic {

namespace sysvar {

void init ();
size_t address ();

void set (size_t var, BlChar value);
void set16 (size_t var, short value);
void set32 (size_t var, BlInteger value);

BlChar get (size_t var);
unsigned short get16 (size_t var);
unsigned long get32 (size_t var);

const size_t
	GraphicsWidth= 0,
	GraphicsHeight= 2,
	NumArgs= 4,
	VersionMajor= 6,
	VersionMinor= 8,
	VersionRelease= 10,
	AutoInit= 12,
	AutoInc= 16,
	CharGen= 20,
	ShellResult= 24,
	TypeOfVal= 25,	// 0: simple, 1: expression evluation,
			// else unimplemented.
	TypeOfNextCheck= 26,	// 0: normal, else ZX-type
	TypeOfDimCheck= 27,	// 0: cannot dim already dimensioned
				// 1: Silently redim
	MaxHistory= 28,	// Max size of history buffer.
	Flags1= 30,	// Bit 0: LOCATE style. 0 Microsoft, 1 Amstrad CPC.
			// Bit 1: TAB style: 0 normal, 1 Spectrum.
			// Bit 2: THEN omitted: 0 is not accepted, 1 accepted.
			// Bit 3: space before number in PRINT, 0 No, 1 Yes.
			// Bit 4: initial space in STR$, 0 No, 1 Yes.
			// Bit 5: convert LF to CR in GET and INKEY$
			// Bit 6: Show debug info on certain errors.
			// Bit 7: Relaxed GOTO mode.
	PrinterLine=  31,	// Type of printer line feed.
				// 0 LF only.
				// 1 CR + LF
				// 2 CR only.
	MaxFnLevel= 32, // Max level of FN calls.
	DebugLevel= 36, // Level for IF_DEBUG
	Zone= 38,	// Size of zone for the , separator of PRINT.
	GraphRotate= 40,	// Type of graphics rotation:
				// 0 no rotation.
				// 1 90 degrees.
				// Other: undefined
	Flags2= 41,	// Bit 0: GO TO and GO SUB separated in listings.
			// Bit 1: if set true is positive.
			// Bit 2: if set logical ops are boolean.
			// Bit 3: if set blank lines in .bas text files
			//	are converted to comments.
			// Bits 4-7: reserved.
	TronChannel= 42, // Last channel used for tron or tron line.
	TronFlags= 44,	// Tron flags.
			// Bit 0: tron ot tron line is active.
			// Bit 1: tron line is active.
			// Bits 2-7: reserved.
	// 45: reserved
	EndSysVar= 46;

// Flags masks.
// Implemented as classes to improve type safety
// and avoid silly mistakes.

class Flags1Bit {
public:
	explicit Flags1Bit (BlChar f) :
		f (f)
	{ }
	BlChar get () const { return f; }
	bool has (const Flags1Bit f1bit) const;
	friend bool operator == (const Flags1Bit f1, const Flags1Bit f2);
	friend bool operator != (const Flags1Bit f1, const Flags1Bit f2);
	friend Flags1Bit operator | (const Flags1Bit f1, const Flags1Bit f2);
	friend Flags1Bit operator & (const Flags1Bit f1, const Flags1Bit f2);
private:
	BlChar f;
};

inline bool Flags1Bit::has (const Flags1Bit f1bit) const
{
	return (f & f1bit.f);
}

inline bool operator == (const Flags1Bit f1, const Flags1Bit f2)
{
	return f1.f == f2.f;
}

inline bool operator != (const Flags1Bit f1, const Flags1Bit f2)
{
	return f1.f != f2.f;
}

inline Flags1Bit operator | (const Flags1Bit f1, const Flags1Bit f2)
{
	return Flags1Bit (f1.f | f2.f);
}

inline Flags1Bit operator & (const Flags1Bit f1, const Flags1Bit f2)
{
	return Flags1Bit (f1.f & f2.f);
}

class Flags2Bit {
public:
	explicit Flags2Bit (BlChar f) :
		f (f)
	{ }
	BlChar get () const { return f; }
	bool has (const Flags2Bit f2bit) const;
	friend bool operator == (const Flags2Bit f1, const Flags2Bit f2);
	friend bool operator != (const Flags2Bit f1, const Flags2Bit f2);
	friend Flags2Bit operator | (const Flags2Bit f1, const Flags2Bit f2);
	friend Flags2Bit operator & (const Flags2Bit f1, const Flags2Bit f2);
private:
	BlChar f;
};

inline bool Flags2Bit::has (const Flags2Bit f2bit) const
{
	return (f & f2bit.f);
}

inline bool operator == (const Flags2Bit f1, const Flags2Bit f2)
{
	return f1.f == f2.f;
}

inline bool operator != (const Flags2Bit f1, const Flags2Bit f2)
{
	return f1.f != f2.f;
}

inline Flags2Bit operator | (const Flags2Bit f1, const Flags2Bit f2)
{
	return Flags2Bit (f1.f | f2.f);
}

inline Flags2Bit operator & (const Flags2Bit f1, const Flags2Bit f2)
{
	return Flags2Bit (f1.f & f2.f);
}

const Flags1Bit
	Flags1Clean (0),
	LocateStyle (1),
	TabStyle (2),
	ThenOmitted (4),
	SpaceBefore (8),
	SpaceStr_s (16),
	ConvertLFCR (32),
	ShowDebugInfo (64),
	RelaxedGoto (128),
	Flags1Full (255);

const Flags2Bit
	Flags2Clean (0),
	SeparatedGoto (1),
	TruePositive (2),
	BoolMode (4),
	BlankComment (8),
	Flags2Full (255);

Flags1Bit getFlags1 ();
Flags2Bit getFlags2 ();

bool hasFlags1 (Flags1Bit f);
bool hasFlags2 (Flags2Bit f);

void setFlags1 (Flags1Bit f);
void setFlags2 (Flags2Bit f);

} // namespace sysvar

} // namespace blassic

#endif

// Fin de sysvar.h
