// A stream that runs one IO at a time.

#ifndef _PHAROS_StreamMultiSync_H_INCLUDED_
#define _PHAROS_StreamMultiSync_H_INCLUDED_ 1

#include <atomic>
#include <mutex>
#include "Stream.H"
#include "PerfStat.H"

#define PHAROS_PRINT_INTERVAL  8  /* in seconds */

class StreamMultiSync: public Stream
{
public:
  // This constructor creates a stream that will run for a fixed number of
  // seconds.
  StreamMultiSync(unsigned int _out_io_count, Actuator& a_in,
	 Generator& g_in,
	 double how_long_in):
    Stream(a_in, g_in, how_long_in),
    out_io_count(_out_io_count),
    max_io_count(g_in.max_io_count())
  {
     io_count = ATOMIC_VAR_INIT(0);
     next_print_time = now() + (double)PHAROS_PRINT_INTERVAL;
     stathist = new fds::StatHistory(a_in.getSessionId(), PHAROS_PRINT_INTERVAL+2, 1);
  }

  // This constructor creates a stream stream that will run for a fixed number
  // of IOs.  The extra boolean argument is to prevent overloading between
  // integral types causing auto-conversion.
  StreamMultiSync(unsigned int _out_io_count, Actuator& a_in,
	 Generator& g_in,
	 unsigned how_long_in,
	 bool):
    Stream(a_in, g_in, how_long_in, true),
    out_io_count(_out_io_count),
    max_io_count(g_in.max_io_count())
  {
     io_count = ATOMIC_VAR_INIT(0);
     next_print_time = now() + (double)PHAROS_PRINT_INTERVAL;
     stathist = new fds::StatHistory(a_in.getSessionId(), PHAROS_PRINT_INTERVAL+2, 1);
  }

  virtual ~StreamMultiSync()
  {
    if (stathist) delete stathist;
  }

  static void start_dispatch_thread(StreamMultiSync *self);

  // Copy constructor and assignment operator are forbidden:
private:
  // StreamMultiSync(StreamMultiSync const& old);
  // StreamMultiSync const& operator=(StreamMultiSync const& rhs);

  void dispatch_io_thread();
  void dispatch_io_thread_howlong();
  void dispatch_io_thread_howmany();
  void recordStat(const fds::IoStat& iostat, double now);

public:
  virtual void run();

private: /* data */
  unsigned int out_io_count; /* max number of outstanding IOs */

  unsigned long max_io_count;
  std::atomic<unsigned long> io_count; /* counting number of IOs we completed */

  /* protects access generator (base class's 'g') and stathist */
  std::mutex write_lock; 

  /* we record stats here */
  double start_time;
  double next_print_time;
  fds::StatHistory* stathist;
};

#endif // _PHAROS_StreamMultiSync_H_INCLUDED_
