#ifndef INCLUDE_BLASSIC_REGEXP_H
#define INCLUDE_BLASSIC_REGEXP_H

// regexp.h
// Revision 7-feb-2005


#include <string>

//#include "runnerline.h"
class RunnerLine;


class Regexp {
public:
	typedef std::string string;
	typedef string::size_type size_type;
	typedef unsigned int flag_t;
	static const flag_t FLAG_NOCASE=  1;
	static const flag_t FLAG_NOBEG=   2;
	static const flag_t FLAG_NOEND=   4;
	static const flag_t FLAG_NEWLINE= 8;

	Regexp (const string & exp, flag_t flags);
	~Regexp ();
	size_type find (const string & searched, size_type init);
	string replace (const string & searched, size_type init,
		const string & replaceby);
	string replace (const string & searched, size_type init,
		RunnerLine & runnerline, const string & fname);
private:
	class Internal;
	Internal * pin;
};

#endif

// End of regexp.h
