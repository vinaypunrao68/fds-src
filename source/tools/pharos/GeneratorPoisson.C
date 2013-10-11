#include <assert.h>

#include "GeneratorPoisson.H"

GeneratorPoisson::GeneratorPoisson(double rate_in):
  rate(rate_in),
  random(1. / rate_in),
  last_time(0.0)
{
  // The following assert is a little silly, as the damage was done above:
  assert(rate > 0.0);
}

void GeneratorPoisson::make(IO& io, double, double)
{
  last_time += random.getDouble();
  io.setTime(last_time);
}
