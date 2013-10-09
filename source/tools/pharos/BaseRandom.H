#ifndef _PHAROS_BASERANDOM_H_INCLUDED_
#define _PHAROS_BASERANDOM_H_INCLUDED_ 1

#include "Types.H"

// Abstract base class: The singleton base random generator.

class BaseRandom
{
public:
  // Once a derived class has constructed a concrete instance, anyone
  // can get to it from here:
  static BaseRandom* instance();

  // All derived classes shall support the following method, which shall
  // initialize to a random seed:
  // static BaseRandom* initialize();
  // If they want, they can also support other forms of initialize() that use
  // fixed seeds.  The following is recommended:
  // static BaseRandom* initialize(int seed);

  // The constructor is protected, since this class can only be
  // constructed by singleton derived classes.
protected:
  BaseRandom();
  virtual ~BaseRandom();
private:
  BaseRandom(BaseRandom const& old);                  // Not implemented
  BaseRandom const& operator=(BaseRandom const& rhs); // Not implemented

public:
  // Returns a value uniformly distributed in [0,1) (including 0, not
  // including 1):
  virtual double getDouble() =0;
  // Returns a value uniformly distributed between -2^31 ... 2^31-1
  // (including both limits):
  virtual i32 getI32() =0;

  // TODO: Should we add a 64-bit version here?

protected:
  // To support having single-instance singletons even with multiple
  // concrete derived classes:
  static BaseRandom* it;
};

#endif
