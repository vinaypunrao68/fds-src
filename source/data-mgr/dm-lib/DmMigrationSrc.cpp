/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmMigrationSrc.h>
#include <DataMgr.h>
#include <util/stringutils.h>

namespace fds {

DmMigrationSrc::DmMigrationSrc(DataMgr& _dataMgr,
                               const NodeUuid& _myUuid,
                               NodeUuid& _destDmUuid,
                               int64_t migrationId,
                               fpi::CtrlNotifyInitialBlobFilterSetMsgPtr _ribfsm,
                               DmMigrationClientDoneHandler _handle,
                               migrationCb _cleanup,
                               uint64_t _maxDeltaBlobs,
                               uint64_t _maxDeltaBlobDescs,
                               int32_t _version)
: DmMigrationClient(_dataMgr,
                    _myUuid,
                    _destDmUuid,
                    migrationId,
                    _ribfsm,
                    _handle,
                    _cleanup,
                    _maxDeltaBlobs,
                    _maxDeltaBlobDescs)
{
    version = _version;
    logStr = util::strformat("[DmMigrationSrc volId: %ld version: %d]",
                            volId.get(),
                            version);
}

DmMigrationSrc::~DmMigrationSrc()
{
	if (thrPtr) {
	    LOGDEBUG << "Waiting for volume " << volId << " work thread to complete";
	    thrPtr->join();
        thrPtr.reset();
	}
}
void DmMigrationSrc::abortMigration()
{
    abortFlag = true;
    LOGWARN << "Abort set: " << logString();
}

void
DmMigrationSrc::run()
{
    thrPtr.reset(new std::thread([this] {
        Error err(ERR_OK);

        do {
            /* We will take a snapshot of the volume here */
            err = processBlobFilterSet();
            if (ERR_OK != err) {
                LOGERROR << logString() << " Processing filter set failed: "
                        << err << ". Exiting run thread";
                break;
            }

            /* Process and send the diffs over to destination */
            err = processBlobDiff();
            break;

        } while (false);

        {
            fds_scoped_lock lock(ssTakenScopeLock);
            if (snapshotTaken) {
                LOGMIGRATE << "Freed snapshot: " << logString();
                /* free the in-memory snapshot diff after completion. */
                fds_verify(dataMgr.timeVolCat_->queryIface()->freeVolumeSnapshot(volId, snap_).ok());
                snapshotTaken = false;
            }
        }

        sendFinishStaticMigrationMsg(err);

        LOGNORMAL << "Waiting for async msgs....." << logString();
        waitForAsyncMsgs();
        LOGNORMAL << "Waiting for async msgs completed: " << logString();

        /* Notify the caller we are done */
        dataMgr.getModuleProvider()->proc_thrpool()->schedule(cleanUp, err);
        dataMgr.getModuleProvider()->proc_thrpool()->schedule(migrDoneHandler, volId, err);

    }));
}

fds_bool_t
DmMigrationSrc::shouldForwardIO(fds_uint64_t dmtVersionIn)
{
    return false;
}

void DmMigrationSrc::turnOnForwarding()
{
}

void DmMigrationSrc::turnOffForwarding()
{
}

void DmMigrationSrc::sendFinishStaticMigrationMsg(const Error &e)
{
    LOGNORMAL << logString() << " Finishing migration with error: " << e;
    Error err(ERR_OK);

    auto msg = MAKE_SHARED<fpi::CtrlNotifyFinishMigrationMsg>();
    msg->volume_id = volId.get();
    msg->status = e.GetErrno();

    auto finishReq = requestMgr->newEPSvcRequest(destDmUuid.toSvcUuid());
    finishReq->setTimeoutMs(dataMgr.dmMigrationMgr->getTimeoutValue());
    finishReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyFinishMigrationMsg), msg);
    // A hack because g++ doesn't like a bind within a macro that does bind
    std::function<void()> abortBind = std::bind(&DmMigrationSrc::asyncMsgFailed, this);
    std::function<void()> passBind = std::bind(&DmMigrationSrc::asyncMsgPassed, this);
    finishReq->onResponseCb(RESPONSE_MSG_HANDLER(DmMigrationBase::dmMigrationCheckResp, abortBind, passBind));
    finishReq->setTaskExecutorId(volId.v);
	asyncMsgIssued();
    finishReq->invoke();
}

}// namespace fds
