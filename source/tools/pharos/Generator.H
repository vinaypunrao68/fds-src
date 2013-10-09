// A generator is something that makes IOs parameters.  It does this
// by modifying IO objects.

// This class is totally abstract.  That's good, because it will be
// multiple inherited from.

#ifndef _PHAROS_Generator_H_INCLUDED_
#define _PHAROS_Generator_H_INCLUDED_ 1

#include "IO.H"

// This is an abstract base class
class Generator
{
public:
  // Constructors are all compiler-generated.
  virtual ~Generator()
  {};

  // Call is_start_time_needed and is_end_time needed to find out
  // whether you have to supply the start time of this IO, and the end
  // time of the previous IO.  If you don't, you can specify zero, and
  // save a little CPU time.
  virtual void make(IO& io, double start_time, double end_time) = 0;

  virtual bool is_start_time_required() const = 0;
  virtual bool is_end_time_required  () const = 0;

  // Call this to find out the largest IO this generator will ever make:
  virtual size_t largest_io() const = 0;
  
  //set time when first I/O should start
  virtual void set_time_first_io(double start_time) = 0;
  
  //Call this to find out max number of IOs this generator can generate
  // 0 means infinity
  virtual int max_io_count() const = 0; 
 
  
};

#endif // _PHAROS_Generator_H_INCLUDED_

