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
#include <S3Client.h>
#include <archive/ArchiveClient.h>

namespace fds {

ArchiveClient::ArchiveClient(DataMgrIf* dataMgrIf)
{
    dataMgrIf_ = dataMgrIf;
    snapDirBase_ = dataMgrIf_->getSnapDirBase();
}

Error ArchiveClient::tarSnap_(uint64_t volId, uint64_t snapId,
                              const std::string &snapDirPath,
                              const std::string &snapName)
{
    std::stringstream ss;
    ss << "cd " << snapDirPath << "; ";
    ss << "tar czf " << snapName << " " << snapId;
    GLOGDEBUG << "cmd: " << ss.str();
    int retCode = std::system(ss.str().c_str());
    if (retCode != 0) {
        GLOGERROR << "Failed to tar snap.  volid: " << volId
            << " snapid: " << snapId;
        return ERR_ARCHIVE_SNAP_TAR_FAILED;
    }
    GLOGDEBUG << "Tarring commpleted.  snapName: " << snapName;
    return ERR_OK;
}

Error ArchiveClient::untarSnap_(uint64_t volId, uint64_t snapId,
                               const std::string &snapDirPath,
                               const std::string &snapName)
{
    std::stringstream ss;
    ss << "cd " << snapDirPath << "; " << "tar xzf " << snapName;
    int retCode = std::system(ss.str().c_str());
    if (retCode != 0) {
        GLOGERROR << "Failed to untar snap.  volid: " << volId
            << " snapid: " << snapId;
        return ERR_ARCHIVE_SNAP_TAR_FAILED;
    } else {
        GLOGDEBUG << "untarring commpleted. snapName: " << snapName;
        return ERR_OK;
    }
}


void ArchiveClient::populateSnapInfo_(const fds_volid_t &volId,
                                      const int64_t &snapId,
                                      std::string &snapName,
                                      std::string &snapDirPath,
                                      std::string &snapPath)
{
    std::stringstream ss;
    /* Construct snap name */
    ss << volId << "_" << snapId << ".tgz";
    snapName = ss.str();
    ss.str(std::string());

    /* Construct snap dir path */
    ss << snapDirBase_ << "/" << volId;
    snapDirPath = ss.str();
    ss.str(std::string());

    /* Construct snap path */
    ss << snapDirPath << "/" << snapName;
    snapPath = ss.str();
    ss.str(std::string());
}

BotoArchiveClient::BotoArchiveClient(const std::string &host,
                                     int port,
                                     const std::string &authEp,
                                     const std::string &user,
                                     const std::string &passwd,
                                     const std::string &s3script,
                                     DataMgrIf *dataMgrIf)
: ArchiveClient(dataMgrIf) 
{
    s3script_ = s3script;
    s3client_ = new S3Client(host, port, authEp, user, passwd);
}

BotoArchiveClient::~BotoArchiveClient()
{
    delete s3client_;
}

Error BotoArchiveClient::connect()
{
    return s3client_->authenticate();
}

Error BotoArchiveClient::putSnap(const fds_volid_t &volId,
                                 const int64_t &snapId)
{
    // TODO(Rao:)
    // 1. Clean up the tar file and snap after upload is done
    Error e;
    std::string snapName;
    std::string snapDirPath;
    std::string snapPath;
    populateSnapInfo_(volId, snapId,
                      snapName, snapDirPath, snapPath);

    e = tarSnap_(volId, snapId, snapDirPath, snapName);
    if (e != ERR_OK) {
        GLOGERROR << "Failed to tar snap.  volid: " << volId
            << " snapid: " << snapId;
        return e;
    }

    std::string sysVolName = dataMgrIf_->getSysVolumeName(volId);
    e = putFile(sysVolName, snapName, snapPath);
    return e;

}

Error BotoArchiveClient::getSnap(const fds_volid_t &volId,
                                const int64_t &snapId)
{
    Error e;
    std::string snapName;
    std::string snapDirPath;
    std::string snapPath;

    populateSnapInfo_(volId, snapId,
                      snapName, snapDirPath, snapPath);

    /* Download the snap */
    std::string sysVolName = dataMgrIf_->getSysVolumeName(volId);
    e = getFile(sysVolName, snapName, snapPath);
    if (e != ERR_OK) {
        GLOGERROR << "Failed to download snap.  volid: " << volId
            << " snapid: " << snapId;
        return ERR_ARCHIVE_GET_FAILED;
    }

    e = untarSnap_(volId, snapId, snapDirPath, snapName);
    return e;
}

Error BotoArchiveClient::getFile(const std::string &bucketName,
                                 const std::string &objName,
                                 const std::string &filePath)
{
    if (!s3client_->hasAccessKey()) {
        GLOGWARN << "Don't have access key";
        return ERR_UNAUTH_ACCESS;
    }

    std::stringstream ss;
    ss << s3script_ << " get" << " "
        << " admin" << " "
        << s3client_->getAccessKey() << " "
        << s3client_->getHost() << " "
        << s3client_->getPort() << " "
        << bucketName << " "
        << objName << " "
        << filePath;
    LOGDEBUG << "cmd: " << ss.str();

    int retCode = std::system(ss.str().c_str());
    if (retCode != 0) {
        GLOGWARN << "Failed to get bucketName: " << bucketName
            << " objName: " << objName << " filePath: " << filePath;
        return ERR_ARCHIVE_GET_FAILED;
    }
    return ERR_OK;
}

Error BotoArchiveClient::putFile(const std::string &bucketName,
                                 const std::string &objName,
                                 const std::string &filePath)
{
    if (!s3client_->hasAccessKey()) {
        GLOGWARN << "Don't have access key";
        return ERR_UNAUTH_ACCESS;
    }

    std::stringstream ss;
    ss << s3script_ << " put" << " "
        << " admin" << " "
        << s3client_->getAccessKey() << " "
        << s3client_->getHost() << " "
        << s3client_->getPort() << " "
        << bucketName << " "
        << objName << " "
        << filePath;
    LOGDEBUG << "cmd: " << ss.str();

    int retCode = std::system(ss.str().c_str());
    if (retCode != 0) {
        GLOGWARN << "Failed to put bucketName: " << bucketName
            << " objName: " << objName << " filePath: " << filePath;
        return ERR_ARCHIVE_PUT_FAILED;
    }
    return ERR_OK;
}

}  // namespace fds

