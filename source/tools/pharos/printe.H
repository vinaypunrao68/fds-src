#ifndef _PHAROS_printe_H_INCLUDED_
#define _PHAROS_printe_H_INCLUDED_ 1

// Print to stderr, as conveniently as to stdout:
void printe(const char* fmt, ...);
// Print to stderr, then exit the program:
void printx(const char* fmt, ...) __attribute__ ((noreturn));

#endif
