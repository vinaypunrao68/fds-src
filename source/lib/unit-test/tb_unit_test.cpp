/*
 * Copyright 2013 Formation Data Systems, Inc.
 * 
 * Tests a single token bucket object 
 */

#include <iostream> // NOLINT(*)
#include <string>
#include <boost/thread/thread.hpp>

#include "qos_htb.h"
#include "test_stat.h"

namespace fds {

class TBUnitTest {
public:
  
  StatIOPS* iops_stats;
  StatIOPS* giops_stats;
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
     std::string gtestname = testname + std::string("_guarnt");

     iops_stats = new StatIOPS(testname, volids);
     giops_stats = new StatIOPS(gtestname, volids);
     next_idle = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(on_seconds);
     next_io = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::microseconds(interarrival_usec);
  }
  ~TBUnitTest() 
  {
     delete iops_stats;
     delete giops_stats;
  }

  /* returns duration of the experiment */
  inline int handle_io_completion_and_wait_io_arrival(boost::posix_time::ptime& start_ts)
  {
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();

    /* record io completion */
    iops_stats->handleIOCompletion(volume_id, now);
    boost::posix_time::time_duration elapsed = now - start_ts;
    int cur_duration = elapsed.seconds();
  
    /* one io finished, so calculate next time */
    if (interarrival_usec > 0)
      next_io += boost::posix_time::microseconds(interarrival_usec);

    /* deal with idle intervals */
    if ( (off_seconds > 0) && (now > next_idle)) {
        boost:this_thread::sleep(boost::posix_time::seconds(off_seconds));
        now = boost::posix_time::microsec_clock::universal_time();
        next_idle = now + boost::posix_time::seconds(on_seconds);
        
        next_io = now + boost::posix_time::microseconds(interarrival_usec);
    }

    /* sleep if io has not arrived yet */
    if ((interarrival_usec > 0) && (now < next_io)) {
      boost::posix_time::time_duration delay = next_io - now;
      long long delay_microsec = delay.total_microseconds();
      if (delay_microsec > 0)
	boost::this_thread::sleep(boost::posix_time::microseconds(delay_microsec));      
    }

    return cur_duration;
  }

  int run_min_max_bucket_test(int run_seconds, fds_uint64_t min_rate, fds_uint64_t max_rate, fds_uint64_t burst)
  {
    int result = 0;
    boost::posix_time::ptime start_ts = boost::posix_time::microsec_clock::universal_time();
    int cur_duration = 0;
    TBQueueState* qstate = new TBQueueState(volume_id, min_rate, max_rate, 1, 100000, burst);
    if (!qstate) {
      std::cout << "Failed to allocate queue state " << std::endl;
      return -1;
    }

    qstate->handleIoEnqueue(NULL); /* parameter is ignored for now */
    while (cur_duration < run_seconds)
      {
	fds_bool_t b_go = false;
	qstate->updateTokens();
	while ( !b_go ) {
	  TBQueueState::tbStateType state = qstate->tryToConsumeAssuredTokens(1);
	  if (state == TBQueueState::TBQUEUE_STATE_OK) {
	    /* we just consumed guaranteed token */
	    giops_stats->handleIOCompletion(volume_id, boost::posix_time::microsec_clock::universal_time());
	    b_go = true;
	  }
	  /* else try to consume up to max */
	  else if (state == TBQueueState::TBQUEUE_STATE_QUEUE_EMPTY) {
	    std::cout << "Unexpected queue state: queue must have at least one IO" << std::endl;
	    result = -1;
	    break;
	  }
	  else if (state != TBQueueState::TBQUEUE_STATE_NO_TOKENS) {
	    /* we have non-guaranteed tokens! */
	    qstate->consumeTokens(1);
	    b_go = true;
	  }
	  else {
	    qstate->updateTokens();
	  }
	}
	if (result != 0) break;
	
	/* execute io -- nothing to do, store is infinite capacity */
	qstate->handleIoDispatch(NULL);

	cur_duration = handle_io_completion_and_wait_io_arrival(start_ts);
	qstate->handleIoEnqueue(NULL);
      }

    if (result == 0) {
      std::cout << "Unit test PASSED, but see perf stats" << std::endl;
    }
    else {
      std::cout << "Unit test Failed " << std::endl;
    }

    delete qstate;
    return result;
  }

  int run(int run_seconds, fds_uint64_t tb_rate, fds_uint64_t tb_burst) 
  {
    int result = 0;
    boost::posix_time::ptime start_ts = boost::posix_time::microsec_clock::universal_time();
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

       cur_duration = handle_io_completion_and_wait_io_arrival(start_ts);
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
  std::string prefix("test");
  std::string testtype("tb");
  fds_uint64_t tb_rate = 100;
  fds_uint64_t tb_burst = 10;
  fds_uint64_t tb_max_rate = 1000;
  fds_uint64_t interarrival_usec = 0;
  int run_seconds = 50;
  int on_seconds = 10; //run_seconds + 10; /* workload always active during the test run */
  int off_seconds = 5;

  for (int i = 1; i < argc; ++i)
    {
      if (strncmp(argv[i], "--rate=", 7) == 0) {
	tb_rate = strtoull(argv[i]+7, NULL, 0);
      }
      else if (strncmp(argv[i], "--max=", 6) == 0) {
	tb_max_rate = strtoull(argv[i]+6, NULL, 0);
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
      else if (strncmp(argv[i], "--prefix=", 9) == 0) {
	prefix = argv[i]+9;
      }
      else if (strncmp(argv[i], "--test=", 7) == 0) {
	testtype = argv[i]+7;
      }
      else {
	std::cout << "Invalid argument " << argv[i] << std::endl;
	return -1;
      }
    }
  if ((testtype != std::string("tb")) && (testtype != std::string("minmax"))) {
    std::cout << "Test type should be either 'tb' or 'minmax'" << std::endl;
    return 0;
  }

  std::string testname = prefix + std::string("_") + testtype;

  std::cout << "Interarrival time between IOs is " << interarrival_usec << " microsec." << std::endl;
  if (off_seconds > 0) {
    std::cout << "Simulating workload with On/Off behavior: on for " << on_seconds 
	      << "seconds and off for " << off_seconds << " seconds." << std::endl;
  }
  std::cout << "Will simulate store with infinite capacity." << std::endl;
  std::cout << "Will run test for " << run_seconds << " seconds" << std::endl; 

  fds::TBUnitTest unit_test(testname, interarrival_usec, on_seconds, off_seconds);

  if (testtype == std::string("tb")) {
    std::cout << "Will test token bucket with rate " << tb_rate 
	      << ", burst " << tb_burst << std::endl;
    ret = unit_test.run(run_seconds, tb_rate, tb_burst);
  }
  else {
    std::cout << "Will test min/max token bucket with min rate " << tb_rate 
	      << ", max_rate " << tb_max_rate 
	      << ", burst " << tb_burst << std::endl;
    ret = unit_test.run_min_max_bucket_test(run_seconds, tb_rate, tb_max_rate, tb_burst);
  }
  

  return 0;
}
