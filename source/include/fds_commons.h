#ifndef __FDS_COMMONS

#define __FDS_COMMONS

//#ifndef __TYPES
//#include <sys/types.h>
//#define __TYPES
#include <linux/types.h>
//#endif

typedef unsigned char doid_t[20];
#define SET_DOID_INV(doid) memset(doid, 0xff, 20)

//#define FALSE 0
//#define TRUE 1


static __inline__  is_doid_inv(doid_t doid) {
  int i;
  for (i = 0; i < sizeof(doid_t); i++) {
    if (doid[i] != 0xff) {
      return (0);
    }
  }
  return (1);
}


typedef unsigned int volid_t;
#define INV_VOL_ID (unsigned int)(-1)

typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#define FDS_UINT16_MAX (unsigned short) 0xffff
#define FDS_UINT32_MAX (unsigned int) 0xffffffff

enum {

  FDS_DMGR_TXN_STATUS_INVALID,
  FDS_DMGR_TXN_STATUS_OPEN,
  FDS_DMGR_TXN_STATUS_COMMITED,
  FDS_DMGR_TXN_STATUS_CANCELED

};

#endif
