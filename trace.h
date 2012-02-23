#ifndef INCLUDE_BLASSIC_TRACE_H
#define INCLUDE_BLASSIC_TRACE_H

// trace.h
// Revision 7-feb-2005

#include <cstddef>
#include <string>


class TraceFunc {
public:
	TraceFunc (const char * strFuncName);
	~TraceFunc ();
	void message (const std::string & str);
	static void show (int);
private:
	const char * strfunc;
	TraceFunc * * previous;
	TraceFunc * next;
};

#ifndef NDEBUG

#define TRACEFUNC(tr,name) \
	TraceFunc tr (name)
#define TRMESSAGE(tr,text) \
	tr.message (text)

#else

#define TRACEFUNC(tr,name)
#define TRMESSAGE(tr,text)

#endif

#endif

// Fin de trace.h
