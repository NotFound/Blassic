// socket.cpp
// Revision 24-apr-2009

#include "socket.h"

#include <string.h>

//------------------------------------------------
// Changed this: now do not use winsock in Cygwin.
//------------------------------------------------
//#if defined _Windows || defined __CYGWIN__ || defined __MINGW32__
//
#if defined _Windows || defined __MINGW32__

#include <winsock.h>

typedef SOCKET TypeSocket;
const TypeSocket InvalidSocket= INVALID_SOCKET;
inline bool isInvalidSocket (TypeSocket s)
	{ return s == INVALID_SOCKET; }

namespace {

class InitWinSock {
	InitWinSock ()
	{
		WSADATA data;
		r= WSAStartup (0x0101, & data);
	}
	~InitWinSock ()
	{
		WSACleanup ();
	}
	int r;
	static InitWinSock init;
	struct avoid_ugly_warning { };
	friend struct avoid_ugly_warning;
};

InitWinSock InitWinSock::init;

} // namespace

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

typedef int TypeSocket;
const TypeSocket InvalidSocket= -1;
inline bool isInvalidSocket (TypeSocket s)
	{ return s < 0; }
inline int closesocket (int handle) { return close (handle); }

#endif

#include <errno.h>

#ifdef __BORLANDC__
#pragma warn -inl
#endif


//**********************************************************
//		SocketError
//**********************************************************

std::string SocketError::strErr= "Socket error in ";

SocketError::SocketError (const std::string & nstr)
{
	str= strErr;
	str+= nstr;
}

SocketError::SocketError (const std::string & nstr, int errnum)
{
	str= strErr;
	str+= nstr;
	str+= ": ";
	str+= strerror (errnum);
}

const char * SocketError::what () const throw ()
{
	return str.c_str ();
}

//**********************************************************
//		Socket::Internal
//**********************************************************

class Socket::Internal {
public:
	Internal () :
		refcount (1), s (InvalidSocket), maxbuf (0), posbuf (0)
	{
	}
	void addref () { ++refcount; }
	void delref ()
	{
		if (--refcount == 0) {
			if (! isInvalidSocket (s) )
				closesocket (s);
			delete this;
		}
	}
	void set (TypeSocket ns) { s= ns; }
	TypeSocket handle () { return s; }
	int connect (sockaddr * name, int namelen)
	{
		//if (! isInvalidSocket (s) )
		//	throw SocketError ("connect: already connected");
		return ::connect (s, name, namelen);
	}
	bool eof ()
	{
		if (posbuf < maxbuf)
			return false;
		getbuffer ();
		return maxbuf == 0;
	}
	int recv (char * bufrec, size_t len, int flags= 0);
	int send (const char * buffer, size_t len, int flags= 0)
	{
		return ::send (s, buffer, len, flags);
	}
	char get ()
	{
		if (posbuf >= maxbuf) {
			getbuffer ();
			if (maxbuf == 0) return 0;
		}
		return buffer [posbuf++];
	}
private:
	size_t refcount;
	TypeSocket s;
	static const size_t bufsiz= 1024;
	char buffer [bufsiz];
	size_t maxbuf, posbuf;
	void getbuffer ()
	{
		posbuf= 0;
		maxbuf= 0;
		int i= ::recv (s, buffer, bufsiz, 0);
		if (i > 0)
			maxbuf= i;
	}
};

int Socket::Internal::recv (char * bufrec, size_t len, int flags)
{
	if (posbuf < maxbuf) {
		size_t send= maxbuf - posbuf;
		if (len < send) send= len;
		for (size_t i= 0, j= posbuf; i < send; ++i, ++j)
			bufrec [i]= buffer [j];
		posbuf+= send;
		return send;
	}
	return ::recv (s, bufrec, len, flags);
}

//**********************************************************
//		Socket
//**********************************************************

Socket::Socket ()
{
	in= new Internal;
}

Socket::Socket (const Socket & nsock)
{
	in= nsock.in;
	in->addref ();
}

Socket::~Socket ()
{
	in->delref ();
}

Socket &  Socket::operator = (const Socket & nsock)
{
	Internal * nin;
	nin= nsock.in;
	nin->addref ();
	in->delref ();
	in= nin;
	return * this;
}

#if 0
TypeSocket Socket::handle ()
{
	return in->handle ();
}
#endif

bool Socket::eof ()
{
	return in->eof ();
}

std::string Socket::readline ()
{
	char c;
	std::string str;
	while ( (c= in->get () ) != '\n' && c != '\0')
		str+= c;
	#if 0
	std::string::size_type l= str.size ();
	if (l > 0 && str [l - 1] == '\r')
		str.erase (l - 1);
	#endif
	return str;
}

int Socket::read (char * str, int len)
{
	return in->recv (str, len);
}

void Socket::write (const std::string & str)
{
	in->send (str.data (), str.size () );
}

void Socket::write (const char * str, size_t len)
{
	in->send (str, len);
}

//**********************************************************
//		TcpSocket
//**********************************************************

TcpSocket::TcpSocket ()
{
	protoent * pe= getprotobyname ("tcp");
	int proto= pe ? pe->p_proto : 0;
	TypeSocket s= socket (PF_INET, SOCK_STREAM, proto);
	in->set (s);
}

//**********************************************************
//		TcpSocketClient
//**********************************************************

TcpSocketClient::TcpSocketClient
	(const std::string & host, unsigned short port)
{
	hostent * hent= gethostbyname (host.c_str () );
	if (! hent)
		throw SocketError (std::string ("search from host ") + host,
			errno);
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_port= htons (port);
	addr.sin_addr= * (in_addr *) hent->h_addr_list [0];
	if (in->connect ( (sockaddr *) & addr, sizeof (addr) ) != 0)
		throw SocketError (std::string ("connect with ") + host,
			errno);
}

// Fin de socket.cpp
