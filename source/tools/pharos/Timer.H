#ifndef _PHAROS_Timer_H_INCLUDED_
#define _PHAROS_Timer_H_INCLUDED_ 1

// This is a singleton.
// TODO: Needs overhaul for inheritance, if there are multiple kinds
// of concrete timers.

class Timer
{
public:

  // If you want a Timer object, this function will give it to you.
  // But the resulting timer might not be initialized.
  friend Timer* clk();

  // Protected constructor, to make sure it stays a singleton.
protected:
  Timer();

  // No copy constructor or assignment operatore, to make sure it
  // stays a sigleton:
private:
  Timer(Timer const& old);
  Timer const& operator=(Timer const& rhs);

public:
  virtual ~Timer()
  {}

public:

  // Initializes this timer object.  Called by the main program
  // quickly during initialization.  Shall not be called twice.  Sets
  // the epoch to zero.
  void initialize();

  // How late is it?   Always returns positive value.
  double t() const;

  // Wait until a certain time (blocking the current process/thread).  If
  // argument is negative, or that time has already passed, nothing happens.
  // If the return value is >0, no wait was necessary, and it is already too
  // late by return value.  If the return value is <0, then |return value| is
  // the time actually waited.
  double waitUntil(double when);

  // Wait for a specified sleep time.  Argument must be positive.
  void sleep(double howlong);

  // Given a struct timeval, return the equivalent time in the metric
  // used by this time object:
  double convertFromTimeval(struct timeval& tv) const;

  // Return the offset from the Unix epoch to the value at which this timer
  // would return zero.  Useful if you are running multiple copies of Pharos,
  // and need to synchronize their time values after the fact.
  double originOffset() const;

private:
  static Timer* it;

  // When were we initialized.  If this is <0, we have not been
  // initialized yet.  Otherwise, it is the time in gettimeofday()
  // format when we were initialized.
  // TODO: For efficiency, convert this to a struct timeval.
  double offset;
};

// Singleton-finder:
Timer* clk();

// Convenience function:
double now();

#endif // _PHAROS_Timer_H_INCLUDED_
