// A stream that runs one IO at a time.

#ifndef _PHAROS_StreamSingleSync_H_INCLUDED_
#define _PHAROS_StreamSingleSync_H_INCLUDED_ 1

#include "Stream.H"

class StreamSingleSync: public Stream
{
public:
  // This constructor creates a stream that will run for a fixed number of
  // seconds.
  StreamSingleSync(Actuator& a_in,
	 Generator& g_in,
	 double how_long_in):
    Stream(a_in, g_in, how_long_in)
  {}

  // This constructor creates a stream stream that will run for a fixed number
  // of IOs.  The extra boolean argument is to prevent overloading between
  // integral types causing auto-conversion.
  StreamSingleSync(Actuator& a_in,
	 Generator& g_in,
	 unsigned how_long_in,
	 bool):
    Stream(a_in, g_in, how_long_in, true)
  {}

  virtual ~StreamSingleSync()
  {}

  // Copy constructor and assignment operator are forbidden:
private:
  // StreamSingleSync(StreamSingleSync const& old);
  // StreamSingleSync const& operator=(StreamSingleSync const& rhs);

public:
  virtual void run();

};

#endif // _PHAROS_StreamSingleSync_H_INCLUDED_
