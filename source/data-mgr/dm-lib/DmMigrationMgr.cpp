/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmIoReq.h>
#include <DmMigrationMgr.h>

namespace fds {

DmMigrationMgr::DmMigrationMgr(DmIoReqHandler *DmReqHandle, DataMgr& _dataMgr)
: DmReqHandler(DmReqHandle),
    dataManager(_dataMgr),
    OmStartMigrCb(nullptr),
    maxConcurrency(1),
    firedMigrations(0),
    mit(NULL),
    DMT_version(DMT_VER_INVALID),
    migrationAborted(false),
    timerStarted(false)
{
    maxConcurrency = fds_uint32_t(MODULEPROVIDER()->get_fds_config()->
                                  get<int>("fds.dm.migration.migration_max_concurrency"));

    enableMigrationFeature = bool(MODULEPROVIDER()->get_fds_config()->
                                  get<bool>("fds.dm.migration.enable_feature"));

    enableResyncFeature = bool(MODULEPROVIDER()->get_fds_config()->
                               get<bool>("fds.dm.migration.enable_resync"));

    maxNumBlobs = uint64_t(MODULEPROVIDER()->get_fds_config()->
                           get<int64_t>("fds.dm.migration.migration_max_delta_blobs"));

    maxNumBlobDesc = uint64_t(MODULEPROVIDER()->get_fds_config()->
                              get<int64_t>("fds.dm.migration.migration_max_delta_blob_desc"));

    deltaBlobTimeout = uint32_t(MODULEPROVIDER()->get_fds_config()->
                                get<int32_t>("fds.dm.migration.migration_max_delta_blobs_to"));

    delayStart = uint32_t(MODULEPROVIDER()->get_fds_config()->
                          get<int32_t>("fds.dm.testing.test_delay_start"));

    /* 5 miutes for idle timeout */
    idleTimeoutSecs = 300;
    auto timer = MODULEPROVIDER()->getTimer();
    auto task = [this] () {
        /* Immediately post to threadpool so we don't hold up timer thread */
        MODULEPROVIDER()->proc_thrpool()->schedule(
            &DmMigrationMgr::migrationIdleTimeoutCheck, this);
            
    };
    auto idleTimeouTask = SHPTR<FdsTimerTask>(new FdsTimerFunctionTask(*timer, task));
    timer->scheduleRepeated(idleTimeouTask, std::chrono::seconds(idleTimeoutSecs));
}


DmMigrationMgr::~DmMigrationMgr()
{

}

void
DmMigrationMgr::mod_shutdown()
{
    {
        SCOPEDREAD(migrClientLock);
        {
            SCOPEDREAD(migrExecutorLock);

            if (!executorMap.empty() || !clientMap.empty()) {
                LOGNOTIFY << "Aborting migration in preperation for shutdown.";
                abortMigration();
            }
        }
    }

    if (abort_thread) {
        abort_thread->join();
        abort_thread = nullptr;
    }
}

Error
DmMigrationMgr::createMigrationExecutor(const NodeUuid& srcDmUuid,
                                        fpi::FDSP_VolumeDescType &vol,
                                        int64_t migrationId,
                                        MigrationType& migrationType,
                                        const fds_bool_t& autoIncrement)
{
    Error err(ERR_OK);

    SCOPEDWRITE(migrExecutorLock);
    /**
     * Make sure that this isn't an ongoing operation.
     * Otherwise, OM bug?
     */
    auto uniqueId = std::make_pair(srcDmUuid, fds_volid_t(vol.volUUID));
    auto search = executorMap.find(uniqueId);
    if (search != executorMap.end()) {
        LOGMIGRATE << "migrationid: " << migrationId
            << " Migration for node " << srcDmUuid
            << " volume " << vol.vol_name << " is a duplicated request.";
        err = ERR_DUPLICATE;
    } else {
        /**
         * Create a new instance of migration Executor
         */
        LOGMIGRATE << "migrationid: " << migrationId 
            << "Creating migration instance for node " << srcDmUuid << " volume id=: " << vol.volUUID
            << " name=" << vol.vol_name;


        startMigrationStopWatch();

        executorMap.emplace(uniqueId,
                            DmMigrationExecutor::unique_ptr(
                            new DmMigrationExecutor(dataManager,
                                                    srcDmUuid,
                                                    vol,
                                                    migrationId,
                                                    autoIncrement,
                                                    std::bind(&DmMigrationMgr::migrationExecutorDoneCb,
                                                              this,
                                                              std::placeholders::_1,
                                                              std::placeholders::_2,
                                                              migrationId,
                                                              std::placeholders::_3),
                                                    deltaBlobTimeout)));
    }
    return err;
}

DmMigrationExecutor::shared_ptr
DmMigrationMgr::getMigrationExecutor(std::pair<NodeUuid, fds_volid_t> uniqueId)
{
    auto search = executorMap.find(uniqueId);
    if (search == executorMap.end()) {
        return nullptr;
    } else {
        return search->second;
    }
}

DmMigrationClient::shared_ptr
DmMigrationMgr::getMigrationClient(std::pair<NodeUuid, fds_volid_t> uniqueId)
{
   auto search = clientMap.find(uniqueId);
   if (search == clientMap.end()) {
       return nullptr;
   } else {
       return search->second;
   }
}

Error
DmMigrationMgr::startMigrationExecutor(DmRequest* dmRequest)
{
    Error err(ERR_OK);

    NodeUuid mySvcUuid(MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid);
    NodeUuid destSvcUuid;
    DmIoMigration* typedRequest = static_cast<DmIoMigration*>(dmRequest);
    fpi::CtrlNotifyDMStartMigrationMsgPtr migrationMsg = typedRequest->message;
    OmStartMigrCb = typedRequest->localCb;
    fds_bool_t autoIncrement = false;
    fds_bool_t loopFireNext = true;
    NodeUuid srcDmSvcUuid;
    int64_t migrationId = migrationMsg->DMT_version;

    // Check if the migraion feature is enabled or disabled.
    if (false == enableMigrationFeature) {
        LOGCRITICAL << "DM Migration is disabled! ignoring start migration msg";
        if (OmStartMigrCb) {
        	LOGDEBUG << "Sending ERR_OK back to OM";
            OmStartMigrCb(ERR_OK);
        }
        return err;
    }

    if (delayStart) {
        LOGNOTIFY << "migrationid: " << migrationId
            << "Delaying creation of DM Migration Manager by " << delayStart << " seconds.";
        sleep(delayStart);
    }

    // Test error return code
    fiu_do_on("abort.dm.migration",\
              LOGNOTIFY << "abort.dm.migration fault point enabled";\
              abortMigration(); return (ERR_NOT_READY););

    waitForMigrationBatchToFinish(MIGR_EXECUTOR);

    // For now, only node addition is supported.
    MigrationType localMigrationType(MIGR_DM_ADD_NODE);

    if (err != ERR_OK) {
        return err;
    }

    // Store DMT version for debugging
    DMT_version = migrationMsg->DMT_version;
    LOGNOTIFY << "migrationid: " << migrationId
        << "Starting Migration executors with DMT version = " << DMT_version;

    firedMigrations = 0;
    for (std::vector<fpi::DMVolumeMigrationGroup>::iterator vmg = migrationMsg->migrations.begin();
         vmg != migrationMsg->migrations.end();
         ++vmg) {

        for (std::vector<fpi::FDSP_VolumeDescType>::iterator vdt = vmg->VolDescriptors.begin();
             vdt != vmg->VolDescriptors.end();
             ++vdt) {
            /**
             * If this is the last executor to be fired, anything from this point on should
             * have the autoIncrement flag set.
             */
            fds_verify(maxConcurrency > 0);
            ++firedMigrations;
            if (firedMigrations == maxConcurrency) {
                autoIncrement = true;
                // from this point on, all the other executors must have autoIncrement == TRUE
            }
            srcDmSvcUuid.uuid_set_val(vmg->source.svc_uuid);

            LOGNOTIFY << "migrationid: " << migrationId
                << "Pull Volume ID: " << vdt->volUUID
                << " Name: " << vdt->vol_name
                << " from SrcDmSvcUuid: " << vmg->source.svc_uuid;

            err = createMigrationExecutor(srcDmSvcUuid,
                                          *vdt,
                                          migrationId,
                                          localMigrationType,
                                          autoIncrement);
            if (!err.OK()) {
                LOGERROR << "migrationid: " << migrationId
                    << "Error creating migrating task.  err=" << err;
                return err;
            }
        }
    }

    /**
     * For now, execute one at a time. Increase parallelism by changing platform.conf
     * Note: If we receive more request than max_tokens is allowed, this method will
     * block. That's ok, because we want to make sure that all the requested migrations
     * coming from the OM at least have an executor on the src and client on the dest
     * sides communicate with each other before we allow the call to unblock and returns
     * a OK response.
     * TODO(Neil) - the "startMigration()" method is not optimized for multithreading.
     * We need to revisit this and work with the maxConcurrency concept to start x concurrent
     * threads.
     */
    SCOPEDREAD(migrExecutorLock);
    mit = executorMap.begin();
    while (loopFireNext && (mit != executorMap.end())) {
        loopFireNext = !(mit->second->shouldAutoExecuteNext());
        mit->second->startMigration();
        if (isMigrationAborted()) {
            /**
             * Abort everything
             */
            err = ERR_DM_MIGRATION_ABORTED;
            break;
        } else if (loopFireNext) {
            mit++;
        }
    }
    return err;
}

Error
DmMigrationMgr::applyDeltaBlobDescs(DmIoMigrationDeltaBlobDesc* deltaBlobDescReq) {
    fpi::CtrlNotifyDeltaBlobDescMsgPtr deltaBlobDescMsg = deltaBlobDescReq->deltaBlobDescMsg;
    fds_volid_t volId(deltaBlobDescMsg->volume_id);
    NodeUuid srcUuid(deltaBlobDescReq->srcUuid);
    auto uniqueId = std::make_pair(srcUuid, volId);
    SCOPEDREAD(migrExecutorLock);
    DmMigrationExecutor::shared_ptr executor = getMigrationExecutor(uniqueId);
    Error err(ERR_OK);
    migrationCb descCb = deltaBlobDescReq->localCb;

    if (executor == nullptr) {
    	if (isMigrationAborted()) {
			LOGMIGRATE << "migrationid: " << deltaBlobDescMsg->DMT_version
                << " Unable to find executor for volume " << volId
                << " during migration abort";
    	} else {
			LOGERROR << "migrationid: " << deltaBlobDescMsg->DMT_version
                << "Unable to find executor for volume " << volId;
			// this is an race cond error that needs to be fixed in dev env.
			// Only panic in debug build.
			// TODO - need to clean migrationAborted up. Avoid assert for now
			// fds_assert(0);
    	}
        return ERR_NOT_FOUND;
    }

    err = executor->processDeltaBlobDescs(deltaBlobDescMsg, descCb);
    LOGDEBUG << "Total size migrated: " << dataManager.counters->totalSizeOfDataMigrated.value();

    if (err == ERR_NOT_READY) {
    	LOGDEBUG << "Blobs descriptor not applied yet.";
    	// This means that the blobs have not been applied yet. Override this to ERR_OK.
    	err = ERR_OK;
    } else if (!err.ok()) {
    	LOGERROR << "migrationid: " << deltaBlobDescMsg->DMT_version
            << " Error applying blob descriptor " << err;
    	dumpDmIoMigrationDeltaBlobDesc(deltaBlobDescReq);
    	abortMigration();
    }

    return err;
}

// process the deltaObject request
Error
DmMigrationMgr::applyDeltaBlobs(DmIoMigrationDeltaBlobs* deltaBlobReq) {
    Error err(ERR_OK);
    fpi::CtrlNotifyDeltaBlobsMsgPtr deltaBlobsMsg = deltaBlobReq->deltaBlobsMsg;
    fds_volid_t volId(deltaBlobsMsg->volume_id);
    NodeUuid srcUuid(deltaBlobReq->srcUuid);
    auto uniqueId = std::make_pair(srcUuid, volId);
    SCOPEDREAD(migrExecutorLock);
    DmMigrationExecutor::shared_ptr executor = getMigrationExecutor(uniqueId);
    if (executor == nullptr) {
    	if (isMigrationAborted()) {
			LOGMIGRATE << "migrationid: " << deltaBlobsMsg->DMT_version
                << " Unable to find executor for volume " << volId
                << " during migration abort";
    	} else {
			LOGERROR << "migrationid: " << deltaBlobsMsg->DMT_version
                << " Unable to find executor for volume " << volId;
			// this is an race cond error that needs to be fixed in dev env.
			// Only panic in debug build.
			// TODO - need to clean migrationAborted up. Avoid assert for now
			// fds_assert(0);
    	}
        return ERR_NOT_FOUND;
    }
    LOGMIGRATE << "Size of deltaBlobMsg in bytes is: " << sizeOfData(deltaBlobsMsg);
    err = executor->processDeltaBlobs(deltaBlobsMsg);
    LOGDEBUG << "Total size migrated: " << dataManager.counters->totalSizeOfDataMigrated.value();

    if (!err.ok()) {
    	LOGERROR << "migrationid: " << deltaBlobsMsg->DMT_version
            << " Processing deltaBlobs failed " << err;
    	dumpDmIoMigrationDeltaBlobs(deltaBlobsMsg);
    	abortMigration();
    }

    return err;
}

Error
DmMigrationMgr::handleForwardedCommits(DmIoFwdCat* fwdCatReq) {
    auto fwdCatMsg = fwdCatReq->fwdCatMsg;
    fds_volid_t volId(fwdCatMsg->volume_id);
    NodeUuid srcUuid(fwdCatReq->srcUuid.uuid_get_val());
    auto uniqueId = std::make_pair(srcUuid, volId);
    SCOPEDREAD(migrExecutorLock);
    DmMigrationExecutor::shared_ptr executor = getMigrationExecutor(uniqueId);
    if (executor == nullptr) {
    	if (isMigrationAborted()) {
			LOGMIGRATE << "migrationid: " << fwdCatMsg->DMT_version
                << " Unable to find executor for volume " << volId
                << " during migration abort";
    	} else {
			LOGERROR << "migrationid: " << fwdCatMsg->DMT_version
                << " Unable to find executor for volume " << volId;

            // this likely means DM died during migration and has come back with the client still forwarding
            // returning an error code will abort the client side migration
    	}
    	return ERR_NOT_FOUND;
    }
    return (executor->processForwardedCommits(fwdCatReq));
}

void
DmMigrationMgr::ackStaticMigrationComplete(const Error &status, int64_t migrationId)
{
    fds_verify(OmStartMigrCb != NULL);
    fds_assert(status == ERR_OK);

    DMT_version = DMT_VER_INVALID;

    LOGNOTIFY << "migrationid: " << migrationId
        << " Telling OM that all static migrations have completed";
    OmStartMigrCb(status);
}

// See note in header file for design decisions
Error
DmMigrationMgr::startMigrationClient(DmRequest* dmRequest)
{
    Error err(ERR_OK);
    NodeUuid mySvcUuid(MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid);
    DmIoResyncInitialBlob* typedRequest = static_cast<DmIoResyncInitialBlob*>(dmRequest);
    NodeUuid destDmUuid(typedRequest->destNodeUuid);
    fpi::CtrlNotifyInitialBlobFilterSetMsgPtr migReqMsg = typedRequest->message;
    migrationCb cleanupCb = typedRequest->localCb;

    waitForMigrationBatchToFinish(MIGR_CLIENT);

    LOGNOTIFY << "migrationid: " << migReqMsg->DMT_version
        <<" received msg for volume " << migReqMsg->volumeId;

    MigrationType localMigrationType(MIGR_DM_ADD_NODE);

    /**
     * TODO(Neil)
     * This is currently a bad design because DmIOResyncInitialBlob message
     * can be super big. The createMigrationClient will not be finished with
     * the big message and return in time to not block QoS thread so we're doing
     * a quick fix to just spawn off another thread. This will avoid any future
     * mysterious I/O timeout because we're holding on to the QoS thread.
     * The real fix is to break the incoming msg into chunks, service them,
     * and then return.
     */
    std::thread t1([this, destDmUuid, mySvcUuid, migReqMsg, cleanupCb] () mutable {
        this->createMigrationClient(destDmUuid, mySvcUuid, migReqMsg, cleanupCb);
    });
    t1.detach();
    // std::thread t1([&] {this->createMigrationClient(destDmUuid, mySvcUuid, migReqMsg, cleanupCb);});
    // This is bad but there's no control for now. Once we have a better abort, we
    // will revisit this
    // t1.detach();

	/**
	 * When we return below, the ack will be sent back to the Executor.
	 * The dmReq still exists, and will depend on cleanupCb above to be called by the
	 * client to delete and ensure no memory leaks.
	 */
    return err;
}

Error
DmMigrationMgr::createMigrationClient(NodeUuid& destDmUuid,
                                      const NodeUuid& mySvcUuid,
                                      fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& filterSet,
                                      migrationCb cleanUp)
{
    Error err(ERR_OK);
    /**
     * Make sure that this isn't an ongoing operation.
     * Otherwise, DM bug
     */
    auto fds_volid = fds_volid_t(filterSet->volumeId);
    auto search = clientMap.find(std::make_pair(destDmUuid, fds_volid));
    DmMigrationClient::shared_ptr client = nullptr;
    if (search != clientMap.end()) {
        LOGERROR << "migrationid: " << filterSet->DMT_version
            << " Client received request for destination node: " << destDmUuid
            << " volume " << filterSet->volumeId << " but it already exists";
        err = ERR_DUPLICATE;
        abortMigration();
    } else {
        startMigrationStopWatch();
        /**
         * Create a new instance of client and start it.
         */
    	auto uniqueId = std::make_pair(destDmUuid, fds_volid);
        {
        	SCOPEDWRITE(migrClientLock);
            LOGNORMAL << "migrationid: " << filterSet->DMT_version
                << "Creating migration client for node: "
                << destDmUuid << " volume ID# " << fds_volid;
			clientMap.emplace(uniqueId,
							  (client = DmMigrationClient::shared_ptr(
                              new DmMigrationClient(DmReqHandler,
                                                    dataManager,
                                                    mySvcUuid,
                                                    destDmUuid,
                                                    filterSet->DMT_version,
                                                    filterSet,
                                                    std::bind(&DmMigrationMgr::migrationClientDoneCb,
                                                              this,
                                                              std::placeholders::_1,
                                                              filterSet->DMT_version,
                                                              std::placeholders::_2),
                                                    maxNumBlobs,
                                                    maxNumBlobDesc))));
        }
        {
        	SCOPEDREAD(migrClientLock);
        	client = getMigrationClient(uniqueId);
        	fds_assert(client != nullptr);

			err = client->processBlobFilterSet();
			if (ERR_OK != err) {
                LOGERROR << client->logString() << " Processing filter set failed: " << err;
				abortMigration();
				err = ERR_DM_CAT_MIGRATION_DIFF_FAILED;
				goto out;
			}

			err = client->processBlobFilterSet2();
			if (ERR_OK != err) {
				// This one doesn't have an async callback to decrement so we fail it manually
                LOGERROR << client->logString() << " Processing blob diff failed: " << err;
				abortMigration();
				// Shared the same error code, so look for above's msg
				err = ERR_DM_CAT_MIGRATION_DIFF_FAILED;
				goto out;
			}
        }
    }

out:
    if (cleanUp) {
        cleanUp(err);
    }
    return err;
}

void
DmMigrationMgr::migrationExecutorDoneCb(NodeUuid srcNode,
                                        fds_volid_t volId,
                                        int64_t migrationId,
                                        const Error &result)
{
    MigrationState expectedState(MIGR_IN_PROGRESS);
    /**
     * Make sure we can find it
     */
    auto key = std::make_pair(srcNode,volId);
    DmMigrationExecMap::iterator mapIter = executorMap.find(key);
    fds_verify(mapIter != executorMap.end());

    LOGMIGRATE << "migrationid: " << migrationId
        << "Migration Executor complete for volume="
        << std::hex << volId << std::dec
                << " with error=" << result;

    if (!result.OK()) {
        LOGERROR << "migrationid: " << migrationId
            << " Volume=" << volId << " failed migration with error: " << result;
        abortMigration();
    } else {
        /**
         * Normal exit. Really doesn't do much as we're waiting for the clients to come back.
         */
        if (mit->second->shouldAutoExecuteNext()) {
            ++mit;
            if (mit != executorMap.end()) {
                mit->second->startMigration();
            } else {
                ackStaticMigrationComplete(result, migrationId);
            }
        }
    }
}

void
DmMigrationMgr::migrationClientDoneCb(fds_volid_t uniqueId, int64_t migrationId, const Error &result)
{
    if (!result.OK()) {
        LOGERROR << "migrationid: " << migrationId
            << " Volume=" << uniqueId << " failed migration client with error: " << result;
        abortMigration();
    } else {
    	LOGMIGRATE << "migrationid: " << migrationId
            << " Client done with volume " << uniqueId;
    }
}

fds_bool_t
DmMigrationMgr::shouldForwardIO(fds_volid_t volId, fds_uint64_t dmtVersion)
{
    SCOPEDREAD(migrClientLock);
    // If at least one client says "forward", then return true.
    for (auto cit : clientMap) {
    	auto key = cit.first;
    	auto dmClient = cit.second;
    	if ((key.second == volId) && (dmClient->shouldForwardIO(dmtVersion))) {
    		return true;
    	}
    }
    return (false);
}

void
DmMigrationMgr::stopAllClientForwarding()
{
	DmMigrationClientMap::iterator mapIter (clientMap.begin());
	for (; mapIter != clientMap.end(); mapIter++) {
		LOGMIGRATE << mapIter->second->logString()
            << " Turning off forwarding for node" << mapIter->first.first
            << " vol: " << mapIter->first.second;
		mapIter->second->turnOffForwarding();
	}
}

Error
DmMigrationMgr::forwardCatalogUpdate(fds_volid_t volId,
                                    DmIoCommitBlobTx *commitBlobReq,
                                    blob_version_t blob_version,
                                    const BlobObjList::const_ptr& blob_obj_list,
                                    const MetaDataList::const_ptr& meta_list)
{
	Error err(ERR_NOT_FOUND);

    SCOPEDREAD(migrClientLock);

    /**
     * Populate the # of clients first, so we don't run the risk of
     * the first client finishing and calling cleanup to remove the io request
     * before the second client.
     */
    if (!isMigrationAborted()) {
        {
            std::lock_guard<std::mutex> lock(commitBlobReq->migrClientCntMtx);
            for (auto client : clientMap) {
                auto pair = client.first;
                if (pair.second == volId) {
                    auto destUuid = pair.first;
                    auto dmClient = client.second;
                    if (dmClient) {
                        commitBlobReq->migrClientCnt++;
                    }
                }
            }
        }

        LOGDEBUG << "This IO request " << commitBlobReq << " is to forward to " << commitBlobReq->migrClientCnt << " clients";

        for (auto client : clientMap) {
            auto pair = client.first;
            if (pair.second == volId) {
                auto destUuid = pair.first;
                auto dmClient = client.second;
                if (dmClient == nullptr) {
                    if (isMigrationAborted()) {
                        LOGMIGRATE << "Unable to find client for volume " << volId << " during migration abort";
                    } else {
                        LOGERROR << "Unable to find client for volume " << volId;
                        // this is an race cond error that needs to be fixed in dev env.
                        // Only panic in debug build.
                        // TODO - need to clean migrationAborted up. Avoid assert for now
                        // fds_assert(0);
                    }
                } else {
                    LOGMIGRATE << dmClient->logString()
                        << " On volume " << volId
                        << " commit, forwarding to node " << destUuid;
                    err = dmClient->forwardCatalogUpdate(commitBlobReq, blob_version, blob_obj_list, meta_list);
                    if (!err.ok()) {
                        LOGWARN << dmClient->logString()
                            << " Error on volume " << volId << " node " << destUuid;
                        break;
                    }
                }
            }
        }
    }

	return err;
}

Error
DmMigrationMgr::finishActiveMigration(MigrationRole role, int64_t migrationId)
{
	Error err(ERR_OK);
	MigrationState expectedState(MIGR_IN_PROGRESS);
	if (role == MIGR_CLIENT) {
		{
			SCOPEDWRITE(migrClientLock);
			/*
			 * DMT Close is a broadcasted event. There's no need to handle each node separately.
			 * Just nuke all the clientMap because DMT Close can only happen once all the executors
			 * have Cb'ed to the OM.
			 */
			std::lock_guard<std::mutex> lk(migrationBatchMutex);

			dataManager.counters->totalVolumesSentMigration.incr(clientMap.size());
			clearClients();
			LOGNORMAL << "migrationid: " << migrationId
                << "Migration clients cleared and state reset";
		}
	} else if (role == MIGR_EXECUTOR) {
		{
			SCOPEDWRITE(migrExecutorLock);
			LOGNORMAL << "migrationid: " << migrationId
                << " Migration executors state reset";
			/**
			 * The key point is that the executor's finishActiveMigration() resumes I/O.
			 * This gets called once every node's executor's all done.
			 */
			std::lock_guard<std::mutex> lk(migrationBatchMutex);
			clearExecutors();
		}
	}
	migrationCV.notify_one();
	return err;
}

Error
DmMigrationMgr::finishActiveMigration(NodeUuid destUuid, fds_volid_t volId, int64_t migrationId)
{
	Error err(ERR_OK);
	{
		SCOPEDREAD(migrExecutorLock);
		auto uniqueId = std::make_pair(destUuid, volId);
		DmMigrationExecutor::shared_ptr dmExecutor = getMigrationExecutor(uniqueId);
		if (dmExecutor == nullptr) {
			if (isMigrationAborted()) {
				LOGMIGRATE << "migrationid: " << migrationId
                    << " Unable to find executor for volume " << volId
                    << " during migration abort";
			} else {
				LOGERROR << "migrationid: " << migrationId
                    << " Unable to find executor for volume " << volId;
				// this is an race cond error that needs to be fixed in dev env.
				// Only panic in debug build.
				// fds_assert(0);
			}
			return ERR_NOT_FOUND;
		}

		err = dmExecutor->finishActiveMigration();

		// Check to see if all migration executors are finished
		bool allExecutorsDone(true);
		for (auto it = executorMap.begin(); it != executorMap.end(); ++it) {
			if (!it->second->isMigrationComplete()) {
				allExecutorsDone = false;
				break;
			}
		}
		if (allExecutorsDone) {
			LOGMIGRATE << "migrationid: " << migrationId
                << " All migration executors have finished. Starting cleanup thread...";
			std::thread t1([this, migrationId] {this->finishActiveMigration(MIGR_EXECUTOR, migrationId);});
			t1.detach();
		}
	}
	return (err);
}

// process the TxState request
Error
DmMigrationMgr::applyTxState(DmIoMigrationTxState* txStateReq) {
    fpi::CtrlNotifyTxStateMsgPtr txStateMsg = txStateReq->txStateMsg;
    fds_volid_t volId(txStateMsg->volume_id);
    auto uniqueId = std::make_pair(txStateReq->destUuid, volId);
    SCOPEDREAD(migrExecutorLock);
    DmMigrationExecutor::shared_ptr executor = getMigrationExecutor(uniqueId);
    if (executor == nullptr) {
    	if (isMigrationAborted()) {
			LOGMIGRATE << "Unable to find executor for volume " << volId << " during migration abort";
    	} else {
			LOGERROR << "Unable to find executor for volume " << volId;
			// this is an race cond error that needs to be fixed in dev env.
			// Only panic in debug build.
			// fds_assert(0);
    	}
        return ERR_NOT_FOUND;
    }

    Error err(ERR_OK);
    err = executor->processTxState(txStateMsg);

    if (!err.ok()) {
        LOGERROR << "Error applying migrated commit log: " << err;
    	abortMigration();
    }

    return (err);
}

/**
 * This thread should always be safe to call while holding locks, so don't add any locking
 */
void
DmMigrationMgr::abortMigration()
{
	bool notYetAborted = false;
	if (!std::atomic_compare_exchange_strong(&migrationAborted, &notYetAborted, true)) {
		// If not the first client or first executor, don't worry about the rest.
		return;
	}

	// Need to release a thread while this original one exits the read lock
	abort_thread = new std::thread(&DmMigrationMgr::abortMigrationReal, this);
}

void
DmMigrationMgr::abortMigrationReal()
{
	LOGERROR << "migrationid: " << DMT_version
        <<  "DM Migration aborting Migration with DMT version = " << DMT_version;

	{
		SCOPEDWRITE(migrExecutorLock);
		{
			SCOPEDWRITE(migrClientLock);
			DmMigrationClientMap::const_iterator citer (clientMap.begin());
			DmMigrationExecMap::const_iterator eiter (executorMap.begin());
			while (citer != clientMap.end()) {
				citer->second->abortMigration();
				++citer;
			}
			while (eiter != executorMap.end()) {
				eiter->second->abortMigration();
				++eiter;
			}
			std::lock_guard<std::mutex> lk(migrationBatchMutex);
			fds_verify(migrationAborted);
			dataManager.counters->totalMigrationsAborted.incr(1);
			clearClients();
			clearExecutors();
		}
	}

    if (OmStartMigrCb) {
    	LOGDEBUG << "Sending migration abort to OM";
    	OmStartMigrCb(ERR_DM_MIGRATION_ABORTED);
    }

    /*
     * At this time, we're done cleaning up migration abort so in case
     * another migration was initiated from the OM and are waiting,
     * we need to wake them up now.
     */
    migrationCV.notify_one();

	// set it to false to clean things up
	bool expected = true;
	std::atomic_compare_exchange_strong(&migrationAborted, &expected, false);

}

void
DmMigrationMgr::dumpDmIoMigrationDeltaBlobs(DmIoMigrationDeltaBlobs *deltaBlobReq)
{
	if (!deltaBlobReq) {return;}
	fpi::CtrlNotifyDeltaBlobsMsgPtr ptr = deltaBlobReq->deltaBlobsMsg;
	LOGMIGRATE << "Dumping DmIoMigrationDeltaBlobs msg: " << deltaBlobReq;
	dumpDmIoMigrationDeltaBlobs(ptr);
}

void
DmMigrationMgr::dumpDmIoMigrationDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg)
{
	std::string blobInfo;
	for (auto obj : msg->blob_obj_list) {
		std::string blobObjInfo = "Blob name: ";
		blobObjInfo.append(obj.blob_name);
		blobInfo.append(blobObjInfo);
	}

	LOGMIGRATE << "CtrlNotifyDeltaBlobsMSg volume = " << msg->volume_id
			<< " msg_seq_id = " << msg->msg_seq_id << " last ? " << msg->last_msg_seq_id
			<< " " << blobInfo;
}

void
DmMigrationMgr::dumpDmIoMigrationDeltaBlobDesc(DmIoMigrationDeltaBlobDesc *deltaBlobReq)
{
	if (!deltaBlobReq) {return;}
	fpi::CtrlNotifyDeltaBlobDescMsgPtr ptr = deltaBlobReq->deltaBlobDescMsg;
	LOGMIGRATE << "Dumping DmIoMigrationDeltaBlobDesc msg: " << deltaBlobReq;
	dumpDmIoMigrationDeltaBlobDesc(ptr);
}

void
DmMigrationMgr::dumpDmIoMigrationDeltaBlobDesc(fpi::CtrlNotifyDeltaBlobDescMsgPtr &msg)
{
	std::string blobInfo;
	for (auto obj : msg->blob_desc_list) {
		std::string blobObjInfo = "Blob name: ";
		blobObjInfo.append(obj.vol_blob_name);
		blobInfo.append(blobObjInfo);
	}

	LOGMIGRATE << "CtrlNotifyDeltaBlobsMSg volume = " << msg->volume_id
			<< " msg_seq_id = " << msg->msg_seq_id << " last ? " << msg->last_msg_seq_id
			<< " " << blobInfo;
}

void
DmMigrationMgr::waitForMigrationBatchToFinish(MigrationRole role)
{
    // If executor - wait for the maps to be empty.
    // If client - only wait for the abort to be done
	LOGMIGRATE << "Waiting for previous migrations to finish, if there is any.";
	if (role == MIGR_EXECUTOR) {
	    std::unique_lock<std::mutex> lk(migrationBatchMutex);
	    migrationCV.wait(lk, [this]{return (executorMap.empty() && clientMap.empty());});
	}

    // TODO: can we just use the join in place of the CV?
    if (abort_thread) {
        abort_thread->join();
        abort_thread = nullptr;

        LOGMIGRATE << "Done waiting for previous migration abort to finish";
    }

	LOGMIGRATE << "Done waiting for previous migrations to finish";
}

bool
DmMigrationMgr::shouldFilterDmt(fds_volid_t volId, fds_uint64_t dmt_version) {
    return (dmt_watermark[volId] >= dmt_version);
}

void
DmMigrationMgr::setDmtWatermark(fds_volid_t volId, fds_uint64_t dmt_version) {
    dmt_watermark[volId] = dmt_version;
}

void
DmMigrationMgr::clearClients() {

    LOGMIGRATE << " Waiting for all outstanding client async messages to be finished";

    util::StopWatch sw;
    for (auto client : clientMap) {
        auto dmClient = client.second;
        sw.reset();
        dmClient->waitForAsyncMsgs();
        auto elapsedMs = sw.getElapsedNanos()/1000000;
        if (elapsedMs > 2000) {
            LOGWARN << dmClient->logString() << " migrationclient for volid: "
                << dmClient->volId << " took: " << elapsedMs << " ms to drain outstanding IO";
        }
    }
    clientMap.clear();
	dataManager.counters->numberOfActiveMigrClients.set(0);

	stopMigrationStopWatch();
	dumpStats();
}

void
DmMigrationMgr::clearExecutors() {

    LOGMIGRATE << " Waiting for all outstanding executor async messages to be finished";

    util::StopWatch sw;
    for (auto executor : executorMap) {
        auto dmExecutor = executor.second;
        sw.reset();
        dmExecutor->waitForAsyncMsgs();
        auto elapsedMs = sw.getElapsedNanos()/1000000;
        if (elapsedMs > 2000) {
            LOGWARN << dmExecutor->logString() << " migrationexecutor for volid: "
                << dmExecutor->volumeUuid << " took: " << elapsedMs << " ms to drain outstanding IO";
        }
    }
	executorMap.clear();
	dataManager.counters->numberOfActiveMigrExecutors.set(0);

	stopMigrationStopWatch();
	dumpStats();
}

void
DmMigrationMgr::dumpStats() {
    dm::Counters* c = dataManager.counters;

    LOGNORMAL << "========================================================";
    LOGNORMAL << "Migration Statistics:";
    LOGNORMAL << "--------------------------------------------------------";
    LOGNORMAL << "Total Volumes Pulled: " << c->totalVolumesReceivedMigration.value();
    LOGNORMAL << "Total Volumes Sent: " << c->totalVolumesSentMigration.value();
    LOGNORMAL << "Total Migrations Aborted: " << c->totalMigrationsAborted.value();
    LOGNORMAL << "Total Bytes Migrated: " << c->totalSizeOfDataMigrated.value();
    LOGNORMAL << "Seconds spent for current migration: " << c->timeSpentForCurrentMigration.value();
    LOGNORMAL << "Seconds spent for all migrations: " << c->timeSpentForAllMigrations.value();
    LOGNORMAL << "========================================================";
}

void
DmMigrationMgr::migrationIdleTimeoutCheck()
{
    bool abort = false;
    auto curTsSec = util::getTimeStampSeconds();
    {
        SCOPEDREAD(migrExecutorLock);
        for (const auto &e : executorMap) {
            if (e.second->isMigrationIdle(curTsSec)) {
                abort = true;
                LOGWARN << e.second->logString()
                    << "Aborting migration. "
                    << " Migration for node " << e.first.first << " volume: "
                    << e.first.second << " has been idle."
                    << " currentTsSec: " << curTsSec
                    << " lastUpdateTsSec: " << e.second->getLastUpdateFromClientTsSec();
                break;
            }
        }
    }
    if (abort) {
        abortMigration();
    }
}

void
DmMigrationMgr::startMigrationStopWatch()
{
    bool notYetStarted = false;
    if (!std::atomic_compare_exchange_strong(&timerStarted, &notYetStarted, true)) {
        // First guy. start timer
        migrationStopWatch.reset();
        dataManager.counters->timeSpentForCurrentMigration.set(0);
        migrationStopWatch.start();
    }
}

void
DmMigrationMgr::stopMigrationStopWatch()
{
	bool expected = true;

 	if (std::atomic_compare_exchange_strong(&timerStarted, &expected, false)) {
	    // Time in seconds
	    dataManager.counters->timeSpentForCurrentMigration.set(migrationStopWatch.getElapsedNanos()/(1000.0*100*100*100));
	    dataManager.counters->timeSpentForAllMigrations.incr(dataManager.counters->timeSpentForCurrentMigration.value());
	    migrationStopWatch.reset();
	}
}
}  // namespace fds
