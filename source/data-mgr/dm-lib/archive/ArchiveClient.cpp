/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <sstream>
#include <cstdlib>
#include <boost/shared_ptr.hpp>
#include <fds_types.h>
#include <fds_volume.h>
#include <DataMgrIf.h>
#include <archive/ArchiveClient.h>

namespace fds {

ArchiveClient::ArchiveClient(DataMgrIf* dataMgrIf,
                             fds_threadpoolPtr threadpool)
: FdsRequestQueueActor("ArchiveClient", nullptr, threadpool)
{
    dataMgrIf_ = dataMgrIf;
    snapDirBase_ = dataMgrIf_->getSnapDirBase();
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

Error ArchiveClient::putSnapSync(const fds_volid_t &volId,
                                 const int64_t &snapId)
{
    Error retErr;
    concurrency::TaskStatus taskStatus;
    putSnap(volId, snapId,
            [&](const Error &err) {
                retErr = err;
                taskStatus.done();
            });
    taskStatus.await();
    return retErr;
}

void ArchiveClient::getSnap(const fds_volid_t &volId,
                            const int64_t &snapId,
                            ArchiveGetCb cb)
{
    auto payload = boost::make_shared<ArchiveClGetReq>(volId, snapId, cb);
    send_actor_request(boost::make_shared<FdsActorRequest>(
            FAR_ID(ArchiveClGetReq), payload));
}

Error ArchiveClient::getSnapSync(const fds_volid_t &volId,
                                 const int64_t &snapId)
{
    Error retErr;
    concurrency::TaskStatus taskStatus;
    getSnap(volId, snapId,
            [&](const Error &err) {
                retErr = err;
                taskStatus.done();
            });
    taskStatus.await();
    return retErr;
}

Error ArchiveClient::handle_actor_request(FdsActorRequestPtr req)
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
    default:
    {
        GLOGERROR <<  "Unknown type: " << req->type;
        err = ERR_FAR_INVALID_REQUEST;
    }
    }
    return err;
}

void ArchiveClient::handlePutSnap_(ArchiveClPutReqPtr &putPayload)
{
    std::stringstream ss;
    // TODO(Rao:)
    // 1. Clean up the tar file and snap after upload is done

    /* tar+zip the snapshot */
    std::string snapDirName = dataMgrIf_->getSnapDirName(putPayload->volId, putPayload->snapId);
    std::string snapDirPath = snapDirBase_ + "/" + snapDirName;
    std::string snapName = snapDirName + ".tgz";
    std::string snapPath = snapDirBase_ + "/" + snapName;

    ss << "cd " << snapDirBase_ << "; tar czf " << snapName << " " << snapDirName;
    GLOGDEBUG << "cmd: " << ss.str();
    int retCode = std::system(ss.str().c_str());
    if (retCode != 0) {
        GLOGERROR << "Failed to tar snap.  volid: " << putPayload->volId << " snapid: "
            << putPayload->snapId;
        putPayload->cb(ERR_ARCHIVE_SNAP_TAR_FAILED);
        return;
    }
    GLOGDEBUG << "Tarring commpleted.  snapName: " << snapName;

    /* Upload to s3 */
    std::string sysVolName = dataMgrIf_->getSysVolumeName(putPayload->volId);
    auto uploadStatus = s3client_->putFile(sysVolName, snapName, snapPath);
    if (uploadStatus != S3StatusOK) {
        GLOGERROR << "Failed to upload snap.  volid: " << putPayload->volId << " snapid: "
            << putPayload->snapId;
        putPayload->cb(ERR_ARCHIVE_SNAP_PUT_FAILED);
    } else {
        GLOGDEBUG << "upload commpleted.  snapName: " << snapName;
        putPayload->cb(ERR_OK);
    }
}

void ArchiveClient::handleGetSnap_(ArchiveClGetReqPtr &getPayload)
{
    std::stringstream ss;
    std::string snapDirName = dataMgrIf_->getSnapDirName(getPayload->volId, getPayload->snapId);
    std::string snapDirPath = snapDirBase_ + "/" + snapDirName;
    std::string snapName = snapDirName + ".tgz";
    std::string snapPath = snapDirBase_ + "/" + snapName;

    // TODO(Rao:)
    // 1. Clean up the tar file after untar is complete
    // 2. Make sure the snapshot doesn't exist as a dir or as a tar

    /* Download from s3 */
    std::string sysVolName = dataMgrIf_->getSysVolumeName(getPayload->volId);
    auto downloadStatus = s3client_->getFile(sysVolName, snapName, snapPath);
    if (downloadStatus != S3StatusOK) {
        GLOGERROR << "Failed to upload snap.  volid: " << getPayload->volId << " snapid: "
            << getPayload->snapId;
        getPayload->cb(ERR_ARCHIVE_SNAP_GET_FAILED);
        return;
    }
    GLOGDEBUG << "download commpleted. snapName: " << snapName;

    /* untar snap */
    ss << "cd " << snapDirBase_ << "; " << "tar xzf " << snapName;
    int retCode = std::system(ss.str().c_str());
    if (retCode != 0) {
        GLOGERROR << "Failed to untar snap.  volid: " << getPayload->volId << " snapid: "
            << getPayload->snapId;
        getPayload->cb(ERR_ARCHIVE_SNAP_TAR_FAILED);
    } else {
        GLOGDEBUG << "untarring commpleted. snapName: " << snapName;
        getPayload->cb(ERR_OK);
    }
}
}  // namespace fds

