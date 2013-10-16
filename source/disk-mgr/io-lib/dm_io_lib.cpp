#include <iostream>
#include <disk-mgr/dm_io.h>
#include <persistentdata.h>

using namespace std;

namespace diskio {

// ----------------------------------------------------------------------------
DataIOModule        dataIOMod("Disk IO Module");
static DataIO       *dataIO;
static fds::Module  *dataIOIntern[] =
{
    &dataIndexMod,
    nullptr
};

DataIOModule::DataIOModule(char const *const name)
    : Module(name)
{
    mod_intern = dataIOIntern;
}

DataIOModule::~DataIOModule() {}

void
DataIOModule::mod_init(fds::SysParams const *const param)
{
    Module::mod_init(param);
    cout << "DataIOModule init..." << endl;
}

void
DataIOModule::mod_startup()
{
    Module::mod_startup();
    cout << "DataIOModule startup..." << endl;
}

void
DataIOModule::mod_shutdown()
{
}

// ----------------------------------------------------------------------------
// Hard coded value to map objects to file layout.
//

typedef struct pdata_flayout pdata_flayout_t;
struct pdata_flayout
{
    int            fd_fd;
    char           *fl_path;
    fds_uint64_t   fd_cur_off;
};

static const int ssd_count = 2;
static const int hdd_count = 12;
static const char *fs_root = "/fds/mnt/";

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
    pd_queue.rq_enqueue(req, pd_ioq_rd_pending);
    disk_do_read(req);

    // If the request was created with non-blocking option, this is no-op.
    req->req_wait();
}

// \PersisDataIO::disk_read_done
// -----------------------------
//
void
PersisDataIO::disk_read_done(DiskRequest *req)
{
    // If the request was created with blocking option, this will wake it up.
    req->req_complete();
}

// \PersisDataIO::disk_write
// -------------------------
//
void
PersisDataIO::disk_write(DiskRequest *req)
{
    pd_queue.rq_enqueue(req, pd_ioq_rd_pending);
    disk_do_write(req);

    // If the request was created with non-blocking option, this is no-op.
    req->req_wait();
}

// \PersisDataIO::disk_write_done
// ------------------------------
//
void
PersisDataIO::disk_write_done(DiskRequest *req)
{
    // If the request was created with blocking option, this will wake it up.
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
}

// \DataIO::disk_route_request
// ---------------------------
//
PersisDataIO *
DataIO::disk_route_request(DiskRequest *req)
{
    return nullptr;
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
    iop->disk_read(req);
}

// \DataIO::disk_remap_obj
// -----------------------
//
void
DataIO::disk_remap_obj(meta_obj_id_t &obj_id,
                       meta_vol_io_t &cur_vio,
                       meta_vol_io_t &new_vio)
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
