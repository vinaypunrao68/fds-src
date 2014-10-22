/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <errno.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/nbd.h>

#include <am-nbd.h>
#include <fds_process.h>
#include <util/Log.h>

namespace fds {

BlockMod                    *gl_BlockMod;
NbdBlockMod                  gl_NbdBlockMod;

/**
 * -----------------------------------------------------------------------------------
 * Generic block module.
 * -----------------------------------------------------------------------------------
 */
BlkVol::~BlkVol() {}
BlkVol::BlkVol(const char *name, const char *dev,
               fds_uint64_t uuid, fds_uint64_t vol_sz, fds_uint32_t blk_sz)
    : vol_uuid(uuid), vol_sz_blks(vol_sz), vol_blksz_byte(blk_sz)
{
    memcpy(vol_name, name, FDS_MAX_VOL_NAME);
    memcpy(vol_dev, dev, FDS_MAX_VOL_NAME);
}

BlockMod::~BlockMod() {}
BlockMod::BlockMod() : Module("FDS Block") {}

/**
 * blk_creat_vol
 * -------------
 */
BlkVol::ptr
BlockMod::blk_creat_vol(const blk_vol_creat_t *r)
{
    BlkVol::ptr vol;

    vol = blk_alloc_vol(r);
    if (vol == NULL) {
        LOGNORMAL << "[Blk] Failed to alloc vol " << r->v_name << ", dev " << r->v_dev;
        return NULL;
    }
    blk_splck.lock();
    auto it = blk_vols.find(r->v_uuid);
    fds_verify(it == blk_vols.end());

    blk_vols.insert(std::make_pair(r->v_uuid, vol));
    blk_splck.unlock();

    return vol;
}

/**
 * blk_detach_vol
 * --------------
 */
int
BlockMod::blk_detach_vol(fds_uint64_t uuid)
{
    blk_splck.lock();
    blk_vols.erase(uuid);
    blk_splck.unlock();

    return 0;
}

/**
 * -----------------------------------------------------------------------------------
 * Ev Block Volume and Module.
 * -----------------------------------------------------------------------------------
 */
EvBlkVol::EvBlkVol(const blk_vol_creat_t *r, struct ev_loop *loop)
    : BlkVol(r->v_name, r->v_dev, r->v_uuid, r->v_vol_blksz, r->v_blksz),
      vio_sk(-1), vio_ev_loop(loop), vio_read(NULL), vio_write(NULL) {}

/**
 * ev_vol_event
 * ------------
 */
void
EvBlkVol::ev_vol_event(struct ev_loop *loop, ev_io *ev, int revents)
{
    FdsAIO *vio;

    fds_assert(vio_ev_loop == loop);
    if (((revents & EV_WRITE) != 0) || (vio_write != NULL)) {
        vio_write->aio_write();
        if (vio_write != NULL) {
            /* Don't do anything until we can push this write out. */
            return;
        }
    }
    fds_assert(vio_write == NULL);
    if (revents & EV_READ) {
        if (vio_read == NULL) {
            vio_read = static_cast<FdsAIO *>(ev_alloc_vio());
        }
        vio_read->aio_read();
    }
}

/**
 * ev_vol_rearm
 * ------------
 * Rearm the EV watcher with new events.
 */
void
EvBlkVol::ev_vol_rearm(int events)
{
    ev_io_stop(vio_ev_loop, &vio_watch);
    ev_io_set(&vio_watch, vio_sk, events);
    ev_io_start(vio_ev_loop, &vio_watch);
}

/**
 * mod_init
 * --------
 */
int
EvBlockMod::mod_init(SysParams const *const p)
{
    gl_BlockMod = &gl_NbdBlockMod;
    ev_blk_loop = EV_DEFAULT;

    return Module::mod_init(p);
}

/**
 * mod_startup
 * -----------
 */
void
EvBlockMod::mod_startup()
{
    Module::mod_startup();
}

/**
 * mod_enable_service
 * ------------------
 */
void
EvBlockMod::mod_enable_service()
{
    fds_threadpool *pool = g_fdsprocess->proc_thrpool();

    pool->schedule(&EvBlockMod::blk_run_loop, this);
    Module::mod_enable_service();
}

/**
 * mod_shutdown
 * ------------
 */
void
EvBlockMod::mod_shutdown()
{
    Module::mod_shutdown();
}

/**
 * ev_idle_cb
 * ----------
 */
static void
ev_idle_cb(struct ev_loop *loop, ev_idle *ev, int revents)
{
}

/**
 * blk_run_loop
 * ------------
 */
void
EvBlockMod::blk_run_loop()
{
    ev_idle_init(&ev_idle_evt, ev_idle_cb);
    ev_idle_start(ev_blk_loop, &ev_idle_evt);
    ev_run(ev_blk_loop, 0);
}

/**
 * -----------------------------------------------------------------------------------
 *  NBD Block Volume
 * -----------------------------------------------------------------------------------
 */
NbdBlkVol::NbdBlkVol(const blk_vol_creat_t *r, struct ev_loop *loop) : EvBlkVol(r, loop)
{
    vio_buffer = new char [1 << 20];
}

NbdBlkVol::~NbdBlkVol()
{
    delete [] vio_buffer;
}

/**
 * nbd_vol_read
 * ------------
 */
void
NbdBlkVol::nbd_vol_read(NbdBlkIO *vio)
{
}

/**
 * nbd_vol_write
 * -------------
 */
void
NbdBlkVol::nbd_vol_write(NbdBlkIO *vio)
{
}

/**
 * nbd_vol_close
 * -------------
 */
void
NbdBlkVol::nbd_vol_close()
{
}

/**
 * nbd_vol_flush
 * -------------
 */
void
NbdBlkVol::nbd_vol_flush()
{
}

/**
 * nbd_vol_trim
 * ------------
 */
void
NbdBlkVol::nbd_vol_trim()
{
}

/**
 * ev_alloc_vio
 * ------------
 */
FdsAIO *
NbdBlkVol::ev_alloc_vio()
{
    return new NbdBlkIO(this, 0, vio_sk);
}

/*
 * -----------------------------------------------------------------------------------
 * NBD Async Socket IO
 * -----------------------------------------------------------------------------------
 */
NbdBlkIO::NbdBlkIO(NbdBlkVol::ptr vol, int iovcnt, int fd)
    : FdsAIO(iovcnt, fd), nbd_vol(vol)
{
    nbd_repl.magic = htonl(NBD_REPLY_MAGIC);
    nbd_repl.error = htonl(0);
    aio_set_iov(0, reinterpret_cast<char *>(&nbd_reqt), sizeof(nbd_reqt));
}

/**
 * aio_on_error
 * ------------
 */
void
NbdBlkIO::aio_on_error()
{
    std::cout << "We got error\n";
    nbd_vol->vio_write = NULL;
    FdsAIO::aio_on_error();
}

/**
 * aio_rearm_read
 * --------------
 */
void
NbdBlkIO::aio_rearm_read()
{
}

/**
 * aio_read_complete
 * -----------------
 */
void
NbdBlkIO::aio_read_complete()
{
    ssize_t len;

    if (aio_cur_iov()== 1) {
        /* We got the header. */
        if (nbd_reqt.magic != htonl(NBD_REQUEST_MAGIC)) {
            aio_on_error();
            return;
        }
        memcpy(nbd_repl.handle, nbd_reqt.handle, sizeof(nbd_repl.handle));
        nbd_cur_off = ntohll(nbd_reqt.from);

        len = ntohl(nbd_reqt.len);
        fds_verify((len > 0) && (len < (1 << 20)));

        switch (ntohl(nbd_reqt.type)) {
        case NBD_CMD_READ:
            nbd_vol->nbd_vol_read(this);

            /* For now, assume we have data to send back. */
            this->vio_setup_write_reply();
            aio_set_iov(0, reinterpret_cast<char *>(&nbd_repl), sizeof(nbd_repl));
            aio_set_iov(1, nbd_vol->vio_buffer, len);
            this->aio_write();
            break;

        case NBD_CMD_WRITE:
            aio_set_iov(1, nbd_vol->vio_buffer, len);
            this->aio_read();
            break;

        case NBD_CMD_DISC:
            nbd_vol->nbd_vol_close();
            break;

        case NBD_CMD_FLUSH:
            nbd_vol->nbd_vol_flush();
            break;

        case NBD_CMD_TRIM:
            nbd_vol->nbd_vol_trim();
            break;

        default:
            break;
        }
    } else {
        /* We got the data for NBD_CMD_WRITE from the socket. */
        fds_verify(aio_cur_iov() == 2);
        nbd_vol->nbd_vol_write(this);

        /* Send the response back. */
        this->vio_setup_write_reply();
        aio_set_iov(0, reinterpret_cast<char *>(&nbd_repl), sizeof(nbd_repl));
        this->aio_write();
    }
}

/**
 * aio_rearm_write
 * ---------------
 */
void
NbdBlkIO::aio_rearm_write()
{
    nbd_vol->ev_vol_rearm(EV_READ | EV_WRITE);
}

/**
 * aio_write_complete
 * ------------------
 */
void
NbdBlkIO::aio_write_complete()
{
    fds_assert(nbd_vol->vio_write == nbd_vol->vio_read);
    nbd_vol->vio_write = NULL;

    /* Stop NBD socket write watch. */
    vio_setup_new_read();
    nbd_vol->ev_vol_rearm(EV_READ);
}

/*
 * -----------------------------------------------------------------------------------
 *  NBD Block Module
 * -----------------------------------------------------------------------------------
 */
NbdBlockMod::~NbdBlockMod() {}
NbdBlockMod::NbdBlockMod() : EvBlockMod() {}

/**
 * blk_alloc_vol
 * -------------
 */
BlkVol::ptr
NbdBlockMod::blk_alloc_vol(const blk_vol_creat *r)
{
    fds_verify(ev_blk_loop != NULL);
    return new NbdBlkVol(r, ev_blk_loop);
}

/**
 * mod_startup
 * -----------
 */
void
NbdBlockMod::mod_startup()
{
    EvBlockMod::mod_startup();
}

/**
 * nbd_vol_callback
 * ----------------
 */
static void
nbd_vol_callback(struct ev_loop *loop, ev_io *ev, int revents)
{
    NbdBlkVol::ptr vol;

    vol = reinterpret_cast<NbdBlkVol *>(ev->data);
    vol->ev_vol_event(loop, ev, revents);
}

/**
 * blk_attach_vol
 * --------------
 */
int
NbdBlockMod::blk_attach_vol(const blk_vol_creat_t *r)
{
    int            ret, tfd, fd, sp[2];
    BlkVol::ptr    vb;
    NbdBlkVol::ptr vol;

    vb = blk_creat_vol(r);
    if (vb == NULL) {
        return -1;
    }
    vol = blk_vol_cast<NbdBlkVol>(vb);
    fd  = open(r->v_dev, O_RDWR);

    if (fd < 0) {
        /* The vol will be freed when it's out of scope. */
        return -1;
    }
    ret = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sp);
    fds_verify(ret == 0);

    ret = ioctl(fd, NBD_SET_SIZE, r->v_vol_blksz * r->v_blksz);
    if (ret < 0) {
        return ret;
    }
    ret = ioctl(fd, NBD_CLEAR_SOCK);
    if (ret < 0) {
        return ret;
    }
    if (!fork()) {
        /*
         * We need to connect the child's socket to NBD to hookup the parent.
         * NBD protocol requires a thread to do NBD_DO_IT ioctl and be blocked there.
         */
        close(sp[0]);
        if (ioctl(fd, NBD_SET_SOCK, sp[1]) < 0) {
            /* Vy: What to do here? */
        } else {
            ret = ioctl(fd, NBD_DO_IT);
        }
        ioctl(fd, NBD_CLEAR_QUE);
        ioctl(fd, NBD_CLEAR_SOCK);
        _exit(0);
    }
    close(fd);
    close(sp[1]);

    vol->vio_sk = sp[0];
    fds_assert(vol->vio_sk > 0);

    ev_init(&vol->vio_watch, nbd_vol_callback);
    vol->vio_watch.data = reinterpret_cast<void *>(vol.get());
    ev_io_set(&vol->vio_watch, vol->vio_sk, EV_READ);
    ev_io_start(ev_blk_loop, &vol->vio_watch);
    return 0;
}

/**
 * blk_detach_vol
 * --------------
 */
int
NbdBlockMod::blk_detach_vol(fds_uint64_t uuid)
{
    return 0;
}

/**
 * blk_suspend_vol
 * ---------------
 */
int
NbdBlockMod::blk_suspend_vol(fds_uint64_t uuid)
{
    return 0;
}

}  // namespace fds
