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
#include "fds_volume.h"
#include "util/enum_util.h"

namespace fds {

class FdsBaseCounter;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PerfEventType,
    (TRACE)  // generic event
    (TRACE_ERR)

    // Store Manager
    (SM_PUT_IO)                 /* Latency of processing PUT in the layer below QoS control */
    (SM_E2E_PUT_OBJ_REQ)        /* End-to-end PUT object request latency in SM */
    (SM_PUT_OBJ_REQ_ERR)        /* Counts SM returns error on PUT request */
    (SM_PUT_QOS_QUEUE_WAIT)     /* PUT object request time in QoS queue in SM */
    (SM_PUT_OBJ_TASK_SYNC_WAIT) /* Latency of PUT request waiting on object task synchronizer */

    (SM_GET_IO)                 /* Latency of processing GET in the layer below QoS control */
    (SM_E2E_GET_OBJ_REQ)        /* End-to-end GET object request latency in SM */
    (SM_GET_OBJ_REQ_ERR)        /* Counts SM returns error on GET request */
    (SM_GET_QOS_QUEUE_WAIT)     /* GET object request time in QoS queue in SM */
    (SM_GET_OBJ_TASK_SYNC_WAIT) /* Latency of GET request waiting on object task synchronizer */

    (SM_DELETE_IO)                 /* Latency of processing DELETE in the layer below QoS control */
    (SM_E2E_DELETE_OBJ_REQ)        /* End-to-end DELETE object request latency in SM */
    (SM_DELETE_OBJ_REQ_ERR)        /* Counts SM returns error on DELETE request */
    (SM_DELETE_QOS_QUEUE_WAIT)     /* DELETE object request time in QoS queue in SM */
    (SM_DELETE_OBJ_TASK_SYNC_WAIT) /* Latency of DELETE req waiting on object task synchronizer */

    (SM_E2E_ADD_OBJ_REF_REQ)        /* End-to-end add object reference count request latency in SM */
    (SM_ADD_OBJ_REF_REQ_ERR)        /* Counts SM returns error on add ref count request */
    (SM_ADD_OBJ_REF_IO)             /* Add obj ref req processing latency in the layer below QoS */
    (SM_ADD_OBJ_REF_QOS_QUEUE_WAIT) /* Wait in qos queue for add object ref request in SM */
    (SM_ADD_OBJ_REF_TASK_SYNC_WAIT) /* Add obj ref req wait time on object task synchronizer */

    (SM_PUT_DUPLICATE_OBJ)        /* put duplicate object (update metadata only) */
    (SM_OBJ_METADATA_DB_READ)     /* put object metadata entry into DB */
    (SM_OBJ_METADATA_DB_WRITE)    /* get object metadata entry from DB */
    (SM_OBJ_METADATA_DB_REMOVE)   /* actual removal of metadata entry from DB */
    (SM_OBJ_METADATA_CACHE_HIT)   /* object metadata cache hits (get path) */
    (SM_OBJ_DATA_CACHE_HIT)       /* object data cache hits (get path) */
    (SM_OBJ_DATA_DISK_READ)       /* persistent layer read object data */
    (SM_OBJ_DATA_DISK_WRITE)      /* persistent layer write object data */
    (SM_OBJ_DATA_SSD_WRITE)       /* persistent layer write object data to SSD (SSD hits) */
    (SM_OBJ_DATA_SSD_READ)        /* persistent layer read object data from SSD (SSD hits) */
    (SM_OBJ_MARK_DELETED)         /* mark object deleted in obj metadata */
    (SM_APPLY_OBJECT_METADATA)    /* Time applyObjectMetaData on the migration path*/
    (SM_READ_OBJ_DELTA_SET)       /* Migration's readObjDeltaSet latency (debug) */
    (SM_MIGRATION_SECOND_PHASE)   /* Numeric counter incremented when the second phase of */
                                  /* migration is entered (debug) */

    // Access Manager
    (AM_GET_OBJ_REQ)            /* End2end GET latency */
    (AM_GET_QOS)                /* Latency GET operations spend in QOS */
    (AM_GET_SM)                 /* End2end SM latency from AM */
    (AM_GET_DM)                 /* End2end DM latency from AM */
    (AM_GET_HASH)               /* */

    (AM_PUT_OBJ_REQ)            /* End2end PUT latency */
    (AM_PUT_QOS)                /* Latency PUT operations spend in QOS */
    (AM_PUT_SM)                 /* End2end SM latency from AM */
    (AM_PUT_DM)                 /* End2end DM latency from AM */
    (AM_PUT_HASH)               /* */

    (AM_DELETE_OBJ_REQ)         /* End2end latency for DELETE */
    (AM_DELETE_QOS)             /* Latency DELETE operations spend in QOS */
    (AM_DELETE_SM)              /* End2end SM latency from AM */
    (AM_DELETE_DM)              /* End2end DM latency from AM */
    (AM_DELETE_HASH)            /* */

    (AM_GET_BLOB_REQ)            /* End2end GET Blob request */
    (AM_STAT_BLOB_OBJ_REQ)      /* End2end stat_blob latency */
    (AM_SET_BLOB_META_OBJ_REQ)  /* End2end set_blob_meta latency */
    (AM_GET_BLOB_META_OBJ_REQ)  /* End2end get_blob_meta latency */
    (AM_START_BLOB_OBJ_REQ)     /* End2end start_blob */
    (AM_COMMIT_BLOB_OBJ_REQ)    /* End2end commit_blob */
    (AM_ABORT_BLOB_OBJ_REQ)     /* End2end abort_blob */
    (AM_VOLUME_ATTACH_REQ)      /* End2end volume_attach */
    (AM_VOLUME_DETACH_REQ)      /* End2end volume_detach */
    (AM_VOLUME_CONTENTS_REQ)    /* End2end volume_contents */
    (AM_STAT_VOLUME_REQ)        /* End2end stat_volume*/
    (AM_SET_VOLUME_METADATA_REQ)/* End2end set_volume_matadata */
    (AM_GET_VOLUME_METADATA_REQ)/* End2end get_volume_metadata*/

    (AM_QOS_QUEUE_SIZE)         /* Histogram of AM QoS size */

    (AM_DESC_CACHE_HIT)         /* AM descriptor cache hits */
    (AM_OFFSET_CACHE_HIT)       /* AM offset cache hits */
    (AM_OBJECT_CACHE_HIT)       /* AM object cache hits */

    // Data Manager
    (DM_TX_OP)                  /* DM IO latency */
    (DM_TX_OP_ERR)              /* DM IO number of errors */
    (DM_TX_OP_REQ)              /* End2end latency of DM transactions */
    (DM_TX_OP_REQ_ERR)          /* Number of errors for DM transactions */

    (DM_TX_STARTED)             /* Number of started transactions */
    (DM_TX_COMMITTED)           /* Number of committed transactions */
    (DM_TX_COMMIT_REQ)          /* Latency DM commit */
    (DM_TX_COMMIT_REQ_ERR)      /* NUmber of DM commit errors */
    (DM_VOL_CAT_WRITE)          /* Latency of volume catalog write */
    (DM_TX_QOS_WAIT)            /* QoS wait of a DM transaction*/

    (DM_QUERY_REQ)              /* DM query request latency */
    (DM_QUERY_REQ_ERR)          /* DM query request #errors */
    (DM_VOL_CAT_READ)           /* Volume catalog read latency*/
    (DM_QUERY_QOS_WAIT)         /* DM query QoS wait */

    (DM_CACHE_HIT)              /* Number of DM cache hits*/
)

class PerfEventHash {
  public:
    fds_uint32_t operator()(PerfEventType evt) const {
        return static_cast<fds_uint32_t>(evt);
    }
};

extern const unsigned PERF_CONTEXT_TIMEOUT;

typedef struct PerfContext_ {
    PerfContext_() :
            type(PerfEventType::TRACE),
            name(""),
            volid_valid(false),
            volid(0),
            enabled(true),
            timeout(PERF_CONTEXT_TIMEOUT),
            start_cycle(0),
            end_cycle(0),
            data(0) {}

    PerfContext_(PerfEventType type_, std::string const& name_ = "") :
            type(type_),
            name(name_),
            volid_valid(false),
            volid(0),
            enabled(true),
            timeout(PERF_CONTEXT_TIMEOUT),
            start_cycle(0),
            end_cycle(0),
            data(0) {
        fds_assert(false);  // TODO(matteo): remove
        fds_assert(fds_enum::get_index<PerfEventType>(type) < fds_enum::get_size<PerfEventType>());
    }

    PerfContext_(PerfEventType type_, fds_volid_t volid_, std::string const& name_ = "") :
            type(type_),
            name(name_),
            volid_valid(true),
            volid(volid_),
            enabled(true),
            timeout(PERF_CONTEXT_TIMEOUT),
            start_cycle(0),
            end_cycle(0),
            data(0) {
        fds_assert(fds_enum::get_index<PerfEventType>(type) < fds_enum::get_size<PerfEventType>());
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
    std::once_flag once;
} PerfContext;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PERFTYPES_H_
