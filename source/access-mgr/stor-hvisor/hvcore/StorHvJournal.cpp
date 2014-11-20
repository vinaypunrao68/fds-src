/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <algorithm>
#include <stdexcept>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_timer.h>
#include <StorHvisorNet.h>
#include <StorHvJournal.h>
#include <fds_defines.h>
#include "PerfTrace.h" //NOLINT

extern StorHvCtrl *storHvisor;

namespace fds {

std::ostream&
operator<<(ostream& os, const TxnState& state) {
    switch (state) {
        ENUMCASEOS(FDS_TRANS_EMPTY             , os);
        ENUMCASEOS(FDS_TRANS_BLOB_START        , os);
        ENUMCASEOS(FDS_TRANS_OPEN              , os);
        ENUMCASEOS(FDS_TRANS_OPENED            , os);
        ENUMCASEOS(FDS_TRANS_COMMITTED         , os);
        ENUMCASEOS(FDS_TRANS_SYNCED            , os);
        ENUMCASEOS(FDS_TRANS_DONE              , os);
        ENUMCASEOS(FDS_TRANS_VCAT_QUERY_PENDING, os);
        ENUMCASEOS(FDS_TRANS_GET_OBJ           , os);
        ENUMCASEOS(FDS_TRANS_DEL_OBJ           , os);
        ENUMCASEOS(FDS_TRANS_GET_BUCKET        , os);
        ENUMCASEOS(FDS_TRANS_BUCKET_STATS      , os);
        ENUMCASEOS(FDS_TRANS_PENDING_DLT       , os);
        default:
            os << "Unknown AM OP "
               << static_cast<fds_uint32_t>(state);
    }
    return os;
}

StorHvJournalEntry::StorHvJournalEntry() : je_mutex("Journal Entry Mutex") {
    trans_state = FDS_TRANS_EMPTY;
    replc_cnt = 0;
    sm_msg = 0;
    dm_msg = 0;
    sm_ack_cnt = 0;
    dm_ack_cnt = 0;
    nodeSeq = 0;
    is_in_use = false;

    putMsg = NULL;
    getMsg = NULL;
    delMsg = NULL;

    blobOffset = 0;
    blobName.clear();

    dmt_version = 0;
}

StorHvJournalEntry::~StorHvJournalEntry() {}

void
StorHvJournalEntry::init(unsigned int transid,
                         StorHvJournal *jrnl_tbl,
                         FdsTimer *ioTimer) {
    trans_id = transid;
    ioTimerTask.reset(new StorHvIoTimerTask(this, jrnl_tbl, ioTimer));
}

void
StorHvJournalEntry::reset() {
    trans_state = FDS_TRANS_EMPTY;
    replc_cnt = 0;
    sm_msg = 0;
    dm_msg = 0;
    sm_ack_cnt = 0;
    dm_ack_cnt = 0;
    nodeSeq = 0;
    is_in_use = false;

    putMsg = NULL;
    getMsg = NULL;
    delMsg = NULL;
}

void
StorHvJournalEntry::setActive() {
    is_in_use = true;
}

void
StorHvJournalEntry::setInactive() {
    is_in_use = false;
    trans_state = FDS_TRANS_EMPTY;
}

bool
StorHvJournalEntry::isActive() {
    return ((is_in_use) || (trans_state != FDS_TRANS_EMPTY));
}

void
StorHvJournalEntry::lock() {
    je_mutex.lock();
}

void
StorHvJournalEntry::unlock() {
    je_mutex.unlock();
}

// Caller should hold the lock on the transaction
int
StorHvJournalEntry::fds_set_dmack_status(int ipAddr,
                                         fds_uint32_t port) {
    for (fds_uint32_t node = 0;
         node < FDS_MAX_DM_NODES_PER_CLST;
         node++) {
        if ((memcmp(&dm_ack[node].ipAddr, &ipAddr, 4) == 0) &&
            (dm_ack[node].port == port)) {
            if (dm_ack[node].ack_status < FDS_SET_ACK) {
                dm_ack_cnt++;
                LOGDEBUG << "updating the DM ACK: "
                         << (fds_uint32_t)dm_ack_cnt;
                dm_ack[node].ack_status = FDS_SET_ACK;
                return 0;
            }
        }
    }

    return 0;
}

// Caller should hold the lock on the transaction
int
StorHvJournalEntry::fds_set_dm_commit_status(int ipAddr,
                                             fds_uint32_t port) {
    for (fds_uint32_t node = 0;
         node < FDS_MAX_DM_NODES_PER_CLST;
         node++) {
        if ((memcmp(&dm_ack[node].ipAddr, &ipAddr, 4) == 0) &&
            (dm_ack[node].port == port)) {
            if (dm_ack[node].commit_status == FDS_COMMIT_MSG_SENT) {
                dm_commit_cnt++;
                dm_ack[node].commit_status = FDS_COMMIT_MSG_ACKED;
                return 0;
            }
        }
    }

    return 0;
}

// Caller should hold the lock on the transaction
// TODO(Andrew): Move the service UUIDs
int
StorHvJournalEntry::fds_set_smack_status(int ipAddr,
                                         fds_uint32_t port) {
    for (fds_uint32_t node = 0;
         node < FDS_MAX_SM_NODES_PER_CLST;
         node++) {
        /*
         * TODO: Use a request or node id here rather than
         * ip/port.
         */
        if ((memcmp((void *)&sm_ack[node].ipAddr,  // NOLINT
                    (void *)&ipAddr, 4) == 0) &&   // NOLINT
            (sm_ack[node].port == port)) {
            if (sm_ack[node].ack_status != FDS_SET_ACK) {
                sm_ack[node].ack_status = FDS_SET_ACK;
                sm_ack_cnt++;
                return 0;
            }
            // return 0;
        }
    }

    return -1;
}

StorHvJournalEntryLock::StorHvJournalEntryLock(StorHvJournalEntry *jrnl_entry) {
    jrnl_e = jrnl_entry;
    jrnl_e->lock();
}

StorHvJournalEntryLock::~StorHvJournalEntryLock() {
    jrnl_e->unlock();
}

void
StorHvJournalEntry::resumeTransaction(void) {
    Error err(ERR_OK);

    switch (op) {
        case FDS_PUT_BLOB:
        case FDS_IO_WRITE:
            LOGDEBUG << "Resuming putBlob op ID " << trans_id;
            err = storHvisor->resumePutBlob(this);
            break;
            // case FDS_GET_BLOB:
            // case FDS_IO_READ:
            // storHvisor->resumeGetBlob(this);
            // break;
            // case FDS_DELETE_BLOB:
            // storHvisor->resumeDeleteBlob(this);
            // break;
        default:
            fds_panic("Attempting to resume unknown op!");
    }

    fds_verify(err == ERR_OK);
}

StorHvJournal::StorHvJournal(unsigned int max_jrnl_entries)
        :   jrnl_tbl_mutex("Journal Table Mutex"),
            rwlog_tbl(max_jrnl_entries),
            ioTimer(new FdsTimer())
{
    unsigned int i =0;

    max_journal_entries = max_jrnl_entries;

    for (i = 0; i < max_journal_entries; i++) {
        rwlog_tbl[i].init(i, this, ioTimer);
        free_trans_ids.push(i);
    }

    ctime = boost::posix_time::microsec_clock::universal_time();

    return;
}

StorHvJournal::StorHvJournal() {
    StorHvJournal(FDS_READ_WRITE_LOG_ENTRIES);
}

StorHvJournal::~StorHvJournal() {
    ioTimer->destroy();
}

void StorHvJournal::lock() {
    jrnl_tbl_mutex.lock();
}

void StorHvJournal::unlock() {
    jrnl_tbl_mutex.unlock();
}

// Caller must have acquired the Journal Table Write Lock before invoking this.
unsigned int
StorHvJournal::get_free_trans_id() {
    unsigned int trans_id;

    if (free_trans_ids.empty()) {
        // Ideally we should block
        throw "Too many outstanding transactions. Transaction table full";
    }
    trans_id = free_trans_ids.front();
    free_trans_ids.pop();
    return trans_id;
}

// Caller must have acquired the Journal Table Write Lock before invoking this.
void
StorHvJournal::return_free_trans_id(unsigned int trans_id) {
    free_trans_ids.push(trans_id);
}

/**
 * Pushes a transaction id onto the queue of transactions pending
 * a newer DLT version.
 */
void
StorHvJournal::pushPendingDltTrans(fds_uint32_t transId) {
    lock();
    if (transDltSet.count(transId) == 0) {
        transPendingDlt.push(transId);
    }
    unlock();
}

/**
 * Pops the current list of transactions that are pending
 * a newer DLT version from the queue and returns them.
 */
PendingDltQueue
StorHvJournal::popAllDltTrans() {
    lock();
    PendingDltQueue poppedTrans;
    std::swap(poppedTrans, transPendingDlt);
    transDltSet.clear();
    unlock();
    return poppedTrans;
}

/**
 * Resumes the first (if any) of the operations
 * pending completion of this journal op.
 */
void
StorHvJournal::resumePendingTrans(fds_uint32_t trans_id) {
    // Get the journel entry ops may be waiting on
    lock();
    StorHvJournalEntry *journEntry = &rwlog_tbl[trans_id];
    if (journEntry->pendingTransactions.size() == 0) {
        // No ops are waiting
        unlock();
        return;
    }

    // Get the first waiting op
    StorHvJournalEntry *firstJrnlEntry = journEntry->pendingTransactions.front();
    journEntry->pendingTransactions.pop_front();
    // Add the blob/offset op to the journal
    std::string blobName = rwlog_tbl[trans_id].blobName;
    fds_uint64_t blobOffset = rwlog_tbl[trans_id].blobOffset;
    blob_to_jrnl_idx[blobName + ":" + std::to_string(blobOffset)] = firstJrnlEntry->trans_id;
    // Copy over all ops pending to old entry to this entry
    firstJrnlEntry->pendingTransactions = journEntry->pendingTransactions;
    unlock();
    // Start resuming the op in this thread context...
    firstJrnlEntry->resumeTransaction();
}

/**
 * Creates a new transaction ID.
 */
fds_uint32_t
StorHvJournal::create_trans_id(const std::string& blobName,
                               fds_uint64_t blobOffset) {
    fds_uint32_t trans_id;

    lock();
    trans_id = get_free_trans_id();
    unlock();
    StorHvJournalEntryLock je_lock(&rwlog_tbl[trans_id]);
    fds_verify(rwlog_tbl[trans_id].isActive() == false);

    rwlog_tbl[trans_id].blobName = blobName;
    rwlog_tbl[trans_id].blobOffset = blobOffset;

    LOGDEBUG << "Assigned trans id "
             << trans_id << " for blob "
             << blobName << " offset "
             << blobOffset;
    return trans_id;
}

// The function returns a new transaction id (that is currently unused)
// if one is not in use for the block offset.
// If there is one for the passed offset, that trans_id is returned.
// Caller must remember to acquire the lock for the journal entry after
// this call, before using it.
fds_uint32_t
StorHvJournal::get_trans_id_for_blob(const std::string& blobName,
                                     fds_uint64_t blobOffset,
                                     bool& trans_in_progress) {
    fds_uint32_t trans_id;

    trans_in_progress = false;

    lock();
    try {
        trans_id = blob_to_jrnl_idx.at(blobName + ":" + std::to_string(blobOffset));
        trans_in_progress = true;
        LOGDEBUG << "Comparing "
                 << rwlog_tbl[trans_id].blobOffset
                 << " and " << blobOffset
                 << " and "
                 << rwlog_tbl[trans_id].blobName
                 << " and " << blobName;
        unlock();
    } catch(const std::out_of_range& err) {
        /*
         * Catching this exception is our indicaction that the
         * journal entry didn't exist and we need to create it.
         */
        trans_id = get_free_trans_id();
        blob_to_jrnl_idx[blobName + ":" + std::to_string(blobOffset)] = trans_id;
        StorHvJournalEntryLock je_lock(&rwlog_tbl[trans_id]);
        fds_verify(rwlog_tbl[trans_id].isActive() == false);
        rwlog_tbl[trans_id].blobName = blobName;
        rwlog_tbl[trans_id].blobOffset = blobOffset;
        unlock();
    }
    LOGDEBUG << "Assigned trans id "
             << trans_id << " for blob "
             << blobName << " offset "
             << blobOffset;
    return trans_id;
}


// Caller should hold the lock to the transaction before calling this.
// And then release the lock after returning from this call.
void
StorHvJournal::releaseTransId(unsigned int transId) {
    std::string blobName = rwlog_tbl[transId].blobName;
    fds_uint64_t blobOffset = rwlog_tbl[transId].blobOffset;

    lock();

    blob_to_jrnl_idx.erase(blobName + ":" + std::to_string(blobOffset));

    rwlog_tbl[transId].blobOffset = 0;
    rwlog_tbl[transId].blobName.clear();
    rwlog_tbl[transId].reset();
    ioTimer->cancel(rwlog_tbl[transId].ioTimerTask);
    return_free_trans_id(transId);

    unlock();

    LOGDEBUG << "Released transaction id " << transId;
}

StorHvJournalEntry*
StorHvJournal::get_journal_entry(fds_uint32_t trans_id) {
    StorHvJournalEntry *jrnl_e = &rwlog_tbl[trans_id];
    return jrnl_e;
}


// Note: Caller holds the lock to the entry
// And takes care of releasing the transaction id to the free pool
void
StorHvJournalEntry::fbd_process_req_timeout() {
    AmRequest *amReq = static_cast<fds::AmRequest*>(io);
    fds_verify(amReq != NULL);

    fds::Error err(ERR_OK);

    fds_volid_t   volId = amReq->io_vol_id;
    LOGWARN << "Trans id" << trans_id << " timing out, responding to blob "
            << amReq->getBlobName() << " offset" << amReq->blob_offset;
    if (isActive()) {
        // try all the SM nodes and if we are not able to get the data  return error
        StorHvVolume *vol = storHvisor->vol_table->getVolume(volId);
        fds_verify(vol != NULL);  // Should not receive resp for non existant vol

        StorHvVolumeLock vol_lock(vol);
        fds_verify(vol->isValidLocked() == true);

        StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(trans_id);
        fds_verify(txn != NULL);

        fds_verify(txn->isActive() == true);  // Should not receive resp for inactive txn
        if (txn->nodeSeq < (txn->num_sm_nodes-1)) {
            txn->nodeSeq += 1;  // try all the SM nodes
            if ( txn->op == FDS_IO_READ ) {
                err = storHvisor->dispatchSmGetMsg(txn);
                fds_verify(err == ERR_OK);
            } else if (txn->op == FDS_IO_WRITE) {
                fds_panic("Put request timed out");
                /* TODO(Rao) win-340: Until we have a consistent implementation for
                 * timeouts, on first timeout error, we will crash.
                 */
                err = storHvisor->dispatchSmPutMsg(txn,
                        storHvisor->om_client->\
                        getCurrentDLT()->getNode(amReq->obj_id, txn->nodeSeq));
            }
        } else {
            // We searched all SMs, should not happen -- either blob does not exist
            // and we should find out about this earlier from vol catalog or
            // SMs rejecting IO (eg. because queues are full, etc).
            // To debug WIN-366, assert here. It still could be a case we can get out
            // like setting more throttling on AM, etc.
            LOGCRITICAL << "Timed out accessing all SMs for blob " << amReq->getBlobName()
                        << " offset " << amReq->blob_offset << " trans_id " << trans_id;
            fds_panic("Timed out accessing all SMs!");

            // fds::PerfTracer::tracePointEnd(amReq->e2eReqPerfCtx);
            storHvisor->qos_ctrl->markIODone(io);
            vol->journal_tbl->releaseTransId(trans_id);
            amReq->cbWithResult(-1);
            delete amReq;
            amReq = nullptr;
            reset();
        }
    }
}

void StorHvIoTimerTask::runTimerTask() {
    jrnlEntry->lock();
    jrnlEntry->fbd_process_req_timeout();
    jrnlEntry->unlock();
}

}  // namespace fds
