// runnerline.cpp
// Revision 31-jul-2004

#include "runnerline.h"

#include "trace.h"

RunnerLine::RunnerLine (Runner & runner, const CodeLine & newcodeline) :
		runner (runner),
		codeline (newcodeline)
{
	TRACEFUNC (tr, "RunnerLine::RunnerLine");
}

RunnerLine::RunnerLine (Runner & runner) :
		runner (runner)
{
	TRACEFUNC (tr, "RunnerLine::RunnerLine");
}

RunnerLine::~RunnerLine ()
{
	TRACEFUNC (tr, "RunnerLine::~RunnerLine");
}

// End of runnerline.cpp
