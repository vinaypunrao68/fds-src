#ifndef __FDS_COMMONS

#define __FDS_COMMONS


#ifndef LIB_KERNEL
#include <sys/types.h>
#include "string.h"
#include <stdio.h>
#else
#include <linux/types.h>
#endif
#include <stdbool.h>

typedef unsigned int volid_t;
#define INV_VOL_ID (unsigned int)(-1)

/*
  TODO: Move this definition into fds_types.h
*/
typedef long long fds_int64_t;

#define FDS_UINT16_MAX (unsigned short) 0xffff
#define FDS_UINT32_MAX (unsigned int) 0xffffffff

typedef struct {
  fds_int64_t   hash_high;
  fds_int64_t   hash_low;
  char    conflict_id;
} fds_object_id_t;

typedef unsigned char doid_t[20];

typedef union {

  fds_object_id_t obj_id; // object id fields
  unsigned char bytes[20]; // byte array

} fds_doid_t;

#define SET_DOID_INV(p_doid) memset(p_doid, 0xff, sizeof(fds_doid_t))

static __inline__  int is_doid_inv(fds_doid_t *p_doid) {
  unsigned int i;
  for (i = 0; i < sizeof(fds_doid_t); i++) {
    if (p_doid->bytes[i] != 0xff) {
      return (0);
    }
  }
  return (1);
}

typedef unsigned char doid_hex_string[41];

static __inline__ unsigned char *doid_to_hex(doid_t obj_id,
					     unsigned char *buf) {
  unsigned int i;
  
  for (i = 0; i < sizeof(doid_t); i++) {
    sprintf((char*)&(buf[2*i]), "%02x", obj_id[i]);
  } 
  return (buf);
  
}  

enum {

  FDS_DMGR_TXN_STATUS_INVALID,
  FDS_DMGR_TXN_STATUS_OPEN,
  FDS_DMGR_TXN_STATUS_COMMITED,
  FDS_DMGR_TXN_STATUS_CANCELED

};

#endif
