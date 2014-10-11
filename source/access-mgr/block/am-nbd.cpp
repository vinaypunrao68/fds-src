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
      ev_sk(-1), ev_fd(-1)
{
    ev_vol = this;
}

int
EvBlockMod::mod_init(SysParams const *const p)
{
    gl_BlockMod = &gl_NbdBlockMod;
    ev_blk_loop = EV_DEFAULT;

    return Module::mod_init(p);
}

void
EvBlockMod::mod_startup()
{
    Module::mod_startup();
}

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
 * blk_run_loop
 * ------------
 */
void
EvBlockMod::blk_run_loop()
{
    ev_run(ev_blk_loop, 0);
}

/**
 * blk_alloc_vol
 * -------------
 */
BlkVol::ptr
EvBlockMod::blk_alloc_vol(const blk_vol_creat_t *r)
{
    return new EvBlkVol(r);
}

/**
 * -----------------------------------------------------------------------------------
 *  NBD block module methods.
 * -----------------------------------------------------------------------------------
 */
NbdBlkVol::NbdBlkVol(const blk_vol_creat_t *r) : EvBlkVol(r)
{
    nbd_repl.magic = htonl(NBD_REPLY_MAGIC);
    nbd_repl.error = htonl(0);
}

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

/**
 * nbd_vol_callback
 * ----------------
 */
static void
nbd_vol_callback(struct ev_loop *loop, ev_io *ev, int revents)
{
    ssize_t         len;
    NbdBlkVol::ptr  vol;

    vol = reinterpret_cast<NbdBlkVol *>(ev + 1);
    if (revents & EV_READ) {
        len = read(vol->ev_sk, &vol->nbd_reqt, sizeof(vol->nbd_reqt));
        std::cout << "read code " << vol->nbd_reqt.type << ", len " << len << std::endl;
    }
}

/**
 * blk_attach_vol
 * --------------
 */
int
NbdBlockMod::blk_attach_vol(const blk_vol_creat_t *r)
{
    int            ret, fd;
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
    ret = ioctl(fd, NBD_SET_SIZE, r->v_vol_blksz * r->v_blksz);
    if (ret < 0) {
        return ret;
    }
    ret = ioctl(fd, NBD_CLEAR_SOCK);
    if (ret < 0) {
        return ret;
    }
    vol->ev_sk = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    fds_assert(vol->ev_sk > 0);

    ret = ioctl(fd, NBD_SET_SOCK, vol->ev_sk);
    if (ret < 0) {
        return ret;
    }
    ret = ioctl(fd, NBD_DO_IT);
    if (ret < 0) {
        return ret;
    }
    ioctl(fd, NBD_CLEAR_QUE);
    ioctl(fd, NBD_CLEAR_SOCK);
    close(fd);

    ev_init(&vol->ev_watch, nbd_vol_callback);
    ev_io_set(&vol->ev_watch, vol->ev_sk, EV_READ | EV_WRITE);
    ev_io_start(ev_blk_loop, &vol->ev_watch);
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
