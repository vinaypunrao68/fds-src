/*
 * Copyright 2013 Formation Data Systems, Inc.
 * 
 * Tests a single token bucket object 
 */

#include <iostream> // NOLINT(*)
#include <string>
#include <boost/thread/thread.hpp>

#include "qos_tokbucket.h"
#include "test_stat.h"

namespace fds {

class TBUnitTest {
public:
  
  StatIOPS* iops_stats;
  fds_uint32_t volume_id;

  fds_uint64_t interarrival_usec;  /* time in microsec between IO arrival, 0 means as fast as possible */
  int on_seconds;  /* for on-off behavior -- time between idle times  */
  int off_seconds;
  boost::posix_time::ptime next_idle;
  boost::posix_time::ptime next_io;

public: /* methods */
  TBUnitTest(const std::string& testname, fds_uint64_t _interarrival_usec, int _on_seconds, int _off_seconds)
    : interarrival_usec(_interarrival_usec), on_seconds(_on_seconds), off_seconds(_off_seconds), volume_id(2) 
  {
     std::vector<fds_uint32_t> volids;
     volids.push_back(volume_id);

     iops_stats = new StatIOPS(testname, volids);
     next_idle = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(on_seconds);
     next_io = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::microseconds(interarrival_usec);
  }
  ~TBUnitTest() 
  {
     delete iops_stats;
  }

  inline void io_arrival(boost::posix_time::ptime& ts)
  {
    boost::posix_time::ptime now = ts;
  
    /* one io finished, so calculate next time */
    if (interarrival_usec > 0)
      next_io += boost::posix_time::microseconds(interarrival_usec);

    /* deal with idle intervals */
    if ( (off_seconds > 0) && (ts > next_idle)) {
        boost:this_thread::sleep(boost::posix_time::seconds(off_seconds));
        now = boost::posix_time::microsec_clock::universal_time();
        next_idle = now + boost::posix_time::seconds(on_seconds);
        
        next_io = now + boost::posix_time::microseconds(interarrival_usec);
       
    }

    /* sleep if io has not arrived yet */
    if ((interarrival_usec > 0) && (ts < next_io)) {
      boost::posix_time::time_duration delay = next_io - now;
      long long delay_microsec = delay.total_microseconds();
      if (delay_microsec > 0)
	boost::this_thread::sleep(boost::posix_time::microseconds(delay_microsec));      
    }
  }

  int run(int run_seconds, fds_uint64_t tb_rate, fds_uint64_t tb_burst) 
  {
    int result = 0;
    boost::posix_time::ptime start_ts = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::ptime now;
    int cur_duration = 0;
    TokenBucket tb(tb_rate, tb_burst);

    std::cout << "Start running TBUnitTest" << std::endl;

    /* just in main thread */
    while (cur_duration < run_seconds)
    {
       tb.updateTBState();
       while ( !tb.tryToConsumeTokens(1) ) {
          fds_uint64_t delay = tb.getDelayMicrosec(1);
          if (delay > 0) {
             boost::this_thread::sleep(boost::posix_time::microseconds(delay));
          }
          tb.updateTBState();
       }

       /* execute io -- nothing to do, store is infinite capacity */

       /* record io completion */
       now = boost::posix_time::microsec_clock::universal_time();
       iops_stats->handleIOCompletion(volume_id, now);

       boost::posix_time::time_duration elapsed = now - start_ts;
       cur_duration = elapsed.seconds();

       io_arrival(now);
    }

    if (result == 0) {
      std::cout << "Unit test PASSED, but see perf stats" << std::endl;
    }
    else {
      std::cout << "Unit test FAILED " << std::endl;
    }

    return result;
  }

private: /* methods */

};

} //namespace fds



int main(int argc, char *argv[]) 
{
  int ret = 0;
  std::string testname("tb_test");
  fds_uint64_t tb_rate = 100;
  fds_uint64_t tb_burst = 10;
  fds_uint64_t interarrival_usec = 0;
  int run_seconds = 50;
  int on_seconds = 10; //run_seconds + 10; /* workload always active during the test run */
  int off_seconds = 5;

  for (int i = 1; i < argc; ++i)
    {
      if (strncmp(argv[i], "--rate=", 7) == 0) {
	tb_rate = strtoull(argv[i]+7, NULL, 0);
      }
      else if (strncmp(argv[i], "--run=", 6) == 0) {
	run_seconds = strtoul(argv[i]+6, NULL, 0);
      }
      else if (strncmp(argv[i], "--burst=", 8) == 0) {
	tb_burst = strtoull(argv[i]+8, NULL, 0);
      }
      else if (strncmp(argv[i], "--arriv=", 8) == 0) {
	interarrival_usec = strtoull(argv[i]+8, NULL, 0);
      }
      else if (strncmp(argv[i], "--on=", 5) == 0) {
	on_seconds = strtoull(argv[i]+5, NULL, 0);
      }
      else if (strncmp(argv[i], "--off=", 6) == 0) {
	off_seconds = strtoull(argv[i]+6, NULL, 0);
      }
      else if (strncmp(argv[i], "--testname=", 11) == 0) {
	testname = argv[i]+11;
      }
      else {
	std::cout << "Invalid argument " << argv[i] << std::endl;
	return -1;
      }
    }

  std::cout << "Will test token bucket with rate " << tb_rate 
	    << ", burst " << tb_burst << std::endl;
  std::cout << "Interarrival time between IOs is " << interarrival_usec << " microsec." << std::endl;
  if (off_seconds > 0) {
    std::cout << "Simulating workload with On/Off behavior: on for " << on_seconds 
	      << "seconds and off for " << off_seconds << " seconds." << std::endl;
  }
  std::cout << "Will simulate store with infinite capacity." << std::endl;
  std::cout << "Will run test for " << run_seconds << " seconds" << std::endl; 

  fds::TBUnitTest unit_test(testname, interarrival_usec, on_seconds, off_seconds);

  ret = unit_test.run(run_seconds, tb_rate, tb_burst);
  

  return 0;
}
