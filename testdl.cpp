// testdl.cpp

#ifdef _Windows
#define EXTERN extern "C" __declspec (dllexport)
#else
#define EXTERN extern "C"
#endif

#ifdef _Windows
#include <windows.h>

#ifdef __BORLANDC__
#pragma hdrstop
#include <condefs.h>
// string (iterator, iterator) gives this warnig:
#pragma warn -8012
#else
#error Compiler not tested.
#endif

#endif

#include <iostream>
#include <string>

using std::string;

#ifdef _Windows

#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void*)
{
        return 1;
}

#endif

EXTERN int testfunc (int nparams, int * param)
{
	if (nparams != 1)
		return 1;
	string * pstr= reinterpret_cast <string *> (param [0]);
	string nstr ( pstr->rbegin (), pstr->rend () );
        // Do not use * pstr= nstr nor assign a string longer than * pstr.
        pstr->assign (nstr);
	return 0;
}

// Fin de testdl.cpp
