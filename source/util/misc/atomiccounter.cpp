
#include "util/atomiccounter.h"

namespace fds {
namespace util {
AtomicCounter::AtomicCounter() : value(0) {
}

AtomicCounter::AtomicCounter(const AtomicCounter& c) {
    value = c.value.load(std::memory_order_relaxed);
}

uint64_t AtomicCounter::get() const {
    return value.load(std::memory_order_relaxed);
}

uint64_t AtomicCounter::incr(const uint64_t v) {
    return value.fetch_add(v, std::memory_order_relaxed);
}

uint64_t AtomicCounter::decr(const uint64_t v) {
    return value.fetch_sub(v, std::memory_order_relaxed);
}

void AtomicCounter::set(const uint64_t v) {
    value.store(v, std::memory_order_relaxed);
}
}  // namespace util
}  // namespace fds
