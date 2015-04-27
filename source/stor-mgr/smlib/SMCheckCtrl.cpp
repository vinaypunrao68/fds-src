/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <fds_process.h>
#include <dlt.h>
#include <ObjectId.h>

#include <object-store/ObjectMetaDb.h>
#include <object-store/ObjectStore.h>
#include <object-store/SmDiskMap.h>


#include <SMCheckCtrl.h>
#include <SMCheck.h>

namespace fds {

SMCheckControl::SMCheckControl(const std::string &moduleName,
                               SmDiskMap::ptr diskmap,
                               SmIoReqHandler *datastore)
        : Module(moduleName.c_str())
{
    SMChk = new SMCheckOnline(datastore, diskmap);
}

SMCheckControl::~SMCheckControl()
{
    delete SMChk;
}

int
SMCheckControl::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    return 0;
}

void
SMCheckControl::mod_startup()
{
}

void
SMCheckControl::mod_shutdown()
{
}

// Update the DLT from DLT close event.
void
SMCheckControl::updateSMCheckDLT(const DLT *latestDLT)
{
    // This will actually clone the DLT.
    SMChk->updateDLT(latestDLT);
}

// Start the online SM checker.
Error
SMCheckControl::startSMCheck(std::set<fds_token_id> tgtTokens)
{
    Error err(ERR_OK);

    err = SMChk->startIntegrityCheck(tgtTokens);

    return err;
}

// TODO(Sean):  Need a method to stop
Error
SMCheckControl::stopSMCheck()
{
    Error err(ERR_OK);

    return err;
}

// Get status fo the SM Checker.
Error
SMCheckControl::getStatus(fpi::CtrlNotifySMCheckStatusRespPtr resp)
{
    Error err(ERR_OK);

    SMChk->getStats(resp);

    return err;
}

}  // namespace fds

