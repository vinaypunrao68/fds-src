#include <iostream>
#include <ctime>
#include <fds_timer.h>

using namespace fds;  // NOLINT

/* FdsTimer test code begins from this point */
class TimerTaskImpl : public FdsTimerTask {
 public:
  TimerTaskImpl(FdsTimer &timer, int i )
      : FdsTimerTask(timer)
  {
      run_cnt = 0;
      id = i;
  }

  virtual void runTimerTask() {
      run_cnt++;
      total_run++;

      using namespace std::chrono;
      auto now = high_resolution_clock::now();
      auto nanos = duration_cast<milliseconds>(now.time_since_epoch()).count();
      std::cout << "id: " << id << "time: "
          << nanos;
  }
  int id;
  std::atomic<int> run_cnt;
  static std::atomic<int> total_run;
};
std::atomic<int> TimerTaskImpl::total_run;

#if 0
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
#endif

/* Test basic functionality of schedule() and cancel() methods */
void test_fds_timer2() {
    int task_cnt = 100;
    std::vector<FdsTimerTaskPtr> tasks;
    FdsTimer timer;
    bool ret;

    TimerTaskImpl::total_run = 0;

    /* create a bunch of tasks and schedule them */
    for (int i = 0; i < task_cnt; i++) {
        boost::shared_ptr<FdsTimerTask> taskPtr(new TimerTaskImpl(timer, i));
        tasks.push_back(taskPtr);
        ret = timer.schedule(taskPtr, std::chrono::milliseconds(10*i));
        fds_verify(ret == true);
    }

    /* cancel few tasks */
    int cancel_cnt = task_cnt / 10;
    for (int i = 0; i < cancel_cnt; i++) {
        ret = timer.cancel(tasks[tasks.size()-1-i]);
        fds_verify(ret == true);
    }

    /* Ensure all tasks except the cancelled ones are run */
    std::this_thread::sleep_for( std::chrono::seconds(3));
    fds_verify(TimerTaskImpl::total_run == (task_cnt-cancel_cnt));

    timer.destroy();
}

/* Test basic functionality of schedule() and cancel() methods */
void test_fds_repeated_timer1() {
    FdsTimer timer;
    boost::shared_ptr<FdsTimerTask> taskPtr(new TimerTaskImpl(timer, 1));
    TimerTaskImpl *taskImplPtr = (TimerTaskImpl*) taskPtr.get();
    bool ret;
    
    /*Schedule repeated timer and make sure it's invoked multiple times*/
    ret = timer.scheduleRepeated(taskPtr, std::chrono::milliseconds(20));
    fds_verify(ret == true);
    std::this_thread::sleep_for( std::chrono::milliseconds(200));
    ret = timer.cancel(taskPtr);
    fds_verify(ret == true);
    fds_verify(taskImplPtr->run_cnt >= 6);

    /* cancel prior to schduling should return fasle */
    ret = timer.cancel(taskPtr);
    fds_verify(ret == true);

    /* schedule and cancel immediately.  Task shouldn't be run */
    int prev_run_cnt = taskImplPtr->run_cnt;
    ret = timer.schedule(taskPtr, std::chrono::milliseconds(20));
    fds_verify(ret == true);
    ret = timer.cancel(taskPtr);
    fds_verify(ret == true);
    std::this_thread::sleep_for( std::chrono::milliseconds(20));
    fds_verify(taskImplPtr->run_cnt == prev_run_cnt);

    timer.destroy();
}

int main()
{
    TimerTaskImpl::total_run = 0;
    test_fds_timer2();
    test_fds_repeated_timer1();
    return 0;
}
