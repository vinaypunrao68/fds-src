#ifndef INCLUDE_SHARED_FDS_TYPES_H_
#define INCLUDE_SHARED_FDS_TYPES_H_

/*
 * Define all basic fds types portable in all platforms and run time
 * environment.  Don't include any header files not in this directory.
 */
#ifdef __cplusplus
#define c_decls_begin        extern "C" {
#define c_decls_end          }
#else
#define c_decls_begin
#define c_decls_end
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
typedef bool                 fds_bool_t;

/* Do not change enum assignment in this type. */
typedef enum {
    FDS_DISK_DEFAULT         = 0,
    FDS_DISK_SSD             = 1,
    FDS_DISK_SAS             = 2,
    FDS_DISK_SATA            = 3
} fds_disk_type_t;

#endif /* INCLUDE_SHARED_FDS_TYPES_H_ */
