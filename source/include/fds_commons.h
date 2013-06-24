#ifndef __FDS_COMMONS

#define __FDS_COMMONS
#include <sys/types.h>

typedef unsigned char doid_t[20];
#define SET_DOID_INV(doid) memet(doid, 0xff, 20)

#define FALSE 0
#define TRUE 1


static __inline__  is_doid_inv(doid_t doid) {
  int i;
  for (i = 0; i < sizeof(doid_t); i++) {
    if (doid[i] != 0xff) {
      return (FALSE);
    }
  }
  return (TRUE);
}


typedef unsigned int volid_t;
#define INV_VOL_ID (unsigned int)(-1)

typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
// typedef long long int64_t;
typedef unsigned long long uint64_t;

#define UINT16_MAX (unsigned short) 0xffff
#define UINT32_MAX (unsigned int) 0xffffffff

#endif
