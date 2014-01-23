/* Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_STOPWATCH_H_
#define SOURCE_INCLUDE_FDS_STOPWATCH_H_

#include <boost/timer/timer.hpp>

namespace fds
{
class FdsStopwatch
{
 public:
  void start()
  {
      timer_.start();
  }
  uint64_t elapsed()
  {
      timer_.stop();
      return timer_.elapsed().wall;
  }
  void reset()
  {
      timer_.stop();
  }
 private:
  boost::timer::cpu_timer timer_;
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_STOPWATCH_H_
