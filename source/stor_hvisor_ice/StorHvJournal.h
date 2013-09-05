#ifndef __STOR_HV_JRNL_H_
#define __STOR_HV_JRNL_H_

#include <queue>
#include <unordered_map>
#include "util/concurrency/Mutex.h"

#define  FDS_TRANS_EMPTY                0x00
#define  FDS_TRANS_OPEN                 0x1
#define  FDS_TRANS_OPENED               0x2
#define  FDS_TRANS_COMMITTED            0x3
#define  FDS_TRANS_SYNCED               0x4
#define  FDS_TRANS_DONE                 0x5
#define  FDS_TRANS_VCAT_QUERY_PENDING   0x6
#define  FDS_TRANS_GET_OBJ	        0x7

#define  FDS_MIN_ACK                    1
#define  FDS_CLS_ACK                    0
#define  FDS_SET_ACK                    1

#define  FDS_COMMIT_MSG_SENT            1
#define  FDS_COMMIT_MSG_ACKED           2


class FDSP_IpNode {
public:
        long  ipAddr;
        short  ack_status;
        short  commit_status;
};

enum FDS_IO_Type {
   FDS_IO_READ,
   FDS_IO_WRITE,
   FDS_IO_REDIR_READ,
   FDS_IO_OFFSET_WRITE
};

#define  FDS_MAX_DM_NODES_PER_CLST      16
#define  FDS_MAX_SM_NODES_PER_CLST      16
#define  FDS_READ_WRITE_LOG_ENTRIES 	10*1024

typedef void (*complete_req_cb_t)(void *arg1, void *arg2, fbd_request_t *treq, int res);

using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

namespace fds {

class   StorHvJournalEntry {

 private:
  fds_mutex *je_mutex;

public:
  StorHvJournalEntry();
  ~StorHvJournalEntry();
  void reset();
  void setActive();
  void setInactive();
  bool isActive();
  void lock();
  void unlock();
  int fds_set_dmack_status( int ipAddr);
  int fds_set_dm_commit_status( int ipAddr);
  int fds_set_smack_status( int ipAddr);
  void fbd_complete_req(fbd_request_t *req, int status);

  bool   is_in_use;
  unsigned int trans_id;
         short  replc_cnt;
        short  sm_ack_cnt;
        short  dm_ack_cnt;
        short  dm_commit_cnt;
        short  trans_state;
	unsigned short incarnation_number;
        FDS_IO_Type   op;
        FDS_ObjectIdType data_obj_id;
        int      data_obj_len;
	unsigned int block_offset;
        void     *fbd_ptr;
        void     *read_ctx;
        void     *write_ctx;
	complete_req_cb_t comp_req;
	void 	*comp_arg1;
	void 	*comp_arg2;
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
  std::unordered_map<unsigned int, unsigned int> block_to_jrnl_idx;
  std::queue<unsigned int> free_trans_ids;
  unsigned int max_journal_entries;

  unsigned int get_free_trans_id();
  void return_free_trans_id(unsigned int trans_id);

public:
 	StorHvJournal();
	StorHvJournal(unsigned int max_jrnl_entries);
 	~StorHvJournal();

	void lock();
	void unlock();

	StorHvJournalEntry *get_journal_entry(int trans_id);
	unsigned int get_trans_id_for_block(unsigned int block_offset);
	void release_trans_id(unsigned int trans_id);

};

} // namespace fds

#endif // __STOR_HV_JRNL_H_
