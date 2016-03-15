/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_BLOCKTASK_H_
#define SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_BLOCKTASK_H_

#include <string>
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

    explicit BlockTask(uint64_t const hdl);
    virtual ~BlockTask() = default;

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

    /// Task setters
    void setError(fpi::ErrorCode const& error) { opError = error; }

    /// Buffer operations
    buffer_ptr_type getNextReadBuffer(uint32_t& context) {
        if (context >= bufVec.size()) {
            return nullptr;
        }
        return bufVec[context++];
    }

    /**
     * Split and trim returned buffers for actual requested data
     */
    void handleReadResponse(std::vector<buffer_ptr_type>& buffers,
                            buffer_ptr_type& empty_buffer,
                            uint32_t len,
                            uint32_t const maxObjectSizeInBytes);

    int64_t handle;

  private:
    BlockOp operation {OTHER};

    // error of the operation
    fpi::ErrorCode opError {fpi::OK};

    // to collect read responses or first and last buffer for write op
    std::vector<buffer_ptr_type> bufVec;

    // offset
    uint64_t offset {0};
    uint32_t length {0};
};


}  // namespace fds

#endif  // SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_BLOCKTASK_H_
