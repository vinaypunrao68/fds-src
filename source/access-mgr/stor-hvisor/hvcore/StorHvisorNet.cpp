/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <iostream>
#include <chrono>
#include <cstdarg>
#include <fds_module.h>
#include <fds_timer.h>
#include "StorHvisorNet.h"
#include <hash/MurmurHash3.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NetSession.h"
#include "StorHvCtrl.h"

StorHvCtrl *storHvisor;

using namespace std;
using namespace FDS_ProtocolInterface;

void CreateSHMode(int argc,
                  char *argv[],
                  fds_bool_t test_mode,
                  fds_uint32_t sm_port,
                  fds_uint32_t dm_port)
{

    fds::Module *io_dm_vec[] = {
        nullptr
    };

    fds::ModuleVector  io_dm(argc, argv, io_dm_vec);

    if (test_mode == true) {
        storHvisor = new StorHvCtrl(argc, argv, io_dm.get_sys_params(),
                                    StorHvCtrl::TEST_BOTH, sm_port, dm_port);
    } else {
        storHvisor = new StorHvCtrl(argc, argv, io_dm.get_sys_params(),
                                    StorHvCtrl::NORMAL);
    }

    /*
     * Start listening for OM control messages
     * Appropriate callbacks were setup by data placement and volume table objects
     */
    storHvisor->StartOmClient();
    storHvisor->qos_ctrl->runScheduler();

    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << "StorHvisorNet - Created storHvisor " << storHvisor;
}

void DeleteStorHvisor()
{
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << " StorHvisorNet -  Deleting the StorHvisor";
    delete storHvisor;
}

void ctrlCCallbackHandler(int signal)
{
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << "StorHvisorNet -  Received Ctrl C " << signal;
    // SAN   storHvisor->_communicator->shutdown();
    DeleteStorHvisor();
}

void processBlobReq(AmRequest *amReq) {
    fds::PerfTracer::tracePointEnd(amReq->qos_perf_ctx);

    fds_verify(amReq->io_module == FDS_IOType::STOR_HV_IO);
    fds_verify(amReq->magicInUse() == true);

    switch (amReq->io_type) {
        case fds::FDS_START_BLOB_TX:
            storHvisor->amProcessor->startBlobTx(amReq);
            break;

        case fds::FDS_COMMIT_BLOB_TX:
            storHvisor->amProcessor->commitBlobTx(amReq);
            break;

        case fds::FDS_ABORT_BLOB_TX:
            storHvisor->amProcessor->abortBlobTx(amReq);
            break;

        case fds::FDS_ATTACH_VOL:
            storHvisor->attachVolume(amReq);
            break;

        case fds::FDS_IO_READ:
        case fds::FDS_GET_BLOB:
            storHvisor->amProcessor->getBlob(amReq);
            break;

        case fds::FDS_IO_WRITE:
        case fds::FDS_PUT_BLOB_ONCE:
        case fds::FDS_PUT_BLOB:
            storHvisor->amProcessor->putBlob(amReq);
            break;

        case fds::FDS_SET_BLOB_METADATA:
            storHvisor->amProcessor->setBlobMetadata(amReq);
            break;

        case fds::FDS_GET_VOLUME_METADATA:
            storHvisor->amProcessor->getVolumeMetadata(amReq);
            break;

        case fds::FDS_DELETE_BLOB:
            storHvisor->amProcessor->deleteBlob(amReq);
            break;

        case fds::FDS_STAT_BLOB:
            storHvisor->amProcessor->statBlob(amReq);
            break;

        case fds::FDS_VOLUME_CONTENTS:
            storHvisor->amProcessor->volumeContents(amReq);
            break;

        default :
            LOGCRITICAL << "unimplemented request: " << amReq->io_type;
            amReq->cb->call(ERR_NOT_IMPLEMENTED);
            break;
    }
}

