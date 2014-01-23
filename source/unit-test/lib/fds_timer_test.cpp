#include <iostream>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>
#include <thread>
#include <fds_globals.h>
#include <fds_timer.h>
#include <fds_process.h>
//#include <boost/date_time/posix_time/posix_time.hpp>

using namespace fds;  // NOLINT

/* Below code is written to understand how boost asio
 * works with timers.
 */
void print(const boost::system::error_code& e)
{
    std::cout << "Handler invoked " << e << std::endl;
}

class TimerEntry {
 public:
  TimerEntry(boost::asio::io_service &io) 
      : t(io)
  {
  }  
  void handler() {
      func();
      delete this;
  }
  boost::asio::steady_timer t;
  std::function<void(void)> func;

};

class Timer {
 public:
  Timer(): work_(io_) {
  }
  void schedule(int sec, std::function<void(void)> f) {
      TimerEntry *t = new TimerEntry(io_);
      t->t.expires_from_now(std::chrono::seconds(sec));
      t->func = f;
      t->t.async_wait(boost::bind(&TimerEntry::handler, t));
  }

  void run() {
      io_.run();
  }
 private:
  boost::asio::io_service io_;
  boost::asio::io_service::work work_;
};

void h1() {
    std::cout << "after one sec " << std::endl;
}

void h2() {
    std::cout << "after three secs " << std::endl;
}

void threesec() {
}

void Timer_test() {
    Timer t;

    std::thread iothread([&t]() {t.run();});

    t.schedule(1, h1);
    t.schedule(3, h2);

    iothread.join();
}

void cancel_test_h2(const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted) {
        std::cout << __FUNCTION__ << " cancelled " << std::endl;
    } else if (e) {
        std::cout << __FUNCTION__ << " error: " << e << std::endl;
    } else {
        std::cout << __FUNCTION__ << " completed" << std::endl;
    }
}
void cancel_test_h1(const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted) {
        std::cout << __FUNCTION__ << " cancelled " << std::endl;
    } else if (e) {
        std::cout << __FUNCTION__ << " error: " << e << std::endl;
    } else {
        std::cout << __FUNCTION__ << " completed" << std::endl;
    }
}

void async_wait_test1() {
    boost::asio::io_service io;
    boost::asio::io_service::work work(io);

    std::thread iothread([&io]() {io.run();});

    boost::asio::steady_timer t(io);
    std::cout << "Expired cnt: " << t.expires_from_now(std::chrono::milliseconds(2)) << std::endl;
    t.async_wait(&cancel_test_h1);
    t.async_wait(&cancel_test_h2);
    std::this_thread::sleep_for( std::chrono::milliseconds(1200));
    std::cout << "Expired cnt: " << t.expires_from_now(std::chrono::milliseconds(2)) << std::endl;
    t.async_wait(&cancel_test_h1);

    iothread.join();
    std::cout << "exiting" << std::endl;
}

void async_wait_test2() {
    boost::asio::io_service io;
    boost::asio::io_service::work work(io);

    std::thread iothread([&io]() {io.run();});

    boost::asio::steady_timer t(io);
    std::cout << "Expired cnt: " << t.expires_from_now(std::chrono::milliseconds(10)) << std::endl;
    t.async_wait(&cancel_test_h1);
    std::cout << "Expired cnt: " << t.expires_from_now(std::chrono::milliseconds(10)) << std::endl;

    iothread.join();
    std::cout << "exiting" << std::endl;
}


/* FdsTimer test code begins from this point */
class TimerTaskImpl : public FdsTimerTask {
 public:
  TimerTaskImpl(FdsTimer &timer)
      : FdsTimerTask(timer)
  {
      run_cnt = 0;
  }

  virtual void runTimerTask() {
      run_cnt++;
      total_run++;
  }

  int run_cnt;
  static int total_run;
};
int TimerTaskImpl::total_run = 0;

/* Test basic functionality of schedule() and cancel() methods */
void test_fds_timer1() {
    FdsTimer timer;
    boost::shared_ptr<FdsTimerTask> taskPtr(new TimerTaskImpl(timer));
    TimerTaskImpl *taskImplPtr = (TimerTaskImpl*) taskPtr.get();
    bool ret;

    ret = timer.schedule(taskPtr, std::chrono::milliseconds(2));
    fds_verify(ret == true);
    std::this_thread::sleep_for( std::chrono::milliseconds(20));
    fds_verify(taskImplPtr->run_cnt == 1);
    
    /* cancel prior to schduling should return fasle */
    ret = timer.cancel(taskPtr);
    fds_verify(ret == false);

    /* schedule and cancel immediately.  Task shouldn't be run */
    ret = timer.schedule(taskPtr, std::chrono::milliseconds(2));
    fds_verify(ret == true);
    ret = timer.cancel(taskPtr);
    fds_verify(ret == true);
    std::this_thread::sleep_for( std::chrono::milliseconds(20));
    fds_verify(taskImplPtr->run_cnt == 1);

    /* schedule same task twice.  Task should be run only once */
    ret = timer.schedule(taskPtr, std::chrono::milliseconds(2));
    fds_verify(ret == true);
    ret = timer.schedule(taskPtr, std::chrono::milliseconds(2));
    fds_verify(ret == false);
    std::this_thread::sleep_for( std::chrono::milliseconds(20));
    fds_verify(taskImplPtr->run_cnt == 2);

    /* cancel prior to schduling should return fasle */
    ret = timer.cancel(taskPtr);
    fds_verify(ret == false);

    timer.destroy();
}

/* Test basic functionality of schedule() and cancel() methods */
void test_fds_timer2() {
    int task_cnt = 100;
    std::vector<FdsTimerTaskPtr> tasks;
    FdsTimer timer;
    bool ret;

    TimerTaskImpl::total_run = 0;

    /* create a bunch of tasks and schedule them */
    for (int i = 0; i < task_cnt; i++) {
        boost::shared_ptr<FdsTimerTask> taskPtr(new TimerTaskImpl(timer));
        tasks.push_back(taskPtr);
        ret = timer.schedule(taskPtr, std::chrono::milliseconds(50));
        fds_verify(ret == true);
    }

    /* cancel few tasks */
    int cancel_cnt = task_cnt / 10;
    for (int i = 0; i < cancel_cnt; i++) {
        ret = timer.cancel(tasks[i]);
        fds_verify(ret == true);
    }

    /* Ensure all tasks except the cancelled ones are run */
    std::this_thread::sleep_for( std::chrono::seconds(1));
    fds_verify(TimerTaskImpl::total_run == (task_cnt-cancel_cnt));

    timer.destroy();
}

/* Test basic functionality of schedule() and cancel() methods */
void test_fds_repeated_timer1() {
    FdsTimer timer;
    boost::shared_ptr<FdsTimerTask> taskPtr(new TimerTaskImpl(timer));
    TimerTaskImpl *taskImplPtr = (TimerTaskImpl*) taskPtr.get();
    bool ret;
    
    /*Schedule repeated timer and make sure it's invoked multiple times*/
    ret = timer.scheduleRepeated(taskPtr, std::chrono::milliseconds(1));
    fds_verify(ret == true);
    std::this_thread::sleep_for( std::chrono::milliseconds(20));
    ret = timer.cancel(taskPtr);
    fds_verify(ret == true);
    fds_verify(taskImplPtr->run_cnt >= 10);

    /* cancel prior to schduling should return fasle */
    ret = timer.cancel(taskPtr);
    fds_verify(ret == false);

    /* schedule and cancel immediately.  Task shouldn't be run */
    int prev_run_cnt = taskImplPtr->run_cnt;
    ret = timer.schedule(taskPtr, std::chrono::milliseconds(2));
    fds_verify(ret == true);
    ret = timer.cancel(taskPtr);
    fds_verify(ret == true);
    std::this_thread::sleep_for( std::chrono::milliseconds(20));
    fds_verify(taskImplPtr->run_cnt == prev_run_cnt);

    timer.destroy();
}
int main()
{
    init_process_globals("fds-timer-test.log");

    test_fds_timer1();
    test_fds_timer2();
    test_fds_repeated_timer1();
    return 0;
}
