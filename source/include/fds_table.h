/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_TABLE_H_
#define SOURCE_INCLUDE_FDS_TABLE_H_

extern "C" {
#include "math.h"
}
#include <atomic>

#include "shared/fds_types.h"
#include "serialize.h"

namespace fds
{

struct FDS_Table : public serialize::Serializable
{
    using callback_type = std::function<void(Error const&)>;

    FDS_Table(fds_uint32_t const _width,
              fds_uint32_t const _depth,
              fds_uint64_t const _version) :
        depth(_depth),
        width(_width),
        columns(pow(2, width)),
        version(_version)
    { }
    virtual ~FDS_Table() = default;

    Error setCallback(callback_type const& _cb)
    { 
        if (0 < outstanding_io.load()) {
            cb = _cb;
            return ERR_IO_PENDING;
        }
        return ERR_OK;
    }

    size_t incRefcnt()
    { return outstanding_io.fetch_add(1, std::memory_order_relaxed) + 1; }

    size_t decRefcnt()
    {
        auto ref = outstanding_io.fetch_sub(1, std::memory_order_relaxed) - 1;
        // If this was the last IO and we had a callback registered, lets
        // call it now to indicate the pending IOs have completed
        if (0 == ref && cb) {
            cb(ERR_OK);
            cb = nullptr;
        }
        return ref;
    }

 protected:
    /** Depth of group */
    fds_uint32_t depth;

    /** Bits per token group */ 
    fds_uint32_t width;

    /** Columns (2^width) */
    fds_uint32_t columns;

    /** Table version */
    fds_uint64_t version;

 private:
    /** Ref count for outstanding IO */
    std::atomic_size_t outstanding_io {0};

    /** Callback to notify when all pending IO is clear */
    callback_type cb {nullptr};
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_TABLE_H_
