#ifndef _PHAROS_Actuator_H_INCLUDED_
#define _PHAROS_Actuator_H_INCLUDED_ 1

#include <stdio.h>
#include <random>
#include <atomic>

#include "IOStat.H"
#include "Store.H"
#include "Generator.H"

#define DEFAULT_KIOS_COUNT 40

class Actuator
{
public:
  // No default constructor.

  // Use this constructor if you don't want an IO trace generated:
  Actuator(Store& store_in, int sid_in);

  // Use this constructor if you want to generate an IO trace:
  Actuator(Store& store_in, char const* tracefilename_in, int sid_in);

  virtual ~Actuator();

  inline void setStartTime(double _start_time) { start_time = _start_time; }
  inline int getSessionId() const { return session_id; }
  FILE *getTracefile() { return tracefile; }
 
  // Copy constructor and assignment operator are forbidden:
private:
  // Actuator(Actuator const& old);
  // Actuator const& operator=(Actuator const& rhs);

public:
  virtual void initialize(Generator& g); // Use this if you have a generator
  virtual void initialize(size_t largest_io_in); // Use this if you know the largest IO
  virtual void close();

  /* base class implementation is thread-safe */
  virtual void doIO(IOStat& ios);

protected:
  void initializeInternal(unsigned n_buffers);
  void waitBeforeIO(IOStat& ios);
  void TraceSessionStats();
  
protected:
  Store& store;
  int fd;
  int session_id;
  double session_period; //in seconds
  // TODO: Use a string object, don't copy a pointer around
  char const* tracefilename;
  FILE* tracefile;
  size_t largest_io_size;
  size_t page_align, actual_buffer_size;
  char* raw_buffer; // This buffer is not necessary page aligned
  char* buffer;     // This buffer is guaranteed to be page aligned

  location_t prev_io_loc;
  
  // Statistics needed to report whether/how many IOs were delayed.
  std::atomic<unsigned long> c_io;
  std::atomic<unsigned long> c_timed_io;
  std::atomic<unsigned long> c_delayed_io;
  std::atomic<unsigned long long> total_delay_microsec;
  std::atomic<unsigned long long> max_delay_microsec; 
  
    //statistics
  double last_io_released;   
  double start_time, end_time;
  unsigned n_initial_io_count;
  unsigned ios_count;  
  int written;
 
  /* we accumulate one second worth of stats and then dump to file */
  int cur_sec_since_start;
  long slot_ios;
  double slot_ave_lat;

  std::default_random_engine rgen;

};

#endif // _PHAROS_Actuator_H_INCLUDED_
