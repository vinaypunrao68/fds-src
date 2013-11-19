#ifndef INCLUDE_SHARED_FDS_TYPES_H_
#define INCLUDE_SHARED_FDS_TYPES_H_

/*
 * Define all basic fds types portable in all platforms and run time
 * environments.  Don't include any header files not under this directory.
 */
#ifdef __cplusplus
#define c_decls_begin        extern "C" {
#define c_decls_end          }
typedef bool                  fds_bool_t;
#else
#define c_decls_begin
#define c_decls_end
typedef int                  fds_bool_t;
#endif

typedef unsigned long long   fds_blk_t;
typedef unsigned long long   fds_uint64_t;
typedef long long            fds_int64_t;
typedef long                 fds_long_t;
typedef unsigned int         fds_uint32_t;
typedef int                  fds_int32_t;
typedef unsigned short       fds_uint16_t;
typedef short                fds_int16_t;
typedef unsigned char        fds_uint8_t;
typedef char                 fds_int8_t;
typedef char                 fds_char_t;

/* Get the offset of a field y inside struct X. */
#define fds_offset_of(X, y)   ((unsigned long)((void *)&(((X *)0)->y)))

/* Get the address of the obj from addr of a field in it. */
#define fds_object_of(obj, field, ptr)                                        \
    ((obj *)(((char *)(ptr)) - (char *)fds_offset_of(obj, field)))

#define FDS_ARRAY_ELEM(array)         (sizeof(array)/sizeof(array[0]))

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

#endif /* INCLUDE_SHARED_FDS_TYPES_H_ */
