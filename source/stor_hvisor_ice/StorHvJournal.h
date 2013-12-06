#ifndef __STOR_HV_JRNL_H_
#define __STOR_HV_JRNL_H_

#include <queue>
#include <unordered_map>
#include <concurrency/Mutex.h>
#include "fds_types.h"
#include "StorHvisorCPP.h"

#define  FDS_TRANS_EMPTY                0x00
#define  FDS_TRANS_OPEN                 0x1
#define  FDS_TRANS_OPENED               0x2
#define  FDS_TRANS_COMMITTED            0x3
#define  FDS_TRANS_SYNCED               0x4
#define  FDS_TRANS_DONE                 0x5
#define  FDS_TRANS_VCAT_QUERY_PENDING   0x6
#define  FDS_TRANS_GET_OBJ	        0x7
#define  FDS_TRANS_DEL_OBJ	        0x8
#define  FDS_TRANS_GET_BUCKET           0x9
#define  FDS_TRANS_BUCKET_STATS         0xA

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
using namespace Ice;

namespace fds {

class StorHvJournal;
class StorHvJournalEntry;

using namespace IceUtil;
class StorHvIoTimerTask : public IceUtil::TimerTask {
public:
 StorHvJournalEntry *jrnlEntry;
 StorHvJournal      *jrnlTbl;

  void runTimerTask();
  StorHvIoTimerTask(StorHvJournalEntry *jrnl_entry, StorHvJournal *jrnl_tbl) {
     jrnlEntry = jrnl_entry;
     jrnlTbl = jrnl_tbl;
  }
};

class   StorHvJournalEntry {

 private:
  fds_mutex *je_mutex;

public:
  StorHvJournalEntry();
  ~StorHvJournalEntry();

  void init(unsigned int transid, StorHvJournal* jrnl_tbl);
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

  IceUtil::TimerTaskPtr ioTimerTask;
  bool   is_in_use;
  unsigned int trans_id;
  short  replc_cnt;
  short  sm_ack_cnt;
  short  dm_ack_cnt;
  short  dm_commit_cnt;
  short  trans_state;
  unsigned short incarnation_number;
  fds_io_op_t   op;
  FDS_ObjectIdType data_obj_id;
  int      data_obj_len;

  std::string blobName;
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
  int      lt_flag;
  int      st_flag;
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
  IceUtil::TimerPtr ioTimer;

  boost::posix_time::ptime ctime; /* time the journal was created */

public:
 	StorHvJournal();
	StorHvJournal(unsigned int max_jrnl_entries);
 	~StorHvJournal();

	void lock();
	void unlock();

	StorHvJournalEntry *get_journal_entry(fds_uint32_t trans_id);
	fds_uint32_t get_trans_id_for_block(fds_uint64_t block_offset);  // Legacy block
        fds_uint32_t get_trans_id_for_blob(const std::string& blobName,
                                          fds_uint64_t blobOffset);
	void release_trans_id(unsigned int trans_id);  // Legacy block
        void releaseTransId(fds_uint32_t transId);
        void schedule(const TimerTaskPtr& task, const IceUtil::Time& interval) {
             ioTimer->schedule(task, interval);
        }
	long microsecSinceCtime(boost::posix_time::ptime timestamp) {
	  boost::posix_time::time_duration elapsed = timestamp - ctime;
	  return elapsed.total_microseconds();
	}

};


} // namespace fds

#endif // __STOR_HV_JRNL_H_
