#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <fdsp/FDSP.h>
#include "include/fds_err.h"
#include "include/fds_types.h"
#include "fds_client/include/ubd.h"
#include "StorHvJournal.h"

StorHvJournalEntry::StorHvJournalEntry()
{
   trans_state = FDS_TRANS_EMPTY;
   block_offset = 0;
   replc_cnt = 0;
   sm_ack_cnt = 0;
   dm_ack_cnt = 0;
   st_flag = 0;
   lt_flag = 0;
   incarnation_number = 0;
}

StorHvJournalEntry::reinitialize()
{
   trans_state = FDS_TRANS_EMPTY;
   block_offset = 0;
   replc_cnt = 0;
   sm_ack_cnt = 0;
   dm_ack_cnt = 0;
   st_flag = 0;
   lt_flag = 0;
   incarnation_number ++;
}

StorHvJournal::StorHvJournal(unsigned int max_jrnl_entries)
{
	int i =0;

	max_journal_entries = max_jrnl_entries;

	rwlog_tbl = new StorHvJournalEntry[max_journal_entries];
  
	for (i = 0; i < max_journal_entries; i++) {
	  free_trans_ids.push(i);
	}

	return;
}

StorHvJournal::StorHvJournal()
{
  StorHvJournal(FDS_READ_WRITE_LOG_ENTRIES);
}

// Caller must have acquired the Journal Table Write Lock before invoking this.
unsigned int StorHvJournal::get_free_trans_id() {
  
  unsigned int trans_id;

  if (free_trans_ids.empty()) {
    throw Exception("Too many outstanding transactions. Transaction table full");
  }
  trans_id = free_trans_ids.front();
  free_trans_ids.pop();

}

// Caller must have acquired the Journal Table Write Lock before invoking this.
void StorHvJournal::return_free_trans_id(unsigned int trans_id) {

  free_trans_ids.push(trans_id);

}

// The function returns a new transaction id (that is currently unused) if one is not in use for the block offset.
// If there is one for the passed offset, that trans_id is returned.
// Caller must remember to acquire the lock for the journal entry before using it, via a get transaction entry call.
 
unsigned int StorHvJournal::get_trans_id_for_block(unsigned int block_offset)
{
  unsigned int trans_id;
 
  // Acquire Journal Table Write Lock here
  try{
    trans_id = block_to_journal_idx.at(block_offset);
    if (rwlog_tbl[trans_id)->block_offset != block_offset) {
      throw Exception("Corrupt journal table, block offsets do not match");
    }
  }
  catch (const std::out_of_range& err) {
    trans_id = get_free_trans_id();
    if (rwlog_tbl[trans_id]->trans_state != FDS_TRANS_EMPTY) {
      throw Exception("Corrupt journal table, allocated transaction not in empty state\n");
    }
    rwlog_tbl[trans_id]->block_offset = block_offset;
    block_to_journal_idx[block_offset] = trans_id;
  }
  // Release Journal Table Lock here
  return trans_id;
}

void StorHvJournal::release_trans_id(unsigned int block_offset, unsigned int trans_id)
{

  // Acquire Journal Table Write Lock here
  block_to_journal_idx.erase(block_offset);
  // Acquire Journal Entry Lock here
  rwlog_tbl[trans_id]->reinitialize();
  // Release Journal Entry Lock here
  return_free_trans_id(trans_id);
  // Release Journal Table Lock here

}

StorHvJournalEntry *get_journal_entry(int trans_id) {

  // Acquire Journal Table Read Lock here

  StorHvJournalEntry *jrnl_e = rwlog_tbl[trans_id];
  
  // Acquire Journal Entry Lock here
  
  // Release Journal Table Read Lock here

  return jrnl_e;
}

void put_journal_entry(int trans_id) {

  // Release Journal Entry Lock here

}
