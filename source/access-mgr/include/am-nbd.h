/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AM_NBD_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AM_NBD_H_

#include <ev.h>
#include <linux/nbd.h>

#include <string>
#include <fds_ptr.h>
#include <fds-aio.h>
#include <access-mgr/am-block.h>
#include <concurrency/spinlock.h>
#include <fdsp/fds_service_types.h>

namespace fds {

class NbdBlkIO;
class NbdBlkVol;
class NbdBlockMod;
class QuorumSvcRequest;
class FailoverSvcRequest;

/**
 * Event based volume descriptor.
 */
class EvBlkVol : public BlkVol
{
  public:
    typedef bo::intrusive_ptr<EvBlkVol> ptr;
    explicit EvBlkVol(const blk_vol_creat_t *r, struct ev_loop *loop);

    void ev_vol_rearm(int events);
    void ev_vol_event(struct ev_loop *loop, ev_io *ev, int revents);

  protected:
    friend class NbdBlkIO;
    friend class NbdBlockMod;

    int                      vio_sk;
    ev_io                    vio_watch;
    fds_uint64_t             vio_txid;
    struct ev_loop          *vio_ev_loop;
    FdsAIO                  *vio_read;
    FdsAIO                  *vio_write;

    /* Factory method. */
    virtual FdsAIO *ev_alloc_vio() = 0;

    /* The run loop used by ev lib. */
    void blk_run_loop();
};

/*
 * Ev based block interface module.
 */
class EvBlockMod : public BlockMod
{
  public:
    virtual ~EvBlockMod() {}
    EvBlockMod() : BlockMod(), ev_prim_loop(NULL) {}

    /* Module methods. */
    virtual int  mod_init(SysParams const *const p) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

  protected:
    struct ev_loop          *ev_prim_loop;
};

/**
 * NBD event based volume descriptor.
 */
class NbdBlkVol : public EvBlkVol
{
  public:
    typedef bo::intrusive_ptr<NbdBlkVol> ptr;

    virtual ~NbdBlkVol();
    explicit NbdBlkVol(const blk_vol_creat_t *r, struct ev_loop *loop);

    void nbd_vol_read(NbdBlkIO *io);
    void nbd_vol_write(NbdBlkIO *io);
    void nbd_vol_close();
    void nbd_vol_flush();
    void nbd_vol_trim();

    void nbd_vol_write_cb(fpi::UpdateCatalogOnceMsgPtr, QuorumSvcRequest *,
                          const Error &, bo::shared_ptr<std::string>);

    void nbd_vol_read_cb(NbdBlkIO *vio, FailoverSvcRequest *req,
                         const Error &, bo::shared_ptr<std::string>);

  protected:
    friend class NbdBlkIO;
    char                    *vio_buffer;

    /* Factory method. */
    virtual FdsAIO *ev_alloc_vio();
};

/**
 * Async NBD Block IO request.
 */
class NbdBlkIO : public FdsAIO
{
  public:
    NbdBlkIO(NbdBlkVol::ptr vol, int iovcnt, int fd);

    virtual void aio_on_error();

    virtual void aio_rearm_read();
    virtual void aio_read_complete();

    virtual void aio_rearm_write();
    virtual void aio_write_complete();

  protected:
    friend class NbdBlkVol;

    NbdBlkVol::ptr           nbd_vol;
    struct nbd_request       nbd_reqt;
    struct nbd_reply         nbd_repl;
    ssize_t                  nbd_cur_len;
    ssize_t                  nbd_cur_off;

    inline void vio_setup_new_read()
    {
        aio_reset();
        aio_set_iov(0, reinterpret_cast<char *>(&nbd_reqt), sizeof(nbd_reqt));
    }
    inline void vio_setup_write_reply()
    {
        fds_assert(nbd_vol->vio_read == this);
        fds_assert(nbd_vol->vio_write == NULL);
        aio_reset();
        nbd_vol->vio_write = this;
    }
};

/**
 * NBD block module.
 */
class NbdBlockMod : public EvBlockMod
{
  public:
    virtual ~NbdBlockMod();
    NbdBlockMod();

    static NbdBlockMod *nbd_singleton() { return &gl_NbdBlockMod; }

    virtual void mod_startup() override;

    /* Block driver methods. */
    virtual int blk_attach_vol(blk_vol_creat_t *r) override;
    virtual int blk_detach_vol(fds_uint64_t uuid) override;
    virtual int blk_suspend_vol(fds_uint64_t uuid) override;

  protected:
    virtual BlkVol::ptr blk_alloc_vol(const blk_vol_creat_t *r) override;

  private:
    int                      nbd_devno;
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_AM_NBD_H_
