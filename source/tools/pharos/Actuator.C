#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "Actuator.H"
#include "Timer.H"
#include "Experiment.H"
#include "printe.H"

Actuator::Actuator(Store& store_in, int sid_in):
  store(store_in),
  fd(-1), session_id(sid_in),
  tracefilename(NULL), tracefile(NULL), 
  largest_io_size(0), page_align(4096), actual_buffer_size(0),
  raw_buffer(NULL), buffer(NULL), // Initialized in initialize()
  c_io(0), c_timed_io(0), c_delayed_io(0), total_delay(0.0), max_delay(0.0)
{
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
  raw_buffer(NULL), buffer(NULL), // Initialized in initialize()
  c_io(0), c_timed_io(0), c_delayed_io(0), total_delay(0.0), max_delay(0.0)  
{
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
  if (tracefile) {
    (void) fclose(tracefile);
 }
  tracefile = 0;
  
  printf("Executed %u IOs", c_io);
  if (c_timed_io >0) 
    printf(" of which %u were timed (rest ASAP)\n", c_timed_io);
  else
    printf(" (all ASAP).\n");
  if (c_delayed_io >0) {
    printf("Warning: %u IOs = %.1f%% were delayed, average delay %.6fs (average\n"
	   "         amortized over all timed IOs %.6fs), maximum delay %.6fs.\n"
	   "         Probably disk too slow, or not enough parallel IOs allowed.\n",
	   c_delayed_io,
	   100. * (double) c_delayed_io / (double) c_io,
	   total_delay / (double) c_delayed_io,
	   total_delay / (double) c_timed_io,
	   max_delay);
  }	   
}

void Actuator::waitBeforeIO(IOStat& ios)
{
  ++c_io;
  if (! ios.isNow()) {
    ++c_timed_io;
    double waittime = clk()->waitUntil(ios.getTime());
    if (waittime > 0.0) {
      ++c_delayed_io;
      total_delay += waittime;
      if (waittime > max_delay)
	max_delay = waittime;
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
  waitBeforeIO(ios);

  assert(fd>=0);

  ios.setStarted();
  size_t size = ios.getSize();
  assert(size <= largest_io_size);
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

  if (tracefile)
    (void) fprintf(tracefile, "Q %d %s @%Lu %.6f %.6f \n",
		   ios.getSize(),
		   ios.getDir() == IO::Read ? "r" : "w",
		   ios.getLocation(),
		   ios.get_U_Start(),
		   ios.get_U_Stop());

}
