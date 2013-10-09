// Base class for Generators that can be stacked.

#ifndef _PHAROS_GeneratorStackable_H_INCLUDED_
#define _PHAROS_GeneratorStackable_H_INCLUDED_ 1

#include "Generator.H"

class GeneratorStackable: public Generator
{
public:
  // Constructor must take an argument:
  GeneratorStackable(Generator& base_in):
    base(base_in)
  {}

  virtual ~GeneratorStackable()
  {}

  // Copy constructor and assignment operator are compiler generated,
  // but that's mostly irrelevant, as most other Generators don't
  // allow them.

protected:
  Generator& base;
};

#endif // _PHAROS_GeneratorStackable_H_INCLUDED_
