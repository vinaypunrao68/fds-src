#ifndef _PHAROS_RANDOM_H_INCLUDED_
#define _PHAROS_RANDOM_H_INCLUDED_ 1

// Abstract base class: Interface all random number generators have
// to follow.

// Concrete random number generators all use one base generator, which
// is a singleton, and manages seeding.  These random generators
// actually don't generate random numbers; instead they convert base
// generator random numbers into particular distributions.

#include "Types.H"
#include "BaseRandom.H"

class Random
{
public:
  Random();
  virtual ~Random();
private:
  Random(Random const& old); // Not implemented
  Random const& operator=(Random const& rhs); // Not implemented

public:
  // Get integer and unsigned and double values:
  virtual i32    getI32() =0;
  virtual i64    getI64() =0;
  virtual u32    getU32() =0;
  virtual u64    getU64() =0;
  virtual double getDouble() =0;

protected:
  // Implementation helpers: Return result converted between ranges
  i32 doubleToI32();
  u32 doubleToU32();
  i64 doubleToI64();
  u64 doubleToU64();
  double i32ToDouble();
  double u32ToDouble();
  double i64ToDouble();
  double u64ToDouble();

  // The base generator, set in the constructor:
  BaseRandom* base;

};

#endif
