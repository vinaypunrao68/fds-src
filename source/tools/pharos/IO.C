#include <assert.h>

#include "IO.H"

/* static */ const double IO::Now = -1.0;

double IO::getTime() const
{
  assert(time_set);
  return time;
}

location_t IO::getLocation() const
{
  assert(location_set);
  return location;
}

size_t IO::getSize() const
{
  assert(size_set);
  return size;
}

IO::dir_t IO::getDir() const
{
  assert(dir_set);
  return dir;
}

double IO::getDeadline() const
{
	assert(deadline_set);
	return deadline;
}

bool IO::isNow() const
{
  assert(time_set);
  return time < 0.0;
}

void IO::print() const
{
  // TODO: Should verify that all the parameters are set.
  //       Or else print only the set ones.
  if (isNow())
    printf("%s %u bytes at %Lu=0x%Lx now",
	   getDir() == IO::Read ? "read" : "write",
	   getSize(),
	   getLocation(), getLocation());
  else
    printf("%s %u bytes at %Lu=0x%Lx at time %.6f",
	   getDir() == IO::Read ? "read" : "write",
	   getSize(),
	   getLocation(), getLocation(),
	   getTime());	

}
