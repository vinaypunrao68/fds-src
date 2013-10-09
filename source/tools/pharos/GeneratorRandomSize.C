#include "GeneratorRandomSize.H"
#include "RandomUniform.H"

void GeneratorRandomSize::make(IO& io, double start_time, double end_time)
{
  base.make(io, start_time, end_time);

  RandomUniform size_kb(1.0, 128.0);
  size_t iosize = ((int)size_kb.getDouble())*1024;

  io.setSize(iosize);
  io.setDir(dir);
}
