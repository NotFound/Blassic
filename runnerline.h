#ifndef INCLUDE_BLASSIC_RUNNERLINE_H
#define INCLUDE_BLASSIC_RUNNERLINE_H

// runnerline.h
// Revision 18-jul-2004

#include "codeline.h"
#include "function.h"
#include "result.h"

class Runner;
class LocalLevel;

class RunnerLine {
public:
	RunnerLine (Runner & runner);
	RunnerLine (Runner & runner, const CodeLine & newcodeline);
	virtual ~RunnerLine ();

	CodeLine & getcodeline () { return codeline; }
	virtual void execute ()= 0;
	virtual ProgramPos getposactual () const= 0;
	virtual void callfn (Function & f, const std::string & fname,
		LocalLevel & ll, blassic::result::BlResult & result)= 0;
protected:
	Runner & runner;
	CodeLine codeline;
};

RunnerLine * newRunnerLine (Runner & runner, const CodeLine & newcodeline);

#endif

// End of runnerline.h
