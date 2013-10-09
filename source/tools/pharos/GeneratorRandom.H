// Set the location to all random, uniformly distributed within the whole
// capacity of the store.  The IOs must have their size set beforehand.

#ifndef _PHAROS_GeneratorRandom_H_INCLUDED_
#define _PHAROS_GeneratorRandom_H_INCLUDED_ 1

#include "GeneratorStackable.H"
#include "Store.H"
#include "RandomUniform.H"

class GeneratorRandom: public GeneratorStackable
{
public:
  GeneratorRandom(Generator& base_in, Store const& store_in):
    GeneratorStackable(base_in),
    store_capacity(store_in.getCapacity()),
    random(0, store_capacity)
  {}

  virtual ~GeneratorRandom()
  {}

  // Copy constructor and assignment operator are compiler-generated.

  virtual void make(IO& io, double start_time, double end_time);

  virtual bool is_start_time_required() const
  { return base.is_start_time_required(); }
  virtual bool is_end_time_required  () const
  { return base.is_end_time_required(); }
  virtual size_t largest_io() const
  { return base.largest_io(); }
  
  virtual void set_time_first_io(double start_time)
  { base.set_time_first_io(start_time); }
  
  virtual int max_io_count() const 
  { return base.max_io_count(); } 

private:
  location_t store_capacity;
  RandomUniform random;
};

#endif // _PHAROS_GeneratorRandom_H_INCLUDED_

