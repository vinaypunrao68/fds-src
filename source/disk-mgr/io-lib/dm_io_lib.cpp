#include <disk-mgr/dm_io.h>

namespace diskio {

// \DataIO
// -------
//
DataIO::DataIO(const DataIndexProxy &idx,
               const DataIOFunc     &iofn,
               int                  nr_queue,
               int                  max_depth)
    : io_queue(nr_queue, max_depth)
{
}

DataIO::~DataIO()
{
}

// \disk_singleton
// ---------------
//
const DataIO &
DataIO::disk_singleton()
{
}

// \disk_queue
// -----------
//
const fdsio::RequestQueue &
DataIO::disk_queue()
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
