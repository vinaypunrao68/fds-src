// Concrete class: random number generator with a uniform distribution.
// The distribution includes the lower bound but not the upper bound.
#ifndef _PHAROS_RANDOMUNIFORM_H_INCLUDED_
#define _PHAROS_RANDOMUNIFORM_H_INCLUDED_ 1

#include "Random.H"

class RandomUniform: public Random
{
public:
  // TODO: Do we need versions with integer arguments?
  RandomUniform(double l_in, double u_in);
  virtual ~RandomUniform();
private:
  RandomUniform(RandomUniform const& old); // Not implemented
  RandomUniform const& operator=(RandomUniform const& rhs); // Not implemented

public:
  virtual i32    getI32() { return doubleToI32(); }
  virtual i64    getI64() { return doubleToI64(); }
  virtual u32    getU32() { return doubleToU32(); }
  virtual u64    getU64() { return doubleToU64(); }
  virtual double getDouble();

private:
  double l, u;
  double u_minus_l;

};

#endif
