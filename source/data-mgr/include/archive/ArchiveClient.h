/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_ARCHIVE_CLIENT_H_
#define SOURCE_DATA_MGR_INCLUDE_ARCHIVE_CLIENT_H_

#include <boost/shared_ptr.hpp>
#include <fds_types.h>

namespace fds {

/* Forward declarations */
struct S3Client;

/**
* @brief Base interface for archiving snapshots.  For now the functionality is very limited.
* We only support uploading and download snapshot.
*/
struct ArchiveClient
{
    explicit ArchiveClient(DataMgrIf* dataMgrIf);
    virtual ~ArchiveClient() {}

    virtual Error connect() = 0;

    virtual Error putSnap(const fds_volid_t &volId,
                 const fds_volid_t &snapId) = 0;
    virtual Error getSnap(const fds_volid_t &volId,
                 const fds_volid_t &snapId) = 0;
    virtual Error getFile(const std::string &bucketName,
                          const std::string &objName,
                          const std::string &filePath)  = 0;
    virtual Error putFile(const std::string &bucketName,
                          const std::string &objName,
                          const std::string &filePath)  = 0;

 protected:
    void populateSnapInfo_(const fds_volid_t &volId, const fds_volid_t &snapId,
                           std::string &snapName,
                           std::string &snapDirPath,
                           std::string &snapPath);
    Error tarSnap_(fds_volid_t volId, fds_volid_t snapId,
                 const std::string &snapDirPath,
                 const std::string &snapName);
    Error untarSnap_(fds_volid_t volId, fds_volid_t snapId,
                    const std::string &snapDirPath,
                    const std::string &snapName);

    std::string snapDirBase_;
    DataMgrIf *dataMgrIf_;
};
typedef boost::shared_ptr<ArchiveClient> ArchiveClientPtr;

/**
* @brief Boto based implementation of archive client
*/
struct BotoArchiveClient : ArchiveClient
{
    BotoArchiveClient(const std::string &host,
             int port,
             const std::string &authEp,
             const std::string &user,
             const std::string &passwd,
             const std::string &s3script,
             DataMgrIf *dataMgrIf);
    ~BotoArchiveClient();

    virtual Error connect() override;

    virtual Error putSnap(const fds_volid_t &volId,
                         const fds_volid_t &snapId) override;
    virtual Error getSnap(const fds_volid_t &volId,
                         const fds_volid_t &snapId) override;
    virtual Error getFile(const std::string &bucketName,
                          const std::string &objName,
                          const std::string &filePath) override;
    virtual Error putFile(const std::string &bucketName,
                          const std::string &objName,
                          const std::string &filePath) override;
 protected:
    /* We use s3client just for getting access token.  Ideally we would use S3Client for puts
     * and gets as well.  However authentication is incomplete in S3Client
     */
    S3Client *s3client_;
    /* We use s3script (python script) for doing puts/gets */
    std::string s3script_;
};

}  // namespace fds
#endif
