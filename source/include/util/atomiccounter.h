/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_ATOMICCOUNTER_H_
#define SOURCE_INCLUDE_UTIL_ATOMICCOUNTER_H_
#include <atomic>

namespace fds {
namespace util {
struct AtomicCounter {
    AtomicCounter();
    ~AtomicCounter() = default;
    uint64_t get() const;
    uint64_t incr(const uint64_t v = 1);
    uint64_t decr(const uint64_t v = 1);
    void set(const uint64_t v);
    AtomicCounter(const AtomicCounter& c);
  protected:
    std::atomic<uint64_t> value;
};
}  // namespace util
}  // namespace fds
#endif  // SOURCE_INCLUDE_UTIL_ATOMICCOUNTER_H_
