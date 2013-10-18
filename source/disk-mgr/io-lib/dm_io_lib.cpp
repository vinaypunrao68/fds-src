#include <persistentdata.h>

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <iostream>
#include <disk-mgr/dm_io.h>

using namespace std;

namespace diskio {

static const int  sgt_ssd_count = 2;
static const int  sgt_hdd_count = 11;
static const char *sgt_dt_file = "/data-";

// ----------------------------------------------------------------------------
DataIOModule            gl_dataIOMod("Disk IO Module");
DataDiscoveryModule     dataDiscoveryMod("Data Discovery Module");

static DataIO           *sgt_dataIO;
static DataIndexLDb     *sgt_oidIndex;
static DataIndexLDb     *sgt_vioIndex;
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
void
DataIOModule::mod_init(fds::SysParams const *const param)
{
    Module::mod_init(param);

    cout << "DataIOModule init..." << endl;
    sgt_dataIO = new DataIO;
    for (int i = 0; i < sgt_hdd_count; i++) {
        sgt_hddIO[i] =
            new FilePersisDataIO(dataDiscoveryMod.disk_hdd_path(i), i);
    }
}

// \mod_startup
// ------------
//
void
DataIOModule::mod_startup()
{
    Module::mod_startup();
    cout << "DataIOModule startup..." << endl;
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
DataDiscoveryModule::disk_detect_label(std::string &path, bool hdd)
{
    int         fd, limit;
    std::string file, base = path + std::string(sgt_dt_file);

    limit = hdd == true ? sgt_hdd_count : sgt_ssd_count;
    for (int i = 0; i < limit; i++) {
        file = base + std::to_string(i);
        if ((fd = open(file.c_str(), O_DIRECT | O_NOATIME)) > 0) {
            // Found the valid label.
            if (hdd == true) {
                pd_hdd_labeled[i] = file;
            } else {
                pd_ssd_labeled[i] = file;
            }
            close(fd);
            path.clear();
            cout << "Found label " << file << endl;
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
DataDiscoveryModule::disk_make_label(std::string &base, int diskno)
{
    int fd;

    fds_verify(diskno < pd_hdd_found);
    pd_hdd_labeled[diskno] = base +
        std::string(sgt_dt_file) + std::to_string(diskno);

    fd = open(pd_hdd_labeled[diskno].c_str(),
              O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);

    fds_verify(fd > 0);
    base.clear();
    close(fd);
    cout << "Created label " << pd_hdd_labeled[diskno] << endl;
}

// \mod_init
// ---------
//
void
DataDiscoveryModule::mod_init(fds::SysParams const *const param)
{
    DIR               *dfd;
    struct dirent     *dp;
    fds::SimEnvParams *sim;

    Module::mod_init(param);
    sim = param->fds_sim;
    dfd = opendir(param->fds_root.c_str());
    if (dfd == nullptr) {
        cout << "Need to setup root storage directory "
             << param->fds_root << endl;
        exit(1);
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
    while ((dp = readdir(dfd)) != nullptr) {
        struct stat  stbuf;
        std::string  path(param->fds_root + std::string(dp->d_name));

        if (stat(path.c_str(), &stbuf) < 0) {
            continue;
        }
        if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
            if (dp->d_name[0] == '.') {
                continue;
            }
            if (pd_hdd_found < pd_hdd_count) {
                pd_hdd_raw[pd_hdd_found] = path;
                disk_detect_label(pd_hdd_raw[pd_hdd_found], true);
                pd_hdd_found++;
            }
        }
    }
    closedir(dfd);

    if (sim != nullptr) {
        std::string base(param->fds_root + sim->sim_disk_prefix);

        for (char sd = 'a'; pd_hdd_found < pd_hdd_count; sd++) {
            std::string hdd = base + sd;

            umask(0);
            if (mkdir(hdd.c_str(), 0755) == 0) {
                pd_hdd_raw[pd_hdd_found++] = hdd;
                continue;
            }
            if ((errno == EACCES) || (sd == 'z')) {
                cout << "Don't have permission to fds root: "
                     << param->fds_root << endl;
                exit(1);
            }
        }
    }
    for (int i = 0; i < pd_hdd_found; i++) {
        if (!pd_hdd_raw[i].empty()) {
            fds_verify(pd_hdd_labeled[i].empty());

            disk_make_label(pd_hdd_raw[i], i);
            fds_verify(!pd_hdd_labeled[i].empty());
        } else {
            fds_verify(!pd_hdd_labeled[i].empty());
        }
    }
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
    : pd_queue(2, 1000)
{
}

// \PersisDataIO::~PersisDataIO
// ----------------------------
//
PersisDataIO::~PersisDataIO()
{
}

// \PersisDataIO::disk_read
// ------------------------
// Handle all the queueing for persistent read IO.
//
void
PersisDataIO::disk_read(DiskRequest *req)
{
    bool block = req->req_blocking_mode();

    pd_queue.rq_enqueue(req, pd_ioq_rd_pending);
    disk_do_read(req);

    // In non-blocking mode, the request may already be freed when we're here.
    if (block == true) {
        // If the request was created with non-blocking option, this is no-op.
        req->req_wait();
    }
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
void
PersisDataIO::disk_write(DiskRequest *req)
{
    bool block = req->req_blocking_mode();

    pd_queue.rq_enqueue(req, pd_ioq_rd_pending);
    disk_do_write(req);

    // In non-blocking mode, the request may already be freed when we're here.
    if (block == true) {
        // If the request was created with non-blocking option, this is no-op.
        req->req_wait();
    }
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
    int idx = 0;
    meta_obj_id_t const *const oid = req->req_get_oid();

    if (obj_id_is_valid(oid)) {
        idx = (oid->oid_hash_hi + oid->oid_hash_lo) % sgt_hdd_count;
    } else {
        meta_vol_io_t const *const vio = req->req_get_vio();

        if (vadr_is_valid(vio->vol_adr)) {
            idx = (vio->vol_uuid + vio->vol_blk_off) % sgt_hdd_count;
        }
    }
    return sgt_hddIO[idx];
}

// \DataIO::disk_read
// ------------------
//
void
DataIO::disk_read(DiskRequest *req)
{
    PersisDataIO *iop = disk_route_request(req);
    iop->disk_read(req);
}

// \DataIO::disk_write
// -------------------
//
void
DataIO::disk_write(DiskRequest *req)
{
    PersisDataIO *iop = disk_route_request(req);
    iop->disk_write(req);
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
DataIO::disk_destroy_vol(meta_vol_adr_t &vol)
{
}

// \DataIO::disk_loc_path_info
// ---------------------------
//
void
DataIO::disk_loc_path_info(fds_uint16_t loc_id, std::string *path)
{
}

} // namespace diskio
