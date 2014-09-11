/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdexcept>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_assert.h>
#include <TransJournal.h>
#include "PerfTrace.h"

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
TransJournal<KeyT, JEntryT>::TransJournal(unsigned int max_jrnl_entries,
        FDS_QoSControl *qos_controller, fds_log *log)
//  : ioTimer(new IceUtil::Timer())
: _qos_controller(qos_controller),
  _log(log),
  _pending_cnt(0),
  _active_cnt(0),
  _rescheduled_cnt(0)
{
	unsigned int i =0;

	_max_journal_entries = max_jrnl_entries;

	_rwlog_tbl = new JEntryT[_max_journal_entries];

	for (i = 0; i < _max_journal_entries; i++) {
          _rwlog_tbl[i].init(i, this);
	  _free_trans_ids.push_back(i);
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
  _free_trans_ids.pop_front();

  fds_assert(trans_id < _max_journal_entries);
  fds_assert(!_rwlog_tbl[trans_id].is_active());
  _rwlog_tbl[trans_id].set_fdsio(io);
  _rwlog_tbl[trans_id].set_key(key);

  _key_to_transinfo_tbl[key].push_back(TransJournalKeyInfo(io, trans_id));

  return ERR_OK;
}

template<typename KeyT, typename JEntryT>
Error TransJournal<KeyT, JEntryT>::
create_transaction(const KeyT& key, FDS_IOType *io, TransJournalId &trans_id,
        std::function<void(TransJournalId)> cb)
{
  fds_mutex::scoped_lock lock(*_jrnl_tbl_mutex);

  typename KeyToTransInfoTable::iterator itr = _key_to_transinfo_tbl.find(key);
  bool new_transaction = (itr == _key_to_transinfo_tbl.end());
  Error e = _assign_transaction_to_key(key, io, trans_id);
  if (e != ERR_OK) {
      LOGERROR << "Error assigning a transaction id.  key : " <<  key.ToString()
              << " active: " << _active_cnt << " pending: " << _pending_cnt;
      return e;
  }

  if (new_transaction) {
      _rwlog_tbl[trans_id].set_active(true);
      _active_cnt++;
      LOGDEBUG << "New transaction.  key: " << key.ToString() << " id: " << trans_id
              << " active: " << _active_cnt << " pending: " << _pending_cnt;
  } else {
      PerfTracer::tracePointBegin(io->opTransactionWaitCtx);

      _rwlog_tbl[trans_id].set_active(false);
      _pending_cnt++;
      LOGDEBUG << "Queued transaction.  key: " << key.ToString() << " id: " << trans_id
              << " active: " << _active_cnt << " pending: " << _pending_cnt;
      e = ERR_TRANS_JOURNAL_REQUEST_QUEUED;
  }
  cb(trans_id);
  return e;
}

template<typename KeyT, typename JEntryT>
JEntryT* TransJournal<KeyT, JEntryT>::
get_transaction(const TransJournalId &trans_id)
{
  fds_mutex::scoped_lock lock(*_jrnl_tbl_mutex);
  return &_rwlog_tbl[trans_id];
}

template<typename KeyT, typename JEntryT>
JEntryT* TransJournal<KeyT, JEntryT>::
get_transaction_nolock(const TransJournalId &trans_id)
{
  return &_rwlog_tbl[trans_id];
}

template<typename KeyT, typename JEntryT>
void TransJournal<KeyT, JEntryT>::
release_transaction(const TransJournalId &trans_id)
{
    _jrnl_tbl_mutex->lock();
    KeyT key = _rwlog_tbl[trans_id].get_key();

    typename KeyToTransInfoTable::iterator pending_qitr = _key_to_transinfo_tbl.find(key);

    /* Free up the transaction associated with trans_id */
    fds_assert(pending_qitr != _key_to_transinfo_tbl.end());
    fds_assert(_active_cnt > 0 &&
            _rwlog_tbl[trans_id].is_active() &&
            _rwlog_tbl[trans_id].get_fdsio() == pending_qitr->second.front().io);
    fds_assert(trans_id < _max_journal_entries);

    _rwlog_tbl[trans_id].reset();
    _free_trans_ids.push_back(trans_id);
    _active_cnt--;
    pending_qitr->second.pop_front();

    LOGDEBUG << "Release transaction.  key: " << key.ToString() << " id: " << trans_id
            << " active: " << _active_cnt << " pending: " << _pending_cnt;

    /* schedule any pending transactions for the key */
    if (pending_qitr->second.size() > 0) {
        Error e;
        FDS_IOType *io = pending_qitr->second.front().io;
        TransJournalId new_trans_id = pending_qitr->second.front().trans_id;
        fds_assert(_pending_cnt > 0 &&
                io != nullptr &&
                new_trans_id != INVALID_TRANS_ID &&
                !_rwlog_tbl[new_trans_id].is_active());

        PerfTracer::tracePointEnd(io->opTransactionWaitCtx);

        _active_cnt++;
        _rwlog_tbl[new_trans_id].set_active(true);
        _pending_cnt--;
        _rescheduled_cnt++;

        LOGDEBUG << "Scheduling queued transaction.  key: " <<  key.ToString()
                                  << " id: " << new_trans_id
                                  << " active: " << _active_cnt << " pending: " << _pending_cnt;
        /* Letting to off the lock before calling into enqueueIO */
        _jrnl_tbl_mutex->unlock();

        e = _qos_controller->enqueueIO(io->io_vol_id, io);
        if (e != ERR_OK) {
            /* NOTE: Here we are unable to reply back to the caller.  We expect
             * the caller time out
             */
            PerfTracer::tracePointEnd(io->opReqLatencyCtx);
            PerfTracer::incr(io->opReqFailedPerfEventType, io->io_vol_id, io->perfNameStr);

            LOGERROR << "Failed to enque io.  Type: " << io->io_type
                    << " Req Id: " << io->io_req_id;
            fds_assert(!"Failed to enqueue io");
        }
        return;
    } else {
        _key_to_transinfo_tbl.erase(pending_qitr);
    }
    _jrnl_tbl_mutex->unlock();
}

template class TransJournal<ObjectID, ObjectIdJrnlEntry>;
} // namespace fds
