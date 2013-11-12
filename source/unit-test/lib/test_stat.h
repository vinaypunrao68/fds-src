#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <IceUtil/IceUtil.h>

#include "fds_types.h"
#include <util/PerfStat.h>

namespace fds {

class StatIOPS;
using namespace IceUtil;
class TestStatTimerTask : public IceUtil::TimerTask {
public:
  StatIOPS *stat_iops;

  TestStatTimerTask(StatIOPS* _stat_iops);
  ~TestStatTimerTask() {}

  void runTimerTask();  
};


class StatIOPS
{
public:
  StatIOPS(const std::string& prefix, std::vector<fds_uint32_t>& volids)
   : statTimer(new IceUtil::Timer),
     statTimerTask(new TestStatTimerTask(this))
  {
    std::string dirname("test_stats//");
    if ( !boost::filesystem::exists(dirname.c_str()) ) {
      boost::filesystem::create_directory(dirname.c_str());
    }
    std::string fname = dirname + prefix  + std::string(".stat");
    statfile.open(fname.c_str(), std::ios::out);

    /* initialize histories */
    for (std::vector<fds_uint32_t>::iterator it = volids.begin();
         it != volids.end();
         ++it)
    {
        StatHistory* hist = new StatHistory(*it, 12, 1);
        vol_hist[*it] = hist;
    }

    start_time = boost::posix_time::second_clock::universal_time();
    last_print_time = boost::posix_time::microsec_clock::universal_time();    

    IceUtil::Time interval = IceUtil::Time::seconds(10);
    statTimer->scheduleRepeated(statTimerTask, interval);
  }

  ~StatIOPS() {
    statTimer->destroy();
    if (statfile.is_open()) {
      statfile.close();
    }

    for (std::unordered_map<fds_uint32_t, StatHistory*>::iterator it = vol_hist.begin();
         it != vol_hist.end();
         ++it) 
    {
        StatHistory* hist = it->second;
        delete hist;
    }
    vol_hist.clear();
  }

  inline void handleIOCompletion(fds_uint32_t volid, boost::posix_time::ptime ts) 
  {
    StatHistory* hist = vol_hist[volid];
    boost::posix_time::time_duration elapsed = ts - start_time;
    hist->addIo(elapsed.total_seconds(),0);
   }

  void printPerfStats()
  {
     boost::posix_time::ptime tnow  = boost::posix_time::microsec_clock::local_time();
     for (std::unordered_map<fds_uint32_t, StatHistory*>::iterator it = vol_hist.begin();
         it != vol_hist.end();
         ++it) 
     {
       it->second->print(statfile, tnow);
     }
  }

private:
  std::ofstream statfile;
  std::unordered_map<fds_uint32_t, StatHistory*> vol_hist;

  boost::posix_time::ptime start_time;

  boost::posix_time::ptime last_print_time;
  int print_interval_sec;

  IceUtil::TimerPtr statTimer;
  IceUtil::TimerTaskPtr statTimerTask;
};




/* timer methods */

TestStatTimerTask::TestStatTimerTask(StatIOPS* _stat_iops)
{
  stat_iops = _stat_iops;
}

void TestStatTimerTask::runTimerTask()  
{
   stat_iops->printPerfStats();
}






} // namespace fds
