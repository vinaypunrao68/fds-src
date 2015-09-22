/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <fds_assert.h>

#include <functional>

namespace fds {
namespace dm {

ForwardCatalogUpdateHandler::ForwardCatalogUpdateHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::ForwardCatalogMsg, handleRequest);
    }
}

void ForwardCatalogUpdateHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::ForwardCatalogMsg>& message) {
    auto dmReq = new DmIoFwdCat(message);
    DBG(LOGMIGRATE << "Enqueued new forward request " << logString(*asyncHdr)
        << " " << *reinterpret_cast<DmIoFwdCat*>(dmReq));
    /* Route the message to right executor.  If static migration is in progress
     * the message will be buffered.  Otherwise it will be handed to qos ctrlr
     * to be applied immediately
     */
    auto error = dataManager.dmMigrationMgr->handleForwardedCommits(dmReq);

    asyncHdr->msg_code = error.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::ForwardCatalogRspMsgTypeId, fpi::ForwardCatalogRspMsg());
}

void ForwardCatalogUpdateHandler::handleQueueItem(DmRequest* dmRequest) {
    DmIoFwdCat* typedRequest = static_cast<DmIoFwdCat*>(dmRequest);
    QueueHelper helper(dataManager, typedRequest);

    if (typedRequest->fwdCatMsg->lastForward &&
        typedRequest->fwdCatMsg->blob_name.empty()) {
    	LOGMIGRATE << "Received empty last forward for volume "
    			<< typedRequest->fwdCatMsg->volume_id;
    	/**
    	 * Need to signal to the migration manager that active Forwards is complete and
    	 * there will not be anymore forwards.
    	 */
    	dataManager.dmMigrationMgr->
			finishActiveMigration(fds_volid_t(typedRequest->fwdCatMsg->volume_id));
    	return;
    }

    DmCommitLog::ptr commitLog;
    auto err = dataManager.timeVolCat_->getCommitlog(fds_volid_t(typedRequest->fwdCatMsg->volume_id), commitLog);

    if (!err.ok() || !commitLog) {
        LOGMIGRATE << "Failed to get commit log when processing fwd blob " << *typedRequest;
        helper.err = ERR_DM_FORWARD_FAILED;
    } else {
        auto txDesc = boost::make_shared<const BlobTxId>(typedRequest->fwdCatMsg->txId);
        commitLog->rollbackTx(txDesc);

        LOGMIGRATE << "Will commit fwd blob " << *typedRequest << " to tvc";

        helper.err = dataManager.timeVolCat_->updateFwdCommittedBlob(
            static_cast<fds_volid_t>(typedRequest->fwdCatMsg->volume_id),
            typedRequest->fwdCatMsg->blob_name,
            typedRequest->fwdCatMsg->blob_version,
            typedRequest->fwdCatMsg->obj_list,
            typedRequest->fwdCatMsg->meta_list,
            typedRequest->fwdCatMsg->sequence_id);
    }
}

}  // namespace dm
}  // namespace fds
