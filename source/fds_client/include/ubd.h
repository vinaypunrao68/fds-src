typedef double                     fbd_sector_t;

struct fbd_request {
  int                          op;
  char                         *buf;
  fbd_sector_t                 sec;
  int                          secs;
  void                         *req_data;
  int 				len;
  double 		        volUUID;
};

typedef struct fbd_request            fbd_request_t;
