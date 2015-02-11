/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_

#include <vector>
#include <string>
#include <map>
#include <utility>
#include <unordered_map>

#include <boost/enable_shared_from_this.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "fds_types.h"
#include "concurrency/RwLock.h"
#include "apis/apis_types.h"
#include "concurrency/Mutex.h"
#include "AmAsyncResponseApi.h"
#include "AmAsyncDataApi.h"

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
    typedef boost::lockfree::spsc_queue<entry_type, boost::lockfree::capacity<size>> queue_type;  // NOLINT
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

    explicit NbdResponseVector(int64_t hdl, NbdOperation op,
                               uint64_t off, uint32_t len, uint32_t maxOSize,
                               uint32_t objCnt)
      : handle(hdl), operation(op), doneCount(), offset(off), length(len),
        maxObjectSizeInBytes(maxOSize), objCount(objCnt), opError(ERR_OK) {
        if (op == READ) {
            bufVec.resize(objCnt, NULL);
        } else {
            bufVec.resize(2, NULL);
        }
    }
    ~NbdResponseVector() {}
    typedef boost::shared_ptr<NbdResponseVector> shared_ptr;

    fds_bool_t isReady() const {
        fds_uint32_t doneCnt = doneCount.load();
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
            // Note, we're always setting the most recent
            // responses's error code.
            opError = err;
        }
        uint32_t doneCnt = doneCount.fetch_add(1);
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
    std::atomic_uint doneCount;
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
    int64_t handle;
    uint32_t seq;
};

class NbdOperations
    :   public boost::enable_shared_from_this<NbdOperations>,
        public AmAsyncResponseApi<HandleSeqPair>
{
    using req_api_type = AmAsyncDataApi<HandleSeqPair>;
    using resp_api_type = AmAsyncResponseApi<HandleSeqPair>;

    typedef resp_api_type::handle_type handle_type;
    typedef SectorLockMap<handle_type, 1024> sector_type;
    typedef std::unordered_map<fds_int64_t, NbdResponseVector*> response_map_type;
  public:
    explicit NbdOperations(NbdOperationsResponseIface* respIface);
    ~NbdOperations();
    typedef boost::shared_ptr<NbdOperations> shared_ptr;
    void init(req_api_type::shared_string_type vol_name, uint32_t _maxObjectSizeInBytes);

    void read(fds_uint32_t length, fds_uint64_t offset, fds_int64_t handle);

    void write(req_api_type::shared_buffer_type& bytes, fds_uint32_t length, fds_uint64_t offset, fds_int64_t handle);

    // The two response types we do support
    void getBlobResp(const resp_api_type::error_type &error,
                     handle_type& requestId,
                     resp_api_type::shared_buffer_type buf,
                     resp_api_type::size_type& length);

    void updateBlobResp(const resp_api_type::error_type &error, handle_type& requestId);

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
    response_map_type responses;
    fds_mutex respLock;

    sector_type sector_map;

    // AmAsyncResponseApi un-implemented responses
    void abortBlobTxResp       (const resp_api_type::error_type &, handle_type&) {}
    void attachVolumeResp      (const resp_api_type::error_type &, handle_type&) {}
    void commitBlobTxResp      (const resp_api_type::error_type &, handle_type&) {}
    void deleteBlobResp        (const resp_api_type::error_type &, handle_type&) {}
    void getBlobWithMetaResp   (const resp_api_type::error_type &, handle_type&, resp_api_type::shared_buffer_type, resp_api_type::size_type&, resp_api_type::shared_descriptor_type&) {}  // NOLINT
    void startBlobTxResp       (const resp_api_type::error_type &, handle_type&, resp_api_type::shared_tx_ctx_type&) {}  // NOLINT
    void statBlobResp          (const resp_api_type::error_type &, handle_type&, shared_descriptor_type&) {}  // NOLINT
    void updateBlobOnceResp    (const resp_api_type::error_type &, handle_type&) {}
    void updateMetadataResp    (const resp_api_type::error_type &, handle_type&) {}
    void volumeContentsResp    (const resp_api_type::error_type &, handle_type&, shared_descriptor_vec_type&) {}  // NOLINT
    void volumeStatusResp      (const resp_api_type::error_type &, handle_type&, shared_status_type&) {}  // NOLINT
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDOPERATIONS_H_
