#ifndef INCLUDE_BLASSIC_SOCKET_H
#define INCLUDE_BLASSIC_SOCKET_H

// socket.h
// Revision 6-feb-2005

#include <string>
#include <stdexcept>

#ifdef __BORLANDC__
#pragma warn -8026
#endif

class SocketError : public std::exception {
public:
	SocketError (const std::string & nstr);
	SocketError (const std::string & nstr, int errnum);
	~SocketError () throw () { }
	const char * what () const throw ();
private:
	std::string str;
	static std::string strErr;
};

class Socket {
public:
	Socket ();
	Socket (const Socket & nsock);
	~Socket ();
	Socket & operator = (const Socket & nsock);
	//TypeSocket handle ();
	bool eof ();
	std::string readline ();
	int read (char * str, int len);
	void write (const std::string & str);
	void write (const char * str, size_t len);
protected:
	class Internal;
	Internal * in;
};

class TcpSocket : public Socket {
public:
	TcpSocket ();
};

class TcpSocketClient : public TcpSocket {
public:
	TcpSocketClient (const std::string & host, unsigned short port);
};

#endif

// End of socket.h
