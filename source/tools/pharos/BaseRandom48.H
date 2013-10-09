#ifndef _PHAROS_BASERANDOM48_H_INCLUDED_
#define _PHAROS_BASERANDOM48_H_INCLUDED_ 1

// Concrete class: Base random number generator, using
// operating-system provided drand48 family of generators.

#include "BaseRandom.H"

class BaseRandom48: public BaseRandom
{
public:
  // Instance function, gets you a copy of the singleton.  If you use this
  // version, you get a random seed that is truly random (derived from the time
  // of day).
  static BaseRandom* initialize();

  // Same function, but the caller can explicitly set the seed:
  static BaseRandom* initialize(int seed);

  // The constructor is protected, since this class can only be
  // constructed by singleton derived classes.
protected:
  BaseRandom48();
  BaseRandom48(int seed);
  virtual ~BaseRandom48();
private:
  BaseRandom48(BaseRandom48 const& old);                  // Not implemented
  BaseRandom48 const& operator=(BaseRandom48 const& rhs); // Not implemented

public:
  virtual double getDouble();
  virtual i32 getI32();

};

#endif
