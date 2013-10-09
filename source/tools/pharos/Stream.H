// Stream: Something that runs many IOs, and then stops.

// The only invariants all streams have in common:
// - They need a generator stack to generate the IOs
// - They need an actuator to execute the IOs
// - They need a termination criterion.  For now, the only one is a fixed time.
// Once you have set them up with those three, you can run the stream.

#ifndef _PHAROS_Stream_H_INCLUDED_
#define _PHAROS_Stream_H_INCLUDED_ 1

#include "Generator.H"
#include "Actuator.H"

class Stream
{
public:
  // This constructor creates a stream that will run for a fixed number of
  // seconds.
  Stream(Actuator& a_in,
	 Generator& g_in,
	 double how_long_in):
    a(a_in),
    g(g_in),
    how_long(how_long_in),
    how_many(0)
  {}

  // This constructor creates a stream stream that will run for a fixed number
  // of IOs.  The extra boolean argument is to prevent overloading between
  // integral types causing auto-conversion.
  Stream(Actuator& a_in,
	 Generator& g_in,
	 unsigned how_many_in,
         bool):
    a(a_in),
    g(g_in),
    how_long(-1.0),
    how_many(how_many_in)
  {}

  virtual ~Stream()
  {}

  // Copy constructor and assignment operator are forbidden:
private:
  // Stream(Stream const& old);
  // Stream const& operator=(Stream const& rhs);

public:
  virtual void run() = 0;

protected:
  Actuator& a;
  Generator& g;
  double how_long;
  unsigned how_many;
};

#endif // _PHAROS_Stream_H_INCLUDED_
