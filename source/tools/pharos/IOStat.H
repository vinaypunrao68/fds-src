// An IO with statistics gathering.  TODO: Too zygoria-specific.
// Subclass from an abstract generic one.  Also add all the functions
// for handling statistics (like write to a trace or log file) to this
// class.

#ifndef _PHAROS_IOStat_H_INCLUDED_
#define _PHAROS_IOStat_H_INCLUDED_ 1

#include "IO.H"
#include "Timer.H"

class IOStat: public IO
{
// On all times, <0 means not set yet

public:
  // No default constructor

  IOStat(IO const& io_in);

  virtual ~IOStat()
  {}

  // Copy constructor and assignment operater: The compiler-generated
  // ones are good enough.
  // IOStat(IOStat const& old);
  // IOStat const& operator=(IOStat const& rhs);

  // Accessors for the times:
  double get_U_Start() const
  { return t_U_Start; }
  
  double get_U_Stop() const
  { return t_U_Stop; }
  
  // TODO: Maybe define accessors for the commonly used combinations,
  // like differences.

  // Setters:  Not really setters; rather, they record the time.

  void setStarted(); // When starting the IO
  void setStopped(); // Call this when you get the IO back in user mode

private:
  double t_U_Start;
  double t_U_Stop;
    unsigned int reason;
  int diskqueue;
  unsigned int sessionqueue;

  char reasonstring[6];
  
  static bool zygaria_is_broken;
};

#endif // _PHAROS_IOStat_H_INCLUDED_
