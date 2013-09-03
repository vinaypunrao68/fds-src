#include <stdexcept>

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>

#include "FDSP.h"
#include "include/fds_err.h"
#include "include/fds_types.h"
#include <StorHvisorNet.h>
//#include "fds_client/include/ubd.h"
#include "StorHvJournal.h"


extern StorHvCtrl *storHvisor;
namespace fds {

StorHvJournalEntry::StorHvJournalEntry()
{
   trans_state = FDS_TRANS_EMPTY;
   block_offset = 0;
   replc_cnt = 0;
   sm_msg = 0;
   dm_msg = 0;
   sm_ack_cnt = 0;
   dm_ack_cnt = 0;
   st_flag = 0;
   lt_flag = 0;
   is_in_use = false;
   write_ctx = 0;
   read_ctx = 0;
   comp_arg1 = 0;
   comp_arg2 = 0;
   incarnation_number = 0;
   // Initialize lock to unlocked state here
   je_mutex = new fds_mutex("Journal Entry Mutex");
}

StorHvJournalEntry::~StorHvJournalEntry()
{
  delete je_mutex;
}

void StorHvJournalEntry::reset()
{
   trans_state = FDS_TRANS_EMPTY;
   replc_cnt = 0;
   sm_msg = 0;
   dm_msg = 0;
   sm_ack_cnt = 0;
   dm_ack_cnt = 0;
   st_flag = 0;
   lt_flag = 0;
   is_in_use = false;
   write_ctx = 0;
   read_ctx = 0;
   comp_arg1 = 0;
   comp_arg2 = 0;
   incarnation_number ++;
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
int StorHvJournalEntry::fds_set_dmack_status( int ipAddr)
{
	int node =0;
	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&dm_ack[node].ipAddr, &ipAddr,4) == 0) {
	    if (dm_ack[node].ack_status < FDS_SET_ACK) {
	      dm_ack_cnt++; 
	      dm_ack[node].ack_status = FDS_SET_ACK;
	    }
	    return (0);
	  }
	}

	return 0;

}

// Caller should hold the lock on the transaction
int StorHvJournalEntry::fds_set_dm_commit_status( int ipAddr)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&dm_ack[node].ipAddr, &ipAddr,4) == 0) {
	    if (dm_ack[node].commit_status == FDS_COMMIT_MSG_SENT) {
	      dm_commit_cnt++; 
	      dm_ack[node].commit_status = FDS_COMMIT_MSG_ACKED;
	    }
	    return (0);
	  }
	}

	return 0;

}

// Caller should hold the lock on the transaction
int StorHvJournalEntry::fds_set_smack_status( int ipAddr)
{
  int node =0;

  for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++) {
     if (memcmp((void *)&sm_ack[node].ipAddr, (void *)&ipAddr, 4) == 0) {
	    if (sm_ack[node].ack_status != FDS_SET_ACK) {
	      sm_ack[node].ack_status = FDS_SET_ACK;
	      sm_ack_cnt++;
	    }
	    return (0);
     }
  }

  return 0;
}

// Caller should hold the lock on the transaction
void StorHvJournalEntry::fbd_complete_req(fbd_request_t *req, int status)
{
   comp_req(comp_arg1, comp_arg2, req, status);
}

StorHvJournalEntryLock::StorHvJournalEntryLock(StorHvJournalEntry *jrnl_entry) {
    jrnl_e = jrnl_entry;
    jrnl_e->lock();
}

StorHvJournalEntryLock::~ StorHvJournalEntryLock() {
  jrnl_e->unlock();
}

StorHvJournal::StorHvJournal(unsigned int max_jrnl_entries)
{
	unsigned int i =0;

	max_journal_entries = max_jrnl_entries;

	rwlog_tbl = new StorHvJournalEntry[max_journal_entries];
  
	for (i = 0; i < max_journal_entries; i++) {
	  rwlog_tbl[i].trans_id = i;
	  free_trans_ids.push(i);
	}

	jrnl_tbl_mutex = new fds_mutex("Journal Table Mutex");

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
 
unsigned int StorHvJournal::get_trans_id_for_block(unsigned int block_offset)
{
  unsigned int trans_id;
 
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
   FDS_PLOG(storHvisor->GetLog()) << " StorHvJournal - Assigned transaction id " << trans_id << " for block " << block_offset;
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
  return_free_trans_id(trans_id);

  unlock();

   FDS_PLOG(storHvisor->GetLog()) << " StorHvJournal - Released transaction id " << trans_id ;

}

StorHvJournalEntry *StorHvJournal::get_journal_entry(int trans_id) {

  StorHvJournalEntry *jrnl_e = &rwlog_tbl[trans_id];
  return jrnl_e;

}

} // namespace fds
