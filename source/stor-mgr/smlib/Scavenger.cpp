/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <vector>
#include <StorMgr.h>
#include <TokenCompactor.h>
#include <Scavenger.h>
#include <persistentdata.h>

namespace fds {
#define SCAV_BITS_PER_TOKEN 8
extern ObjectStorMgr *objStorMgr;
extern DataDiscoveryModule  dataDiscoveryMod;

void tokenCompactionHandler(fds_token_id tokId, const Error& error) { 
}

ScavControl::ScavControl(fds_log *log, fds_uint32_t num_thrds)
{
  scav_log = log;
  max_disks_compacting = num_thrds;
  num_disks = diskio::dataDiscoveryMod.disk_hdd_discovered();
  scav_mutex = new fds_mutex("Scav Mutex");
  for (fds_uint32_t i=0; i < num_disks; i++) { 
     DiskScavenger *diskScav = new DiskScavenger(log, i, 1);
     diskScavTbl[i] = diskScav;
  }
}

ScavControl::~ScavControl() {
  for (fds_uint32_t i=0; i < num_disks; i++) { 
     DiskScavenger *diskScav = diskScavTbl[i];
     if (diskScav != NULL) { 
        delete diskScav;
     }
  }

}


void ScavControl::startScavengeProcess()
{
    LOGNORMAL << "Starting Scavenger cycle... ";
    scav_mutex->lock();
    for (fds_uint32_t i=0; i < num_disks; i++) { 
       DiskScavenger *diskScav = diskScavTbl[i];
       if (diskScav != NULL) { 
           diskScav->startScavenge();
       }
    }
    scav_mutex->unlock();


}


void ScavControl::stopScavengeProcess()
{
    LOGNORMAL << "Stop Scavenger cycle... ";

}

fds::Error ScavControl::addTokenCompactor(fds_token_id tok_id, fds_uint32_t disk_idx) {
    Error err(ERR_OK);
    scav_mutex->lock();
    DiskScavenger *diskScav = diskScavTbl[disk_idx];
    scav_mutex->unlock();

    if (diskScav != NULL) { 
       diskScav->disk_scav_mutex->lock();
       diskScav->tokenCompactorDb[tok_id] = new TokenCompactor(objStorMgr);
       diskScav->disk_scav_mutex->unlock();
    }
    return err;
}

fds::Error ScavControl::deleteTokenCompactor(fds_token_id tok_id, fds_uint32_t disk_idx) {
    Error err(ERR_OK);
    scav_mutex->lock();
    DiskScavenger *diskScav = diskScavTbl[disk_idx];
    diskScavTbl[disk_idx] = NULL;
    scav_mutex->unlock();
    delete diskScav;
    return err;
}

DiskScavenger::DiskScavenger(fds_log *log, fds_uint32_t disk_id, fds_uint32_t proc_max_tokens) { 
   scav_log = log;
   max_tokens_in_proc = proc_max_tokens;
   last_token =0;
   disk_idx = disk_id;
   disk_scav_mutex = new fds_mutex("Disk-Scav Mutex");
}

DiskScavenger::~DiskScavenger() { 
}

void DiskScavenger::startScavenge() {
 fds_uint32_t i = 0;
  disk_scav_mutex->lock();
  for (auto it = tokenCompactorDb.cbegin(); it != tokenCompactorDb.cend(); it++) {
    TokenCompactor *tokCompactor  = it->second; 
    fds_token_id tok_id = it->first;
    tokCompactor->startCompaction(tok_id, SCAV_BITS_PER_TOKEN, &tokenCompactionHandler );
  }
  disk_scav_mutex->unlock();
}

void DiskScavenger::stopScavenge() {
}

}  // namespace fds
