#include <stdexcept>

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>

#include <fdsp/FDSP.h>
#include <fds_err.h>
#include <fds_types.h>
#include <TransJournal.h>

using namespace IceUtil;

namespace fds {

ObjectIdJrnlEntry::ObjectIdJrnlEntry()
: _mutex("ObjectIdJrnlEntry")
{
}

void ObjectIdJrnlEntry::init(unsigned int transid, TransJournal<ObjectID, ObjectIdJrnlEntry> *jrnlTbl)
{
	_active = false;
	memset(&_key, sizeof(_key), 0);
	_transid = transid;
	_jrnlTbl = jrnlTbl;
}

void ObjectIdJrnlEntry::lock()
{
	_mutex.lock();
}

void ObjectIdJrnlEntry::unlock()
{
	_mutex.unlock();
}

fds_mutex* ObjectIdJrnlEntry::getLock()
{
	return &_mutex;
}

boost::condition* ObjectIdJrnlEntry::getCond()
{
	return &_condition;
}

void ObjectIdJrnlEntry::reset()
{
	memset(&_key, sizeof(_key), 0);
	_active = false;
}

void ObjectIdJrnlEntry::setActive(bool active)
{
	_active = active;
}

bool ObjectIdJrnlEntry::isActive()
{
	return _active;
}

void ObjectIdJrnlEntry::setJournalKey(const ObjectID &key)
{
	_key = key;
}


ObjectID ObjectIdJrnlEntry::getJournalKey()
{
	return _key;
}

template <typename JEntryT>
TransJournalEntryLock<JEntryT>::TransJournalEntryLock(JEntryT *jrnl_entry)
{
    jrnl_e = jrnl_entry;
    jrnl_e->lock();
}

template <typename JEntryT>
TransJournalEntryLock<JEntryT>::~ TransJournalEntryLock()
{
  jrnl_e->unlock();
}


template<typename KeyT, typename JEntryT>
TransJournal<KeyT, JEntryT>::TransJournal(unsigned int max_jrnl_entries)
//  : ioTimer(new IceUtil::Timer())
{
	unsigned int i =0;

	max_journal_entries = max_jrnl_entries;

	rwlog_tbl = new JEntryT[max_journal_entries];

	for (i = 0; i < max_journal_entries; i++) {
          rwlog_tbl[i].init(i, this);
	  free_trans_ids.push(i);
	}

	jrnl_tbl_mutex = new fds_mutex("Journal Table Mutex");

	ctime = boost::posix_time::microsec_clock::universal_time();
	// printf("Created journal table lock %p for Journal Table %p \n", jrnl_tbl_mutex, this);

	return;
}

template<typename KeyT, typename JEntryT>
TransJournal<KeyT, JEntryT>::TransJournal()
  : TransJournal(FDS_READ_WRITE_LOG_ENTRIES)
{
}

template<typename KeyT, typename JEntryT>
TransJournal<KeyT, JEntryT>::~TransJournal()
{
  delete jrnl_tbl_mutex;
//  ioTimer->destroy();
}

template<typename KeyT, typename JEntryT>
void TransJournal<KeyT, JEntryT>::lock()
{
  // cout << "Acquiring journal table lock" << endl;
  jrnl_tbl_mutex->lock();
  // cout << "Acquired journal table lock" << endl;
}

template<typename KeyT, typename JEntryT>
void TransJournal<KeyT, JEntryT>::unlock()
{
  jrnl_tbl_mutex->unlock();
  // cout << "Released journal table lock"<< endl;
}

// Caller must have acquired the Journal Table Write Lock before invoking this.
template<typename KeyT, typename JEntryT>
unsigned int TransJournal<KeyT, JEntryT>::get_free_trans_id() {

  unsigned int trans_id;

  if (free_trans_ids.empty()) {
    // Ideally we should block
    throw "Too many outstanding transactions. Transaction table full";
  }
  trans_id = free_trans_ids.front();
  free_trans_ids.pop();
  return (trans_id);

}

// Caller must have acquired the Journal Table Write Lock before invoking this.
template<typename KeyT, typename JEntryT>
void TransJournal<KeyT, JEntryT>::return_free_trans_id(unsigned int trans_id) {

  free_trans_ids.push(trans_id);

}

// The function returns a new transaction id (that is currently unused) if one is not in use for the block offset.
// If there is one for the passed offset, that trans_id is returned.
// Caller must remember to acquire the lock for the journal entry after this call, before using it.
template<typename KeyT, typename JEntryT>
unsigned int TransJournal<KeyT, JEntryT>::get_trans_id_for_key(const KeyT &key)
{
  unsigned int trans_id;

  lock();
  try{
    trans_id = key_to_jrnl_idx.at(key);
    if (rwlog_tbl[trans_id].getJournalKey() != key) {
      unlock();
      throw "Corrupt journal table, block offsets do not match";
    }
    unlock();
  }
  catch (const std::out_of_range& err) {
    trans_id = get_free_trans_id();
    key_to_jrnl_idx[key] = trans_id;
    unlock();
    TransJournalEntryLock<JEntryT> je_lock(&rwlog_tbl[trans_id]);
    if (rwlog_tbl[trans_id].isActive()) {
      throw "Corrupt journal table, allocated transaction is Active\n";
    }
    rwlog_tbl[trans_id].setJournalKey(key);
  }
//   FDS_PLOG(storHvisor->GetLog()) << " TransJournal:" << "IO-XID:" << trans_id <<" - Assigned transaction id " << trans_id << " for block " << block_offset;
  return trans_id;
}


// Caller should hold the lock to the transaction before calling this.
// And then release the lock after returning from this call.
template<typename KeyT, typename JEntryT>
void TransJournal<KeyT, JEntryT>::release_trans_id(unsigned int trans_id)
{

  KeyT key;

  key = rwlog_tbl[trans_id].getJournalKey();

  lock();

  key_to_jrnl_idx.erase(key);
  rwlog_tbl[trans_id].reset();
//  ioTimer->cancel(rwlog_tbl[trans_id].ioTimerTask);
  return_free_trans_id(trans_id);

  unlock();

//   FDS_PLOG(storHvisor->GetLog()) << " TransJournal:" << "IO-XID:" << trans_id <<  " - Released transaction id  " << trans_id ;

}

template<typename KeyT, typename JEntryT>
JEntryT *TransJournal<KeyT, JEntryT>::get_journal_entry(int trans_id) {

  JEntryT *jrnl_e = &rwlog_tbl[trans_id];
  return jrnl_e;

}


// Marks the journal entry identified by key to be in use.  If the journal entry
// is already is use the calling thread will wait until journal entry is released.
template<typename KeyT, typename JEntryT>
JEntryT* TransJournal<KeyT, JEntryT>::set_journal_entry_in_use(const KeyT& key)
{
	/* Get transaction */
	unsigned int trans_id = get_trans_id_for_key(key);
	JEntryT *jrnl_e = &rwlog_tbl[trans_id];
	jrnl_e->lock();
	while (jrnl_e->isActive()) {
		jrnl_e->getCond()->wait(*jrnl_e->getLock());
	}
	jrnl_e->setActive(true);
	jrnl_e->unlock();

	return jrnl_e;
}

// Releases the in use journal entry and notifies any waiting thread
template<typename KeyT, typename JEntryT>
void TransJournal<KeyT, JEntryT>::release_journal_entry_with_notify(JEntryT *jrnl_e)
{
	jrnl_e->lock();
	assert(jrnl_e->isActive());
	jrnl_e->reset();
	jrnl_e->unlock();
	jrnl_e->getCond()->notify_one();
}

template class TransJournal<ObjectID, ObjectIdJrnlEntry>;
} // namespace fds
