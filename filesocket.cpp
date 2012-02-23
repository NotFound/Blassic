// filesocket.cpp
// Revision 9-jan-2005

//#include "filesocket.h"
#include "file.h"

#include "socket.h"
#include "util.h"

#include "trace.h"

namespace blassic {

namespace file {

class BlFileSocket : public BlFile {
public:
	BlFileSocket (const std::string & host, short port);
	~BlFileSocket ();
	bool isfile () const { return true; }
	void getline (std::string & str, bool endline= true);
	bool eof ();
	void flush ();
	std::string read (size_t n);
private:
	void outstring (const std::string & str);
	void outchar (char c);

	class Internal;
	Internal * pin;
};

//***********************************************
//              BlFileSocket::Internal
//***********************************************

class BlFileSocket::Internal
{
	TcpSocketClient socket;
public:
	Internal (const std::string & host, short port);
	~Internal ();
	void getline (std::string & str, bool endline);
	bool eof ();
	void flush ();
	std::string read (size_t n);
	void outstring (const std::string & str);
	void outchar (char c);
private:
	Internal (const Internal &); // Forbidden
	void operator = (const Internal &); // Forbidden
};

BlFileSocket::Internal::Internal (const std::string & host, short port) :
	socket (host, port)
{
}

BlFileSocket::Internal::~Internal ()
{
	TRACEFUNC (tr, "BlFileSocke::Internal::~Internal");
}

bool BlFileSocket::Internal::eof ()
{
	return socket.eof ();
}

void BlFileSocket::Internal::flush ()
{
	// There is no work to do.
}

void BlFileSocket::Internal::getline (std::string & str, bool)
{
	str= socket.readline ();
}

std::string BlFileSocket::Internal::read (size_t n)
{
	util::auto_buffer <char> buf (n);
	int r= socket.read (buf, n);
	std::string result;
	if (r > 0)
		result.assign (buf, r);
	return result;
}

void BlFileSocket::Internal::outstring (const std::string & str)
{
	socket.write (str);
}

void BlFileSocket::Internal::outchar (char c)
{
	socket.write (& c, 1);
}

//***********************************************
//              BlFileSocket
//***********************************************

BlFile * newBlFileSocket (const std::string & host, short port)
{
	return new BlFileSocket (host, port);
}

BlFileSocket::BlFileSocket (const std::string & host, short port) :
	BlFile (OpenMode (Input | Output) ),
	pin (new Internal (host, port) )
{ }

BlFileSocket::~BlFileSocket ()
{
	delete pin;
}

bool BlFileSocket::eof ()
{
	return pin->eof ();
}

void BlFileSocket::flush ()
{
	pin->flush ();
}

void BlFileSocket::getline (std::string & str, bool endline)
{
	pin->getline (str, endline);
}

std::string BlFileSocket::read (size_t n)
{
	return pin->read (n);
}

void BlFileSocket::outstring (const std::string & str)
{
	pin->outstring (str);
}

void BlFileSocket::outchar (char c)
{
	pin->outchar (c);
}

} // namespace file

} // namespace blassic

// End of filesocket.cpp
