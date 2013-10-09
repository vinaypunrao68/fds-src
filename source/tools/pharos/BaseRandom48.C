#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "BaseRandom48.H"

/* static */
BaseRandom* BaseRandom48::initialize()
{
  if (!it)
  {
    it = new BaseRandom48();
  }
  return it;
}

/* static */
BaseRandom* BaseRandom48::initialize(int seed)
{
  if (!it)
  {
    it = new BaseRandom48(seed);
  }
  return it;
}

BaseRandom48::BaseRandom48()
{
  unsigned short s[3];

  struct timeval tv;
  (void) gettimeofday(&tv, 0);
  assert( sizeof(tv.tv_sec) >= 4 );
  assert( sizeof(tv.tv_usec) >= 4);
  short* ssss = (short*) &tv.tv_sec;
  s[0] = ssss[0];
  s[1] = ssss[1];
  ssss = (short*) &tv.tv_usec;
  s[2] = ssss[0] + ssss[1];

  (void) seed48(s);
}

BaseRandom48::BaseRandom48(int seed)
{
  // srand48 requires a 32-bit number:
  assert(sizeof(seed) == 4);
  (void) srand48(seed);
}

BaseRandom48::~BaseRandom48()
{
  // Interestingly, the destructor will never be called.
}

double BaseRandom48::getDouble()
{
  return drand48();
}

i32 BaseRandom48::getI32()
{
  return mrand48();
}
