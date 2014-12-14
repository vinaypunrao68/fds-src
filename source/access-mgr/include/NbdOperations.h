/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_

#include <vector>
#include <string>
#include <map>
#include <fds_types.h>
#include <apis/apis_types.h>
#include <concurrency/Mutex.h>
#include <AmAsyncResponseApi.h>
#include <AmAsyncDataApi.h>

namespace fds {

class NbdResponseVector {
  public:
    enum NbdOperation {
        READ = 0,
        WRITE = 1
    };

    explicit NbdResponseVector(fds_int64_t hdl, NbdOperation op, fds_uint32_t objCnt)
            : handle(hdl), operation(op), objCount(objCnt) {
        doneCount = ATOMIC_VAR_INIT(0);
        if (op == READ) {
            bufVec.resize(objCnt, NULL);
        }
    }
    ~NbdResponseVector() {}
    typedef boost::shared_ptr<NbdResponseVector> shared_ptr;

    fds_bool_t isReady() const {
        fds_uint32_t doneCnt = atomic_load(&doneCount);
        return (doneCnt == objCount);
    }
    fds_bool_t isRead() const { return (operation == READ); }
    inline fds_int64_t getHandle() const { return handle; }
    boost::shared_ptr<std::string> getNextReadBuffer(fds_uint32_t& context) {
        if (context >= objCount) {
            return NULL;
        }
        return bufVec[context++];
    }

    /**
     * \return true if all responses were received
     */
    fds_bool_t handleReadResponse(boost::shared_ptr<std::string> retBuf,
                                  fds_uint32_t length,
                                  fds_uint32_t seqId) {
        fds_verify(operation == READ);
        fds_verify(seqId < bufVec.size());
        bufVec[seqId] = retBuf;
        fds_uint32_t doneCnt = atomic_fetch_add(&doneCount, (fds_uint32_t)1);
        return ((doneCnt + 1) == objCount);
    }

    /**
     * \return true if all responses were received
     */
    fds_bool_t handleWriteResponse(fds_uint32_t seqId) {
        fds_verify(operation == WRITE);
        fds_uint32_t doneCnt = atomic_fetch_add(&doneCount, (fds_uint32_t)1);
        return ((doneCnt + 1) == objCount);
    }

  private:
    fds_int64_t handle;
    NbdOperation operation;
    std::atomic<fds_uint32_t> doneCount;
    fds_uint32_t objCount;

    // to collect read responses
    std::vector<boost::shared_ptr<std::string>> bufVec;

    // write op info
    boost::shared_ptr<std::string> bytes;
    // seqId is 0
    boost::shared_ptr<apis::ObjectOffset> firstOff;
    // seqId is objCount - 1
    boost::shared_ptr<apis::ObjectOffset> lastOff;
};

// Response interface for NbdOperations
class NbdOperationsResponseIface {
  public:
    virtual ~NbdOperationsResponseIface() {}

    virtual void readWriteResp(const Error& error,
                               fds_int64_t handle,
                               NbdResponseVector* response) = 0;
};

class NbdOperations : public AmAsyncResponseApi {
  public:
    explicit NbdOperations(NbdOperationsResponseIface* respIface);
    ~NbdOperations();
    typedef boost::shared_ptr<NbdOperations> shared_ptr;

    void read(boost::shared_ptr<std::string>& volumeName,
              fds_uint32_t maxObjectSizeInBytes,
              fds_uint32_t length,
              fds_uint64_t offset,
              fds_int64_t handle);

    void write(boost::shared_ptr<std::string>& volumeName,
               fds_uint32_t maxObjectSizeInBytes,
               boost::shared_ptr<std::string>& bytes,
               fds_uint32_t length,
               fds_uint64_t offset,
               fds_int64_t handle);

    // AmAsyncResponseApi implementation
    void attachVolumeResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId) {}

    void startBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId,
                         boost::shared_ptr<apis::TxDescriptor>& txDesc) {}
    void abortBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId) {}
    void commitBlobTxResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId) {}

    void updateBlobResp(const Error &error,
                        boost::shared_ptr<apis::RequestId>& requestId) {}
    void updateBlobOnceResp(const Error &error,
                            boost::shared_ptr<apis::RequestId>& requestId);
    void updateMetadataResp(const Error &error,
                            boost::shared_ptr<apis::RequestId>& requestId) {}
    void deleteBlobResp(const Error &error,
                        boost::shared_ptr<apis::RequestId>& requestId) {}

    void statBlobResp(const Error &error,
                      boost::shared_ptr<apis::RequestId>& requestId,
                      boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {}
    void volumeStatusResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId,
                          boost::shared_ptr<apis::VolumeStatus>& volumeStatus) {}
    void volumeContentsResp(
        const Error &error,
        boost::shared_ptr<apis::RequestId>& requestId,
        boost::shared_ptr<std::vector<apis::BlobDescriptor>>& volContents) {}

    void getBlobResp(const Error &error,
                     boost::shared_ptr<apis::RequestId>& requestId,
                     boost::shared_ptr<std::string> buf,
                     fds_uint32_t& length);
    void getBlobWithMetaResp(const Error &error,
                             boost::shared_ptr<apis::RequestId>& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length,
                             boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {}

  private:
    void parseRequestId(boost::shared_ptr<apis::RequestId>& requestId,
                        fds_int64_t* handle,
                        fds_int32_t* seqId);

    // api we've built
    AmAsyncDataApi::shared_ptr amAsyncDataApi;

    // interface to respond to nbd passed down in constructor
    NbdOperationsResponseIface* nbdResp;

    // for all reads/writes to AM
    boost::shared_ptr<std::string> blobName;
    boost::shared_ptr<std::string> domainName;
    boost::shared_ptr<fds_int32_t> blobMode;
    boost::shared_ptr< std::map<std::string, std::string> > emptyMeta;

    // for now we are supporting <=4K requests
    // so keep current handles for which we are waiting responses
    std::map<fds_int64_t, NbdResponseVector*> responses;
    fds_mutex respLock;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_
