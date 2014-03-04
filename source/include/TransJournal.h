/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef _TRANS_JOURNAL_H_
#define _TRANS_JOURNAL_H_

#include <queue>
#include <deque>
#include <unordered_map>
#include <functional>
#include <concurrency/Mutex.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread/condition.hpp>
#include "fds_types.h"
#include <qos_ctrl.h>

#define  FDS_READ_WRITE_LOG_ENTRIES 	10*1024


using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

namespace fds {

class TransJournalUt;
typedef unsigned int TransJournalId;

template<typename KeyT, typename JEntryT>
class TransJournal {
public:
  friend class TransJournalUt;
  const static TransJournalId INVALID_TRANS_ID = (TransJournalId) -1;

  struct TransJournalKeyInfo {
    TransJournalKeyInfo(FDS_IOType *in_io, TransJournalId in_trans_id) {
      io = in_io;
      trans_id = in_trans_id;
    }
    TransJournalKeyInfo(FDS_IOType *in_io) {
      io = in_io;
      trans_id = INVALID_TRANS_ID;
    }

    FDS_IOType *io;
    TransJournalId trans_id;
  };

  typedef std::unordered_map<KeyT, std::list<TransJournalKeyInfo> > KeyToTransInfoTable;

public:
 	TransJournal(FDS_QoSControl *qos_controller, fds_log *log);
	TransJournal(unsigned int max_jrnl_entries, FDS_QoSControl *qos_controller, fds_log *log);
 	~TransJournal();

 	Error create_transaction(const KeyT& key, FDS_IOType *io,
 	        TransJournalId &trans_id,
 	        std::function<void(TransJournalId)> cb);

 	JEntryT* get_transaction(const TransJournalId &trans_id);
 	void release_transaction(const TransJournalId &trans_id);

 	uint32_t get_active_cnt() {return _active_cnt;}
 	uint32_t get_pending_cnt() {return _pending_cnt;}

private:
 	Error _assign_transaction_to_key(const KeyT& key,
 	    FDS_IOType *io, TransJournalId &trans_id);

private:
  fds_mutex *_jrnl_tbl_mutex;
  JEntryT  *_rwlog_tbl;
  KeyToTransInfoTable _key_to_transinfo_tbl;
  std::deque<unsigned int> _free_trans_ids;
  unsigned int _max_journal_entries;

  FDS_QoSControl *_qos_controller;
  fds_log *_log;
  boost::posix_time::ptime _ctime; /* time the journal was created */

  /* Counters */
  uint32_t _active_cnt;
  uint32_t _pending_cnt;
  uint32_t _rescheduled_cnt;
};

class ObjectIdJrnlEntry {
public:
	ObjectIdJrnlEntry();
	void init(unsigned int transid, TransJournal<ObjectID, ObjectIdJrnlEntry> *jrnlTbl);
	void reset();

	void set_active(bool bActive);
	bool is_active();

	void set_key(const ObjectID &key);
	ObjectID get_key();

	void set_fdsio(FDS_IOType *fdsio);
        FDSP_MsgHdrTypePtr& getMsgHdr() { 
             return msg_hdr;
        }
        void setMsgHdr(FDSP_MsgHdrTypePtr msghdr) { 
             msg_hdr = msghdr;
        }
	FDS_IOType* get_fdsio();

	unsigned int get_transid();
private:
	fds_mutex _mutex;
	unsigned int _transid;
	TransJournal<ObjectID, ObjectIdJrnlEntry> *_jrnlTbl;
	ObjectID _key;
	bool _active;
	FDS_IOType *_fdsio;
        FDSP_MsgHdrTypePtr msg_hdr;
};

} // namespace fds
#endif // _TRANS_JOURNAL_H_
