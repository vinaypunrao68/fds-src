/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_ARCHIVE_CLIENT_H_
#define SOURCE_DATA_MGR_INCLUDE_ARCHIVE_CLIENT_H_

#include <boost/shared_ptr.hpp>
#include <fds_types.h>
#include <concurrency/fds_actor.h>
#include <S3Client.h>

namespace fds {

typedef std::function<void (const Error &err)> ArchivePutCb;
typedef std::function<void (const Error &err)> ArchiveGetCb;
struct DataMgrIf;

/**
* @brief Get request message
*/
struct ArchiveClPutReq {
    ArchiveClPutReq(fds_volid_t volId,
                    int64_t snapId,
                    ArchivePutCb cb)
    {
        this->volId = volId;
        this->snapId = snapId;
        this->cb = cb;
    }
    fds_volid_t volId;
    int64_t snapId;
    ArchivePutCb cb;
};
typedef boost::shared_ptr<ArchiveClPutReq> ArchiveClPutReqPtr;

/**
* @brief Put request message
*/
struct ArchiveClGetReq {
    ArchiveClGetReq(fds_volid_t volId,
                    int64_t snapId,
                    ArchiveGetCb cb)
    {
        this->volId = volId;
        this->snapId = snapId;
        this->cb = cb;
    }
    fds_volid_t volId;
    int64_t snapId;
    ArchiveGetCb cb;
};
typedef boost::shared_ptr<ArchiveClGetReq> ArchiveClGetReqPtr;

/**
* @brief 
*/
struct ArchiveClient : FdsRequestQueueActor
{
    ArchiveClient(DataMgrIf* dataMgrIf,
                  fds_threadpoolPtr threadpool);
    void connect(const std::string &host,
                 const std::string &accessKey,
                 const std::string &secretKey);
    void putSnap(const fds_volid_t &volId,
                 const int64_t &snapId,
                 ArchivePutCb cb);
    Error putSnapSync(const fds_volid_t &volId,
                 const int64_t &snapId);
    void getSnap(const fds_volid_t &volId,
                 const int64_t &snapId,
                 ArchiveGetCb cb);
    Error getSnapSync(const fds_volid_t &volId,
                 const int64_t &snapId);

    virtual Error handle_actor_request(FdsActorRequestPtr req) override;

 protected:
    void handlePutSnap_(ArchiveClPutReqPtr &putPayload);
    void handleGetSnap_(ArchiveClGetReqPtr &getPayload);

#if 0
    std::string getSysVolumeName_(const fds_volid_t &volId);
    std::string getSnapDirName_(const fds_volid_t &volId, const int64_t snapId);
#endif

    std::unique_ptr<S3Client> s3client_;
    std::string snapDirBase_;
    DataMgrIf *dataMgrIf_;
};
typedef boost::shared_ptr<ArchiveClient> ArchiveClientPtr;

}  // namespace fds
#endif
