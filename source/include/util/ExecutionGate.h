///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Standard includes.
#include <condition_variable>
#include <mutex>

#ifndef SOURCE_INCLUDE_UTIL_EXECUTIONGATE_H_
#define SOURCE_INCLUDE_UTIL_EXECUTIONGATE_H_

namespace fds {
namespace util {

///
/// Halts execution until signaled.
///
class ExecutionGate final
{
public:
    ///
    /// Constructor.
    ///
    ExecutionGate();

    ///
    /// Close the gate. No-op if the gate is already closed.
    ///
    void close();

    ///
    /// Open the gate. No-op if the gate is already open. Otherwise, any currently blocking calls
    /// to waitUntilOpened() will return.
    ///
    void open();

    ///
    /// Halt execution of the calling thread if the gate is closed. Blocks until gate is opened.
    ///
    void waitUntilOpened();

private:
    ///
    /// Whether execution will continue.
    ///
    bool _is_closed;

    ///
    /// Synchronize access to _is_open and _cv.
    ///
    std::mutex _lock;

    ///
    /// Blocks until notified.
    ///
    std::condition_variable _cv;
};

inline ExecutionGate::ExecutionGate()
    : _is_closed(true)
{}

inline void ExecutionGate::close()
{
    std::unique_lock<std::mutex> lockManager(_lock);

    _is_closed = true;
}

inline void ExecutionGate::open()
{
    std::unique_lock<std::mutex> lockManager(_lock);

    if (_is_closed)
    {
        _is_closed = false;
        _cv.notify_all();
    }
}

inline void ExecutionGate::waitUntilOpened()
{
    // Acquire the lock. Will be released while waiting and on scope exit.
    std::unique_lock<std::mutex> lockManager(_lock);

    // Wait until the gate is opened. We can't just use the condition variable due to spurious
    // wakeups. Lock is held during this test.
    while (_is_closed)
    {
        // Release the lock and wait to be signaled. Lock is re-acquired when signaled.
        _cv.wait(lockManager);
    }
}

}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_EXECUTIONGATE_H_
