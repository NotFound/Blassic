// key.cpp
// Revision 9-jan-2005

#include "key.h"
#include "trace.h"
#include "blassic.h"

#include <map>

#include <iostream> // For debug info only.

//#ifdef _Windows
#ifdef BLASSIC_USE_WINDOWS

#include <windows.h>

#elif defined BLASSIC_USE_X

#include <X11/keysym.h>

#endif

const std::string
	strPAGEUP   ("PAGEUP"),
	strPAGEDOWN ("PAGEDOWN"),
	strEND      ("END"),
	strHOME     ("HOME"),
	strLEFT     ("LEFT"),
	strUP       ("UP"),
	strRIGHT    ("RIGHT"),
	strDOWN     ("DOWN"),
	strINSERT   ("INSERT"),
	strDELETE   ("DELETE"),
	strF1       ("F1"),
	strF2       ("F2"),
	strF3       ("F3"),
	strF4       ("F4"),
	strF5       ("F5"),
	strF6       ("F6"),
	strF7       ("F7"),
	strF8       ("F8"),
	strF9       ("F9"),
	strF10      ("F10"),
	strF11      ("F11"),
	strF12      ("F12"),
	strENTER    ("\n"),
	strCLICK    ("CLICK"),
	strSCLICK   ("SCLICK"),
	strRELEASE  ("RELEASE"),
	strSRELEASE ("SRELEASE");

namespace {

typedef std::map <unsigned int, std::string> mapkey_t;

mapkey_t mapkey;

inline void assignmapkey (unsigned int code, const std::string & name)
{
	#ifndef NDEBUG
	mapkey_t::iterator it;
	if ( (it= mapkey.find (code) ) != mapkey.end () )
	{
		std::cerr << "Error: key " << name <<
			" has equal code than " << it->second <<
			std::endl;
		throw 1;
	}
	#endif
	mapkey [code]= name;
}

bool initmapkey ()
{
	TRACEFUNC (tr, "initmapkey");

	//#ifdef _Windows
	#ifdef BLASSIC_USE_WINDOWS

	assignmapkey (VK_PRIOR,  strPAGEUP);
	assignmapkey (VK_NEXT,   strPAGEDOWN);
	assignmapkey (VK_END,    strEND);
	assignmapkey (VK_HOME,   strHOME);
	assignmapkey (VK_LEFT,   strLEFT);
	assignmapkey (VK_UP,     strUP);
	assignmapkey (VK_RIGHT,  strRIGHT);
	assignmapkey (VK_DOWN,   strDOWN);
	assignmapkey (VK_INSERT, strINSERT);
	assignmapkey (VK_DELETE, strDELETE);
	assignmapkey (VK_F1,     strF1);
	assignmapkey (VK_F2,     strF2);
	assignmapkey (VK_F3,     strF3);
	assignmapkey (VK_F4,     strF4);
	assignmapkey (VK_F5,     strF5);
	assignmapkey (VK_F6,     strF6);
	assignmapkey (VK_F7,     strF7);
	assignmapkey (VK_F8,     strF8);
	assignmapkey (VK_F9,     strF9);
	assignmapkey (VK_F10,    strF10);
	assignmapkey (VK_F11,    strF11);
	assignmapkey (VK_F12,    strF12);

	#elif defined BLASSIC_USE_X

	assignmapkey (XK_Prior,     strPAGEUP);
	assignmapkey (XK_KP_Prior,  strPAGEUP);
	assignmapkey (XK_Next,      strPAGEDOWN);
	assignmapkey (XK_KP_Next,   strPAGEDOWN);
	assignmapkey (XK_End,       strEND);
	assignmapkey (XK_KP_End,    strEND);
	assignmapkey (XK_Home,      strHOME);
	assignmapkey (XK_KP_Home,   strHOME);
	assignmapkey (XK_Left,      strLEFT);
	assignmapkey (XK_KP_Left,   strLEFT);
	assignmapkey (XK_Up,        strUP);
	assignmapkey (XK_KP_Up,     strUP);
	assignmapkey (XK_Right,     strRIGHT);
	assignmapkey (XK_KP_Right,  strRIGHT);
	assignmapkey (XK_Down,      strDOWN);
	assignmapkey (XK_KP_Down,   strDOWN);
	assignmapkey (XK_Insert,    strINSERT);
	assignmapkey (XK_KP_Insert, strINSERT);
	assignmapkey (XK_Delete,    strDELETE);
	assignmapkey (XK_KP_Delete, strDELETE);
	assignmapkey (XK_F1,        strF1);
	assignmapkey (XK_F2,        strF2);
	assignmapkey (XK_F3,        strF3);
	assignmapkey (XK_F4,        strF4);
	assignmapkey (XK_F5,        strF5);
	assignmapkey (XK_F6,        strF6);
	assignmapkey (XK_F7,        strF7);
	assignmapkey (XK_F8,        strF8);
	assignmapkey (XK_F9,        strF9);
	assignmapkey (XK_F10,       strF10);
	assignmapkey (XK_F11,       strF11);
	assignmapkey (XK_F12,       strF12);

	#endif

	return true;
}

const bool mapkeyinited= initmapkey ();

} // namespace

std::string string_from_key (unsigned int key)
{
	mapkey_t::const_iterator it= mapkey.find (key);
	if (it != mapkey.end () )
		return it->second;
	return std::string ();
}

// End of key.cpp
