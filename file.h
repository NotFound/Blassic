#ifndef INCLUDE_BLASSIC_FILE_H
#define INCLUDE_BLASSIC_FILE_H

// file.h
// Revision 7-feb-2005

#ifdef __BORLANDC__
#pragma warn -8022
#endif

#include "blassic.h"

#include "dim.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

namespace blassic {

namespace file {

enum OpenMode {
	Input= 1, Output= 2, InOut= 3,
	Append= 6, Random= 8, Binary= 16,
	WithErr= 32
};

class BlFile {
public:
	struct field_element {
		size_t size;
		std::string name;
		Dimension dim;
		field_element (size_t n,
			const std::string & str, const Dimension & dim)
			:
			size (n), name (str), dim (dim)
		{ }
	};
	enum Align { AlignRight, AlignLeft };

	BlFile (OpenMode nmode);
	virtual ~BlFile ();

	virtual void closein ();
	virtual void closeout ();

	virtual void reset (int x1, int x2, int y1, int y2);

	virtual bool isfile () const = 0;
	virtual bool istextwindow () const;
	virtual bool eof ();
	virtual size_t loc ();
	virtual void flush ();
	virtual size_t getwidth () const;
	virtual void movecharforward ();
	virtual void movecharforward (size_t n);
	virtual void movecharback ();
	virtual void movecharback (size_t n);
	virtual void movecharup ();
	virtual void movecharup (size_t n);
	virtual void movechardown ();
	virtual void movechardown (size_t n);
	virtual void showcursor ();
	virtual void hidecursor ();
	virtual std::string getkey ();
	virtual std::string inkey ();
	virtual void getline (std::string & str, bool endline= true);
	char delimiter () { return cDelimiter; }
	void delimiter (char delim) { cDelimiter= delim; }
	char quote () { return cQuote; }
	void quote (char qu) { cQuote= qu; }
	char escape () { return cEscape; }
	void escape (char esc) { cEscape= esc; }
	friend BlFile & operator << (BlFile & bf, const std::string & str);
	friend BlFile & operator << (BlFile & bf, char c);
	friend BlFile & operator << (BlFile & bf, BlNumber n);
	friend BlFile & operator << (BlFile & bf, BlInteger n);
	friend BlFile & operator << (BlFile & bf, BlLineNumber l);
	friend BlFile & operator << (BlFile & bf, unsigned short n);
	void putspaces (size_t n);
	virtual void tab ();
	virtual void tab (size_t n);
	virtual void endline ();
	virtual void put (size_t pos);
	virtual void get (size_t pos);
	virtual void field_clear ();
	virtual void field (const std::vector <field_element> & elem);
	virtual void field_append (const std::vector <field_element> & elem);
	virtual bool assign (const std::string & name, const Dimension & dim,
		const std::string & value, Align align);
	virtual bool assign_mid (const std::string & name,
		const Dimension & dim,
		const std::string & value,
		size_t inipos, std::string::size_type len);
	virtual std::string read (size_t n);
	virtual void gotoxy (int x, int y);
	virtual void setcolor (int color);
	virtual int getcolor ();
	virtual void setbackground (int color);
	virtual int getbackground ();
	virtual void cls ();
	virtual std::string copychr (BlChar from, BlChar to);
	virtual int pos ();
	virtual int vpos ();
	virtual void tag ();
	virtual void tagoff ();
	virtual bool istagactive ();
	virtual void inverse (bool active);
	virtual bool getinverse ();
	virtual void bright (bool active);
	virtual bool getbright ();
	virtual void setwidth (size_t w);
	virtual void setmargin (size_t m);
	virtual BlInteger lof ();
	virtual bool poll ();
	virtual void scroll (int nlines);
private:
	virtual void outstring (const std::string & str);
	virtual void outchar (char c);

	BlFile (const BlFile &); // Forbidden
	void operator = (const BlFile &); // Forbidden.

	OpenMode mode;
	char cDelimiter, cQuote, cEscape;
protected:
	OpenMode getmode () const { return mode; }
};

//		Create BlFile functions.

BlFile * newBlFileConsole ();
BlFile * newBlFileWindow (BlChannel ch);
BlFile * newBlFileWindow (BlChannel ch, int x1, int x2, int y1, int y2);
BlFile * newBlFileOutString ();
BlFile * newBlFileOutput (std::ostream & os);
BlFile * newBlFileRegular (const std::string & name, OpenMode mode);
BlFile * newBlFileRandom (const std::string & name, size_t record_len);
BlFile * newBlFilePopen (const std::string & name, OpenMode mode);
BlFile * newBlFileSocket (const std::string & host, short port);
BlFile * newBlFilePrinter ();

class BlFileOut : public BlFile {
public:
	BlFileOut ();
	BlFileOut (OpenMode mode);
	void flush ();
protected:
	virtual std::ostream & ofs ()= 0;
private:
	void outstring (const std::string & str);
	void outchar (char c);
	//void outnumber (BlNumber n);
	//void outinteger (BlInteger n);
	//void outlinenumber (BlLineNumber l);
};

class BlFileOutString : public BlFileOut {
public:
	BlFileOutString ();
	bool isfile () const { return false; }
	std::string str ();
private:
	std::ostream & ofs ();
	std::ostringstream oss;
};

} // namespace blassic

} // namespace file

#endif

// Fin de file.h
