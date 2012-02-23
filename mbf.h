#ifndef INCLUDE_BLASSIC_MBF_H
#define INCLUDE_BLASSIC_MBF_H

// mbf.h
// Revision 7-feb-2005

// Routines to convert from/to Microsoft Binary Format.


#include <string>


namespace mbf {

double mbf_s (const std::string & s);
double mbf_d (const std::string & s);

std::string to_mbf_s (double v);
std::string to_mbf_d (double v);

} // namespace mbf

#endif

// End of mbf.h
