/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_NBD_NBDTASK_H_
#define SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_NBD_NBDTASK_H_

#include <atomic>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "fds_error.h"
#include "fds_assert.h"

namespace fds
{

struct NbdTask {
    using buffer_type = std::string;
    using buffer_ptr_type = boost::shared_ptr<buffer_type>;
    enum NbdOperation {
        OTHER = 0,
        READ = 1,
        WRITE = 2
    };

    explicit NbdTask(int64_t hdl) :
        handle(hdl)
    {
        bufVec.reserve(objCount);
        offVec.reserve(objCount);
    }

    ~NbdTask() {}
    typedef boost::shared_ptr<NbdTask> shared_ptr;

    bool isRead() const { return (operation == READ); }
    bool isWrite() const { return (operation == WRITE); }
    inline int64_t getHandle() const { return handle; }
    inline Error getError() const { return opError; }
    inline uint64_t getOffset() const { return offset; }
    inline uint32_t getLength() const { return length; }
    inline uint32_t maxObjectSize() const { return maxObjectSizeInBytes; }
    inline void setObjectCount(size_t const count) {
        objCount = count;
        bufVec.reserve(count);
        offVec.reserve(count);
    }

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

    void setMaxObjectSize(uint32_t const size) { maxObjectSizeInBytes = size; };

    buffer_ptr_type getNextReadBuffer(uint32_t& context) {
        if (context >= bufVec.size()) {
            return nullptr;
        }
        return bufVec[context++];
    }

    void keepBufferForWrite(uint32_t const seqId,
                            uint64_t const objectOff,
                            buffer_ptr_type& buf) {
        fds_assert(WRITE == operation);
        bufVec.emplace_back(buf);
        offVec.emplace_back(objectOff);
    }

    uint64_t getOffset(uint32_t const seqId) const {
        return offVec[seqId];
    }

    buffer_ptr_type getBuffer(uint32_t const seqId) const {
        return bufVec[seqId];
    }

    /**
     * \return true if all responses were received or operation error
     */
    void handleReadResponse(std::vector<buffer_ptr_type>& buffers,
                            uint32_t len);

    /**
     * \return true if all responses were received
     */
    fds_bool_t handleWriteResponse(const Error& err) {
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
    std::pair<Error, buffer_ptr_type>
        handleRMWResponse(buffer_ptr_type const& retBuf,
                          uint32_t len,
                          uint32_t seqId,
                          const Error& err);

    int64_t handle;

    // These are the responses we are also in charge of responding to, in order
    // with ourselves being last.
    std::unordered_map<uint32_t, std::deque<NbdTask*>> chained_responses;

  private:
    NbdOperation operation {OTHER};
    std::atomic_uint doneCount {0};
    uint32_t objCount {1};

    // error of the operation
    Error opError {ERR_OK};

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

#endif  // SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_NBD_NBDTASK_H_
