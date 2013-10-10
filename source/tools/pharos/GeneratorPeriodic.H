#ifndef _PHAROS_GeneratorPeriodic_H_INCLUDED_
#define _PHAROS_GeneratorPeriodic_H_INCLUDED_ 1

#include "Generator.H"
#include "RandomUniform.H"

class GeneratorPeriodic: public Generator
{
public:
    GeneratorPeriodic(long period_in, long rate_in, long edt_in);
    
    GeneratorPeriodic(long period_in, long rate_in, int ios_in);

  virtual ~GeneratorPeriodic()
  {}

  // Copy constructor and assignment operator are compiler-generated.

  virtual void make(IO& io, double start_time, double end_time);

  virtual bool is_start_time_required() const { return false; }
  virtual bool is_end_time_required  () const { return false; }

  virtual size_t largest_io() const { return 0; }
  
  virtual void set_time_first_io(double start_time) 
  	{ 
  		if (last_time <= 0.0)
  			last_time = start_time;  		 
  	}
  
  virtual int max_io_count() const { return 0;} 

private:
  long rate; /* in disk-usec/sec */
  double period; /* in sec */
  long exp_dt; /* in usec */
  
  int rand_period;
  RandomUniform random_period;
  RandomUniform random_ios;
  
  double last_time;
  double period_start;
  double ios_per_period;
  double ios_left_in_period; 
};


#endif /*_PHAROS_GeneratorPeriodic_H_INCLUDED_*/
