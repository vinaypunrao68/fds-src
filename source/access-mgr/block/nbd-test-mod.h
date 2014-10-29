/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_BLOCK_NBD_TEST_MOD_H_
#define SOURCE_ACCESS_MGR_BLOCK_NBD_TEST_MOD_H_

#include <string>
#include <am-nbd.h>

namespace fds {

/**
 * Different test volumes to measure performance.
 */
class NbdSmVol : public NbdBlkVol
{
  public:
    NbdSmVol(const blk_vol_creat_t *r, struct ev_loop *loop) : NbdBlkVol(r, loop) {}

    virtual void nbd_vol_read(NbdBlkIO *io);
    virtual void nbd_vol_write(NbdBlkIO *io);

    void nbd_vol_read_cb(NbdBlkIO *vio, FailoverSvcRequest *,
                         const Error &, bo::shared_ptr<std::string>);
    void nbd_vol_write_cb(NbdBlkIO *vio, QuorumSvcRequest *,
                          const Error &, bo::shared_ptr<std::string>);
};

/**
 * NBD block module driving SM IO.
 */
class NbdSmMod : public NbdBlockMod
{
  public:
    NbdSmMod() : NbdBlockMod() {}

  protected:
    virtual BlkVol::ptr blk_alloc_vol(const blk_vol_creat_t *r) override;
};

extern NbdSmMod              gl_NbdSmMod;

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_BLOCK_NBD_TEST_MOD_H_
