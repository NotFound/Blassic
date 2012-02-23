#ifndef INCLUDE_BLASSIC_PROGRAM_H
#define INCLUDE_BLASSIC_PROGRAM_H

// program.h
// Revision 31-jul-2004

#include "blassic.h"
#include "codeline.h"
#include "file.h"

class Program {
public:
	Program ();
	virtual ~Program ();
	virtual BlChar * programptr ()= 0;
	virtual BlLineNumber getlabel (const std::string & str)= 0;
	virtual CodeLine getfirstline ()= 0;
	virtual void getnextline (CodeLine & codeline)= 0;
	virtual void getline (BlLineNumber num, CodeLine & codeline)= 0;
	virtual void getline (ProgramPos pos, CodeLine & codeline)= 0;
	virtual void insert (const CodeLine & codeline)= 0;
	virtual void deletelines
		(BlLineNumber iniline, BlLineNumber endline)= 0;
	virtual void listline (const CodeLine & codeline,
		blassic::file::BlFile & out) const= 0;
	virtual void list (BlLineNumber iniline, BlLineNumber endline,
		blassic::file::BlFile & out) const= 0;
	virtual void save (const std::string & name) const= 0;
	virtual void load (const std::string & name)= 0;
	virtual void load (std::istream & is)= 0;
	virtual void merge (const std::string & name,
		BlLineNumber inidel= LineNoDelete,
		BlLineNumber enddel= LineNoDelete
		)= 0;
	virtual void renew ()= 0;
	virtual void renum (BlLineNumber blnNew, BlLineNumber blnOld,
		BlLineNumber blnInc, BlLineNumber blnStop)= 0;
private:
	Program (const Program &); // Prohibido
	Program & operator= (const Program &); // Prohibido
};

Program * newProgram ();

#endif

// End of program.h
