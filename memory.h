#ifndef INCLUDE_BLASSIC_MEMORY_H
#define INCLUDE_BLASSIC_MEMORY_H

// memory.h
// Revision 1-feb-2005

#include "blassic.h"

namespace blassic {

namespace memory {

size_t dyn_alloc (size_t memsize);
void dyn_free (size_t mempos);
void dyn_freeall ();

} // namespace memory

} // namespace blassic

#endif

// End of memory.h
