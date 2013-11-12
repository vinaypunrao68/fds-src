#ifndef __STOR_HV_JRNL_EX_H_
#define __STOR_HV_JRNL_EX_H_

#include <queue>
#include <unordered_map>
#include <concurrency/Mutex.h>
#include <boost/thread/condition.hpp>
#include "fds_types.h"

//#define  FDS_TRANS_EMPTY                0x00
//#define  FDS_TRANS_OPEN                 0x1
//#define  FDS_TRANS_OPENED               0x2
//#define  FDS_TRANS_COMMITTED            0x3
//#define  FDS_TRANS_SYNCED               0x4
//#define  FDS_TRANS_DONE                 0x5
//#define  FDS_TRANS_VCAT_QUERY_PENDING   0x6
//#define  FDS_TRANS_GET_OBJ	        0x7
//
//#define  FDS_MIN_ACK                    1
//#define  FDS_CLS_ACK                    0
//#define  FDS_SET_ACK                    1
//
//#define  FDS_COMMIT_MSG_SENT            1
//#define  FDS_COMMIT_MSG_ACKED           2


//class FDSP_IpNode {
//public:
//  long         ipAddr;
//  fds_uint32_t port;
//  short        ack_status;
//  short        commit_status;
//};
//
//#define  FDS_MAX_DM_NODES_PER_CLST      16
//#define  FDS_MAX_SM_NODES_PER_CLST      16
#define  FDS_READ_WRITE_LOG_ENTRIES 	10*1024


using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;
using namespace Ice;

namespace fds {

template<typename KeyT, typename JEntryT>
class StorHvJournalEx;

using namespace IceUtil;
//class StorHvIoTimerTask : public IceUtil::TimerTask {
//public:
// StorHvJournalEntryEx *jrnlEntry;
// StorHvJournalEx      *jrnlTbl;
//
//  void runTimerTask();
//  StorHvIoTimerTask(StorHvJournalEntryEx *jrnl_entry, StorHvJournalEx *jrnl_tbl) {
//     jrnlEntry = jrnl_entry;
//     jrnlTbl = jrnl_tbl;
//  }
//};

//class   StorHvJournalEntryEx {
//
// private:
//  fds_mutex *je_mutex;
//
//public:
//  StorHvJournalEntryEx();
//  ~StorHvJournalEntryEx();
//
//  void init(unsigned int transid, StorHvJournalEx* jrnl_tbl);
//  void reset();
//  void setActive();
//  void setInactive();
//  bool isActive();
//  void lock();
//  void unlock();
//  int fds_set_dmack_status(int ipAddr,
//                           fds_uint32_t port);
//  int fds_set_dm_commit_status(int ipAddr,
//                               fds_uint32_t port);
//  int fds_set_smack_status(int ipAddr,
//                           fds_uint32_t port);
//  void fbd_complete_req(fbd_request_t *req, int status);
//  void fbd_process_req_timeout();
//
//  IceUtil::TimerTaskPtr ioTimerTask;
//  bool   is_in_use;
//  unsigned int trans_id;
//  short  replc_cnt;
//  short  sm_ack_cnt;
//  short  dm_ack_cnt;
//  short  dm_commit_cnt;
//  short  trans_state;
//  unsigned short incarnation_number;
//  fds_io_op_t   op;
//  FDS_ObjectIdType data_obj_id;
//  int      data_obj_len;
//  unsigned int block_offset;
//  void     *fbd_ptr;
//  void     *read_ctx;
//  void     *write_ctx;
//  blkdev_complete_req_cb_t comp_req;
//  void 	*comp_arg1;
//  void 	*comp_arg2;
//  FDS_IOType             *io;
//  FDSP_MsgHdrTypePtr     sm_msg;
//  FDSP_MsgHdrTypePtr     dm_msg;
//  int      lt_flag;
//  int      st_flag;
//  short    num_dm_nodes;
//  FDSP_IpNode    dm_ack[FDS_MAX_DM_NODES_PER_CLST];
//  short    num_sm_nodes;
//  FDSP_IpNode    sm_ack[FDS_MAX_SM_NODES_PER_CLST];
//};
template <typename JEntryT>
class  StorHvJournalEntryLockEx {

 private:
  JEntryT *jrnl_e;

 public:
  StorHvJournalEntryLockEx(JEntryT *jrnl_entry);
  ~ StorHvJournalEntryLockEx();

};

template<typename KeyT, typename JEntryT>
class StorHvJournalEx {
private:

  fds_mutex *jrnl_tbl_mutex;
  JEntryT  *rwlog_tbl;
  std::unordered_map<KeyT, unsigned int> key_to_jrnl_idx;
  std::queue<unsigned int> free_trans_ids;
  unsigned int max_journal_entries;

  unsigned int get_free_trans_id();
  void return_free_trans_id(unsigned int trans_id);
//  IceUtil::TimerPtr ioTimer;

  boost::posix_time::ptime ctime; /* time the journal was created */

public:
 	StorHvJournalEx();
	StorHvJournalEx(unsigned int max_jrnl_entries);
 	~StorHvJournalEx();

	void lock();
	void unlock();

	JEntryT *get_journal_entry(int trans_id);
	unsigned int get_trans_id_for_key(const KeyT &key);
	void release_trans_id(unsigned int trans_id);
//        void schedule(const TimerTaskPtr& task, const IceUtil::Time& interval) {
//             ioTimer->schedule(task, interval);
//        }
	long microsecSinceCtime(boost::posix_time::ptime timestamp) {
	  boost::posix_time::time_duration elapsed = timestamp - ctime;
	  return elapsed.total_microseconds();
	}

	JEntryT* set_journal_entry_in_use(const KeyT& key);
	void release_journal_entry_with_notify(JEntryT *entry);
};

class ObjectIdJrnlEntry {
public:
	ObjectIdJrnlEntry();
	void init(unsigned int transid, StorHvJournalEx<ObjectID, ObjectIdJrnlEntry> *jrnlTbl);
	void lock();
	void unlock();
	fds_mutex* getLock();
	boost::condition* getCond();
	void reset();
	void setActive(bool bActive);
	bool isActive();
	void setJournalKey(const ObjectID &key);
	ObjectID getJournalKey();
private:
	ObjectID _key;
	fds_mutex _mutex;
	boost::condition _condition;
	bool _active;
	unsigned int _transid;
	StorHvJournalEx<ObjectID, ObjectIdJrnlEntry> *_jrnlTbl;

};

} // namespace fds
#endif // __STOR_HV_JRNL_H_
