/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PERSISTENT_LAYER_PM_UNIT_TEST_H_
#define SOURCE_INCLUDE_PERSISTENT_LAYER_PM_UNIT_TEST_H_

#include <concurrency/ThreadPool.h>
#include <persistent-layer/dm_io.h>

namespace fds {

class DiskReqTest : public diskio::DiskRequest
{
  public:
    DiskReqTest(fds_threadpool *const wr,
                fds_threadpool *const rd,
                meta_vol_io_t       &vio,
                meta_obj_id_t       &oid,
                fds::ObjectBuf      *buf,
                bool                block,
                diskio::DataTier    tier)
    : tst_wr(wr), tst_rd(rd), tst_iter(0),
      diskio::DiskRequest(vio, oid, buf, block, tier) {
        tst_verf.size = buf->size;
        tst_verf.data.reserve(buf->size);
    }
    DiskReqTest(meta_vol_io_t       &vio,
                meta_obj_id_t       &oid,
                fds::ObjectBuf      *buf,
                bool                block,
                diskio::DataTier    tier)
        : tst_iter(0), tst_wr(nullptr), tst_rd(nullptr),
          diskio::DiskRequest(vio, oid, buf, block, tier) {}

    virtual ~DiskReqTest() {
        delete dat_buf;
    }
    void req_submit() {
        fdsio::Request::req_submit();
    }
    void req_complete();
    void req_verify();
    void req_gen_pattern();

  private:
    int                        tst_iter;
    fds::ObjectBuf             tst_verf;
    fds::fds_threadpool *const tst_wr;
    fds::fds_threadpool *const tst_rd;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PERSISTENT_LAYER_PM_UNIT_TEST_H_
