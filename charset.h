#ifndef INCLUDE_BLASSIC_CHARSET_H
#define INCLUDE_BLASSIC_CHARSET_H

// charset.h
// Revision 13-jun-2006

namespace charset {

typedef unsigned char chardata [8];
typedef chardata chardataset [256];

extern const chardataset default_data;
extern const chardataset cpc_data;
extern const chardataset spectrum_data;
extern const chardataset msx_data;

extern chardataset data;

extern const chardataset * default_charset;

}

#endif

// End of charset.h
