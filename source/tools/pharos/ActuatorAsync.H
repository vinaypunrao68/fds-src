#ifndef _PHAROS_ActuatorAsync_H_INCLUDED_
#define _PHAROS_ActuatorAsync_H_INCLUDED_ 1

#include <aio.h>

#include "Actuator.H"

class ActuatorAsync: public Actuator
{
public:
  // No default constructor.

  // Use this constructor if you don't want an IO trace generated:
  ActuatorAsync(unsigned q_in, Store& store_in, int sid_in);

  // Use this constructor if you want to generate an IO trace:
  ActuatorAsync(unsigned q_in, Store& store_in, char const* tracefilename_in, int sid_in);

  virtual ~ActuatorAsync();

  // Copy constructor and assignment operator are forbidden:
private:
  // ActuatorAsync(ActuatorAsync const& old);
  // ActuatorAsync const& operator=(ActuatorAsync const& rhs);

public:
  virtual void initialize(Generator& g);
  virtual void initialize(size_t largest_io_in);
  virtual void close();

  // Do one IO.  If it can't be run right now (because too many IOs are
  // queued), then wait until it can be run.
  virtual void doIO(IOStat& ios);

protected:
  void initializeAsyncInternal();
  void checkTermination(unsigned i_cb);
  unsigned findCb(bool wait_if_all_busy);

  void pollAllIOs();
  void suspendUntilOneIOFinishes();
  
private:
  unsigned q; // Maximum # of IOs that can be running

  // The AIO control blocks, and a flag to indicate whether the IO can be
  // running
  aiocb* cbs;
  bool* running;
  unsigned n_running; // How many are outstanding (upper limit)
  IOStat** ioss;      // Copies of the IOStat used

  // If this is set by PHAROS_ASYNC_IMMEDIATE, we poll all IOs all the time, to
  // get the user-land finish time of IOs most accurately.
  bool immediate_poll;

  // The most IOs we ever had running.  Only makes sense if immediate_poll is set.
  unsigned max_running;
 
 
};

#endif // _PHAROS_ActuatorAsync_H_INCLUDED_
