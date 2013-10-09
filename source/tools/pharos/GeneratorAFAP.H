// Sets all times to AFAP.

#ifndef _PHAROS_GeneratorAFAP_H_INCLUDED_
#define _PHAROS_GeneratorAFAP_H_INCLUDED_ 1

#include "Generator.H"

class GeneratorAFAP: public Generator
{
public:
  GeneratorAFAP()
  {}

  virtual ~GeneratorAFAP()
  {}

  // Copy constructor and assignment operator are compiler-generated.

  virtual void make(IO& io, double start_time, double end_time);

  virtual bool is_start_time_required() const { return false; }
  virtual bool is_end_time_required  () const { return false; }
  virtual size_t largest_io() const { return 0; }
  virtual void set_time_first_io(double start_time) {}
  virtual int max_io_count() const { return 0;} 
  

};

#endif // _PHAROS_GeneratorAFAP_H_INCLUDED_

