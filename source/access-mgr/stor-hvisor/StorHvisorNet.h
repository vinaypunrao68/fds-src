/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef __StorHvisorNet_h__
#define __StorHvisorNet_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>


#include <list>
#include "StorHvDataPlace.h"
#include <fds_error.h>
#include <fds_types.h>

/* TODO: Use API header in include directory. */
#include <lib/OMgrClient.h>
#include "StorHvVolumes.h"
#include "qos_ctrl.h" 
#include "fds_qos.h" 
#include "StorHvQosCtrl.h" 
#include <lib/StatsCollector.h>
#include <hash/md5.h>
#include <FdsRandom.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <fdsp/FDSP_MetaDataPathReq.h>
#include <fdsp/FDSP_MetaDataPathResp.h>
#include <fdsp/FDSP_ControlPathReq.h>
#include <fdsp/FDSP_ControlPathResp.h>
#include <fdsp/FDSP_ConfigPathReq.h>
#include <net/SvcRequest.h>
#include <am-tx-mgr.h>
#include <AmCache.h>
#include <AmDispatcher.h>
#include <AmProcessor.h>
#include <AmReqHandlers.h>
#include "AmRequest.h"

#include <map>
// #include "util/concurrency/Thread.h"
#include <concurrency/Synchronization.h>
#include <fds_counters.h>
#include "PerfTrace.h"

#undef  FDS_TEST_SH_NOOP              /* IO returns (filled with 0s for read) as soon as SH receives it from ubd */
#undef FDS_TEST_SH_NOOP_DISPATCH     /* IO returns (filled with 0s for read) as soon as dispatcher takes it from the queue */



#define FDS_IO_LONG_TIME 60 // seconds
#define FDS_REPLICATION_FACTOR          1
#define MAX_DM_NODES                    4

#define  FDS_NODE_OFFLINE               0
#define  FDS_NODE_ONLINE                1

#define  FDS_STORAGE_TYPE_SSD           1
#define  FDS_STORAGE_TYPE_FLASH         2
#define  FDS_STORAGE_TYPE_HARD          3
#define  FDS_STORAGE_TYPE_TAPE          4


#define  FDS_NODE_TYPE_PRIM             1
#define  FDS_NODE_TYPE_SEND             2
#define  FDS_NODE_TYPE_BCKP             3


#define  FDS_SUCCESS                    0
#define  FDS_FAILURE                    1

#define  FDS_TIMER_TIMEOUT              1

#define HVISOR_SECTOR_SIZE 		512

typedef unsigned int volid_t;

// Just a couple forward-declarations to satisfy the function
// prototypes below.
namespace fds {
struct CommitBlobTxReq;
struct PutBlobReq;
}

using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

typedef struct {
    double   hash_high;
    double   hash_low;
    char    conflict_id;
} fds_object_id_t;


typedef union {

    fds_object_id_t obj_id; // object id fields
    unsigned char bytes[20]; // byte array

} fds_doid_t;

typedef unsigned char doid_t[20];

/**
 * @brief Storage manager counters
 */
class AMCounters : public FdsCounters
{
 public:
    AMCounters(const std::string &id, FdsCountersMgr *mgr)
    : FdsCounters(id, mgr),
      put_reqs("put_reqs", this),
      get_reqs("get_reqs", this),
      puts_latency("puts_latency", this),
      gets_latency("gets_latency", this)
    {
    }

    NumericCounter put_reqs;
    NumericCounter get_reqs;
    LatencyCounter puts_latency;
    LatencyCounter gets_latency;
};
/*************************************************************************** */

class StorHvCtrl : public HasLogger {
public:
    /*
     * Defines specific test modes used to
     * construct the object.
     */
    typedef enum {
        DATA_MGR_TEST, /* Only communicate with DMs */
        STOR_MGR_TEST, /* Only communicate with SMs */
        TEST_BOTH,     /* Communication with DMs and SMs */
        NORMAL,        /* Normal, non-test mode */
        MAX
    } sh_comm_modes;

    StorHvCtrl(int argc, char *argv[], SysParams *params);
    StorHvCtrl(int argc, char *argv[], SysParams *params,
               sh_comm_modes _mode, fds_uint32_t instanceId = 0);
    StorHvCtrl(int argc,
               char *argv[],
               SysParams *params,
               sh_comm_modes _mode,
               fds_uint32_t sm_port_num,
               fds_uint32_t dm_port_num,
               fds_uint32_t instanceId = 0);
    ~StorHvCtrl();
    //imcremental checksum  for header and payload 
    checksum_calc   *chksumPtr;

    // Data Members
    StorHvDataPlacement        *dataPlacementTbl;
    boost::shared_ptr<netSessionTbl> rpcSessionTbl; // RPC calls Switch Table
    StorHvVolumeTable          *vol_table;  
    fds::StorHvQosCtrl             *qos_ctrl; // Qos Controller object
    OMgrClient                 *om_client;
    FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr dInfo;

    std::string                 myIp;
    std::string                 my_node_name;

    /// Toggle AM standalone mode for testing
    fds_bool_t toggleStandAlone;

    /// Dispatcher layer module
    AmDispatcher::shared_ptr amDispatcher;

    /// Processor layer module
    AmProcessor::unique_ptr amProcessor;

    RandNumGenerator::ptr randNumGen;
    // TODO(Andrew): Move this to a unique_ptr and only into
    // AmProcessor once that's ready
    AmTxManager::shared_ptr amTxMgr;
    AmCache::shared_ptr amCache;

    Error sendTestBucketToOM(const std::string& bucket_name,
                             const std::string& access_key_id = "",
                             const std::string& secret_access_key = "");

    void initVolInfo(FDSP_VolumeInfoTypePtr vol_info,
                     const std::string& bucket_name);

    void attachVolume(AmRequest *amReq);
    void enqueueAttachReq(const std::string& volumeName,
                         CallbackPtr cb);
    fds::Error pushBlobReq(AmRequest *blobReq);
    void enqueueBlobReq(AmRequest *blobReq);

    // Stuff for pending offset operations
    // TODO(Andrew): Reconcile with dispatchSm...
    fds::Error processSmPutObj(PutBlobReq *putBlobReq,
                               StorHvJournalEntry *journEntry);
    fds::Error processDmUpdateBlob(PutBlobReq *putBlobReq,
                                   StorHvJournalEntry *journEntry);
    fds::Error resumePutBlob(StorHvJournalEntry *journEntry);

    void statBlobResp(const FDSP_MsgHdrTypePtr rxMsg, 
                      const FDS_ProtocolInterface::
                      BlobDescriptorPtr blobDesc);
    void InitDmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
    void InitSmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
    void fbd_process_req_timeout(unsigned long arg);

    SysParams* getSysParams();
    void StartOmClient();
    sh_comm_modes GetRunTimeMode() { return mode; }
    Error dispatchSmPutMsg(StorHvJournalEntry *journEntry, const NodeUuid &send_uuid);
    Error dispatchSmGetMsg(StorHvJournalEntry *journEntry);
    Error dispatchDmUpdMsg(StorHvJournalEntry *journEntry,
                           const NodeUuid &send_uuid);

    struct TxnResponseHelper {
        StorHvCtrl* storHvisor = NULL;
        fds_uint32_t txnId = 0xFFFFFFFF;
        fds_volid_t  volId = 0;
        StorHvJournalEntry *txn = NULL;
        StorHvVolumeLock *vol_lock = NULL;
        StorHvJournalEntryLock *je_lock = NULL;
        StorHvVolume* vol = NULL;
        fds::AmRequest* blobReq = NULL;

        TxnResponseHelper(StorHvCtrl* storHvisor, fds_volid_t  volId, fds_uint32_t txnId);
        void setStatus(FDSN_Status  status);
        ~TxnResponseHelper();
    };

    struct TxnRequestHelper {
        StorHvCtrl* storHvisor = NULL;
        fds_uint32_t txnId = 0;
        fds_volid_t  volId = 0;
        FDSN_Status  status = FDSN_StatusNOTSET;
        StorHvVolume *shVol = NULL;
        StorHvJournalEntry *txn = NULL;
        StorHvJournalEntryLock *jeLock = NULL;
        StorHvVolume* vol = NULL;
        AmRequest *amReq;
        fds::AmRequest* blobReq = NULL;

        TxnRequestHelper(StorHvCtrl* storHvisor, AmRequest *amReq);

        bool getPrimaryDM(fds_uint32_t& ip, fds_uint32_t& port);
        bool isValidVolume();
        bool setupTxn();
        bool hasError();
        void setStatus(FDSN_Status  status);
        void scheduleTimer();
        ~TxnRequestHelper();
    };

    struct ResponseHelper {
        StorHvCtrl* storHvisor = NULL;
        fds_volid_t  volId = 0;
        StorHvVolumeLock *vol_lock = NULL;
        StorHvVolume* vol = NULL;
        fds::AmRequest* blobReq = NULL;
        ResponseHelper(StorHvCtrl* storHvisor, AmRequest *amReq);
        void setStatus(FDSN_Status  status);
        ~ResponseHelper();
    };

    struct RequestHelper {
        StorHvCtrl* storHvisor = NULL;
        fds_volid_t  volId = 0;
        FDSN_Status  status = FDSN_StatusNOTSET;
        StorHvVolume *shVol = NULL;
        StorHvVolume* vol = NULL;
        AmRequest *amReq;
        fds::AmRequest* blobReq = NULL;

        RequestHelper(StorHvCtrl* storHvisor, AmRequest *amReq);

        bool isValidVolume();
        bool hasError();
        void setStatus(FDSN_Status  status);
        ~RequestHelper();
    };

    struct BlobRequestHelper {
        StorHvCtrl* storHvisor = NULL;
        fds_volid_t volId = invalid_vol_id;
        AmRequest *blobReq = NULL;
        const std::string& volumeName;

        explicit BlobRequestHelper(StorHvCtrl* storHvisor, const std::string& volumeName);
        void setupVolumeInfo();
        fds::Error processRequest();
    };

    inline AMCounters& getCounters()
    {
        return counters_;
    }
private:
    SysParams *sysParams;
    sh_comm_modes mode;

    /// Toggles the local volume catalog cache
    fds_bool_t disableVcc;
    /** Counters */
    AMCounters counters_;
};

extern StorHvCtrl *storHvisor;

/*
 * Static function for process IO via a threadpool
 */
static void processBlobReq(AmRequest *amReq) {
    fds::PerfTracer::tracePointEnd(amReq->qos_perf_ctx);

    fds_verify(amReq->io_module == FDS_IOType::STOR_HV_IO);
    fds_verify(amReq->magicInUse() == true);

    switch (amReq->io_type) {
        case fds::FDS_START_BLOB_TX:
            storHvisor->amProcessor->startBlobTx(amReq);
            break;

        case fds::FDS_COMMIT_BLOB_TX:
            storHvisor->amProcessor->commitBlobTx(amReq);
            break;

        case fds::FDS_ABORT_BLOB_TX:
            storHvisor->amProcessor->abortBlobTx(amReq);
            break;

        case fds::FDS_ATTACH_VOL:
            storHvisor->attachVolume(amReq);
            break;

        case fds::FDS_IO_READ:
        case fds::FDS_GET_BLOB:
            storHvisor->amProcessor->getBlob(amReq);
            break;

        case fds::FDS_IO_WRITE:
        case fds::FDS_PUT_BLOB_ONCE:
        case fds::FDS_PUT_BLOB:
            storHvisor->amProcessor->putBlob(amReq);
            break;

        case fds::FDS_SET_BLOB_METADATA:
            storHvisor->amProcessor->setBlobMetadata(amReq);
            break;

        case fds::FDS_GET_VOLUME_METADATA:
            storHvisor->amProcessor->getVolumeMetadata(amReq);
            break;

        case fds::FDS_DELETE_BLOB:
            storHvisor->amProcessor->deleteBlob(amReq);
            break;

        case fds::FDS_STAT_BLOB:
            storHvisor->amProcessor->statBlob(amReq);
            break;

        case fds::FDS_VOLUME_CONTENTS:
            storHvisor->amProcessor->volumeContents(amReq);
            break;

        default :
            LOGCRITICAL << "unimplemented request: " << amReq->io_type;
            amReq->cb->call(ERR_NOT_IMPLEMENTED);
            break;
    }
}

#endif  // SOURCE_STOR_HVISOR_STORHVISORNET_H_
