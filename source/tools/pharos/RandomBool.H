#ifndef _PHAROS_RANDOMBOOL_H_INCLUDED_
#define _PHAROS_RANDOMBOOL_H_INCLUDED_ 1

// Concrete class: generator booleans (decisions) with fixed
// probability.  Warning: Unlike other Random... classes, it does not
// inherit from class Random.

#include "Types.H"
#include "BaseRandom.H"

class RandomBool
{
public:
  // Argument is the probability of getting true:
  RandomBool(double p_in);
  virtual ~RandomBool();
private:
  RandomBool(RandomBool const& old); // Not implemented
  RandomBool const& operator=(RandomBool const& rhs); // Not implemented

public:
  virtual bool getBool();

private:
  double p;
  BaseRandom* base;

};

#endif
