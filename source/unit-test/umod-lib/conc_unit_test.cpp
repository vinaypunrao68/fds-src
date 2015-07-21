/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream>  // NOLINT(*)
#include <vector>

#include <concurrency/Mutex.h>
#include <concurrency/SpinLock.h>
#include <concurrency/RwLock.h>
#include <concurrency/Thread.h>
#include <concurrency/Synchronization.h>
#include <concurrency/ThreadPool.h>

// #include "util/Log.h"

// #define BOOST_LOG_DYN_LINK 1
// #define BOOST_ALL_DYN_LINK 1

namespace fds {

class ConcUnitTest {
 private:
  std::list<std::string>  unit_tests;
  fds_mutex        *global_mutex;
  fds_rwlock       *global_rwlock;
  fds_spinlock     *global_spinlock;
  fds_uint32_t      global_int;
  fds_notification *global_notify;
  fds_uint32_t      global_waiters;
  fds_uint32_t      global_done;

  typedef enum {
    SHARED = 0,
    COND   = 1,
    EXCL   = 2,
  } rw_modes;
  
  /*
   * Helper functions
   */
  static void mt_thread_loop(ConcUnitTest *ut, int seed, int its, bool use_mutex) {
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);

    if (use_mutex) {
      ut->global_mutex->lock();
    }
    for (int i = its; i >= 0; i--) {
      std::cout << "Thread " << seed << " has " << i
                << " iterations left" << std::endl;
      xt.sec += 1;
      boost::thread::sleep(xt);
    }
    std::cout << "Thread " << seed << " is exiting" << std::endl;

    if (use_mutex) {
      ut->global_mutex->unlock();
    }
  }

  static void spinlock_thread_loop(ConcUnitTest *ut, int seed, int its) {
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);

    ut->global_spinlock->lock();

    for (int i = its; i >= 0; i--) {
      std::cout << "Thread " << seed << " has " << i
                << " iterations left" << std::endl;
      xt.sec += 1;
      boost::thread::sleep(xt);
    }
    std::cout << "Thread " << seed << " is exiting" << std::endl;

    ut->global_spinlock->unlock();
  }

  static void rw_thread_loop(ConcUnitTest *ut, rw_modes mode, int seed, int its) {
    if (mode == SHARED) {
      ut->global_rwlock->read_lock();
      for (int i = 0; i < its; i++) {
        std::cout << "Read lock " << seed << " reading value "
                  << ut->global_int << " in iteration "
                  << its << std::endl;
      }
      ut->global_rwlock->read_unlock();
    } else if (mode == COND) {
      ut->global_rwlock->cond_write_lock();
      ut->global_rwlock->upgrade();
      for (int i = 0; i < its; i++) {
        std::cout << "Upgraded lock " << seed << " update value to "
                  << ut->global_int++ << " in iteration "
                  << its << std::endl;
      }
      ut->global_rwlock->write_unlock();
    } else {
      assert(mode == EXCL);
      ut->global_rwlock->write_lock();
      for (int i = 0; i < its; i++) {
        std::cout << "Write lock " << seed << " update value to "
                  << ut->global_int++ << " in iteration "
                  << its << std::endl;
      }
      ut->global_rwlock->write_unlock();
    }
  }

  static void wait_thread_loop(ConcUnitTest *ut, int seed) {
    std::cout << "Thread " << seed << " is going to wait" << std::endl;
    ut->global_notify->wait_for_notification();
    std::cout << "Thread " << seed << " is awake!" << std::endl;
  }

  boost::thread* start_mt_thread(int seed, int its, bool use_mutex) {
    boost::thread *_t;
    _t = new boost::thread(boost::bind(&mt_thread_loop, this, seed, its, use_mutex));
    
    return _t;
  }
  
  boost::thread* start_spinlock_thread(int seed, int its) {
    boost::thread *_t;
    _t = new boost::thread(boost::bind(&spinlock_thread_loop, this, seed, its));

    return _t;
  }

  boost::thread* start_rw_thread(rw_modes mode, int seed, int its) {
    boost::thread *_t;
    _t = new boost::thread(boost::bind(&rw_thread_loop, this, mode, seed, its));

    return _t;
  }

  boost::thread* start_wait_thread(int seed) {
    boost::thread *_t;
    _t = new boost::thread(boost::bind(&wait_thread_loop, this, seed));

    return _t;
  }

  static void do_tp_work(int seed) {
    std::cout << "Thread " << seed
              << " is starting some work!" << std::endl;

    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);
    xt.sec += 5;
    boost::thread::sleep(xt);

    std::cout << "Thread " << seed
              << " is done working!"<< std::endl;
  }
  
  /*
   * Unit test funtions
   */
  int basic_mutex() {
    fds_mutex *_m;
    _m = new fds_mutex("unit_test");
    
    _m->lock();
    if (_m->try_lock()) {
      std::cout << "Mutex was unexpectently unlocked!" << std::endl;
      _m->unlock();
      delete _m;
      return -1;
    } else {
      std::cout << "Mutex was already locked as expected!" << std::endl;
    }
    
    _m->unlock();
    if (_m->try_lock()) {
      std::cout << "Mutex was unlocked as expected!" << std::endl;
    } else {
      std::cout << "Mutex was unexpectently locked!" << std::endl;
      _m->unlock();
      delete _m;
      return -1;
    }

    _m->unlock();
    delete _m;
    return 0;
  }

  int multi_thread() {
    std::vector<boost::thread*> threads;
    int iterations = 5;
    
    /*
     * Start threads and save reference.
     */
    for (int i = 0; i < 10; i++) {
      threads.push_back(start_mt_thread(i, iterations, false));
    }
    
    /*
     * Wait for all of the threads.
     */
    for (int i = 0; i < 10; i++) {
      threads[i]->join();
    }
    
    return 0;
  }

  int mt_mutex() {
    std::vector<boost::thread*> threads;
    int iterations = 5;

    /*
     * Start threads and save reference.
     */
    for (int i = 0; i < 10; i++) {
      threads.push_back(start_mt_thread(i, iterations, true));
    }
    
    /*
     * Wait for all of the threads.
     */
    for (int i = 0; i < 10; i++) {
      threads[i]->join();
    }

    return 0;
  }

  int spinlock() {
    std::vector<boost::thread*> threads;
    int iterations = 5;

    /*
     * Start threads and save reference.
     */
    for (int i = 0; i < 10; i++) {
      threads.push_back(start_spinlock_thread(i, iterations));
    }

    /*
     * Wait for all of the threads.
     */
    for (int i = 0; i < 10; i++) {
      threads[i]->join();
    }

    return 0;
  }

  int rwlock() {
    std::vector<boost::thread*> threads;
    int iterations = 5;

    /*
     * Start reader threads.
     */
    for (int i = 0; i < 5; i++) {
      threads.push_back(start_rw_thread(SHARED, i, iterations));
    }
    
    /*
     * Start conditional writer threads.
     */
    for (int i = 0; i < 5; i++) {
      threads.push_back(start_rw_thread(COND, i, iterations));
    }

    /*
     * Start writer threads.
     */
    for (int i = 0; i < 5; i++) {
      threads.push_back(start_rw_thread(EXCL, i, iterations));
    }
    
    /*
     * Wait for all of the threads.
     */
    for (int i = 0; i < 15; i++) {
      threads[i]->join();
    }

    global_rwlock->read_lock();
    if (global_int != 50) {
      std::cout << "The shared value is " << global_int
                << " instead of 50 as expected" << std::endl;
      global_rwlock->read_unlock();
      return -1;
    }

    std::cout << "Shared value computed " << global_int
              << " as expected" << std::endl;
    global_rwlock->read_unlock();
    return 0;
  }

  int notify() {
    std::vector<boost::thread*> threads;

    /*
     * Start threads
     */
    for (int i = 0; i < 5; i++) {
      threads.push_back(start_wait_thread(i));
    }
    
    /*
     * Sleep for a bit then wake the threads
     */
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);
    xt.sec += 3;
    boost::thread::sleep(xt);
    std::cout << "Main thread is going to notify" << std::endl;
    for (int i = 0; i < 5; i++) {
      /*
       * We sleep here to give time for the
       * previously notified thread to actually
       * wake up. Fire hosing notify requests
       * can cause multiple outstanding notify
       * requests. The fds_notification aims
       * to have only a single outstanding
       * notification request.
       */
      xt.sec += 1;
      boost::thread::sleep(xt);
      global_notify->notify();
    }

    /*
     * Wait for all of the threads.
     */
    for (int i = 0; i < 5; i++) {
      threads[i]->join();
    }
    
    return 0;
  }

  int threadpool() {
    fds_uint32_t tp_size = 10;
    fds_threadpool _tp(tp_size);

    for (fds_uint32_t i = 0; i < (2 * tp_size); i++) {
      _tp.schedule(do_tp_work, i);
    }
    std::cout << "All jobs scheduled" << std::endl;

    return 0;
  }

 public:
  explicit ConcUnitTest() {
    global_mutex   = new fds_mutex("global mutex");
    global_rwlock  = new fds_rwlock();
    global_spinlock = new fds_spinlock;
    global_int     = 0;
    global_notify  = new fds_notification();
    global_waiters = 0;
    global_done    = 0;

    unit_tests.push_back("basic_mutex");
    unit_tests.push_back("multi-thread");
    unit_tests.push_back("mt-mutex");
    unit_tests.push_back("spinlock");
    unit_tests.push_back("rwlock");
    unit_tests.push_back("notify");
    unit_tests.push_back("threadpool");
  }

  ~ConcUnitTest() {
    delete global_mutex;
    delete global_spinlock;
    delete global_rwlock;
    delete global_notify;
  }
  
  void Run(const std::string& testname) {
    int result = -1;
    std::cout << "Running unit test \"" << testname << "\"" << std::endl;
    
    if (testname == "basic_mutex") {
      result = basic_mutex();
    } else if (testname == "multi-thread") {
      result = multi_thread();
    } else if (testname == "mt-mutex") {
      result = mt_mutex();
    } else if (testname == "spinlock") {
      result = spinlock();
    } else if (testname == "rwlock") {
      result = rwlock();
    } else if (testname == "notify") {
      result = notify();
    } else if (testname == "threadpool") {
      result = threadpool();
    } else {
      std::cout << "Unknown unit test " << testname << std::endl;
    }
    
    if (result == 0) {
      std::cout << "Unit test \"" << testname << "\" PASSED"  << std::endl;
    } else {
      std::cout << "Unit test \"" << testname << "\" FAILED" << std::endl;
    }
    std::cout << std::endl;
  }
  
  void Run() {
    for (std::list<std::string>::iterator it = unit_tests.begin();
         it != unit_tests.end();
         ++it) {
      Run(*it);
    }
  }
};

}  // namespace fds

int main(int argc, char* argv[]) {

  std::string testname;

    for (int i = 1; i < argc; i++) {
      if (strncmp(argv[i], "--testname=", 11) == 0) {
        testname = argv[i] + 11;
      } else {
        std::cout << "Invalid argument " << argv[i] << std::endl;
        return -1;
      }
    }

    fds::ConcUnitTest unittest;

    if (testname.empty()) {
      unittest.Run();
    } else {
      unittest.Run(testname);
    }

    // BOOST_LOG_TRIVIAL(trace) << "A trace severity message";

  return 0;
}
