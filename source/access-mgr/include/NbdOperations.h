/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_

#include <vector>
#include <string>
#include <map>
#include <utility>

#include <boost/enable_shared_from_this.hpp>

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

    explicit NbdResponseVector(fds_int64_t hdl, NbdOperation op,
                               fds_uint64_t off, fds_uint32_t len, fds_uint32_t maxOSize,
                               fds_uint32_t objCnt)
      : handle(hdl), operation(op), offset(off), length(len),
        maxObjectSizeInBytes(maxOSize), objCount(objCnt), opError(ERR_OK) {
        doneCount = ATOMIC_VAR_INIT(0);
        if (op == READ) {
            bufVec.resize(objCnt, NULL);
        } else {
            bufVec.resize(2, NULL);
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
    inline Error getError() const { return opError; }
    inline fds_uint64_t getOffset() const { return offset; }
    inline fds_uint32_t getLength() const { return length; }
    inline fds_uint32_t maxObjectSize() const { return maxObjectSizeInBytes; }
    boost::shared_ptr<std::string> getNextReadBuffer(fds_uint32_t& context) {
        if (context >= objCount) {
            return NULL;
        }
        return bufVec[context++];
    }

    void keepBufferForWrite(fds_uint32_t seqId,
                            fds_uint32_t objectOff,
                            boost::shared_ptr<std::string> buf) {
        fds_verify(operation == WRITE);
        fds_verify((seqId == 0) || (seqId == (objCount - 1)));
        if (seqId == 0) {
            bufVec[0] = buf;
            offVec[0] = objectOff;
        } else {
            bufVec[1] = buf;
            offVec[1] = objectOff;
        }
    }


    /**
     * \return true if all responses were received or operation error
     */
    fds_bool_t handleReadResponse(boost::shared_ptr<std::string> retBuf,
                                  fds_uint32_t len,
                                  fds_uint32_t seqId,
                                  const Error& err);

    /**
     * \return true if all responses were received
     */
    fds_bool_t handleWriteResponse(fds_uint32_t seqId, const Error& err) {
        fds_verify(operation == WRITE);
        if (!err.ok()) {
            opError = err;
            return true;
        }
        fds_uint32_t doneCnt = atomic_fetch_add(&doneCount, (fds_uint32_t)1);
        return ((doneCnt + 1) == objCount);
    }

    /**
     * Handle read response for read-modify-write
     * \return true if all responses were received or operation error
     */
    std::pair<fds_uint32_t, boost::shared_ptr<std::string>>
        handleRMWResponse(boost::shared_ptr<std::string> retBuf,
                          fds_uint32_t len,
                          fds_uint32_t seqId,
                          const Error& err) {
        fds_verify(operation == WRITE);
        if (!err.ok() && (err != ERR_BLOB_OFFSET_INVALID) &&
                         (err != ERR_BLOB_NOT_FOUND)) {
            opError = err;
        } else {
            fds_uint32_t iOff = (seqId == 0) ? offset % maxObjectSizeInBytes : 0;
            fds_uint32_t index = (seqId == 0) ? 0 : 1;
            boost::shared_ptr<std::string> writeBytes = bufVec[index];

            boost::shared_ptr<std::string> fauxBytes;
            if ((err == ERR_BLOB_OFFSET_INVALID) ||
                (err == ERR_BLOB_NOT_FOUND)) {
                // we tried to read unwritten block, so create
                // an empty block buffer to place the data
                fauxBytes = boost::make_shared<std::string>(maxObjectSizeInBytes, 0);
                fauxBytes->replace(iOff, writeBytes->length(),
                                   writeBytes->c_str(), writeBytes->length());
            } else {
                fds_verify(len == maxObjectSizeInBytes);
                // Need to copy retBut into a modifiable buffer since retBuf is owned
                // by AM and should not be modified here.
                // TODO(Andrew): Make retBuf a const
                fauxBytes = boost::make_shared<std::string>(retBuf->c_str(), retBuf->length());
                fauxBytes->replace(iOff, writeBytes->length(),
                                   writeBytes->c_str(), writeBytes->length());
            }
            return std::make_pair(offVec[index], fauxBytes);
        }
        return std::make_pair(0, boost::shared_ptr<std::string>());
    }


    void setError(const Error& err) { opError = err; }
    fds_int64_t handle;

  private:
    NbdOperation operation;
    std::atomic<fds_uint32_t> doneCount;
    fds_uint32_t objCount;

    // error of the operation
    Error opError;

    // to collect read responses or first and last buffer for write op
    std::vector<boost::shared_ptr<std::string>> bufVec;
    // when writing, we need to remember the object offsets for rwm buffers
    std::array<fds_uint32_t, 2> offVec;

    // offset
    fds_uint64_t offset;
    fds_uint32_t length;
    fds_uint32_t maxObjectSizeInBytes;
};

// Response interface for NbdOperations
class NbdOperationsResponseIface {
  public:
    virtual ~NbdOperationsResponseIface() {}

    virtual void readWriteResp(NbdResponseVector* response) = 0;
};

class NbdOperations
    :   public boost::enable_shared_from_this<NbdOperations>,
        public AmAsyncResponseApi
{
  public:
    explicit NbdOperations(NbdOperationsResponseIface* respIface);
    ~NbdOperations();
    typedef boost::shared_ptr<NbdOperations> shared_ptr;
    void init(boost::shared_ptr<std::string> vol_name, fds_uint32_t _maxObjectSizeInBytes);

    void read(fds_uint32_t length,
              fds_uint64_t offset,
              fds_int64_t handle);

    void write(boost::shared_ptr<std::string>& bytes,
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
                        boost::shared_ptr<apis::RequestId>& requestId);
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
                     boost::shared_ptr<std::string> buf,
                     fds_uint32_t& length);
    void getBlobWithMetaResp(const Error &error,
                             boost::shared_ptr<apis::RequestId>& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length,
                             boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {}

    void shutdown()
    { amAsyncDataApi.reset(); }

  private:
    void parseRequestId(boost::shared_ptr<apis::RequestId>& requestId,
                        fds_int64_t* handle,
                        fds_int32_t* seqId);
    fds_uint32_t getObjectCount(fds_uint32_t length,
                                fds_uint64_t offset);

    // api we've built
    std::unique_ptr<AmAsyncDataApi> amAsyncDataApi;
    boost::shared_ptr<std::string> volumeName;
    fds_uint32_t maxObjectSizeInBytes;

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
