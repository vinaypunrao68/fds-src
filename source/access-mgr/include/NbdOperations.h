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
#include <AmAsyncResponseApi.h>
#include <AmAsyncDataApi.h>

namespace fds {

class ReadRespVector {
  public:
    explicit ReadRespVector(fds_uint32_t objCnt) : doneCount(0), objCount(objCnt) {
        bufVec.resize(objCnt, NULL);
    }
    ~ReadRespVector() {}
    typedef boost::shared_ptr<ReadRespVector> shared_ptr;

    fds_bool_t isReady() const { return (doneCount == objCount); }
    boost::shared_ptr<std::string> getNextBuffer(fds_uint32_t& context) {
        if (context >= objCount) {
            return NULL;
        }
        return bufVec[context++];
    }

    /**
     * \return true if all responses were received
     */
    fds_bool_t handleResponse(char* retBuf,
                              fds_uint32_t length,
                              fds_uint32_t seqId) {
        fds_verify(seqId < bufVec.size());
        // copy buf to shared pointer if it's valid, leave
        // empty in case of error
        if (NULL != retBuf) {
            boost::shared_ptr<std::string> buf(new std::string(retBuf, length));
            bufVec[seqId] = buf;
        }
        ++doneCount;
        return (doneCount == objCount);
    }

  private:
    fds_uint32_t doneCount;
    fds_uint32_t objCount;
    std::vector<boost::shared_ptr<std::string>> bufVec;
};

class WriteResponseVector {
  public:
    explicit WriteResponseVector(fds_uint32_t objCnt) : remainCount(objCnt) {
    }
    ~WriteResponseVector() {}

  private:
    fds_uint32_t remainCount;

    boost::shared_ptr<std::string> devName;
    boost::shared_ptr<std::string> volumeName;
    boost::shared_ptr<std::string> bytes;

    // map of sequence id to object offset
    // for the objects that we read-modify-write
    std::map<fds_int32_t, boost::shared_ptr<apis::ObjectOffset>> readOff;
};

// Response interface for NbdOperations
class NbdOperationsResponseIface {
  public:
    virtual ~NbdOperationsResponseIface() {}

    virtual void readResp(const Error& error,
                          fds_int64_t handle,
                          ReadRespVector::shared_ptr response) = 0;
};

class NbdOperations : public AmAsyncResponseApi {
  public:
    explicit NbdOperations(AmAsyncDataApi::shared_ptr& amApi,
                           NbdOperationsResponseIface* respIface);
    ~NbdOperations();
    typedef boost::shared_ptr<NbdOperations> shared_ptr;

    void read(boost::shared_ptr<std::string>& volumeName,
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
                            boost::shared_ptr<apis::RequestId>& requestId) {}
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
                     char* buf,
                     fds_uint32_t& length);
    void getBlobWithMetaResp(const Error &error,
                             boost::shared_ptr<apis::RequestId>& requestId,
                             char* buf,
                             fds_uint32_t& length,
                             boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {}

  private:
    void parseRequestId(boost::shared_ptr<apis::RequestId>& requestId,
                        fds_int64_t* handle,
                        fds_int32_t* seqId);

    // api passed down
    AmAsyncDataApi::shared_ptr amAsyncDataApi;

    // async response handler
    boost::shared_ptr<NbdOperations> responseApi;

    // interface to respond to nbd passed down in constructor
    NbdOperationsResponseIface* nbdResp;

    // for now we are supporting <=4K requests
    // so keep current handles for which we are waiting responses
    std::map<fds_int64_t, ReadRespVector::shared_ptr> readResponses;
    std::map<fds_int64_t, fds_uint32_t> writeResponses;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_
