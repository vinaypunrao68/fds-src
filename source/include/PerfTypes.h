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
    PUT_IO,
    PUT_OBJ_REQ,
    PUT_OBJ_REQ_ERR,
    PUT_QOS_QUEUE_WAIT,
    PUT_OBJ_TASK_SYNC_WAIT,
    PUT_SSD_OBJ,
    PUT_HDD_OBJ,

    DUPLICATE_OBJ,
    HASH_COLLISION,

    GET_IO,
    GET_OBJ_REQ,
    GET_OBJ_REQ_ERR,
    GET_QOS_QUEUE_WAIT,
    GET_OBJ_TASK_SYNC_WAIT,
    GET_SSD_OBJ,
    GET_HDD_OBJ,

    DELETE_IO,
    DELETE_OBJ_REQ,
    DELETE_OBJ_REQ_ERR,
    DELETE_QOS_QUEUE_WAIT,
    DELETE_OBJ_TASK_SYNC_WAIT,

    ADD_OBJECT_REF_REQ,
    ADD_OBJECT_REF_REQ_ERR,
    ADD_OBJECT_REF_IO,
    ADD_OBJECT_REF_TRANS_QUEUE_WAIT,
    ADD_OBJECT_REF_QOS_QUEUE_WAIT,

    MURMUR3_HASH,
    DLT_LKUP,
    DMT_LKUP,

    PERSIST_DISK_WRITE,
    PUT_OBJ_LOC_INDX_UPDATE,

    GET_OBJ_LKUP_LOC_INDX,
    GET_OBJ_PL_READ_DISK,

    COMMIT_LOG_WRITE,
    SM_OBJ_METADATA_DB_READ,     /* put object metadata entry into DB */
    SM_OBJ_METADATA_DB_WRITE,    /* get object metadata entry from DB */
    SM_OBJ_METADATA_DB_REMOVE,   /* actual removal of metadata entry from DB */
    SM_OBJ_METADATA_CACHE_HIT,   /* object metadata cache hits (get path) */
    SM_OBJ_DATA_CACHE_HIT,       /* object data cache hits (get path) */
    SM_OBJ_DATA_DISK_READ,       /* persistent layer read object data */
    SM_OBJ_DATA_DISK_WRITE,      /* persistent layer write object data */
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
