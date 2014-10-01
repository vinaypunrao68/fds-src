/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_HVISOR_STORHVJOURNAL_H_
#define SOURCE_STOR_HVISOR_STORHVJOURNAL_H_

#include <string>
#include <list>
#include <set>
#include <iostream>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <concurrency/Mutex.h>
#include <fds_types.h>
#include <fds_timer.h>

#define  FDS_MIN_ACK                    1
#define  FDS_CLS_ACK                    0
#define  FDS_SET_ACK                    1

#define  FDS_COMMIT_MSG_SENT            1
#define  FDS_COMMIT_MSG_ACKED           2

class FDSP_IpNode {
  public:
    fds_uint32_t ipAddr;
    fds_uint32_t port;
    fds_uint8_t  ack_status;
    fds_uint8_t  commit_status;
};

#define FDS_MAX_DM_NODES_PER_CLST 16
#define FDS_MAX_SM_NODES_PER_CLST 16
#define FDS_READ_WRITE_LOG_ENTRIES 10*1024

namespace fps = FDS_ProtocolInterface;

namespace fds {

/// AM operation states
enum TxnState {
    FDS_TRANS_EMPTY,
    FDS_TRANS_BLOB_START,
    FDS_TRANS_OPEN,
    FDS_TRANS_OPENED,
    FDS_TRANS_COMMITTED,
    FDS_TRANS_SYNCED,
    FDS_TRANS_DONE,
    FDS_TRANS_VCAT_QUERY_PENDING,
    FDS_TRANS_GET_OBJ,
    FDS_TRANS_DEL_OBJ,
    FDS_TRANS_GET_BUCKET,
    FDS_TRANS_BUCKET_STATS,
    FDS_TRANS_PENDING_DLT,
    FDS_TRANS_MULTIDM,
    FDS_TRANS_PENDING_WAIT  // Waiting for existing journal
};
std::ostream& operator<<(std::ostream& os, const TxnState& state);

// Forward declares
class StorHvJournal;
class StorHvJournalEntry;

class StorHvIoTimerTask : public fds::FdsTimerTask {
  public:
    StorHvJournalEntry *jrnlEntry;
    StorHvJournal      *jrnlTbl;

    void runTimerTask();
    StorHvIoTimerTask(StorHvJournalEntry *jrnl_entry,
                      StorHvJournal *jrnl_tbl,
                      FdsTimer *ioTimer)
            : FdsTimerTask(*ioTimer) {
        jrnlEntry = jrnl_entry;
        jrnlTbl = jrnl_tbl;
    }
    typedef boost::shared_ptr<StorHvIoTimerTask> ptr;
};

typedef boost::shared_ptr<StorHvIoTimerTask> StorHvIoTimerTaskPtr;

/// Forward declare to avoid circular dependency
class AmQosReq;

/// Represents an outstanding operations to a blob
class  StorHvJournalEntry {
  private:
    fds_mutex *je_mutex;

  public:
    StorHvJournalEntry();
    ~StorHvJournalEntry();
    boost::shared_ptr<StorHvJournalEntry> ptr;

    /// List of operations waiting for others to finish
    std::list<StorHvJournalEntry*> pendingTransactions;
    /// Resumes the next waiting operation
    void  resumeTransaction();

    void init(unsigned int transid, StorHvJournal* jrnl_tbl, FdsTimer *ioTimer);
    void reset();
    void setActive();
    void setInactive();
    bool isActive();
    void lock();
    void unlock();
    int fds_set_dmack_status(int ipAddr,
                             fds_uint32_t port);
    int fds_set_dm_commit_status(int ipAddr,
                                 fds_uint32_t port);
    int fds_set_smack_status(int ipAddr,
                             fds_uint32_t port);
    void fbd_process_req_timeout();

    FdsTimerTaskPtr ioTimerTask;
    bool is_in_use;
    unsigned int trans_id;
    fds_uint8_t replc_cnt;
    fds_uint8_t sm_ack_cnt;
    fds_uint8_t dm_ack_cnt;
    fds_uint8_t dm_commit_cnt;
    TxnState trans_state;
    fds_io_op_t op;
    FDS_ObjectIdType data_obj_id;
    fds_uint32_t data_obj_len;

    std::string blobName;
    std::string session_uuid;
    fds_uint64_t blobOffset;

    FDS_IOType             *io;
    fpi::FDSP_MsgHdrTypePtr     sm_msg;
    fpi::FDSP_MsgHdrTypePtr     dm_msg;

    /**
     * Stashing the put/get/del requests
     * here in case we need to resend them.
     * Only the requests for the specific
     * transaction type are valid. All others
     * are NULL.
     */
    fpi::FDSP_PutObjTypePtr    putMsg;
    fpi::FDSP_GetObjTypePtr    getMsg;
    fpi::FDSP_DeleteObjTypePtr delMsg;
    fpi::FDSP_UpdateCatalogTypePtr updCatMsg;

    int nodeSeq;
    fds_uint8_t num_dm_nodes;
    FDSP_IpNode dm_ack[FDS_MAX_DM_NODES_PER_CLST];
    fds_uint8_t num_sm_nodes;
    FDSP_IpNode sm_ack[FDS_MAX_SM_NODES_PER_CLST];
    fds_uint64_t dmt_version;
};

class StorHvJournalEntryLock {
  private:
    StorHvJournalEntry *jrnl_e;

  public:
    explicit StorHvJournalEntryLock(StorHvJournalEntry *jrnl_entry);
    ~StorHvJournalEntryLock();
};

typedef std::queue<fds_uint32_t> PendingDltQueue;
typedef std::set<fds_uint32_t> PendingDltSet;

class StorHvJournal {
  private:
    fds_mutex *jrnl_tbl_mutex;
    StorHvJournalEntry  *rwlog_tbl;
    std::unordered_map<fds_uint64_t, fds_uint32_t> block_to_jrnl_idx;
    /*
     * New jrnl index for blobs
     */
    std::unordered_map<std::string, fds_uint32_t> blob_to_jrnl_idx;
    std::queue<unsigned int> free_trans_ids;
    unsigned int max_journal_entries;

    unsigned int get_free_trans_id();
    void return_free_trans_id(unsigned int trans_id);
    FdsTimer  *ioTimer;

    boost::posix_time::ptime ctime; /* time the journal was created */

    /**
     * Queue of transactions that are currently waiting for
     * a new DLT version to be recieved before continuing
     * processing.
     * These requests had prior requests rejected because
     * the local DLT version was not update to date.
     */
    PendingDltQueue transPendingDlt;
    /**
     * Set of unique transactions that are waiting
     * for a new DLT. Helps ensure the queue does not
     * contain duplicates.
     */
    PendingDltSet transDltSet;

  public:
    StorHvJournal();
    explicit StorHvJournal(unsigned int max_jrnl_entries);
    ~StorHvJournal();

    void lock();
    void unlock();

    void pushPendingDltTrans(fds_uint32_t transId);
    PendingDltQueue popAllDltTrans();

    StorHvJournalEntry *get_journal_entry(fds_uint32_t trans_id);
    // TODO(Andrew): Don't pass a non-const ref, make a shared ptr
    // TODO(Andrew): Don't require blob offset...using the whole blob
    fds_uint32_t get_trans_id_for_block(fds_uint64_t block_offset);
    fds_uint32_t get_trans_id_for_blob(const std::string& blobName,
                                       fds_uint64_t blobOffset,
                                       bool& trans_in_progress);
    void releaseTransId(fds_uint32_t transId);

    // Handle existing offsets
    fds_uint32_t create_trans_id(const std::string& blobName,
                                 fds_uint64_t blobOffset);
    void resumePendingTrans(unsigned int trans_id);

    template<typename Rep, typename Period>
    fds_bool_t schedule(FdsTimerTaskPtr& task,
                  const std::chrono::duration<Rep, Period>& interval) {
        return ioTimer->schedule(task, interval);
    }

    fds_uint32_t microsecSinceCtime(boost::posix_time::ptime timestamp) {
        boost::posix_time::time_duration elapsed = timestamp - ctime;
        return elapsed.total_microseconds();
    }
};
}  // namespace fds

#endif  // SOURCE_STOR_HVISOR_STORHVJOURNAL_H_
