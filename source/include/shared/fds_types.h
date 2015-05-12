/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SHARED_FDS_TYPES_H_
#define SOURCE_INCLUDE_SHARED_FDS_TYPES_H_

#include <stdint.h>

/*
 * Define all basic fds types portable in all platforms and run time
 * environments.  Don't include any header files not under this directory.
 *
 * This directory is shared between C/C++, so all header files here must be C only.
 */
#ifdef __cplusplus
#define c_decls_begin        extern "C" {
#define c_decls_end          }
typedef bool                 fds_bool_t;
#else
#define c_decls_begin
#define c_decls_end
typedef int                  fds_bool_t;
#endif

c_decls_begin

typedef unsigned long long   fds_blk_t;                     /* NOLINT */
typedef unsigned long long   fds_uint64_t;                  /* NOLINT */
typedef int64_t              fds_int64_t;                   /* NOLINT */
typedef long                 fds_long_t;                    /* NOLINT */
typedef unsigned int         fds_uint32_t;                  /* NOLINT */
typedef int                  fds_int32_t;                   /* NOLINT */
typedef unsigned short       fds_uint16_t;                  /* NOLINT */
typedef short                fds_int16_t;                   /* NOLINT */
typedef unsigned char        fds_uint8_t;                   /* NOLINT */
typedef char                 fds_int8_t;                    /* NOLINT */
typedef char                 fds_char_t;                    /* NOLINT */
typedef fds_uint8_t          fds_byte_t;                    /* NOLINT */

/* Get the offset of a field y inside struct X. */
#define fds_offset_of(X, y)   ((unsigned long)((void *)&(((X *)0)->y)))  /* NOLINT */

/* Get the address of the obj from addr of a field in it. */
#define fds_object_of(obj, field, ptr)                                        \
    ((obj *)(((char *)(ptr)) - (char *)fds_offset_of(obj, field))) /* NOLINT */

#define FDS_ARRAY_ELEM(array)         (sizeof(array)/sizeof(array[0]))

/* Alignment macros. */
#define fds_align(n, a)               (((n) + ((a) - 1)) & ~((a) - 1))
#define fds_align_ptr(p, a)                                                   \
    (void *)(((fds_uint64_t)(p) + /* NOLINT */                                \
              ((fds_uint64_t)(a) - 1)) & ~((fds_uint64_t)(a) - 1))

/*
 * Do not change enum assignment in these types because they may be persistant
 * accross software releases.
 */
typedef enum {
    FDS_DISK_DEFAULT         = 0,
    FDS_DISK_SSD             = 1,
    FDS_DISK_SAS             = 2,
    FDS_DISK_SATA            = 3
} fds_disk_type_t;

typedef enum {
    FDS_TIER_DRAM            = 0,
    FDS_TIER_NVRAM           = 1,
    FDS_TIER_SSD             = 2,
    FDS_TIER_SAS             = 3,
    FDS_TIER_SATA            = 4,
    FDS_TIER_MAX
} fds_tier_type_e;

#define FDS_ROUND_DOWN(x, y)    ((x) / (y))
#define FDS_ROUND_UP(x, y)      (((x) / (y)) + ((x) % (y) ? 1 : 0))

c_decls_end

#endif /* SOURCE_INCLUDE_SHARED_FDS_TYPES_H_  // NOLINT */
