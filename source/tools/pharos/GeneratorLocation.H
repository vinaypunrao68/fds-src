// Set the location to an arbitrary random number generator, provided by the
// caller.  It is the responsibility of the caller to ensure that the random
// number generator only generates random locations such that the IOs are
// completely contained in the capacity of the store.  This is verified by this
// driver.

#ifndef _PHAROS_GeneratorLocation_H_INCLUDED_
#define _PHAROS_GeneratorLocation_H_INCLUDED_ 1

#include "GeneratorStackable.H"
#include "Store.H"
#include "Random.H"

class GeneratorLocation: public GeneratorStackable
{
public:
  GeneratorLocation(Generator& base_in,
		    Store const& store_in,
		    Random* random_in):
    GeneratorStackable(base_in),
    store_capacity(store_in.getCapacity()),
    random(random_in)
  {}

  virtual ~GeneratorLocation()
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
  virtual int max_io_count() const { return 0;}   

private:
  location_t store_capacity;
  Random* random;
};

#endif // _PHAROS_GeneratorLocation_H_INCLUDED_

