// Concrete class: random number generator with a uniform distribution.
// The distribution includes the lower bound but not the upper bound.
#ifndef _PHAROS_RandomExponential_H_INCLUDED_
#define _PHAROS_RandomExponential_H_INCLUDED_ 1

#include <assert.h>

#include "Random.H"

class RandomExponential: public Random
{
public:
  RandomExponential(double mean_in);
  virtual ~RandomExponential();
private:
  RandomExponential(RandomExponential const& old); // Not implemented
  RandomExponential const& operator=(RandomExponential const& rhs); // Not implemented

public:
  // This generator doesn't allow drawing of integers;
  virtual i32    getI32() { assert(false); }
  virtual i64    getI64() { assert(false); }
  virtual u32    getU32() { assert(false); }
  virtual u64    getU64() { assert(false); }
  virtual double getDouble();

private:
  double mean;
};

#endif
