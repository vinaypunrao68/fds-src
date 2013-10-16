#include <iostream>
#include <disk-mgr/dm_io.h>
#include <persistentdata.h>

using namespace std;

namespace diskio {

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
// ----------------------------------------------------------------------------

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

static pdata_flayout_t gl_pdata[hdd_count];


// \DataIO
// -------
//
DataIO::DataIO(DataIndexProxy &idx,
               DataIOFunc     &iofn,
               int            nr_tier,
               int            max_depth)
    : io_func(iofn), io_index(idx)
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

// \disk_read
// ----------
//
void
DataIO::disk_read(DiskRequest &req)
{
}

// \disk_write
// -----------
//
void
DataIO::disk_write(DiskRequest &req)
{
}

// \disk_remap_obj
// ---------------
//
void
DataIO::disk_remap_obj(meta_obj_id_t &obj_id,
                       meta_vol_io_t &cur_vio,
                       meta_vol_io_t &new_vio)
{
}

// \disk_destroy_vol
// -----------------
//
void
DataIO::disk_destroy_vol(meta_vol_adr_t &vol)
{
}

// \disk_loc_path_info
// -------------------
//
void
DataIO::disk_loc_path_info(fds_uint16_t loc_id, std::string *path)
{
}

} // namespace diskio
