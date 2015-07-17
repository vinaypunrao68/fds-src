/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_TRACEBUFFER_H_
#define SOURCE_INCLUDE_TRACEBUFFER_H_

#include <string>
#include <atomic>
#include <concurrency/SpinLock.h>
#include <fds_assert.h>

/**
* Traces to a process wide trace buffer.  Only trace small amount of data.  Any
* excess data is truncated
* @param __traceId__: Trace entry identfier
* @param ... : variadic args.
*/
#define TRACE(__traceId__, ...)
// TODO(Rao): Get the process wide trace buffer and trace to this buffer.

#define TRACE_BUF_LEN 64
/* Maximum number of trace entries allowed */
#define MAX_TRACEBUFFER_ENTRY_CNT 10


namespace fds {

enum TraceTypeId {
    STRING_TRACE
};

/**
* @brief Trace entry class
*/
struct TraceEntry {
    TraceEntry() {
        reset();
    }
    void reset() {
        typeId = 0;
        threadId = 0;
        timestamp = 0;
        next = nullptr;
    }

    uint16_t    typeId;         /* Type of the trace */
    uint64_t    threadId;       /* Thread ID. We can reduce it to short with some work */
    uint64_t    timestamp;      /* Timestamp.  We can reduce it to int with some work */
    char        buf[TRACE_BUF_LEN];
    TraceEntry  *next;
};

/**
* @brief Maintains pool of trace buffer entries
*/
struct TracebufferPool {
    explicit TracebufferPool(int poolEntryCnt);
    ~TracebufferPool();

    TraceEntry* allocTraceEntry();
    void freeTraceEntry(TraceEntry *e);

 protected:
    fds_spinlock lock_;
    TraceEntry *pool_;
    TraceEntry *freelistHead_;
};

/* Policy for the trace buffer */
enum TracebufferPolicy {
    /* Rotate once trace buffer is full.  We should implement this first */
    TB_RINGBUFFER,
    /* Grow as needed */
    TB_AUTOGROW,
    /* Once trace buffer is full don't add any more entries */
    TB_STOPWHENFULL
};

/**
* For tracing events.  Each event entry is constant size.  Purpose of the trace buffer
* is to trace very small pieces (size in bytes) information.  Essentially we would
* like a recent history of events in the system orderd by some context id.
*/
struct Tracebuffer : HasModuleProvider {
    Tracebuffer(CommonModuleProviderIf* moduleProvider,
                TracebufferPolicy traceBufPolicy);


    /**
    * @brief 
    *
    * @tparam T
    * @param obj
    *
    * @return 
    */
    template<class T>
    bool trace(const T &obj) {
        fds_verify(!"Not impl");
        return false;
    }

    /**
    * @param traceString - string to trace.  Content beyond TRACE_BUF_LEN is
    * truncated
    */
    bool trace(const std::string &traceString);

    uint32_t size() const;

 protected:
    TraceEntry* alloc_();
    void pushBack_(TraceEntry *e);
    TraceEntry* popFront_();

    fds_spinlock        lock_;
    TraceEntry          *head_;
    TraceEntry          *tail_;
    std::atomic<int>    traceCntr_;
};

/* TODO:
 * 1. Implement the above header
 * 2. Create a process wide trace buffer
 * 3. Provide gdb macro for dumping trace entries.
 * 4. Gdb macro for dumping trace entries orderd by trace ids
 */
}  // namespace fds

#endif  // SOURCE_INCLUDE_TRACEBUFFER_H_
