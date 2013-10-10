#include "GeneratorConstSize.H"

void GeneratorConstSize::make(IO& io, double start_time, double end_time)
{
  base.make(io, start_time, end_time);
  io.setSize(size);
  io.setDir(dir);
}
