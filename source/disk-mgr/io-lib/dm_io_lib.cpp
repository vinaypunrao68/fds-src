#include <disk-mgr/dm_io.h>

namespace diskio {

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

const DataIO &
DataIO::disk_singleton()
{
}

const fdsio::RequestQueue &
DataIO::disk_queue()
{
}

void
DataIO::disk_read(DiskRequest &req)
{
}

void
DataIO::disk_write(DiskRequest &req)
{
}

void
DataIO::disk_remap_obj(meta_obj_id_t &obj_id,
                       meta_vol_io_t &cur_vio,
                       meta_vol_io_t &new_vio)
{
}

void
DataIO::disk_destroy_vol(meta_vol_adr_t &vol)
{
}

void
DataIO::disk_loc_path_info(fds_uint16_t loc_id, std::string *path)
{
}

} // namespace diskio
