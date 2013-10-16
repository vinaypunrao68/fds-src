#include <persistentdata.h>

namespace diskio {

void
FilePersisDataIO::disk_do_write(DiskRequest *req)
{
    disk_write_done(req);
}

} // namespace diskio
