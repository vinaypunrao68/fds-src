#ifndef _TRANS_JOURNAL_H_
#define _TRANS_JOURNAL_H_

#include <queue>
#include <unordered_map>
#include <concurrency/Mutex.h>
#include <boost/thread/condition.hpp>
#include "fds_types.h"

#define  FDS_READ_WRITE_LOG_ENTRIES 	10*1024


using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;
using namespace Ice;

namespace fds {

template<typename KeyT, typename JEntryT>
class TransJournal;

using namespace IceUtil;

template <typename JEntryT>
class  TransJournalEntryLock {

 private:
  JEntryT *jrnl_e;

 public:
  TransJournalEntryLock(JEntryT *jrnl_entry);
  ~ TransJournalEntryLock();

};

template<typename KeyT, typename JEntryT>
class TransJournal {
private:

  fds_mutex *jrnl_tbl_mutex;
  JEntryT  *rwlog_tbl;
  std::unordered_map<KeyT, unsigned int> key_to_jrnl_idx;
  std::queue<unsigned int> free_trans_ids;
  unsigned int max_journal_entries;

  unsigned int get_free_trans_id();
  void return_free_trans_id(unsigned int trans_id);

  boost::posix_time::ptime ctime; /* time the journal was created */

public:
 	TransJournal();
	TransJournal(unsigned int max_jrnl_entries);
 	~TransJournal();

	void lock();
	void unlock();

	JEntryT *get_journal_entry(int trans_id);
	unsigned int get_trans_id_for_key(const KeyT &key);
	void release_trans_id(unsigned int trans_id);
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
	void init(unsigned int transid, TransJournal<ObjectID, ObjectIdJrnlEntry> *jrnlTbl);
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
	TransJournal<ObjectID, ObjectIdJrnlEntry> *_jrnlTbl;

};

} // namespace fds
#endif // _TRANS_JOURNAL_H_
