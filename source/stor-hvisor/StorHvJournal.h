#ifndef __STOR_HV_JRNL_H_
#define __STOR_HV_JRNL_H_

#include <iostream>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <concurrency/Mutex.h>
#include "fds_types.h"
#include "StorHvisorCPP.h"
#include <fds_timer.h>

#define  FDS_MIN_ACK                    1
#define  FDS_CLS_ACK                    0
#define  FDS_SET_ACK                    1

#define  FDS_COMMIT_MSG_SENT            1
#define  FDS_COMMIT_MSG_ACKED           2

class FDSP_IpNode {
public:
  long         ipAddr;
  fds_uint32_t port;
  short        ack_status;
  short        commit_status;
};

#define  FDS_MAX_DM_NODES_PER_CLST      16
#define  FDS_MAX_SM_NODES_PER_CLST      16
#define  FDS_READ_WRITE_LOG_ENTRIES 	10*1024


using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

namespace fds {

enum TxnState {
    FDS_TRANS_EMPTY,
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
};

std::ostream& operator<<(ostream& os, const TxnState& state);


class StorHvJournal;
class StorHvJournalEntry;

class StorHvIoTimerTask : public fds::FdsTimerTask {
public:
 StorHvJournalEntry *jrnlEntry;
 StorHvJournal      *jrnlTbl;

  void runTimerTask();
  StorHvIoTimerTask(StorHvJournalEntry *jrnl_entry, StorHvJournal *jrnl_tbl, FdsTimer *ioTimer):FdsTimerTask(*ioTimer) {
     jrnlEntry = jrnl_entry;
     jrnlTbl = jrnl_tbl;
  }
//  :FdsTimerTask(ioTimer);
};

typedef boost::shared_ptr<StorHvIoTimerTask> StorHvIoTimerTaskPtr;

class   StorHvJournalEntry {

 private:
  fds_mutex *je_mutex;

public:
  StorHvJournalEntry();
  ~StorHvJournalEntry();

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
  void fbd_complete_req(fbd_request_t *req, int status);
  void fbd_process_req_timeout();

  FdsTimerTaskPtr  ioTimerTask;
  bool   is_in_use;
  unsigned int trans_id;
  short  replc_cnt;
  short  sm_ack_cnt;
  short  dm_ack_cnt;
  short  dm_commit_cnt;
  TxnState  trans_state;
  unsigned short incarnation_number;
  fds_io_op_t   op;
  FDS_ObjectIdType data_obj_id;
  int      data_obj_len;

  std::string blobName;
  std::string session_uuid;
  fds_uint64_t blobOffset;

  /*
   * These block specific fields can be removed.
   * They are simply left here because legacy, unused
   * functions still reference them, so they are needed
   * for that legacy, unused code to compile
   */
  unsigned int block_offset;
  void     *fbd_ptr;   // TODO: Remove
  void     *read_ctx;  // TODO: Remove
  void     *write_ctx; // TODO: Remove
  blkdev_complete_req_cb_t comp_req; // TODO: Remove
  void 	*comp_arg1; // TODO: Remove
  void 	*comp_arg2; // TODO: Remove

  FDS_IOType             *io;
  FDSP_MsgHdrTypePtr     sm_msg;
  FDSP_MsgHdrTypePtr     dm_msg;

  /**
   * Stashing the put/get/del requests
   * here in case we need to resend them.
   * Only the requests for the specific
   * transaction type are valid. All others
   * are NULL.
   */
  FDSP_PutObjTypePtr    putMsg;
  FDSP_GetObjTypePtr    getMsg;
  FDSP_DeleteObjTypePtr delMsg;

  int      nodeSeq;
  short    num_dm_nodes;
  FDSP_IpNode    dm_ack[FDS_MAX_DM_NODES_PER_CLST];
  short    num_sm_nodes;
  FDSP_IpNode    sm_ack[FDS_MAX_SM_NODES_PER_CLST];
};

class  StorHvJournalEntryLock {

 private:
  StorHvJournalEntry *jrnl_e;

 public:
  StorHvJournalEntryLock(StorHvJournalEntry *jrnl_entry);
  ~ StorHvJournalEntryLock();

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
    StorHvJournal(unsigned int max_jrnl_entries);
    ~StorHvJournal();

    void lock();
    void unlock();

    void pushPendingDltTrans(fds_uint32_t transId);
    PendingDltQueue popAllDltTrans();

    StorHvJournalEntry *get_journal_entry(fds_uint32_t trans_id);
    fds_uint32_t get_trans_id_for_block(fds_uint64_t block_offset);  // Legacy block
    // TODO(Andrew): Don't pass a non-const ref, make a shared ptr
    // TODO(Andrew): Don't require blob offset...using the whole blob
    fds_uint32_t get_trans_id_for_blob(const std::string& blobName,
                                       fds_uint64_t blobOffset,
                                       bool& trans_in_progress);
    void release_trans_id(unsigned int trans_id);  // Legacy block
    void releaseTransId(fds_uint32_t transId);
    template<typename Rep, typename Period>
    bool schedule(FdsTimerTaskPtr& task,
                  const std::chrono::duration<Rep, Period>& interval) {	
        return ioTimer->schedule(task, interval);
    }

    long microsecSinceCtime(boost::posix_time::ptime timestamp) {
        boost::posix_time::time_duration elapsed = timestamp - ctime;
        return elapsed.total_microseconds();
    }
};
} // namespace fds

#endif // __STOR_HV_JRNL_H_
