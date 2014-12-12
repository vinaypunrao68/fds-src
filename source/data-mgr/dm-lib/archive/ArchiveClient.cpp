#include <string>
#include <ArchiveClient.h>

namespace fds {
void ArchiveClient::putSnap(const fds_volid_t &volId,
                            const std::string &snapLoc,
                            ArchivePutCb cb)
{
}

void ArchiveClient::getSnap(const fds_volid_t &volId, const std::string &snapName)
{
}
}  // namespace fds

