#ifndef _PHAROS_Store_H_INCLUDED_
#define _PHAROS_Store_H_INCLUDED_ 1

#include <string>
#include "IO.H" // Needed for location_t

class Store
{
public:
  // No default constructor.
  Store(std::string const name_in, bool readonly_in, int sid_in):
    name(name_in),
    readonly(readonly_in),
    fd(-1),
    capacity(0), // Will be set in initialize, =0 means unitialized.
    session_id(sid_in)
  {}

  ~Store()
  {}

  // Copy constructor and assignment operator are forbidden:
private:
  // Store(Store const& old);
  // Store const& operator=(Store const& rhs);

public:
  void initialize();
  void close();
  location_t getCapacity() const;
  int getFD(); // TODO: Encapsulation violation here.

protected:
  location_t obtainDeviceCapacity();
  
private:
  std::string name;
  bool readonly;
  int fd;
  location_t capacity;
  int session_id;
};

#endif // _PHAROS_Store_H_INCLUDED_
