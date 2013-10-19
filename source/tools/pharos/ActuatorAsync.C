#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ActuatorAsync.H"
#include "printe.H"
#include "Experiment.H"

#undef ASYNC_DEBUG

#ifdef ASYNC_DEBUG
#else
  #define printd(MESSAGE) ((void)0)
#endif

ActuatorAsync::ActuatorAsync(unsigned q_in, Store& store_in, int sid_in):
  Actuator(store_in, sid_in),
  q(q_in),
  n_running(0),
  immediate_poll(false), max_running(0)
{
  cbs = new aiocb[q];
  assert(cbs);
  // They will be initialized in initialize()

  running = new bool[q];
  assert(running);

  ioss = new IOStat*[q];
  assert(ioss);
}

ActuatorAsync::ActuatorAsync(unsigned q_in, Store& store_in, char const* tracefilename_in, int sid_in):
  Actuator(store_in, tracefilename_in, sid_in),
  q(q_in),
  n_running(0),
  immediate_poll(false), max_running(0)
{
  cbs = new aiocb[q];
  assert(cbs);
  // They will be initialized in initialize()

  running = new bool[q];
  assert(running);

  ioss = new IOStat*[q];
  assert(ioss);
}

ActuatorAsync::~ActuatorAsync()
{
  delete[] cbs;
  delete[] running;
  delete[] ioss;
}

void ActuatorAsync::initialize(Generator& g) {
  largest_io_size = g.largest_io();
  assert(largest_io_size > 0);
  initializeAsyncInternal();
}

void ActuatorAsync::initialize(size_t largest_io_in) {
  largest_io_size = largest_io_in;
  assert(largest_io_size > 0);
  initializeAsyncInternal();
}

void ActuatorAsync::initializeAsyncInternal()
{
  if (getenv("PHAROS_ASYNC_IMMEDIATE")) {
    immediate_poll = true;
    printf("Warning: Immediate polling of async IO enabled, will "
	   "use lots of CPU time\n"
           "         to get questionable accuracy benefit.\n");
  }

  if (getenv("PHAROS_ASYNC_INIT")) {
    #if defined(__USE_GNU) && defined(_AIO_H)
    struct aioinit dirtyhack;
    dirtyhack.aio_threads = 0;
    dirtyhack.aio_num = q;
    dirtyhack.aio_idle_time = 0;
    aio_init(&dirtyhack);
    printf("WARNING: PHAROS_ASYNC_INIT requested AIO initialized "
	   "with aio_init.\n");
    #elif defined(__AIO_H)
    printe("WARNING: You requested PHAROS_ASYNC_INIT, "
           "but it is not provided by libposix-aio.\n");
    #else
    printe("WARNING: You requested PHAROS_ASYNC_INIT, "
	   "but it is not provided by the unknown library.\n");
    #endif
  }

  Actuator::initializeInternal(q);

  for (unsigned i=0; i<q; ++i) {
    cbs[i].aio_fildes = fd;
    cbs[i].aio_buf = buffer + i*actual_buffer_size;
    cbs[i].aio_reqprio = 0;
    cbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
    running[i] = false;
    ioss[i] = NULL;
  }
  printf("ActuatorAsync %u initialized.\n", q);
}

void ActuatorAsync::close()
{
  // For all IOs that are still running, wait for them to finish.
  unsigned c_running = 0;
  unsigned c_suspend = 0;

  printf("ActuatorAsync::close()\n");

  // If we want to poll all IOs as quickly as possible, then we suspend on all
  // running IOs, and then poll over all of them.
  if (immediate_poll) {
    while (n_running > 0) {
      ++c_suspend;
      suspendUntilOneIOFinishes();
      for (unsigned i=0; i<q; ++i) {
	if (running[i])
	  checkTermination(i);
      }
    }
    printd(("ActuatorAsync close, had to suspend %u times in poll mode.\n",
	    c_suspend));
  }

  // If we don't want to poll, we suspend on each IO in turn, which is much
  // more efficient:
  else {
    for (unsigned i=0; i<q; ++i) {
      if (running[i]) {
	++c_running;
	checkTermination(i);
      }

      if (running[i]) {
	++c_suspend;
	aiocb* cbp = &cbs[i];
	int status = aio_suspend(&cbp, 1, NULL);
	if (status != 0)
	  printx("Failed to suspend when cleaning out AIO %u: %s\n",
		 i, strerror(errno));
	checkTermination(i);
	if (running[i])
	  printx("Cleaning out AIO %u didn't terminate after suspend.\n", i);
	if (ioss[i] != NULL)
	  printx("Cleaning out AIO %u didn't delete its IOStat.\n", i);
      }
    }
    printd(("ActuatorAsync close, %u IOs were running, "
	    "had to suspend %u times.\n",
	    c_running, c_suspend));
	    }
  printf("ActuatorAsync::close() - 2\n");

  TraceSessionStats();
  Actuator::close();

  if (max_running >0)
    printf("Maximum number of IOs running in parallel was %u\n", max_running);
}

// Used only if we want to poll all the IOs all the time, to get the IO
// termination time most accurately.
void ActuatorAsync::pollAllIOs()
{
  for (unsigned i=0; i<q; ++i) {
    if (running[i])
      checkTermination(i);
  }
  // The +1 accounts for the IO that's about to start
  if (n_running+1 > max_running)
    max_running = n_running+1;
}

// Suspend the process until at least one IO finishes.
void ActuatorAsync::suspendUntilOneIOFinishes()
{
  aiocb* cbp[q];
  unsigned c_cbp = 0;

  for (unsigned i=0; i<q; ++i) {
    if (running[i])
      checkTermination(i);

    if (running[i]) {
      cbp[c_cbp] = &cbs[i];
      ++c_cbp;
    }
  }
  if (c_cbp > 0) {
    int status = aio_suspend(cbp, c_cbp, NULL);
    if (status != 0)
      printx("Failed to suspend when waiting for AIO: %s\n", strerror(errno));
  }
}

// If the IO is still running, check whether it has terminated.
// If it actually has terminated, do all the end processing.
// If the IO is no longer running, this is a no-op.
void ActuatorAsync::checkTermination(unsigned i_cb)
{
  if (!running[i_cb])
    return;

  int status = aio_error(&cbs[i_cb]);

  if (status == EINPROGRESS)
    return;

  else if (status != 0)
    printx("Aio %d failed to get status: %s\n", i_cb, strerror(status));

  // It finished.  Get the actual return code:
  int rc = aio_return(&cbs[i_cb]);
  if (rc < 0)
    printx("Aio %d failed to execute: %s\n", i_cb, strerror(errno));
  if ((size_t) rc != cbs[i_cb].aio_nbytes)
    printx("Aio %d read/wrote %d bytes, expect %u.\n",
	    i_cb, rc, cbs[i_cb].aio_nbytes);


  // OK, so it finished successfully.
  running[i_cb] = false;
  --n_running;
 
  // Do the statistics end processing:
  ioss[i_cb]->setStopped();
  
  end_time = now();  
  ios_count++;
    
  /* here we need to add stats to record IOPs (based on when IO returned, which is now */
  /* we can just record one second of stats, and if we are starting next second, just dump to tracefile */     
  double latency = ioss[i_cb]->get_U_Stop() - ioss[i_cb]->get_U_Start();  
  size_t iosize = ioss[i_cb]->getSize();
  IO::dir_t iotype = ioss[i_cb]->getDir(); // IO::Read or IO::Write
 
   /* are we still recording the same time slot */
   double elapsed_sec = end_time - start_time;
   int sec_slot = elapsed_sec; 
   if (sec_slot != cur_sec_since_start) {
       assert(sec_slot > cur_sec_since_start);
       /* print stats we accumulated for the previous slot */
      if (tracefile)
          (void) fprintf(tracefile, "%.2f,%d,%ld,%ld,%.6f,%d,%d\n",
                         end_time,session_id, (long)start_time + (long)cur_sec_since_start,slot_ios,
                         slot_ave_lat*1000000.0,0,0);    
      /* reset stats */
      slot_ios = 0;
      slot_ave_lat = 0;
      
      /* print zeros if there are slots in between cur_sec_since_start and sec_slot */
      for (int i = (cur_sec_since_start+1); i < sec_slot; ++i)
      {
          if (tracefile)
              (void) fprintf(tracefile, "%.2f,%d,%ld,%d,%d,%d,%d\n",
                             end_time,session_id,(long)start_time + (long)i,0,0,0,0);    
      }
 
      /* start next slot */
      cur_sec_since_start = sec_slot;
   }
   double temp = slot_ios;
   ++slot_ios;
   slot_ave_lat = (temp*slot_ave_lat + latency)/(double)slot_ios;
  		    
  // As we are done with the IO now, we can delete the copy of the IOStat we made:
  delete ioss[i_cb];
  ioss[i_cb] = NULL;
}

// Return the number of a free (not running) CB.
// If all are busy:
//   If  wait_if_all_busy, it will block until one finishes.
//   If !wait_if_all_busy, it will return a number >q.
unsigned ActuatorAsync::findCb(bool wait_if_all_busy)
{
  // If we want IOs to be finished as quickly as possible, then we loop over
  // all of them every time an IO starts.  This is also the only way to measure
  // the maximum number of IOs ever running in parallel.
  if (immediate_poll)
    pollAllIOs();

  printd(("findCb(%s): ", wait_if_all_busy?"wait":"nowait"));

  // We do brute-force, checking all IOs:
  for (unsigned i=0; i<q; ++i) {
    bool was_finished = true;
    if (running[i]) {
      checkTermination(i);
      was_finished = false;
    }
    if (!running[i]) {
      printd(("Nowait found %u, was %s finished.\n", i, was_finished?"already":"not yet"));
      return i;
    }
  }

  // If we get here, all IOs are still running.
  if (!wait_if_all_busy) {
    printd(("None ready, returning.\n"));
    return q+1;
  }

  suspendUntilOneIOFinishes();
  printd(("... waiting "));

  for (unsigned i=0; i<q; ++i) {
    checkTermination(i);
    if (!running[i]) {
      printd(("Had to wait for %u.\n", i));
      return i;
    }
  }

  // If we fell through this loop, aio_suspend lied to us:
  printf("aio_suspend didn't result in any finished IO.\n");
}

void ActuatorAsync::doIO(IOStat& ios)
{	
  waitBeforeIO(ios);

  if (start_time < 0.0) {
  	start_time = now();  
  }
  assert(fd>=0);

  unsigned i_cb = findCb(true);
  printd(("AIO %u starting.\n", i_cb));

  // We need our own copy of the IOStat for as long as the IO is running, to
  // hold the trace timing information.
  ioss[i_cb] = new IOStat(ios);
  ioss[i_cb]->setStarted();
  
  assert(ios.getSize() <= largest_io_size);
  cbs[i_cb].aio_nbytes = ios.getSize();
  cbs[i_cb].aio_offset = ios.getLocation();

  ++n_running;
  running[i_cb] = true;

  int status = 
    ios.getDir() == IO::Read
    ? aio_read(&cbs[i_cb])
    : aio_write(&cbs[i_cb]);
  

  if (status != 0)
    printx("Error queueing AIO %d: %s\n", i_cb, strerror(errno));

  // The statistics end processing will be done in the completion routine
  // checkTermination().
}
