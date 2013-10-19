#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <boost/random/random_device.hpp>

#include "Actuator.H"
#include "Timer.H"
#include "Experiment.H"
#include "printe.H"

Actuator::Actuator(Store& store_in, int sid_in):
  store(store_in),
  fd(-1), session_id(sid_in),
  tracefilename(NULL), tracefile(NULL), 
  largest_io_size(0), page_align(4096), actual_buffer_size(0),
  raw_buffer(NULL), buffer(NULL) // Initialized in initialize()
{
  c_io = ATOMIC_VAR_INIT(0);
  c_timed_io = ATOMIC_VAR_INIT(0);
  c_delayed_io = ATOMIC_VAR_INIT(0);
  total_delay_microsec = ATOMIC_VAR_INIT(0);
  max_delay_microsec = ATOMIC_VAR_INIT(0);

  prev_io_loc = 0;
  last_io_released = 0.0;
  start_time = -1.0;
  end_time = 0.0;
  ios_count = 0;
  n_initial_io_count = 0;

  written = 0;

  cur_sec_since_start = 0;
  slot_ios = 0;
  slot_ave_lat =0.0;
}

Actuator::Actuator(Store& store_in, char const* tracefilename_in, int sid_in):
  store(store_in),
  fd(-1), session_id(sid_in), 
  tracefilename(tracefilename_in), tracefile(NULL),
  largest_io_size(0), page_align(4096), actual_buffer_size(0),
  raw_buffer(NULL), buffer(NULL) // Initialized in initialize()  
{
  c_io = ATOMIC_VAR_INIT(0);
  c_timed_io = ATOMIC_VAR_INIT(0);
  c_delayed_io = ATOMIC_VAR_INIT(0);
  total_delay_microsec = ATOMIC_VAR_INIT(0);
  max_delay_microsec = ATOMIC_VAR_INIT(0);

  prev_io_loc = 0;
  last_io_released = 0.0;
  start_time = -1.0;
  end_time = 0.0;
  ios_count = 0;
  n_initial_io_count = 0;
    
  written = 0;

  cur_sec_since_start = 0;
  slot_ios = 0;
  slot_ave_lat =0.0;
}

Actuator::~Actuator()
{
  if (raw_buffer)
    delete[] raw_buffer;
}

void Actuator::initialize(Generator& g) {
  largest_io_size = g.largest_io();
  assert(largest_io_size > 0);
  initializeInternal(1);
}

void Actuator::initialize(size_t largest_io_size_in) {
  largest_io_size = largest_io_size_in;
  assert(largest_io_size > 0);
  initializeInternal(1);
}
double latency = 0.0;
void Actuator::initializeInternal(unsigned n_buffers)
{
  // By default, the IO buffers are aligned on 4096-byte page boundaries.
  // You can override that with the PHAROS_PAGE_ALIGN variable.  Setting it 0
  // means they are not aligned.
  if (getenv("PHAROS_PAGE_ALIGN")) {
    assert(atoi(getenv("PHAROS_PAGE_ALIGN")) >= 0);
    page_align = (size_t) atoi(getenv("PHAROS_PAGE_ALIGN"));
    printf("Warning: Page alignment overridden with PHAROS_PAGE_ALIGN = %u\n",
	   page_align);
  }

  actual_buffer_size = largest_io_size;
  if (page_align > 0)
    actual_buffer_size = ( largest_io_size + page_align -1 ) -
                         ( largest_io_size + page_align -1 ) % page_align;

  raw_buffer = new char[n_buffers * actual_buffer_size + page_align];
  assert(raw_buffer);
  buffer = raw_buffer + page_align -
           ( (size_t) raw_buffer % page_align );

  fd = store.getFD();

  if (tracefilename) {
    tracefile = fopen(tracefilename, "w");
    if (!tracefile)
      printx("Actuator: Unable to create tracefile %s: %s\n",
	      tracefilename, strerror(errno));

    printf("Actuator: will write to tracefile %s\n", tracefilename);
    /*if (experiment()->isNamePresent())
      fprintf(tracefile, "Trace offset %.6f for %s\n",
	      clk()->originOffset(), experiment()->getName());
    else
      fprintf(tracefile, "Trace offset %.6f\n",
	      clk()->originOffset());      */   
  }  
  
}

void Actuator::close()
{
  unsigned long long totdelay_microsec = atomic_load(&total_delay_microsec);
  unsigned long long maxdelay_microsec = atomic_load(&max_delay_microsec);
  double total_delay = (double)totdelay_microsec / 1000000.0;
  double max_delay = (double)maxdelay_microsec / 1000000.0;
  unsigned long iocount = atomic_load(&c_io);
  unsigned long timed_ios = atomic_load(&c_timed_io);
  unsigned long delayed_ios = atomic_load(&c_delayed_io);

  if (tracefile) {
    (void) fclose(tracefile);
 }
  tracefile = 0;
  
  printf("Executed %lu IOs", iocount);
  if (timed_ios >0) 
    printf(" of which %lu were timed (rest ASAP)\n", timed_ios);
  else
    printf(" (all ASAP).\n");
  if (delayed_ios >0) {
    printf("Warning: %lu IOs = %.1f%% were delayed, average delay %.6fs (average\n"
	   "         amortized over all timed IOs %.6fs), maximum delay %.6fs.\n"
	   "         Probably disk too slow, or not enough parallel IOs allowed.\n",
	   delayed_ios,
	   100. * (double) delayed_ios / (double) iocount,
	   total_delay / (double) delayed_ios,
	   total_delay / (double) timed_ios,
	   max_delay);
  }	   
}

void Actuator::waitBeforeIO(IOStat& ios)
{
  long cur_numio = atomic_fetch_add(&c_io, (unsigned long)1);
  if (! ios.isNow()) {
    atomic_fetch_add(&c_timed_io, (unsigned long)1);
    double waittime = clk()->waitUntil(ios.getTime());
    if (waittime > 0.0) {
      unsigned long long waittime_microsec = (unsigned long long)(waittime*1000000.0);
      unsigned long long cur_max_delay = atomic_load(&max_delay_microsec);
      atomic_fetch_add(&c_timed_io, (unsigned long)1);
      atomic_fetch_add(&total_delay_microsec, waittime_microsec);
      if (waittime_microsec > cur_max_delay)
	atomic_store(&max_delay_microsec, waittime_microsec);
    }
  }
}

void Actuator::TraceSessionStats()
{
	int i;
        
   	//initialize summary trace file -- always written
	/*
  	char tracefn[256];
  	if (experiment()->isNamePresent())
  		sprintf(tracefn, "trace_%s", experiment()->getName());
  	else
  		sprintf(tracefn, "trace_");	
  	    
  	FILE* tf = fopen(tracefn, "w");
  	if (!tf)
      		printx("Unable to create tracefile %s: %s\n", tracefn, strerror(errno));
	*/

        /* get stats */
        double runtime = end_time - start_time;

        double thruput = (double) ios_count / runtime;

	// IOPS per 10 second interval
	/*	for (i = 0; i < 1000; i++)
		{ 
			if (tf) {
			   (void) fprintf(tf, "%d\t%ld\t%.3f\n",
					(i+1)*10,
					s10_io[i]/10, // throughput during 10sec interval
					(ios_10s_latency[i] / (double) s10_io[i])*1000.0 // average latency during 10sec interval in ms
				        );	
			}

		}

	(void) fclose(tf);
	*/
        printf("Experiment ran for %.2f seconds, throughput = %.2f IO/sec\n", runtime, thruput);
	 
}

void Actuator::doIO(IOStat& ios)
{
  boost::random::random_device rd;
  waitBeforeIO(ios);

  assert(fd>=0);

  size_t size = ios.getSize();
  assert(size <= largest_io_size);
  /* fill in buffer with some junk data */
  for (int i = 0; i < size; ++i) {
    buffer[i] = rd();
  }

  ios.setStarted();
  ssize_t got = 
    ios.getDir() == IO::Read
    ? pread(fd, buffer, size, ios.getLocation())
    : pwrite(fd, buffer, size, ios.getLocation());

  if (got != (ssize_t) size) {
      printe("Error %s: Expect %u, got %d bytes.\n",
	      ios.getDir() == IO::Read ? "reading" : "writing",
	      size, got);
      if (got < 0)
	  printe("Reason: %s\n", strerror(errno));
  }
  assert(got == (ssize_t) size);

  ios.setStopped();

/*
  if (tracefile)
    (void) fprintf(tracefile, "Q %d %s @%Lu %.6f %.6f \n",
		   ios.getSize(),
		   ios.getDir() == IO::Read ? "r" : "w",
		   ios.getLocation(),
		   ios.get_U_Start(),
		   ios.get_U_Stop());
*/

}
