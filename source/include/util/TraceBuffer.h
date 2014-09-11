/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_TRACEBUFFER_H_
#define SOURCE_INCLUDE_UTIL_TRACEBUFFER_H_

#include <thread>
#include <string>
#include <sstream>
#include <util/timeutils.h>

#define TRACE(tb) tb << "\n" << std::hex<< std::this_thread::get_id() \
    << std::dec << " " << fds::util::getTimeStampNanos() << " "

namespace fds {
struct TraceBuffer {
    TraceBuffer()
    : TraceBuffer(2048)
    {
    }

    explicit TraceBuffer(const uint32_t limit)
    : limit_(limit)
    {
        filled_ = false;
    }

    template <class T>
    TraceBuffer& operator<< (T val) {
        if (filled_ == true) {
            return *this;
        }

        ss_ << val;

        if (ss_.tellp() > limit_) {
            filled_ = true;
        }
        return *this;
    }

    std::string str() const {
        return ss_.str();
    }
    inline std::stringstream &tr_buffer() {
        return ss_;
    }

 protected:
    /* Total limit */
    const uint32_t limit_;
    /* Filled or not.  We will not add entries once filled */
    bool filled_;
    /* Underneath stream for buffering */
    // TODO(Rao): Replace with a const size buffer
    std::stringstream ss_;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_TRACEBUFFER_H_
