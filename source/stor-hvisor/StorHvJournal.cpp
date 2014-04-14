/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <stdexcept>
#include <fds_err.h>
#include <fds_types.h>
#include <fds_timer.h>
#include <StorHvisorNet.h>
//#include "fds_client/include/ubd.h"
#include "StorHvJournal.h"
#include <fds_defines.h>

extern StorHvCtrl *storHvisor;
namespace fds {

std::ostream& operator<<(ostream& os, const TxnState& state) {
    switch (state) {
        ENUMCASEOS(FDS_TRANS_EMPTY             , os);
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
        default: os << "unknown txn type - " << (int) state << "-";
    }
    return os;
}

StorHvJournalEntry::StorHvJournalEntry()
{
    trans_state = FDS_TRANS_EMPTY;
    block_offset = 0;
    replc_cnt = 0;
    sm_msg = 0;
    dm_msg = 0;
    sm_ack_cnt = 0;
    dm_ack_cnt = 0;
    nodeSeq = 0;
    is_in_use = false;
    write_ctx = 0;
    read_ctx = 0;
    comp_arg1 = 0;
    comp_arg2 = 0;
    incarnation_number = 0;

    putMsg = NULL;
    getMsg = NULL;
    delMsg = NULL;

    blobOffset = 0;
    blobName.clear();

    // Initialize lock to unlocked state here
    je_mutex = new fds_mutex("Journal Entry Mutex");
}

StorHvJournalEntry::~StorHvJournalEntry()
{
    delete je_mutex;
}

void StorHvJournalEntry::init(unsigned int transid, StorHvJournal *jrnl_tbl, FdsTimer *ioTimer)
{
    trans_id = transid;
    ioTimerTask.reset(new StorHvIoTimerTask(this, jrnl_tbl, ioTimer));
}

void StorHvJournalEntry::reset()
{
    trans_state = FDS_TRANS_EMPTY;
    replc_cnt = 0;
    sm_msg = 0;
    dm_msg = 0;
    sm_ack_cnt = 0;
    dm_ack_cnt = 0;
    nodeSeq = 0;
    is_in_use = false;
    write_ctx = 0;
    read_ctx = 0;
    comp_arg1 = 0;
    comp_arg2 = 0;
    incarnation_number ++;

    putMsg = NULL;
    getMsg = NULL;
    delMsg = NULL;

    // TODO: Free any pending timers here
    // And any other necessary cleanup
}

void StorHvJournalEntry::setActive()
{
    is_in_use = true;
}

void StorHvJournalEntry::setInactive()
{
    is_in_use = false;
    trans_state = FDS_TRANS_EMPTY;
}

bool StorHvJournalEntry::isActive()
{
    return ((is_in_use) || (trans_state != FDS_TRANS_EMPTY));
}

void StorHvJournalEntry::lock()
{
    // TODO: convert this and other lock/unlock prints to a debug level log
    // cout << "Acquiring lock for transaction " << trans_id << endl;
    je_mutex->lock();
    // cout << "Acquired lock for transaction " << trans_id << endl;

}

void StorHvJournalEntry::unlock()
{
    je_mutex->unlock();
    // cout << "Released lock for transaction " << trans_id << endl;

}



// Caller should hold the lock on the transaction
int StorHvJournalEntry::fds_set_dmack_status(int ipAddr,
                                             fds_uint32_t port) {
    for (fds_uint32_t node = 0;
         node < FDS_MAX_DM_NODES_PER_CLST;
         node++) {
        if ((memcmp(&dm_ack[node].ipAddr, &ipAddr, 4) == 0) &&
            (dm_ack[node].port == port)) {
            if (dm_ack[node].ack_status < FDS_SET_ACK) {
                dm_ack_cnt++;
                LOGNORMAL << "updating the DM ACK: " << dm_ack_cnt;
                dm_ack[node].ack_status = FDS_SET_ACK;
                return 0;
            }
        }
    }

    return 0;
}

// Caller should hold the lock on the transaction
int StorHvJournalEntry::fds_set_dm_commit_status(int ipAddr,
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
int StorHvJournalEntry::fds_set_smack_status(int ipAddr,
                                             fds_uint32_t port) {
    for (fds_uint32_t node = 0;
         node < FDS_MAX_SM_NODES_PER_CLST;
         node++) {
        /*
         * TODO: Use a request or node id here rather than
         * ip/port.
         */
        if ((memcmp((void *)&sm_ack[node].ipAddr, (void *)&ipAddr, 4) == 0) &&
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

// Caller should hold the lock on the transaction
void StorHvJournalEntry::fbd_complete_req(fbd_request_t *req, int status)
{
    (*req->cb_request)(comp_arg1, comp_arg2, req, status);
}

StorHvJournalEntryLock::StorHvJournalEntryLock(StorHvJournalEntry *jrnl_entry) {
    jrnl_e = jrnl_entry;
    jrnl_e->lock();
}

StorHvJournalEntryLock::~ StorHvJournalEntryLock() {
    jrnl_e->unlock();
}

StorHvJournal::StorHvJournal(unsigned int max_jrnl_entries)
        : ioTimer(new FdsTimer())
{
    unsigned int i =0;

    max_journal_entries = max_jrnl_entries;

    rwlog_tbl = new StorHvJournalEntry[max_journal_entries];

    for (i = 0; i < max_journal_entries; i++) {
        rwlog_tbl[i].init(i, this, ioTimer);
        free_trans_ids.push(i);
    }

    jrnl_tbl_mutex = new fds_mutex("Journal Table Mutex");

    ctime = boost::posix_time::microsec_clock::universal_time();
    // printf("Created journal table lock %p for Journal Table %p \n", jrnl_tbl_mutex, this);

    return;
}

StorHvJournal::StorHvJournal()
{
    StorHvJournal(FDS_READ_WRITE_LOG_ENTRIES);
}

StorHvJournal::~StorHvJournal()
{
    delete jrnl_tbl_mutex;
    ioTimer->destroy();
}

void StorHvJournal::lock()
{
    // cout << "Acquiring journal table lock" << endl;
    jrnl_tbl_mutex->lock();
    // cout << "Acquired journal table lock" << endl;
}

void StorHvJournal::unlock()
{
    jrnl_tbl_mutex->unlock();
    // cout << "Released journal table lock"<< endl;
}

// Caller must have acquired the Journal Table Write Lock before invoking this.
unsigned int StorHvJournal::get_free_trans_id() {
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
void StorHvJournal::return_free_trans_id(unsigned int trans_id) {

    free_trans_ids.push(trans_id);

}

// The function returns a new transaction id (that is currently unused) if one is not in use for the block offset.
// If there is one for the passed offset, that trans_id is returned.
// Caller must remember to acquire the lock for the journal entry after this call, before using it.

fds_uint32_t StorHvJournal::get_trans_id_for_block(fds_uint64_t block_offset)
{
    fds_uint32_t trans_id;

    lock();
    try{
        trans_id = block_to_jrnl_idx.at(block_offset);
        if (rwlog_tbl[trans_id].block_offset != block_offset) {
            unlock();
            throw "Corrupt journal table, block offsets do not match";
        }
        unlock();
    }
    catch (const std::out_of_range& err) {
        trans_id = get_free_trans_id();
        block_to_jrnl_idx[block_offset] = trans_id;
        unlock();
        StorHvJournalEntryLock je_lock(&rwlog_tbl[trans_id]);
        if (rwlog_tbl[trans_id].isActive()) {
            throw "Corrupt journal table, allocated transaction is Active\n";
        }
        rwlog_tbl[trans_id].block_offset = block_offset;
    }
    LOGNORMAL <<"assigned txid:" << trans_id << " for offset:" << block_offset;
    return trans_id;
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

fds_uint32_t StorHvJournal::get_trans_id_for_blob(const std::string& blobName,
                                                  fds_uint64_t blobOffset,
                                                  bool& trans_in_progress) {
    fds_uint32_t trans_id;

    trans_in_progress = false;

    lock();
    try{
        trans_id = blob_to_jrnl_idx.at(blobName + ":" + std::to_string(blobOffset));
        trans_in_progress = true;
        LOGDEBUG << "Comparing "
                 << rwlog_tbl[trans_id].blobOffset
                 << " and " << blobOffset
                 << " and "
                 << rwlog_tbl[trans_id].blobName
                 << " and " << blobName;
        unlock();
    } catch (const std::out_of_range& err) {
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
    LOGNORMAL << "Assigned trans id "
              << trans_id << " for blob "
              << blobName << " offset "
              << blobOffset;
    return trans_id;
}


// Caller should hold the lock to the transaction before calling this.
// And then release the lock after returning from this call.

void StorHvJournal::release_trans_id(unsigned int trans_id)
{

    unsigned int block_offset;

    block_offset = rwlog_tbl[trans_id].block_offset;

    lock();

    block_to_jrnl_idx.erase(block_offset);
    rwlog_tbl[trans_id].reset();
    rwlog_tbl[trans_id].block_offset = 0;
    ioTimer->cancel(rwlog_tbl[trans_id].ioTimerTask);
    return_free_trans_id(trans_id);

    unlock();

    LOGNORMAL << " StorHvJournal:" << "IO-XID:" << trans_id <<  " - Released transaction id  " << trans_id;
}

void StorHvJournal::releaseTransId(unsigned int transId) {

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

    LOGNORMAL << "Released transaction id " << transId;
}

StorHvJournalEntry *StorHvJournal::get_journal_entry(fds_uint32_t trans_id) {

    StorHvJournalEntry *jrnl_e = &rwlog_tbl[trans_id];
    return jrnl_e;

}


// Note: Caller holds the lock to the entry
// And takes care of releasing the transaction id to the free pool
void
StorHvJournalEntry::fbd_process_req_timeout() {
    FdsBlobReq *blobReq = static_cast<fds::AmQosReq*>(io)->getBlobReqPtr();
    fds_verify(blobReq != NULL);

    fds::Error err(ERR_OK);

    fds_volid_t   volId = blobReq->getVolId();
    LOGWARN << __FUNCTION__ << " trans id" << trans_id << " timing out, responding to blob " 
            << blobReq->getBlobName() << " offset" << blobReq->getBlobOffset();
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
            txn->nodeSeq += 1; // try all the SM nodes 
            if ( txn->op == FDS_IO_READ ) {
                err = storHvisor->dispatchSmGetMsg(txn);
                fds_verify(err == ERR_OK);
            } else if ( txn->op == FDS_IO_WRITE) {
                err = storHvisor->dispatchSmPutMsg(txn,
                        storHvisor->om_client->\
                        getCurrentDLT()->getNode(blobReq->getObjId(), txn->nodeSeq));
                fds_verify(err == ERR_OK);
            }
        } else {
            storHvisor->qos_ctrl->markIODone(io);
            write_ctx = 0;
            vol->journal_tbl->releaseTransId(trans_id);
            blobReq->cbWithResult(-1);
            delete blobReq;
            blobReq = NULL;
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
