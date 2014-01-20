/* Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef _GRAPHITE_CLIENT_H
#define _GRAPHITE_CLIENT_H

#include <string>
#include <chrono>

#include <fds_timer.h>
#include <fds_counters.h>

namespace fds {

/**
 * @brief GraphiteClient will open up a udp socket to
 * graphite server and dump counters at an interval
 */
class GraphiteClient : public boost::noncopyable
{
 private:
  class TimerTask : public FdsTimerTask
    {
     public:
      TimerTask(GraphiteClient *parentP)
          : FdsTimerTask(*parentP->timerPtr_)
      {
          parentP_ = parentP;
      }

      virtual ~TimerTask()
      {
      }

      void runTimerTask() override
      {
          std::string str = parentP_->cntrs_mgrPtr_->export_as_graphite();
          std::cout << str << std::endl;
      }

     private:
      GraphiteClient *parentP_;
    };

 public:
  GraphiteClient(const std::string &ip, int port,
                 boost::shared_ptr<FdsTimer> timerPtr,
                 boost::shared_ptr<FdsCountersMgr> cntrs_mgrPtr)
      : timerPtr_(timerPtr),
      cntrs_mgrPtr_(cntrs_mgrPtr)
  {
      // TODO: open up udp socket
      taskPtr_.reset(new GraphiteClient::TimerTask(this));
  }

  virtual ~GraphiteClient()
  {
  }

  void start(int seconds)
  {
      timerPtr_->scheduleRepeated(taskPtr_,
                                  std::chrono::seconds(seconds));
  }

 private:
  boost::shared_ptr<FdsTimer> timerPtr_;
  boost::shared_ptr<FdsCountersMgr> cntrs_mgrPtr_;
  boost::shared_ptr<FdsTimerTask> taskPtr_;
};
}  // namespace fds

#endif
