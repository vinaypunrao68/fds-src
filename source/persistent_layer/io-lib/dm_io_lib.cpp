/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <persistentdata.h>
#include <string>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <iostream>
#include <persistent_layer/dm_io.h>
#include <fds_process.h>

namespace diskio {

static const int  sgt_ssd_count = 2;
static const int  sgt_hdd_count = 11;
static const char *sgt_dt_file = "/data-";

// ----------------------------------------------------------------------------
DataIOModule            gl_dataIOMod("Disk IO Module");
DataDiscoveryModule     dataDiscoveryMod("Data Discovery Module");

static DataIO           *sgt_dataIO;
static PersisDataIO     *sgt_hddIO[sgt_hdd_count];
static PersisDataIO     *sgt_ssdIO[sgt_ssd_count];

// ----------------------------------------------------------------------------

// \DataIOModule
// -------------
//
DataIOModule::DataIOModule(char const *const name)
    : Module(name)
{
    static fds::Module *dataIOIntern[] = {
        &dataDiscoveryMod,
        &dataIndexMod,
        nullptr
    };
    mod_intern = dataIOIntern;
}

DataIOModule::~DataIOModule() {}

// \mod_init
// ---------
//
int
DataIOModule::mod_init(fds::SysParams const *const param)
{
    Module::mod_init(param);

    std::cout << "DataIOModule init..." << std::endl;
    sgt_dataIO = new DataIO;
    for (int i = 0; i < sgt_hdd_count; i++) {
        sgt_hddIO[i] =
            new FilePersisDataIO(dataDiscoveryMod.disk_hdd_path(i), i);
    }
    for (int  i = 0; i < sgt_ssd_count; i++) {
      sgt_ssdIO[i] =
          new FilePersisDataIO(dataDiscoveryMod.disk_ssd_path(i), i);
    }
    return 0;
}

// \mod_startup
// ------------
//
void
DataIOModule::mod_startup()
{
    Module::mod_startup();
    std::cout << "DataIOModule startup..." << std::endl;
}

// \mod_shutdown
// -------------
//
void
DataIOModule::mod_shutdown()
{
    Module::mod_shutdown();
}

// ----------------------------------------------------------------------------

// \DataDiscoveryModule
// --------------------
//
DataDiscoveryModule::DataDiscoveryModule(char const *const name)
    : Module(name), pd_hdd_found(0), pd_ssd_found(0)
{
    pd_hdd_raw     = new std::string [sgt_hdd_count];
    pd_ssd_raw     = new std::string [sgt_ssd_count];
    pd_hdd_labeled = new std::string [sgt_hdd_count];
    pd_ssd_labeled = new std::string [sgt_ssd_count];
}

DataDiscoveryModule::~DataDiscoveryModule()
{
    delete [] pd_hdd_raw;
    delete [] pd_ssd_raw;
    delete [] pd_hdd_labeled;
    delete [] pd_ssd_labeled;
}

// \disk_detect_label
// ------------------
// If the given path contains valid label, assign the disk to the correct
// place in the label array.
//
bool
DataDiscoveryModule::disk_detect_label(std::string *path, DataTier tier)
{
    int         fd, limit;
    std::string file, base = *path + std::string(sgt_dt_file);

    limit = tier == diskTier ? sgt_hdd_count : sgt_ssd_count;
    for (int i = 0; i < limit; i++) {
        file = base + std::to_string(i);
        if ((fd = open(file.c_str(), O_DIRECT | O_NOATIME)) > 0) {
            // Found the valid label.
            if (tier == diskTier) {
                pd_hdd_labeled[i] = file;
            } else if (tier == flashTier) {
                pd_ssd_labeled[i] = file;
            } else {
              fds_panic("Unknown tier label!");
            }
            close(fd);
            path->clear();
            std::cout << "Found label " << file << std::endl;
            return true;
        }
    }
    return false;
}

// \disk_make_label
// ----------------
// Assign the label file at the base path so that we'll have the right
// order of disks at the next time when we come up.
//
void
DataDiscoveryModule::disk_make_label(std::string *base,
                                     DataTier tier,
                                     int diskno)
{
    fds_int32_t   fd;
    std::string  *labeled;
    fds_uint32_t  found;

    if (tier == diskTier) {
        labeled = pd_hdd_labeled;
        found   = pd_hdd_found;
    } else if (tier == flashTier) {
        labeled = pd_ssd_labeled;
        found   = pd_ssd_found;
    } else {
        fds_panic("Unknown tier label!");
    }

    fds_verify((uint)diskno < found);
    labeled[diskno] = *base + std::string(sgt_dt_file) + std::to_string(diskno);

    fd = open(labeled[diskno].c_str(), O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    fds_verify(fd > 0);
    base->clear();
    close(fd);
    std::cout << "Created label " << labeled[diskno] << std::endl;
}

// \parse_device_dir
// ---------
//
void
DataDiscoveryModule::parse_device_dir(const std::string& path, DataTier tier)
{
    int                ret;
    DIR               *dfd;
    struct dirent     *result, dp;

    dfd = opendir(path.c_str());
    fds_verify(dfd != NULL);

    for (ret = readdir_r(dfd, &dp, &result);
         result != NULL && ret == 0; ret = readdir_r(dfd, &dp, &result)) {
        struct stat stbuf;
        std::string subdir(path + "/" + std::string(dp.d_name));

        if (stat(path.c_str(), &stbuf) < 0) {
            continue;
        }
        if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
            if (dp.d_name[0] == '.') {
                continue;
            }
            if (tier == diskTier) {
                if (pd_hdd_found < pd_hdd_count) {
                    /* Located a new hdd path */
                    pd_hdd_raw[pd_hdd_found] = subdir;
                    disk_detect_label(&pd_hdd_raw[pd_hdd_found], diskTier);
                    pd_hdd_found++;
                }
            } else if (tier == flashTier) {
                if (pd_ssd_found < pd_ssd_count) {
                    /* Located a new ssd path */
                    pd_ssd_raw[pd_ssd_found] = subdir;
                    disk_detect_label(&pd_ssd_raw[pd_ssd_found], flashTier);
                    pd_ssd_found++;
                }
            } else {
                fds_panic("Recieved unknown tier to parse!");
            }
        }
    }
}

// \mod_init
// ---------
//
int
DataDiscoveryModule::mod_init(fds::SysParams const *const param)
{
    int                ret;
    DIR               *dfd;
    struct dirent     *result, dp;
    fds::SimEnvParams *sim;

    Module::mod_init(param);
    sim = param->fds_sim;
    dfd = opendir(param->fds_root.c_str());
    if (dfd == nullptr) {
        std::cout << "Need to setup root storage directory "
             << param->fds_root << std::endl;
        return 1;
    }
    pd_hdd_count = sgt_hdd_count;
    pd_ssd_count = sgt_ssd_count;
    if (sim != nullptr) {
        pd_hdd_cap_mb = sim->sim_hdd_mb;
        pd_ssd_cap_mb = sim->sim_ssd_mb;
    } else {
        pd_hdd_cap_mb = 0;
        pd_ssd_cap_mb = 0;
    }
    for (ret = readdir_r(dfd, &dp, &result);
         result != NULL && ret == 0; ret = readdir_r(dfd, &dp, &result)) {
        struct stat  stbuf;
        std::string  path(param->fds_root + std::string(dp.d_name));

        if (stat(path.c_str(), &stbuf) < 0) {
            continue;
        }
        if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
            if (dp.d_name[0] == '.') {
                continue;
            }
            if (path.compare(param->fds_root.size(),
                             std::string::npos,
                             param->hdd_root, 0,
                             param->hdd_root.size() - 1) == 0) {
              /*
               * Parse the hdd directory
               */
              parse_device_dir(path, diskTier);
            } else if (path.compare(param->fds_root.size(),
                                    std::string::npos,
                                    param->ssd_root, 0,
                                    param->ssd_root.size() - 1) == 0) {
              /*
               * Parse the ssd directory
               */
              parse_device_dir(path, flashTier);
            }
        }
    }
    closedir(dfd);

    if (sim != nullptr) {
        std::string base(param->fds_root + param->hdd_root);
        fds::FdsRootDir::fds_mkdir(base.c_str());

        base += sim->sim_disk_prefix;
        for (char sd = 'a'; pd_hdd_found < pd_hdd_count; sd++) {
            std::string hdd = base + sd;

            fds::FdsRootDir::fds_mkdir(hdd.c_str());
            pd_hdd_raw[pd_hdd_found++] = hdd;
        }
        base = param->fds_root + param->ssd_root;
        fds::FdsRootDir::fds_mkdir(base.c_str());

        base += sim->sim_disk_prefix;
        for (char sd = 'a'; pd_ssd_found < pd_ssd_count; sd++) {
            std::string ssd = base + sd;

            fds::FdsRootDir::fds_mkdir(ssd.c_str());
            pd_ssd_raw[pd_ssd_found++] = ssd;
        }
    }
    for (int i = 0; i < pd_hdd_found; i++) {
        if (!pd_hdd_raw[i].empty()) {
            fds_verify(pd_hdd_labeled[i].empty());

            disk_make_label(&pd_hdd_raw[i], diskTier, i);
            fds_verify(!pd_hdd_labeled[i].empty());
        } else {
            fds_verify(!pd_hdd_labeled[i].empty());
        }
    }
    for (int i = 0; i < pd_ssd_found; i++) {
        if (!pd_ssd_raw[i].empty()) {
            fds_verify(pd_ssd_labeled[i].empty());

            disk_make_label(&pd_ssd_raw[i], flashTier, i);
            fds_verify(!pd_ssd_labeled[i].empty());
        } else {
            fds_verify(!pd_ssd_labeled[i].empty());
        }
    }
    return 0;
}

// \disk_hdd_path
// --------------
// Return the root path to access the disk at given route index.
//
char const *const
DataDiscoveryModule::disk_hdd_path(int route_idx)
{
    fds_verify(route_idx < pd_hdd_found);
    fds_verify(!pd_hdd_labeled[route_idx].empty());

    return pd_hdd_labeled[route_idx].c_str();
}

// \disk_ssd_path
// --------------
// Return the root path to access the disk at given route index.
//
char const *const
DataDiscoveryModule::disk_ssd_path(int route_idx)
{
    fds_verify(route_idx < pd_ssd_found);
    fds_verify(!pd_ssd_labeled[route_idx].empty());

    return pd_ssd_labeled[route_idx].c_str();
}

// \mod_startup
// ------------
//
void
DataDiscoveryModule::mod_startup()
{
    Module::mod_startup();
}

// \mod_shutdown
// -------------
//
void
DataDiscoveryModule::mod_shutdown()
{
    Module::mod_shutdown();
}

// ----------------------------------------------------------------------------

// \PersisDataIO::PersisDataIO
// ---------------------------
//
PersisDataIO::PersisDataIO()
    : pd_queue(2, 1000),
    pd_counters_("PM", fds::g_cntrs_mgr.get())
{
}

// \sersisDataIO::~PersisDataIO
// ----------------------------
//
PersisDataIO::~PersisDataIO()
{
}

// \PersisDataIO::disk_read
// ------------------------
// Handle all the queueing for persistent read IO.
//
int
PersisDataIO::disk_read(DiskRequest *req)
{
    int err;
    bool block = req->req_blocking_mode();

    pd_queue.rq_enqueue(req, pd_ioq_rd_pending);
    err = disk_do_read(req);

    // In non-blocking mode, the request may already be freed when we're here.
    if (block == true) {
        // If the request was created with non-blocking option, this is no-op.
        req->req_wait();
    }
    if (err)
      pd_counters_.diskR_Err.incr();
    return err;
}

// \PersisDataIO::disk_read_done
// -----------------------------
//
void
PersisDataIO::disk_read_done(DiskRequest *req)
{
    // If the request was created with blocking option, this will wake it up.
    // Also note that the req may be freed after this call returns.
    req->req_complete();
}

// \PersisDataIO::disk_write
// -------------------------
//
int
PersisDataIO::disk_write(DiskRequest *req)
{
    int err;
    bool block = req->req_blocking_mode();

    pd_queue.rq_enqueue(req, pd_ioq_rd_pending);
    err = disk_do_write(req);

    // In non-blocking mode, the request may already be freed when we're here.
    if (block == true) {
        // If the request was created with non-blocking option, this is no-op.
        req->req_wait();
    }
    if (err)
      pd_counters_.diskR_Err.incr();

    return err;
}

// \PersisDataIO::disk_write_done
// ------------------------------
//
void
PersisDataIO::disk_write_done(DiskRequest *req)
{
    // If the request was created with blocking option, this will wake it up.
    // Also note that the req may be freed after this call returns.
    req->req_complete();
}

// ----------------------------------------------------------------------------

// \DataIO
// -------
//
DataIO::DataIO()
{
}

DataIO::~DataIO()
{
}

// \disk_singleton
// ---------------
//
DataIO &
DataIO::disk_singleton()
{
    return *sgt_dataIO;
}

// \DataIO::disk_route_request
// ---------------------------
//
PersisDataIO *
DataIO::disk_route_request(DiskRequest *req)
{
    fds_uint32_t idx = 0;
    meta_obj_id_t const *const oid = req->req_get_oid();

    fds_uint32_t dev_count = sgt_hdd_count;
    if (req->getTier() == flashTier) {
      dev_count = sgt_ssd_count;
    } else {
      fds_verify(req->getTier() == diskTier);
    }

    if (obj_id_is_valid(oid)) {
        idx = (oid->oid_hash_hi + oid->oid_hash_lo) % dev_count;
    } else {
        meta_vol_io_t const *const vio = req->req_get_vio();

        if (vadr_is_valid(vio->vol_adr)) {
            idx = (vio->vol_uuid + vio->vol_blk_off) % dev_count;
        }
    }

    if (req->getTier() == flashTier) {
      return sgt_ssdIO[idx];
    }

    fds_verify(req->getTier() == diskTier);
    return sgt_hddIO[idx];
}

// \DataIO::disk_read
// ------------------
//
int
DataIO::disk_read(DiskRequest *req)
{
    PersisDataIO *iop = disk_route_request(req);
    return iop->disk_read(req);
}

// \DataIO::disk_write
// -------------------
//
int
DataIO::disk_write(DiskRequest *req)
{
    PersisDataIO *iop = disk_route_request(req);
    return iop->disk_write(req);
}

// \DataIO::disk_remap_obj
// -----------------------
//
void
DataIO::disk_remap_obj(meta_obj_id_t const *const obj_id,
                       meta_vol_io_t const *const cur_vio,
                       meta_vol_io_t const *const new_vio)
{
}

// \DataIO::disk_destroy_vol
// -------------------------
//
void
DataIO::disk_destroy_vol(meta_vol_adr_t *vol)
{
}

// \DataIO::disk_loc_path_info
// ---------------------------
//
void
DataIO::disk_loc_path_info(fds_uint16_t loc_id, std::string *path)
{
}

}  // namespace diskio
