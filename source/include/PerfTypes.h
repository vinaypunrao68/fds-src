/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
/*
 * Performance events types
 */
#ifndef SOURCE_INCLUDE_PERFTYPES_H_
#define SOURCE_INCLUDE_PERFTYPES_H_

#include <string>
#include <mutex>
#include "fds_typedefs.h" //NOLINT

namespace fds {

typedef fds_int64_t fds_volid_t;  // FIXME(matteo): this is ugly, needed to
                                  // break header file circular dependency

class FdsBaseCounter;

// XXX: update eventTypeToStr[] for each event added

typedef enum {
    TRACE,  // generic event
    TRACE_ERR,

    // Store Manager
    SM_PUT_IO,                 /* Latency of processing PUT in the layer below QoS control */
    SM_E2E_PUT_OBJ_REQ,        /* End-to-end PUT object request latency in SM */
    SM_PUT_OBJ_REQ_ERR,        /* Counts SM returns error on PUT request */
    SM_PUT_QOS_QUEUE_WAIT,     /* PUT object request time in QoS queue in SM */
    SM_PUT_OBJ_TASK_SYNC_WAIT, /* Latency of PUT request waiting on object task synchronizer */

    SM_GET_IO,                 /* Latency of processing GET in the layer below QoS control */
    SM_E2E_GET_OBJ_REQ,        /* End-to-end GET object request latency in SM */
    SM_GET_OBJ_REQ_ERR,        /* Counts SM returns error on GET request */
    SM_GET_QOS_QUEUE_WAIT,     /* GET object request time in QoS queue in SM */
    SM_GET_OBJ_TASK_SYNC_WAIT, /* Latency of GET request waiting on object task synchronizer */

    SM_DELETE_IO,                 /* Latency of processing DELETE in the layer below QoS control */
    SM_E2E_DELETE_OBJ_REQ,        /* End-to-end DELETE object request latency in SM */
    SM_DELETE_OBJ_REQ_ERR,        /* Counts SM returns error on DELETE request */
    SM_DELETE_QOS_QUEUE_WAIT,     /* DELETE object request time in QoS queue in SM */
    SM_DELETE_OBJ_TASK_SYNC_WAIT, /* Latency of DELETE req waiting on object task synchronizer */

    SM_E2E_ADD_OBJ_REF_REQ,        /* End-to-end add object reference count request latency in SM */
    SM_ADD_OBJ_REF_REQ_ERR,        /* Counts SM returns error on add ref count request */
    SM_ADD_OBJ_REF_IO,             /* Add obj ref req processing latency in the layer below QoS */
    SM_ADD_OBJ_REF_QOS_QUEUE_WAIT, /* Wait in qos queue for add object ref request in SM */
    SM_ADD_OBJ_REF_TASK_SYNC_WAIT, /* Add obj ref req wait time on object task synchronizer */

    SM_PUT_DUPLICATE_OBJ,        /* put duplicate object (update metadata only) */
    SM_OBJ_METADATA_DB_READ,     /* put object metadata entry into DB */
    SM_OBJ_METADATA_DB_WRITE,    /* get object metadata entry from DB */
    SM_OBJ_METADATA_DB_REMOVE,   /* actual removal of metadata entry from DB */
    SM_OBJ_METADATA_CACHE_HIT,   /* object metadata cache hits (get path) */
    SM_OBJ_DATA_CACHE_HIT,       /* object data cache hits (get path) */
    SM_OBJ_DATA_DISK_READ,       /* persistent layer read object data */
    SM_OBJ_DATA_DISK_WRITE,      /* persistent layer write object data */
    SM_OBJ_DATA_SSD_WRITE,       /* persistent layer write object data to SSD (SSD hits) */
    SM_OBJ_DATA_SSD_READ,        /* persistent layer read object data from SSD (SSD hits) */
    SM_OBJ_MARK_DELETED,         /* mark object deleted in obj metadata */

    // Access Manager
    AM_PUT_OBJ_REQ,
    AM_PUT_QOS,
    AM_PUT_HASH,
    AM_PUT_SM,
    AM_PUT_DM,

    AM_GET_OBJ_REQ,
    AM_GET_QOS,
    AM_GET_HASH,
    AM_GET_SM,
    AM_GET_DM,

    AM_DELETE_OBJ_REQ,
    AM_DELETE_QOS,
    AM_DELETE_HASH,
    AM_DELETE_SM,
    AM_DELETE_DM,

    AM_STAT_BLOB_OBJ_REQ,
    AM_SET_BLOB_META_OBJ_REQ,
    AM_GET_VOLUME_META_OBJ_REQ,
    AM_GET_BLOB_META_OBJ_REQ,
    AM_START_BLOB_OBJ_REQ,
    AM_COMMIT_BLOB_OBJ_REQ,
    AM_ABORT_BLOB_OBJ_REQ,
    AM_VOLUME_ATTACH_REQ,
    AM_VOLUME_CONTENTS_REQ,
    AM_VOLUME_STATS_REQ,

    AM_QOS_QUEUE_SIZE,

    AM_DESC_CACHE_HIT,
    AM_OFFSET_CACHE_HIT,
    AM_OBJECT_CACHE_HIT,

    // Data Manager
    DM_TX_OP,
    DM_TX_OP_ERR,
    DM_TX_OP_REQ,
    DM_TX_OP_REQ_ERR,

    DM_TX_STARTED,
    DM_TX_COMMITTED,
    DM_TX_COMMIT_REQ,
    DM_TX_COMMIT_REQ_ERR,
    DM_VOL_CAT_WRITE,
    DM_TX_QOS_WAIT,

    DM_QUERY_REQ,
    DM_QUERY_REQ_ERR,
    DM_VOL_CAT_READ,
    DM_QUERY_QOS_WAIT,

    DM_CACHE_HIT,

    // XXX: add new entries before this entry
    MAX_EVENT_TYPE  // this should be the last entry in the enum
} PerfEventType;

class PerfEventHash {
  public:
    fds_uint32_t operator()(PerfEventType evt) const {
        return static_cast<fds_uint32_t>(evt);
    }
};

extern const unsigned PERF_CONTEXT_TIMEOUT;

typedef struct PerfContext_ {
    PerfContext_() :
            type(TRACE),
            name(""),
            volid_valid(false),
            volid(0),
            enabled(true),
            timeout(PERF_CONTEXT_TIMEOUT),
            start_cycle(0),
            end_cycle(0),
            data(0),
            once(new std::once_flag()) {}

    PerfContext_(PerfEventType type_, std::string name_ = "") :
            type(type_),
            name(name_),
            volid_valid(false),
            volid(0),
            enabled(true),
            timeout(PERF_CONTEXT_TIMEOUT),
            start_cycle(0),
            end_cycle(0),
            data(0),
            once(new std::once_flag()) {
        fds_assert(false);  // TODO(matteo): remove
        fds_assert(type < MAX_EVENT_TYPE);
    }

    PerfContext_(PerfEventType type_, fds_volid_t volid_, std::string name_ = "") :
            type(type_),
            name(name_),
            volid_valid(true),
            volid(volid_),
            enabled(true),
            timeout(PERF_CONTEXT_TIMEOUT),
            start_cycle(0),
            end_cycle(0),
            data(0),
            once(new std::once_flag()) {
        fds_assert(type < MAX_EVENT_TYPE);
    }

    void reset_volid(fds_volid_t volid_)
    {
        volid_valid = true;
        volid = volid_;
    }

    PerfEventType type;
    std::string name;
    bool volid_valid;
    fds_volid_t volid;
    bool enabled;
    uint64_t timeout;
    uint64_t start_cycle;
    uint64_t end_cycle;
    boost::shared_ptr<FdsBaseCounter> data;
    boost::shared_ptr<std::once_flag> once;
} PerfContext;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PERFTYPES_H_
