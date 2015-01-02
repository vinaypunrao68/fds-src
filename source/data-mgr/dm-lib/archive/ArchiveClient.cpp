/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <cstdlib>
#include <boost/shared_ptr.hpp>
#include <ArchiveClient.h>

namespace fds {

ArchiveClient::ArchiveClient(fds_threadpoolPtr threadpool)
: FdsRequestQueueActor("ArchiveClient", nullptr, threadpool)
{
    // TODO(Rao): Set snapdir base
}

void ArchiveClient::connect(const std::string &host,
                            const std::string &accessKey,
                            const std::string &secretKey)
{
    s3client_.reset(new S3Client(host, accessKey, secretKey));
}

void ArchiveClient::putSnap(const fds_volid_t &volId,
                            const int64_t &snapId,
                            ArchivePutCb cb)
{
    auto payload = boost::make_shared<ArchiveClPutReq>(volId, snapId, cb);
    send_actor_request(boost::make_shared<FdsActorRequest>(
            FAR_ID(ArchiveClPutReq), payload));
}

void ArchiveClient::getSnap(const fds_volid_t &volId,
                            const int64_t &snapId,
                            ArchiveGetCb cb)
{
    auto payload = boost::make_shared<ArchiveClGetReq>(volId, snapId, cb);
    send_actor_request(boost::make_shared<FdsActorRequest>(
            FAR_ID(ArchiveClGetReq), payload));
}

Error ArchiveClient::handle_actor_request(FdsActorRequestPtr req) override
{
    Error err = ERR_OK;
    switch (req->type) {
    case FAR_ID(ArchiveClPutReq):
    {
        auto putPayload = req->get_payload<ArchiveClPutReq>();
        handlePutSnap_(putPayload);
        break;
    }
    case FAR_ID(ArchiveClGetReq):
    {
        auto getPayload = req->get_payload<ArchiveClGetReq>();
        handleGetSnap_(getPayload);
        break;
    }
    }
    return err;
}

void ArchiveClient::handlePutSnap_(ArchiveClPutReqPtr &putPayload)
{
    /* tar+zip the snapshot */
    std::string snapDirName = getSnapDirName_(putPayload->volId, putPayload->snapId);
    std::string snapDirPath = snapDirBase_ + "/" + snapDirName;
    std::strings snapName = snapDirName + ".tgz";
    std::string snapPath = snapDirBase_ + "/" + snapName;
    int retCode = std::system("tar cvzf " + snapPath +  " " + snapDirPath);
    if (retCode != 0) {
        GLOGERROR << "Failed to tar snap.  volid: " << putPayload->volId << " snapid: "
            << putPayload->snapId;
        putPayload->cb(ERR_ARCHIVE_SNAP_TAR_FAILED);
        return;
    }

    /* Upload to s3 */
    std::string sysVolName = getSysVolumeName_(putPayload->volId);
    auto uploadStatus = s3client_->putFile(sysVolName, snapName, snapPath);
    // TODO(Rao): Remove the tar file
    if (uploadStatus != S3StatusOK) {
        GLOGERROR << "Failed to upload snap.  volid: " << putPayload->volId << " snapid: "
            << putPayload->snapId;
        putPayload->cb(ERR_ARCHIVE_SNAP_PUT_FAILED);
    } else {
        putPayload->cb(ERR_OK);
    }
}

void ArchiveClient::handleGetSnap_(ArchiveClGetReqPtr &getPayload)
{
    std::string snapDirName = getSnapDirName_(getPayload->volId, getPayload->snapId);
    std::string snapDirPath = snapDirBase_ + "/" + snapDirName;
    std::strings snapName = snapDirName + ".tgz";
    std::string snapPath = snapDirBase_ + "/" + snapName;

    /* Download from s3 */
    std::string sysVolName = getSysVolumeName_(getPayload->volId);
    auto downloadStatus = s3client_->getFile(sysVolName, snapName, snapPath);
    if (downloadStatus != S3StatusOK) {
        GLOGERROR << "Failed to upload snap.  volid: " << putPayload->volId << " snapid: "
            << putPayload->snapId;
        putPayload->cb(ERR_ARCHIVE_SNAP_GET_FAILED);
    }

    /* untar snap */
    // TODO(Rao:) Set the output director
    int retCode = std::system("tar xvzf " + snapPath);
    if (retCode != 0) {
        GLOGERROR << "Failed to untar snap.  volid: " << getPayload->volId << " snapid: "
            << getPayload->snapId;
        getPayload->cb(ERR_ARCHIVE_SNAP_TAR_FAILED);
    } else {
        // TODO(Rao): Remove the tar file
        getPayload->cb(ERR_OK);
    }
}

std::string ArchiveClient::getSysVolumeName_(const fds_volid_t &volId)
{
    /* TODO(Rao): Return the correct name */
    return "fds_volume";
}

std::string ArchiveClient::getSnapDirName_(const fds_volid_t &volId,
                                           const int64_t snapId)
{
    /* TODO(Rao): Return the correct path */
    return "snap1";
}
}  // namespace fds

