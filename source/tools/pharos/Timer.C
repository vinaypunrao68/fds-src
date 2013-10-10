#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

#include "Timer.H"

#define MICRO 1000000.0

/* static */ Timer* Timer::it = 0;

Timer::Timer():
  offset(-1.0)
{}

void Timer::initialize()
{
  assert(offset < 0.0);

  // Easiest way to measure the offset: Set it to zero, measure the
  // time, and set the offset to it.
  offset = 0.0;
 // offset = t();
}

double Timer::t() const
{
  assert(offset >= 0.0);

  struct timeval tv;
  (void) gettimeofday(&tv, 0);
  return ( (double) tv.tv_sec + tv.tv_usec/MICRO ) - offset;
}

double Timer::waitUntil(double when)
{
  assert(offset >= 0.0);
  double now = t();
  double howlong = when - now;
  if (howlong > 0.0) {
    // TODO: This is broken on OSes where usleep can only sleep <1s.
    // Fortunately, Linux violates Posix, so this happens to work.
    // But it means the longest sleep is about 12 days.
    usleep( (unsigned long) (MICRO * howlong) );
  }
  return -howlong;
}

void Timer::sleep(double howlong)
{
  assert(offset >= 0.0);
  usleep( (unsigned long) (MICRO * howlong) );
}

Timer* clk()
{
  if (Timer::it == 0)
    Timer::it = new Timer();

  return Timer::it;
}

double now()
{
  return clk()->t();
}

double Timer::convertFromTimeval(struct timeval& tv) const
{
  return ( (double) tv.tv_sec + tv.tv_usec/MICRO ) - offset;
}

double Timer::originOffset() const
{
  return offset;
}
