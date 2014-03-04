#include "../blk_dev/tool/fbd.h"

#define 	IO_READ		0
#define 	IO_WRITE	1

typedef double                     fbd_sector_t;

#define FDS_UBD_IO_MAGIC_IN_USE  0xCF17C349
#define FDS_UBD_IO_MAGIC_NOT_IN_USE 0xBE28B25A


struct fbd_request {
  int                          req_magic;
  int                          op;
  int			       io_type;
  char                         *buf;
  fbd_sector_t                 sec;
  int                          secs;
  void                         *req_data;
  int 				len;
  long long                     volUUID;
  long                          sh_queued_usec;
  void				*vbd;
  void				*vReq;
  void				*hvisorHdl;
  void 				(*cb_request)(void *arg1, void *arg2, struct fbd_request *freq, int res);
};

typedef struct fbd_request            fbd_request_t;
