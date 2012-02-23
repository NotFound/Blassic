#ifndef INCLUDE_BLASSIC_CURSOR_H
#define INCLUDE_BLASSIC_CURSOR_H

// cursor.h
// Revision 7-feb-2005


#include <string>


namespace cursor {

void initconsole ();
void quitconsole ();

size_t getwidth ();

void cursorvisible ();
void cursorinvisible ();
void showcursor ();
void hidecursor ();

void cls ();

//void locate (int row, int col);
void gotoxy (int x, int y);
int getcursorx ();
void movecharforward ();
void movecharback ();
void movecharforward (size_t n);
void movecharback (size_t n);
void movecharup ();
void movechardown ();
void movecharup (size_t n);
void movechardown (size_t n);
void savecursorpos ();
void restorecursorpos ();

void textcolor (int color);
void textbackground (int color);

std::string inkey ();
std::string getkey ();
bool pollin ();

void clean_input ();

void ring ();

void set_title (const std::string & title);

} // namespace cursor

#endif

// Fin de cursor.h
