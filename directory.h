#ifndef INCLUDE_BLASSIC_DIRECTORY_H
#define INCLUDE_BLASSIC_DIRECTORY_H

// directory.h
// Revision 7-feb-2005


#include <string>

#include "util.h"


class Directory {
public:
	Directory ();
	~Directory ();
	std::string findfirst (const std::string & str);
	std::string findnext ();
private:
	Directory (const Directory &); // Forbidden
	Directory operator = (const Directory &); // Forbidden
	class Internal;
	Internal * pin;
	//util::pimpl_ptr <Internal> pin;
};

void remove_file (const std::string & filename);
void rename_file (const std::string & orig, const std::string & dest);

void change_dir (const std::string & dirname);
void make_dir (const std::string & dirname);
void remove_dir (const std::string & dirname);

void sleep_milisec (unsigned long n);

#endif

// End of directory.h
