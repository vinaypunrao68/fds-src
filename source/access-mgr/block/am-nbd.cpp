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
        LOGNORMAL << "[Blk] Failed to allo vol " << r->v_name << ", dev " << r->v_dev;
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
 * Ev based module methods.
 * -----------------------------------------------------------------------------------
 */
EvBlkVol::EvBlkVol(const blk_vol_creat_t *r)
    : BlkVol(r->v_name, r->v_dev, r->v_uuid, r->v_vol_blksz, r->v_blksz),
      vio_sk(-1), vio_read(NULL), vio_write(NULL) {}

/**
 * ev_vol_event
 * ------------
 */
void
EvBlkVol::ev_vol_event(struct ev_loop *loop, ev_io *ev, int revents)
{
    FdsAIO *vio;

    if (((revents & EV_WRITE) != 0) && (vio_write != NULL)) {
        vio_write->aio_write();
    }
    if (revents & EV_READ) {
        if (vio_read == NULL) {
            vio_read = static_cast<FdsAIO *>(ev_alloc_vio());
        }
        vio_read->aio_read();
    }
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
 *  NBD block module methods.
 * -----------------------------------------------------------------------------------
 */
NbdBlkVol::NbdBlkVol(const blk_vol_creat_t *r) : EvBlkVol(r)
{
    vio_repl = new NbdBlkIO(this, 0, 0);
    vio_buffer = new char [1 << 20];
}

NbdBlkVol::~NbdBlkVol()
{
    delete vio_repl;
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
    aio_set_iov(0, &nbd_reqt, sizeof(nbd_reqt));
}

/**
 * aio_on_error
 * ------------
 */
void
NbdBlkIO::aio_on_error()
{
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
}

/**
 * aio_rearm_write
 * ---------------
 */
void
NbdBlkIO::aio_rearm_write()
{
}

/**
 * aio_write_complete
 * ------------------
 */
void
NbdBlkIO::aio_write_complete()
{
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
    return new NbdBlkVol(r);
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

static int
write_all(int fd, char *buf, ssize_t len)
{
    ssize_t written;

    while (len > 0) {
        written = write(fd, buf, len);
        if (written < 0) {
            perror("Write error");
            std::cout << "error " << errno << ", wr " << written
                << ", len " << len << std::endl;
        }
        fds_assert(written > 0);
        buf += written;
        len -= written;
    }
    return 0;
}

static void
read_all(int fd, char *buf, ssize_t len)
{
    ssize_t cur;

    while (len > 0) {
        cur = read(fd, buf, len);
        if (cur <= 0) {
            std::cout << "read " << cur << ", error no " << errno << std::endl;
            if (cur == 0) {
                continue;
            }
            if (errno != EAGAIN) {
                return;
            }
            continue;
        }
        len -= cur;
        buf += cur;
    }
}

/**
 * nbd_vol_callback
 * ----------------
 */
static void
nbd_vol_callback(struct ev_loop *loop, ev_io *ev, int revents)
{
#if 0
    ssize_t             len, from;
    NbdBlkVol::ptr      vol;
    struct nbd_reply   *res;
    struct nbd_request *req;

    vol = reinterpret_cast<NbdBlkVol *>(ev->data);
    req = &vol->nbd_reqt;
    res = &vol->nbd_repl;
    read_all(vol->vio_sk, reinterpret_cast<char *>(req), sizeof(*req));
    memcpy(res->handle, req->handle, sizeof(res->handle));

    len  = ntohl(req->len);
    from = ntohll(req->from);
    fds_verify((len > 0) && (len < (1 << 20)));

    switch (ntohl(req->type)) {
    case NBD_CMD_READ:
        write_all(vol->vio_sk, reinterpret_cast<char *>(res), sizeof(*res));
        write_all(vol->vio_sk, vol->nbd_buffer, len);
        break;

    case NBD_CMD_WRITE:
        read_all(vol->vio_sk, vol->nbd_buffer, len);
        write_all(vol->vio_sk, reinterpret_cast<char *>(res), sizeof(*res));
        break;

    case NBD_CMD_DISC:
        break;

    case NBD_CMD_FLUSH:
        break;

    case NBD_CMD_TRIM:
        break;

    default:
        break;
    }
#endif
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
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
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
