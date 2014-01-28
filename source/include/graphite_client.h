/* Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef _GRAPHITE_CLIENT_H
#define _GRAPHITE_CLIENT_H

#include <string>
#include <chrono>
#include <boost/asio.hpp>

#include <fds_timer.h>
#include <fds_counters.h>
#include <util/Log.h>

using boost::asio::ip::udp;

namespace fds {

/**
 * @brief GraphiteClient will open up a udp socket_ to
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
          try
          {
              parentP_->socket_.send_to(boost::asio::buffer(str), parentP_->receiver_endpoint_);
          }
          catch (std::exception& e)
          {
              FDS_PLOG_ERR(g_fdslog) << "GraphiteClient error: " <<  e.what();
              parentP_->timerPtr_->cancel(parentP_->taskPtr_);
          }
      }

      virtual std::string log_string()
      {
          return "Graphite Client timer task";
      }

     private:
      GraphiteClient *parentP_;
    };

 public:
  GraphiteClient(const std::string &ip, int port,
                 boost::shared_ptr<FdsTimer> timerPtr,
                 boost::shared_ptr<FdsCountersMgr> cntrs_mgrPtr)
      : socket_(io_service_),
      timerPtr_(timerPtr),
      cntrs_mgrPtr_(cntrs_mgrPtr)
  {
      try
      {
          udp::resolver resolver(io_service_);
          udp::resolver::query query(udp::v4(), ip, std::to_string(port));
          receiver_endpoint_ = *resolver.resolve(query);

          socket_.open(udp::v4());
      }
      catch (std::exception& e)
      {
          FDS_PLOG_ERR(g_fdslog) << "GraphiteClient error: " <<  e.what();
      }

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

  virtual std::string log_string()
  {
      return "GraphiteClient";
  }

 private:
  boost::asio::io_service io_service_;
  udp::socket socket_;
  udp::endpoint receiver_endpoint_;

  boost::shared_ptr<FdsTimer> timerPtr_;
  boost::shared_ptr<FdsCountersMgr> cntrs_mgrPtr_;
  boost::shared_ptr<FdsTimerTask> taskPtr_;
};
}  // namespace fds

#endif
