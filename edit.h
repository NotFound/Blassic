#ifndef INCLUDE_BLASSIC_EDIT_H
#define INCLUDE_BLASSIC_EDIT_H

// edit.h
// Revision 23-jul-2004

#include "blassic.h"
#include "file.h"

namespace blassic {

namespace edit {

bool editline (blassic::file::BlFile & bf, std::string & str,
	size_t npos, size_t inicol= 0, bool lineend= true);

} // namespace blassic

} // namespace edit

#endif

// End of edit.h
