#include <stdexcept>

#include <fds_err.h>
#include <fds_types.h>
#include <fds_assert.h>
#include <TransJournal.h>


namespace fds {

ObjectIdJrnlEntry::ObjectIdJrnlEntry()
: _mutex("ObjectIdJrnlEntry")
{
}

void ObjectIdJrnlEntry::init(unsigned int transid, TransJournal<ObjectID, ObjectIdJrnlEntry> *jrnlTbl)
{
  _transid = transid;
  _jrnlTbl = jrnlTbl;

	_active = false;
	memset(&_key, 0, sizeof(_key));
	_fdsio = NULL;
}


void ObjectIdJrnlEntry::reset()
{
	memset(&_key, 0, sizeof(_key));
	_active = false;
	_fdsio = NULL;
}

void ObjectIdJrnlEntry::set_active(bool active)
{
	_active = active;
}

bool ObjectIdJrnlEntry::is_active()
{
	return _active;
}

void ObjectIdJrnlEntry::set_key(const ObjectID &key)
{
	_key = key;
}


ObjectID ObjectIdJrnlEntry::get_key()
{
	return _key;
}

void ObjectIdJrnlEntry::set_fdsio(FDS_IOType *fdsio)
{
  _fdsio = fdsio;
}


FDS_IOType* ObjectIdJrnlEntry::get_fdsio()
{
  return _fdsio;
}

unsigned int ObjectIdJrnlEntry::get_transid()
{
	return _transid;
}

template<typename KeyT, typename JEntryT>
TransJournal<KeyT, JEntryT>::TransJournal(unsigned int max_jrnl_entries, FDS_QoSControl *qos_controller, fds_log *log)
//  : ioTimer(new IceUtil::Timer())
: _qos_controller(qos_controller),
  _log(log),
  _pending_cnt(0),
  _active_cnt(0)
{
	unsigned int i =0;

	_max_journal_entries = max_jrnl_entries;

	_rwlog_tbl = new JEntryT[_max_journal_entries];

	for (i = 0; i < _max_journal_entries; i++) {
          _rwlog_tbl[i].init(i, this);
	  _free_trans_ids.push(i);
	}

	_jrnl_tbl_mutex = new fds_mutex("Journal Table Mutex");

	_ctime = boost::posix_time::microsec_clock::universal_time();
	return;
}

template<typename KeyT, typename JEntryT>
TransJournal<KeyT, JEntryT>::TransJournal(FDS_QoSControl *qos_controller, fds_log *log)
  : TransJournal(FDS_READ_WRITE_LOG_ENTRIES, qos_controller, log)
{
}

template<typename KeyT, typename JEntryT>
TransJournal<KeyT, JEntryT>::~TransJournal()
{
  delete _jrnl_tbl_mutex;
//  ioTimer->destroy();
}

template<typename KeyT, typename JEntryT>
Error TransJournal<KeyT, JEntryT>::
_assign_transaction_to_key(const KeyT& key, FDS_IOType *io, TransJournalId &trans_id)
{
  if (_free_trans_ids.empty()) {
    fds_assert(!"out of trans ids");
    return ERR_TRANS_JOURNAL_OUT_OF_IDS;
  }
  trans_id = _free_trans_ids.front();
  _free_trans_ids.pop();

  fds_assert(!_rwlog_tbl[trans_id].is_active());
  _rwlog_tbl[trans_id].set_fdsio(io);
  _rwlog_tbl[trans_id].set_key(key);
  _rwlog_tbl[trans_id].set_active(true);

  _key_to_transinfo_tbl[key].push_front(TransJournalKeyInfo(io, trans_id));

  _active_cnt++;
  return ERR_OK;
}

template<typename KeyT, typename JEntryT>
Error TransJournal<KeyT, JEntryT>::
create_transaction(const KeyT& key, FDS_IOType *io, TransJournalId &trans_id)
{
  fds_mutex::scoped_lock lock(*_jrnl_tbl_mutex);

  typename KeyToTransInfoTable::iterator itr = _key_to_transinfo_tbl.find(key);

  if (itr == _key_to_transinfo_tbl.end()) {
    /* No pending transactions for the given key */
    Error e = _assign_transaction_to_key(key, io, trans_id);
    return e;

  } else if (itr->second.front().io == io) {
    /* Request to create a transaction for existing <key,io> pair */
    TransJournalKeyInfo& ki = itr->second.front();
    fds_assert(_rwlog_tbl[ki.trans_id].is_active() &&
        _rwlog_tbl[ki.trans_id].get_key() == key);
    trans_id = ki.trans_id;
    return ERR_OK;

  } else {
    /* There is already a pending transaction for this key.  Lets enqueue */
    fds_assert(_key_to_transinfo_tbl[key].size() > 0);
    _key_to_transinfo_tbl[key].push_back(TransJournalKeyInfo(io));
    _pending_cnt++;
    FDS_PLOG(_log) << "Adding to pending transactions.  key: " << key.ToString();
    return ERR_TRANS_JOURNAL_REQUEST_QUEUED;
  }
}

template<typename KeyT, typename JEntryT>
JEntryT* TransJournal<KeyT, JEntryT>::
get_transaction(const TransJournalId &trans_id)
{
  fds_mutex::scoped_lock lock(*_jrnl_tbl_mutex);
  fds_assert(_rwlog_tbl[trans_id].is_active());
  return &_rwlog_tbl[trans_id];
}

template<typename KeyT, typename JEntryT>
void TransJournal<KeyT, JEntryT>::
release_transaction(TransJournalId &trans_id)
{
  fds_mutex::scoped_lock lock(*_jrnl_tbl_mutex);
  KeyT key = _rwlog_tbl[trans_id].get_key();
  typename KeyToTransInfoTable::iterator pending_qitr = _key_to_transinfo_tbl.find(key);

  /* Free up the transaction associated with trans_id */
  fds_assert(_rwlog_tbl[trans_id].is_active() &&
             _rwlog_tbl[trans_id].get_fdsio() == pending_qitr->second.front().io);
  _rwlog_tbl[trans_id].reset();
  _free_trans_ids.push(trans_id);
  _active_cnt--;
  pending_qitr->second.pop_front();

  /* schedule any pending transactions for the key */
  if (pending_qitr->second.size() > 0) {
    Error e;
    TransJournalId new_trans_id;
    FDS_IOType *io = pending_qitr->second.front().io;
    fds_assert(pending_qitr->second.front().trans_id == INVALID_TRANS_ID);
    pending_qitr->second.pop_front();
    _pending_cnt--;
    FDS_PLOG(_log) << "Removing from pending transactions.  key: " << key.ToString();

    e = _assign_transaction_to_key(key, io, new_trans_id);
    fds_assert(e.ok());

    // todo: It's a good idea to let go off _jrnl_tbl_mutex here.
    e = _qos_controller->processIO(io);
    if (!e.ok() ) {
      fds_assert(!"Failed to process io");
    }
  }

  if (pending_qitr->second.size() == 0) {
    _key_to_transinfo_tbl.erase(pending_qitr);
  }
}

template class TransJournal<ObjectID, ObjectIdJrnlEntry>;
} // namespace fds
