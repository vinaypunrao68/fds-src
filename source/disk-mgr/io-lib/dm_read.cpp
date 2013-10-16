#include <persistentdata.h>

namespace diskio {

FilePersisDataIO::FilePersisDataIO(const char *file)
    : fi_path(file), fi_mutex("file mutex")
{
}

FilePersisDataIO::~FilePersisDataIO() {}

void
FilePersisDataIO::disk_do_read(DiskRequest *req)
{
    disk_read_done(req);
}

} // namespace diskio
