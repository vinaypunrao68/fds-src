#include <assert.h>

#include "StreamSingleSync.H"
#include "Experiment.H"

void StreamSingleSync::run()
{
  unsigned c_io = 0;
  double start_time = now();
  int max_io_count = g.max_io_count();
  
  assert(how_many==0 || how_long<0.);
  
  g.set_time_first_io(start_time);

  if (how_long>=0.) {
    while(now() < start_time + how_long) {
      IO io;
      g.make(io, 0.0, 0.0);
      IOStat ios(io);
      a.doIO(ios);
      ++c_io;
      
      // stop if generator cannot generate more IOs
      if ((max_io_count > 0) && (c_io > max_io_count))
      	break;      
    }
  }
  else {   
    for (unsigned i=0; i<how_many; ++i) {
      IO io;
      g.make(io, 0.0, 0.0);
      IOStat ios(io);
      a.doIO(ios);
      ++c_io;
      
      // stop if generator cannot generate more IOs
      if ((max_io_count > 0) && (c_io > max_io_count))
      	break;       
    }
  }

  if (experiment()->isNamePresent())
    printf("Experiment_result for %s: ", experiment()->getName());
  else
    printf("Experiment_result: ");
  double runtime = now() - start_time;
  assert(runtime > 0.0);
  printf("%u IOs in %.6lf seconds, rate is %.2lf IOps.\n",
	 c_io, runtime, (double) c_io / runtime);

}
