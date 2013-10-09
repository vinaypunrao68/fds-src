// Makes all IOs the same size, all one direction.

#ifndef _PHAROS_GeneratorRandSize_H_INCLUDED_
#define _PHAROS_GeneratorRandSize_H_INCLUDED_ 1

#include <unistd.h>

#include "GeneratorStackable.H"

#define IOSIZE_KB_MAX 128
#define IOSIZE_KB_MIN 1

class GeneratorRandomSize: public GeneratorStackable
{
public:
  GeneratorRandomSize(Generator& base_in,		  
		     IO::dir_t dir_in):
    GeneratorStackable(base_in),
    dir(dir_in)
  {}

  virtual ~GeneratorRandomSize()
  {}

  // Copy constructor and assignment operator are compiler-generated.

  virtual void make(IO& io, double start_time, double end_time);

  virtual bool is_start_time_required() const
  { return base.is_start_time_required(); }
  virtual bool is_end_time_required  () const
  { return base.is_end_time_required(); }
  virtual size_t largest_io() const 
  { return IOSIZE_KB_MAX*1024; }
  virtual void set_time_first_io(double start_time) 
  	{ base.set_time_first_io(start_time); }
  	
  virtual int max_io_count() const 
  { return base.max_io_count(); } 

private:
  IO::dir_t dir;
};

#endif // _PHAROS_GeneratorRandSize_H_INCLUDED_

