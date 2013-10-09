#include <assert.h>
#include "GeneratorRandom.H"

void GeneratorRandom::make(IO& io, double start_time, double end_time)
{
  base.make(io, start_time, end_time);

  location_t next = random.getU64();
  next = next - next % io.getSize();
  assert(next + io.getSize() <= store_capacity);

  io.setLocation(next);
}
