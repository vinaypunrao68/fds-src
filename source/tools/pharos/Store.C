#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h> // For BLKGETSIZE ioctl
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Store.H"
#include "printe.H"

#define F_SETSID 17

void Store::initialize()
{
  assert(fd<0);

  fd = readonly 
    ? ::open(name.c_str(), O_RDONLY /*| O_DIRECT*/)
       : ::open(name.c_str(), O_RDWR /*  | O_DIRECT*/);
  if (fd < 0)
    printx("Error opening %s: %s\n", name.c_str(), strerror(errno));
  
  capacity = obtainDeviceCapacity();
  assert(capacity>0);
}

void Store::close()
{
  assert(fd>0);
  ::close(fd);
  fd = -1;
  capacity = 0;
}

int Store::getFD()
{
  assert(fd>=0);
  assert(capacity>0);
  return fd;
}

location_t Store::getCapacity() const
{
  assert(fd>=0);
  assert(capacity>0);
  return capacity;
}

location_t Store::obtainDeviceCapacity() {
  assert(fd>=0);

  struct stat st;
  int status = fstat(fd, &st);
  // TODO: Better error message than just assert;
  assert(status==0);

  if (S_ISREG(st.st_mode))
    return st.st_size;

  if (S_ISBLK(st.st_mode)) {
    long n_blocks;
    int status = ioctl(fd, BLKGETSIZE, &n_blocks);
    assert(status==0);
    // Linux has a hard-coded 512 byte block size.
    return 512 * (location_t) n_blocks;
  }

  assert(0); // TODO: Better error message, was neither file nor device.
  // NOT REACHED
  return 0;
}
