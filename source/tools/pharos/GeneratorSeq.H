// Set the location to all sequential.  The IOs must have their size
// set beforehand.

#ifndef _PHAROS_GeneratorSeq_H_INCLUDED_
#define _PHAROS_GeneratorSeq_H_INCLUDED_ 1

#include "GeneratorStackable.H"
#include "Store.H"

class GeneratorSeq: public GeneratorStackable
{
public:
  // Use this constructor if you want to start the sequence at zero:
  GeneratorSeq(Generator& base_in, Store const& store_in):
    GeneratorStackable(base_in),
    store_capacity(store_in.getCapacity()),
    last(0)
  {}

  // Use this constructor if you want to start the sequence at a defined
  // starting point:
  GeneratorSeq(Generator& base_in, Store const& store_in, location_t start):
    GeneratorStackable(base_in),
    store_capacity(store_in.getCapacity()),
    last(start)
  {}

  virtual ~GeneratorSeq()
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
  location_t last;
};

#endif // _PHAROS_GeneratorSeq_H_INCLUDED_

