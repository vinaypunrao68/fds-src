/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AM_NBD_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AM_NBD_H_

#include <ev.h>
#include <linux/nbd.h>
#include <fds_ptr.h>
#include <access-mgr/am-block.h>
#include <concurrency/spinlock.h>

namespace fds {

/**
 * Event based volume descriptor.
 */
class EvBlkVol : public BlkVol
{
  public:
    typedef bo::intrusive_ptr<EvBlkVol> ptr;
    explicit EvBlkVol(const blk_vol_creat_t *r);

    int                      ev_sk;
    int                      ev_fd;
    ev_io                    ev_watch;
    EvBlkVol::ptr            ev_vol;         /* must be next to ev_io */
};

/*
 * Ev based block interface module.
 */
class EvBlockMod : public BlockMod
{
  public:
    virtual ~EvBlockMod() {}
    EvBlockMod() : BlockMod(), ev_blk_loop(NULL) {}

    /* Module methods. */
    virtual int  mod_init(SysParams const *const p) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

    /* The run loop used by ev lib. */
    void blk_run_loop();

  protected:
    ev_io                    ev_stdin;
    struct ev_loop          *ev_blk_loop;

    virtual BlkVol::ptr blk_alloc_vol(const blk_vol_creat_t *r) override;
};

/**
 * NBD event based volume descriptor.
 */
class NbdBlkVol : public EvBlkVol
{
  public:
    typedef bo::intrusive_ptr<NbdBlkVol> ptr;
    explicit NbdBlkVol(const blk_vol_creat_t *r);

    struct nbd_request       nbd_reqt;
    struct nbd_reply         nbd_repl;
};

/**
 * NBD block module.
 */
class NbdBlockMod : public EvBlockMod
{
  public:
    virtual ~NbdBlockMod() {}
    NbdBlockMod() : EvBlockMod() {}

    static NbdBlockMod *nbd_singleton() { return &gl_NbdBlockMod; }

    virtual void mod_startup() override;

    /* Block driver methods. */
    virtual int blk_attach_vol(const blk_vol_creat_t *r) override;
    virtual int blk_detach_vol(fds_uint64_t uuid) override;
    virtual int blk_suspend_vol(fds_uint64_t uuid) override;

  protected:
    virtual BlkVol::ptr blk_alloc_vol(const blk_vol_creat_t *r) override;
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_AM_NBD_H_
