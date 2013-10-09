#include "GeneratorSeq.H"

void GeneratorSeq::make(IO& io, double start_time, double end_time)
{
  base.make(io, start_time, end_time);

  location_t where = last;

  last += io.getSize();
  if (last >= store_capacity)
    where = last = 0;

  io.setLocation(where);
}
