// Sets the times for a poisson-distributed interarrival time.  This
// means: if you specify an IO rate of x IOps, then an IO will get
// started every 1/x seconds.  Each IO will happen at a random time.

#ifndef _PHAROS_GeneratorPoisson_H_INCLUDED_
#define _PHAROS_GeneratorPoisson_H_INCLUDED_ 1

#include "Generator.H"
#include "RandomExponential.H"

class GeneratorPoisson: public Generator
{
public:
    GeneratorPoisson(double rate_in);

  virtual ~GeneratorPoisson()
  {}

  // Copy constructor and assignment operator are compiler-generated.

  virtual void make(IO& io, double start_time, double end_time);

  virtual bool is_start_time_required() const { return false; }
  virtual bool is_end_time_required  () const { return false; }
  virtual size_t largest_io() const { return 0; }
  virtual void set_time_first_io(double start_time) {}
  virtual int max_io_count() const { return 0;} 

private:
  double rate;
  RandomExponential random;
  double last_time;
};

#endif // _PHAROS_GeneratorPoisson_H_INCLUDED_

