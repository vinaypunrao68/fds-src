#include <stdio.h>
#include <stdlib.h>

#include "BaseRandom.H"

/* static */
BaseRandom* BaseRandom::it = 0;

/* static */
BaseRandom* BaseRandom::instance()
{
  if (!it)
  {
    fprintf(stderr, "BaseRandom: Fatal error: not initialized yet.  "
                    "Please call initialize()\n"
                    "            on a concrete derived class before "
 	            "using this class.\n");
    exit(1);
    // TODO: Better error handling here
  }
  return it;
}

BaseRandom::BaseRandom()
{}

BaseRandom::~BaseRandom()
{}

