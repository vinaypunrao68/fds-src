// Makes all IOs the same size, all one direction.

#ifndef _PHAROS_GeneratorConstSize_H_INCLUDED_
#define _PHAROS_GeneratorConstSize_H_INCLUDED_ 1

#include <unistd.h>

#include "GeneratorStackable.H"

class GeneratorConstSize: public GeneratorStackable
{
public:
  GeneratorConstSize(Generator& base_in,
		     size_t size_in,
		     IO::dir_t dir_in):
    GeneratorStackable(base_in),
    size(size_in),
    dir(dir_in)
  {}

  virtual ~GeneratorConstSize()
  {}

  // Copy constructor and assignment operator are compiler-generated.

  virtual void make(IO& io, double start_time, double end_time);

  virtual bool is_start_time_required() const
  { return base.is_start_time_required(); }
  virtual bool is_end_time_required  () const
  { return base.is_end_time_required(); }
  virtual size_t largest_io() const 
  { return size; }
  virtual void set_time_first_io(double start_time) 
  	{ base.set_time_first_io(start_time); }
  	
  virtual int max_io_count() const 
  { return base.max_io_count(); } 

private:
  size_t size;
  IO::dir_t dir;
};

#endif // _PHAROS_GeneratorConstSize_H_INCLUDED_

