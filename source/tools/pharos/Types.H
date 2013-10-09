// Type definitions for common things:

#ifndef _PHAROS_TYPES_H_INCLUDED_
#define _PHAROS_TYPES_H_INCLUDED_ 1

// TODO: This is really terrible.  Instead, use Posix- or C99--defined
// types with guaranteed bit width.
#ifdef __linux
  #define _PHAROS_TYPES_FOUND 1
  typedef int i32;
  typedef unsigned int u32;
  typedef long long i64;
  typedef unsigned long long u64;
#endif

#ifndef _PHAROS_TYPES_FOUND
  #error Unknown host/operating system/compiler in PharosTypes.h
#endif

#endif
