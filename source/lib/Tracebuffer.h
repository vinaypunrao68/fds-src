#ifndef TRACEBUF_H_
#define TRACEBUF_H_

namespace fds {
/**
* Traces to a process wide trace buffer.  Only trace small amount of data.  Any
* excess data is truncated
* @param __traceId__: Trace entry identfier
* @param ... : variadic args.
*/
#define TRACE(__traceId__, ...)
// TODO: Get the process wide trace buffer and trace to this buffer.

typedef short TraceId;
const TraceId INVALID_TRACE_ID = 0xFFFF;
/* Use this trace id for tracing entries that don't need any trace id association */
const TraceId GenericTraceId = 0;

struct TraceEntry {
    TraceId traceId;         /* Unique id so we can sort trance entries based on the id */
    uint64_t threadId;      /* Thread ID. We can reduce it to short with some work */
    uint64_t timestamp;     /* Timestamp.  We can reduce it to int with some work */
    char buf[TRACE_BUF_LEN];
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
* For tracing events.  Each event entry is constant size.
*/
class Tracebuffer {
    Tracebuffer(TracebufferPolicy traceBufPolicy, int maxTraceEntryCnt);

    /**
    * @param traceString - string to trace.  Content beyond TRACE_BUF_LEN is
    * truncated
    */
    void trace(const std::string &traceString);

    /**
    * Allocates unique trace entry identifier.  In case there are no unique ids
    * INVALID_TRACE_ID is returned.
    */
    TraceId allocTraceId();

    /**
    * Release the trace entry identifier
    */
    void releaseTraceId(const TraceId& traceId);
protected:
    /* Trace entry array */
    TraceEntry *traceEntries;
};

/* TODO:
 * 1. Implement the above header
 * 2. Create a process wide trace buffer
 * 3. Provide gdb macro for dumping trace entries.
 * 4. Gdb macro for dumping trace entries orderd by trace ids
 */
}  // namespace fds

#endif
