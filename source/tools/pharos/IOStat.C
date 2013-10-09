#include <sys/time.h>
#include <assert.h>

#include "IOStat.H"
#include "printe.H"

IOStat::IOStat(IO const& io_in):
  IO(io_in),
  t_U_Start(-1.0),
  t_U_Stop(-1.0)
{ 
  assert(io_in.areParamsSet());
}

void IOStat::setStarted()
{ t_U_Start = now(); }

void IOStat::setStopped()
{ t_U_Stop = now(); }
