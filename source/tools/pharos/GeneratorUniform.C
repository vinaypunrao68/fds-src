#include <assert.h>

#include "GeneratorUniform.H"

GeneratorUniform::GeneratorUniform(double rate_in):
  rate(rate_in),
  interarrival_time(0.0),
  last_time(0.0)
{
  assert(rate > 0.0);
  interarrival_time = 1. / rate;
}

void GeneratorUniform::make(IO& io, double, double)
{
  last_time += interarrival_time;
  io.setTime(last_time);
}
