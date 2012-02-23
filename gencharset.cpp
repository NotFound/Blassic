// gencharset.cpp
// Revision 31-jul-2004

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <sstream>
// bitset in gcc 2.95 uses min whithout defining it.
// Then we include something that does.
#include <algorithm>
#include <bitset>

using std::istream;
using std::ostream;
using std::cerr;
using std::endl;
using std::string;
using std::runtime_error;

void gencharset (const string & fin, const string & fout,
	const std::string & name);
void readcharset (istream & in);
void writecharset (ostream & out, const std::string & name);

size_t linenumber= 0;

int main (int argc, char * * argv)
{
	try
	{
		string fin, fout, name;
		if (argc > 1) fin= argv [1];
		if (argc > 2) fout= argv [2];
		if (argc > 3) name= argv [3];
		gencharset (fin, fout, name);
	}
	catch (std::exception & e)
	{
		cerr << e.what ();
		if (linenumber != 0)
			cerr << " in line " << linenumber;
		cerr << endl;
	}
}

typedef unsigned char chardata [8];

chardata default_data;
bool default_defined= false;
chardata data [256];
bool data_defined [256]= { false };

void gencharset (const string & fin, const string & fout,
	const std::string & name)
{
	istream * pin;
	if (fin.empty () )
		pin= & std::cin;
	else
	{
		std::ifstream * pinf= new std::ifstream (fin.c_str () );
		if (! pinf->is_open () )
			throw runtime_error ("File not found");
		pin= pinf;
	}

	readcharset (* pin);

	if (! fin.empty () )
		delete pin;

	linenumber= 0;

	ostream * pout;
	if (fout.empty () )
		pout= & std::cout;
	else
	{
		std::ofstream * poutf= new std::ofstream (fout.c_str () );
		if (! poutf->is_open () )
			throw runtime_error ("Cannot create output file");
		pout= poutf;
	}

	writecharset (* pout, name);

	if (! fout.empty () )
		delete pout;
}

bool readline (istream & in, string & str)
{
	do {
		std::getline (in, str);
		if (! in)
			return false;
		++ linenumber;
		string::size_type l= str.size ();
		if (l > 0 && str [l - 1] == '\r')
			str= str.substr (0, l - 1);
	} while (str.empty () || str [0] == '#');
	return true;
}

void readchar (istream & in, chardata & chd)
{
	string str;
	bool invert= false;
	for (int i= 0; i < 8; ++i)
	{
		if (! readline (in, str) )
			throw runtime_error ("Unexpected eof");
		if (str == "INVERT")
		{
			invert= true;
			if (! readline (in, str) )
				throw runtime_error ("Unexpected eof");
		}
		std::bitset <8> b (str);
		unsigned long l= b.to_ulong ();
		if (invert)
			l= ~ l;
		chd [i]= static_cast <unsigned char> (l);
	}
}

unsigned char getcharcode (const string & str)
{
	if (str.size () == 1)
		return str [0];
	std::istringstream iss (str);
	unsigned int u;
	iss >> u;
	if (! iss)
		throw runtime_error ("Syntax error");
	char c;
	iss >> c;
	if (! iss.eof () )
		throw runtime_error ("Syntax error");
	if (u > 255)
		throw runtime_error ("Invalid char number");
	return static_cast <unsigned char> (u);
}

void readcharset (istream & in)
{
	string str;
	while (readline (in, str) )
	{
		//cerr << str << endl;
		if (str == "DEFAULT")
		{
			if (default_defined)
				throw runtime_error
					("Default already defined");
			//cerr << "Defining default" << endl;
			readchar (in, default_data);
			default_defined= true;
		}
		else
		{
			unsigned char ch= getcharcode (str);
			//cerr << "Defining char: ";
			//if (ch >= 32) cerr << ch;
			//else cerr << static_cast <unsigned int> (ch);
			//cerr << endl;
			if (data_defined [ch] )
				throw runtime_error ("Char already defined");
			readchar (in, data [ch] );
			data_defined [ch]= true;
		}
	}
}

void writecharset (ostream & out, const std::string & name)
{
	if (name.empty () )
		out <<
			"// charset.cpp\n";
	else
		out <<
			"// charset_" << name << ".cpp\n";

	out <<
		"// Automatically generated, do not edit.\n"
		"\n"
		"#include \"charset.h\"\n"
		"\n"
		"const charset::chardataset charset::";

	if (name.empty () )
		out << "default";
	else
		out << name;

	out << "_data= {\n"
	;

	for (int i= 0; i < 256; ++i)
	{
		out << "\t// char " << i << "\n\t{ ";
		chardata * pdata;
		if (data_defined [i] ) pdata= & data [i];
		else pdata= & default_data;
		for (int j= 0; j < 8; ++j)
		{
			out << static_cast <unsigned int> ( (* pdata) [j] );
			if (j < 7) out << ", ";
		}
		out << " },\n";
	}
	out <<

"};\n"
"\n"
"//End of charset.cpp\n"
	;
}

// End of gencharset.cpp
