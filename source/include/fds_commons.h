#ifndef __FDS_COMMONS

#define __FDS_COMMONS

//#include <linux/types.h>
#include <sys/types.h>
#include <stdbool.h>

typedef unsigned char doid_t[20];
#define SET_DOID_INV(doid) memset(doid, 0xff, 20)

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
#if 0
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif

// FDS definitions of variable types
typedef unsigned int  fds_uint32_t;
typedef int           fds_int32_t; 
typedef unsigned short fds_uint16_t;  
typedef short          fds_int16_t;
typedef char           fds_char_t;
typedef bool           fds_bool_t;
typedef unsigned long long fds_uint64_t;

#define FDS_UINT16_MAX (unsigned short) 0xffff
#define FDS_UINT32_MAX (unsigned int) 0xffffffff


typedef struct {
  fds_uint32_t  hash_low;
  fds_uint32_t  hash_high;
  fds_char_t    last[16];
} fds_object_id_t;

enum {

  FDS_DMGR_TXN_STATUS_INVALID,
  FDS_DMGR_TXN_STATUS_OPEN,
  FDS_DMGR_TXN_STATUS_COMMITED,
  FDS_DMGR_TXN_STATUS_CANCELED

};

#endif
