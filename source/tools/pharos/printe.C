#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "printe.H"

// Print error message:
void printe(const char* fmt, ...)
{
  va_list varargs;
  va_start(varargs, fmt);
  (void) vfprintf(stderr, fmt, varargs);
  fprintf(stderr, "\n");
  va_end(varargs);
}

// Print error message, then exit.
void printx(const char* fmt, ...)
{
  va_list varargs;
  va_start(varargs, fmt);
  (void) vfprintf(stderr, fmt, varargs);
  fprintf(stderr, "\n");
  va_end(varargs);
  exit(1);
}

