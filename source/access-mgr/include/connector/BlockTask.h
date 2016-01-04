/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_BLOCKTASK_H_
#define SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_BLOCKTASK_H_

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/shared_ptr.hpp>
#include "fdsp/common_types.h"

namespace fds
{

namespace fpi = FDS_ProtocolInterface;

/**
 * A BlockTask represents a single READ/WRITE operation from a storage
 * interface to a block device. The operation may encompass less than or
 * greater than a single object. This class helps deal with the buffers and
 * offset/length calculations needed during asynchronous i/o
 */
struct BlockTask {
    using buffer_type = std::string;
    using buffer_ptr_type = boost::shared_ptr<buffer_type>;

    /// What type of task is this
    enum BlockOp {
        OTHER = 0,
        READ = 1,
        WRITE = 2
    };

    explicit BlockTask(uint64_t hdl) :
        handle(hdl)
    {
        bufVec.reserve(objCount);
        offVec.reserve(objCount);
    }

    ~BlockTask() {}

    /// Setup task params
    void setRead(uint64_t const off, uint32_t const bytes) {
        operation = READ;
        offset = off;
        length = bytes;
    }

    void setWrite(uint64_t const off, uint32_t const bytes) {
        operation = WRITE;
        offset = off;
        length = bytes;
    }

    /// Task getters
    bool isRead() const         { return (operation == READ); }
    bool isWrite() const        { return (operation == WRITE); }
    uint64_t getHandle() const  { return handle; }
    uint64_t getOffset() const  { return offset; }
    uint32_t getLength() const  { return length; }
    fpi::ErrorCode getError() const      { return opError; }
    uint32_t maxObjectSize() const { return maxObjectSizeInBytes; }

    /// Task setters
    void setError(fpi::ErrorCode const& error) { opError = error; }
    void setObjectCount(size_t const count) {
        objCount = count;
        bufVec.reserve(count);
        offVec.reserve(count);
    }
    void setMaxObjectSize(uint32_t const size) { maxObjectSizeInBytes = size; };

    /// Sub-task operations
    uint64_t getOffset(uint32_t const seqId) const          { return offVec[seqId]; }
    buffer_ptr_type getBuffer(uint32_t const seqId) const   { return bufVec[seqId]; }

    /// Buffer operations
    buffer_ptr_type getNextReadBuffer(uint32_t& context) {
        if (context >= bufVec.size()) {
            return nullptr;
        }
        return bufVec[context++];
    }

    void keepBufferForWrite(uint32_t const seqId,
                            uint64_t const objectOff,
                            buffer_ptr_type& buf) {
        bufVec.emplace_back(buf);
        offVec.emplace_back(objectOff);
    }

    /**
     * \return true if all responses were received or operation error
     */
    void handleReadResponse(std::vector<buffer_ptr_type>& buffers, uint32_t len);

    /**
     * \return true if all responses were received
     */
    bool handleWriteResponse(const fpi::ErrorCode& err) {
        if (fpi::OK != err) {
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
    std::pair<fpi::ErrorCode, buffer_ptr_type>
        handleRMWResponse(buffer_ptr_type const& retBuf,
                          uint32_t len,
                          uint32_t seqId,
                          const fpi::ErrorCode& err);

    void getChain(uint32_t const seqId, std::deque<BlockTask*>& chain);
    void setChain(uint32_t const seqId, std::deque<BlockTask*>&& chain);

    int64_t handle;

  private:
    BlockOp operation {OTHER};
    std::atomic_uint doneCount {0};
    uint32_t objCount {1};

    // These are the responses we are also in charge of responding to, in order
    // with ourselves being last.
    std::unordered_map<uint32_t, std::deque<BlockTask*>> chained_responses;
    std::mutex chain_lock;

    // error of the operation
    fpi::ErrorCode opError {fpi::OK};

    // to collect read responses or first and last buffer for write op
    std::vector<buffer_ptr_type> bufVec;
    // when writing, we need to remember the object offsets for rwm buffers
    std::vector<uint64_t> offVec;

    // offset
    uint64_t offset {0};
    uint32_t length {0};
    uint32_t maxObjectSizeInBytes {0};
};


}  // namespace fds

#endif  // SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_BLOCKTASK_H_
