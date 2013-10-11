// Base class: an IO

#ifndef _PHAROS_IO_H_INCLUDED_
#define _PHAROS_IO_H_INCLUDED_ 1

#include <stdio.h>  // For off64_t and/or off_t on RH7.3 systems
#include <unistd.h> // For off64_t and/or off_t

// An IO has 4 parameters: 
// - Time: Double.  All real times are positive.  Time=0 is defined in
//   some sensible way so all times are small numbers, for example the
//   beginning of the program run.
//   Time < 0 means run the IO immediately; there is an initializer 
//   called "Now" for that, and an accessor isNow().
// - Location: Offset (in bytes) within the device, file, or store.
// - Size in bytes.
// - Direction: Read or write.
// Each parameter can be set, or not.  You can only get it if it has
// been set.

typedef off64_t location_t;

class IO
{
public:
  static const double Now; // Means run the IO as soon as possible
  enum dir_t { Read, Write };

  // Default constructor makes an IO with nothing set:
  IO():
    time(Now),    time_set(false),
    location(0),  location_set(false),
    size(0),      size_set(false),
    dir(Read),    dir_set(false),
    deadline(0.0), deadline_set(false)
  {}

  IO(double time_in, location_t location_in, size_t size_in, dir_t dir_in, double deadline_in):
    time(time_in),          time_set(true),
    location(location_in),  location_set(true),
    size(size_in),          size_set(true),
    dir(dir_in),            dir_set(true),
    deadline(deadline_in),  deadline_set(true)
  {}

  virtual ~IO()
  {}

  // Copy constructor and assignment operater: The compiler-generated
  // ones are good enough.
  // IO(IO const& old);
  // IO const& operator=(IO const& rhs);

  // Read accessors for time, location, size and direction:
  double getTime() const;

  location_t getLocation() const;
  bool isLocationSet() const { return location_set; }

  size_t getSize() const;
  bool isSizeSet() const { return size_set; }

  dir_t getDir() const;
  bool isDirSet() const { return dir_set; }
  
  double getDeadline() const;
  bool isDeadlineSet() const { return deadline_set; }

  bool isNow() const;
  bool isTimeSet() const { return time_set; }

  bool areParamsSet() const
  { return time_set && location_set && size_set && dir_set; }

  void setTime(double time_in)
  { time = time_in;  time_set = true; }

  void setLocation(location_t location_in)
  { location = location_in; location_set = true;}

  void setSize(size_t size_in)
  { size = size_in; size_set = true; }

  void setDir(dir_t dir_in)
  { dir = dir_in; dir_set = true; }
  
  void setDeadline(double deadline_in)
  { deadline = deadline_in; deadline_set = true; }

  void print() const;

private:
  double time;
  bool time_set;
  location_t location;
  bool location_set;
  size_t size;
  bool size_set;
  dir_t dir;
  bool dir_set;    
  
  double deadline;
  bool deadline_set;
};

#endif // _PHAROS_IO_H_INCLUDED_
