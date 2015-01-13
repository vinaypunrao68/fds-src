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
#include <boost/lockfree/queue.hpp>

#include <fds_types.h>
#include <apis/apis_types.h>
#include <concurrency/Mutex.h>
#include <AmAsyncResponseApi.h>
#include <AmAsyncDataApi.h>

namespace fds {

// This class offers a way to "lock" a sector of the blob
// and queue operations modifying the same offset to maintain
// consistency for < maxObjectSize writes.
template <typename E, size_t N>
struct SectorLockMap {
    typedef E entry_type;
    static constexpr size_t size = N;
    typedef fds::fds_rwlock lock_type;
    typedef int64_t key_type;
    typedef boost::lockfree::queue<entry_type, boost::lockfree::capacity<size>> queue_type;  // NOLINT
    typedef std::unordered_map<key_type, std::unique_ptr<queue_type>> map_type;
    typedef typename map_type::iterator map_it;

    enum class QueueResult { FirstEntry, AddedEntry, Failure };

    SectorLockMap() :
        map_lock(), sector_map()
    {}
    ~SectorLockMap() {}

    QueueResult queue_rmw(key_type const& k, entry_type e) {
        QueueResult result = QueueResult::Failure;
        map_lock.cond_write_lock();
        map_it it = sector_map.find(k);
        if (sector_map.end() != it) {
            if ((*it).second->push(e)) result = QueueResult::AddedEntry;
            map_lock.cond_write_unlock();
        } else {
            map_lock.upgrade();
            auto r = sector_map.insert(
                std::make_pair(k, std::move(std::unique_ptr<queue_type>(new queue_type()))));
            fds_assert(r.second);
            result = QueueResult::FirstEntry;
            map_lock.write_unlock();
        }
        return result;
    }

    std::pair<bool, entry_type> pop_and_delete(key_type const& k)
    {
        static std::pair<bool, entry_type> const no = {false, entry_type()};
        map_lock.cond_write_lock();
        map_it it = sector_map.find(k);
        if (sector_map.end() != it) {
            entry_type entry;
            if ((*it).second->pop(entry)) {
                map_lock.cond_write_unlock();
                return std::make_pair(true, entry);
            } else {
                map_lock.upgrade();
                // No more queued requests, return nullptr
                sector_map.erase(it);
                map_lock.write_unlock();
            }
        } else {
            fds_assert(false);  // This shouldn't happen, let's know in debug
            map_lock.cond_write_unlock();
        }
        return no;
    }

 private:
    explicit SectorLockMap(SectorLockMap const& rhs) = delete;  // Non-copyable
    SectorLockMap& operator=(SectorLockMap const& rhs) = delete;  // Non-assignable

    lock_type map_lock;
    map_type sector_map;
};

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
                            fds_uint64_t objectOff,
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

    std::pair<bool, fds_uint64_t> wasRMW(fds_uint32_t seqId) {
        static std::pair<bool, fds_uint64_t> const no = {false, 0};
        if (seqId == 0 && bufVec[0]) {
            return std::make_pair(true, offVec[0]);
        } else if (seqId == (objCount - 1) && bufVec[1]) {
            return std::make_pair(true, offVec[1]);
        }
        return no;
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
    std::pair<fds_uint64_t, boost::shared_ptr<std::string>>
        handleRMWResponse(boost::shared_ptr<std::string> retBuf,
                          fds_uint32_t len,
                          fds_uint32_t seqId,
                          const Error& err);

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
    std::array<fds_uint64_t, 2> offVec;

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

struct HandleSeqPair {
    fds_int64_t handle;
    fds_int32_t seq;
};

class NbdOperations
    :   public boost::enable_shared_from_this<NbdOperations>,
        public AmAsyncResponseApi<HandleSeqPair>
{
    typedef HandleSeqPair handle_type;
    typedef SectorLockMap<handle_type, 1024> sector_type;
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
    void attachVolumeResp(const Error &error, handle_type& requestId) {}

    void startBlobTxResp(const Error &error,
                         handle_type& requestId,
                         boost::shared_ptr<apis::TxDescriptor>& txDesc) {}
    void abortBlobTxResp(const Error &error, handle_type& requestId) {}
    void commitBlobTxResp(const Error &error, handle_type& requestId) {}

    void updateBlobResp(const Error &error, handle_type& requestId);
    void updateBlobOnceResp(const Error &error, handle_type& requestId) {}
    void updateMetadataResp(const Error &error, handle_type& requestId) {}
    void deleteBlobResp(const Error &error, handle_type& requestId) {}

    void statBlobResp(const Error &error,
                      handle_type& requestId,
                      boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {}
    void volumeStatusResp(const Error &error,
                          handle_type& requestId,
                          boost::shared_ptr<apis::VolumeStatus>& volumeStatus) {}
    void volumeContentsResp(const Error &error,
                            handle_type& requestId,
                            boost::shared_ptr<std::vector<apis::BlobDescriptor>>& volContents) {}

    void getBlobResp(const Error &error,
                     handle_type& requestId,
                     boost::shared_ptr<std::string> buf,
                     fds_uint32_t& length);
    void getBlobWithMetaResp(const Error &error,
                             handle_type& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length,
                             boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {}

    void shutdown()
    { amAsyncDataApi.reset(); }

  private:
    fds_uint32_t getObjectCount(fds_uint32_t length,
                                fds_uint64_t offset);

    // api we've built
    std::unique_ptr<AmAsyncDataApi<handle_type>> amAsyncDataApi;
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

    sector_type sector_map;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_
