/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <map>
#include <string>
#include <vector>
#include <AmDataApi.h>
#include <responsehandler.h>
#include "fds_process.h"
#include "StorHvCtrl.h"

#include "requests/requests.h"

namespace fds {

AmDataApi::AmDataApi()
        : io_log_counter(0),
          io_log_interval(100) {
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
    testUturnAll = conf.get_abs<bool>("fds.am.testing.uturn_amserv_all");
    if (true == testUturnAll) {
        LOGDEBUG << "Enabling uturn testing for all AM service APIs";
    }
    testUturnStartTx = conf.get_abs<bool>("fds.am.testing.uturn_amserv_starttx");
    if (true == testUturnStartTx) {
        LOGDEBUG << "Enabling uturn testing for AM service start tx API";
    }
    testUturnUpdateBlob = conf.get_abs<bool>("fds.am.testing.uturn_amserv_updateblob");
    if (true == testUturnUpdateBlob) {
        LOGDEBUG << "Enabling uturn testing for AM service update blob API";
    }
    testUturnUpdateMeta = conf.get_abs<bool>("fds.am.testing.uturn_amserv_updatemeta");
    if (true == testUturnUpdateMeta) {
        LOGDEBUG << "Enabling uturn testing for AM service update metadata API";
    }
    testUturnCommitTx = conf.get_abs<bool>("fds.am.testing.uturn_amserv_committx");
    if (true == testUturnCommitTx) {
        LOGDEBUG << "Enabling uturn testing for AM service commit tx API";
    }
    testUturnAbortTx = conf.get_abs<bool>("fds.am.testing.uturn_amserv_aborttx");
    if (true == testUturnAbortTx) {
        LOGDEBUG << "Enabling uturn testing for AM service abort tx API";
    }
    testUturnStatBlob = conf.get_abs<bool>("fds.am.testing.uturn_amserv_statblob");
    if (true == testUturnStatBlob) {
        LOGDEBUG << "Enabling uturn testing for AM service stat blob API";
    }
    testUturnGetBlob = conf.get_abs<bool>("fds.am.testing.uturn_amserv_getblob");
    if (true == testUturnGetBlob) {
        LOGDEBUG << "Enabling uturn testing for AM service get blob API";
    }
}

AmDataApi::~AmDataApi() {
}

void
AmDataApi::attachVolume(const std::string& domainName,
                        const std::string& volumeName) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::attachVolume(boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName) {
    AttachVolumeResponseHandler::ptr handler(
        new AttachVolumeResponseHandler());
    storHvisor->enqueueAttachReq(*volumeName,
                                 SHARED_DYN_CAST(Callback, handler));
    handler->wait();
    handler->process();
}

void
AmDataApi::volumeStatus(apis::VolumeStatus& _return,
                        const std::string& domainName,
                        const std::string& volumeName) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::volumeStatus(apis::VolumeStatus& _return,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName) {
    StatVolumeResponseHandler::ptr handler(new StatVolumeResponseHandler(_return));
    AmRequest *blobReq = new GetVolumeMetaDataReq(invalid_vol_id,
                                                   *volumeName,
                                                   SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    handler->process();
}

void
AmDataApi::volumeContents(std::vector<apis::BlobDescriptor> & _return,
                          const std::string& domainName,
                          const std::string& volumeName,
                          const int32_t count,
                          const int64_t offset) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::volumeContents(std::vector<apis::BlobDescriptor> & _return,
                          boost::shared_ptr<std::string>& domainName,
                          boost::shared_ptr<std::string>& volumeName,
                          boost::shared_ptr<int32_t>& count,
                          boost::shared_ptr<int64_t>& offset) {
    ListBucketResponseHandler::ptr handler(new ListBucketResponseHandler(_return));
    AmRequest *blobReq = new VolumeContentsReq(invalid_vol_id,
                                               *volumeName,
                                               *count,
                                               SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    handler->process();
}

void
AmDataApi::statBlob(apis::BlobDescriptor& _return,
                    const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::statBlob(apis::BlobDescriptor& _return,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName) {
    if ((true == testUturnAll) ||
        (true == testUturnStatBlob)) {
        LOGDEBUG << "Uturn testing stat blob";
        _return.name = *blobName;
        _return.byteCount = 0;
        return;
    }

    StatBlobResponseHandler::ptr handler(
        new StatBlobResponseHandler(_return));
    AmRequest *blobReq = new StatBlobReq(invalid_vol_id,
                                          *volumeName,
                                          *blobName,
                                          SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    handler->process();
}

void
AmDataApi::startBlobTx(apis::TxDescriptor& _return,
                       const std::string& domainName,
                       const std::string& volumeName,
                       const std::string& blobName,
                       const fds_int32_t blobMode) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::startBlobTx(apis::TxDescriptor& _return,
                       boost::shared_ptr<std::string>& domainName,
                       boost::shared_ptr<std::string>& volumeName,
                       boost::shared_ptr<std::string>& blobName,
                       boost::shared_ptr<fds_int32_t>& blobMode) {
    if ((true == testUturnAll) ||
        (true == testUturnStartTx)) {
        LOGDEBUG << "Uturn testing start blob tx";
        return;
    }

    StartBlobTxResponseHandler::ptr handler(
        new StartBlobTxResponseHandler(_return));

    AmRequest *blobReq = new StartBlobTxReq(invalid_vol_id,
                                             *volumeName,
                                             *blobName,
                                             *blobMode,
                                             SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    handler->process();
}

void
AmDataApi::commitBlobTx(const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const apis::TxDescriptor& txDesc) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::commitBlobTx(boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    if ((true == testUturnAll) ||
        (true == testUturnCommitTx)) {
        LOGDEBUG << "Uturn testing commit blob tx";
        return;
    }

    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
        txDesc->txId));

    SimpleResponseHandler::ptr handler(new SimpleResponseHandler(__func__));

    AmRequest *blobReq = new CommitBlobTxReq(invalid_vol_id,
                                              *volumeName,
                                              *blobName,
                                              blobTxDesc,
                                              SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    if (handler->error != ERR_OK) {
        apis::ApiException fdsE;
        if (handler->error == ERR_BLOB_NOT_FOUND) {
            fdsE.errorCode = apis::MISSING_RESOURCE;
        } else {
            fdsE.errorCode = apis::BAD_REQUEST;
        }
        throw fdsE;
    }
}

void
AmDataApi::abortBlobTx(const std::string& domainName,
                       const std::string& volumeName,
                       const std::string& blobName,
                       const apis::TxDescriptor& txDesc) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::abortBlobTx(boost::shared_ptr<std::string>& domainName,
                       boost::shared_ptr<std::string>& volumeName,
                       boost::shared_ptr<std::string>& blobName,
                       boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    if ((true == testUturnAll) ||
        (true == testUturnAbortTx)) {
        LOGDEBUG << "Uturn testing abort blob tx";
        return;
    }

    SimpleResponseHandler::ptr handler(new SimpleResponseHandler(__func__));

    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
        txDesc->txId));

    AmRequest *blobReq = new AbortBlobTxReq(invalid_vol_id,
                                             *volumeName,
                                             *blobName,
                                             blobTxDesc,
                                             SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    handler->process();
}

void
AmDataApi::getBlob(std::string& _return,
                   const std::string& domainName,
                   const std::string& volumeName,
                   const std::string& blobName,
                   const int32_t length,
                   const apis::ObjectOffset& offset) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::getBlob(std::string& _return,
                   boost::shared_ptr<std::string>& domainName,
                   boost::shared_ptr<std::string>& volumeName,
                   boost::shared_ptr<std::string>& blobName,
                   boost::shared_ptr<int32_t>& length,
                   boost::shared_ptr<apis::ObjectOffset>& objectOffset) {
    if ((true == testUturnAll) ||
        (true == testUturnGetBlob)) {
        LOGDEBUG << "Uturn testing get blob";
        return;
    }

    BucketContextPtr bucket_ctx(
        new BucketContext("host", *volumeName, "accessid", "secretkey"));

    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Create request context
    // Set the pointer we want filled in the handler
    // TODO(Andrew): Get the correctly sized pointer directly
    // from the return string so we can avoid one extra copy.
    GetObjectResponseHandler::ptr getHandler(new GetObjectResponseHandler());

    AmRequest *blobReq= new GetBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        SHARED_DYN_CAST(Callback, getHandler),
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length);
    storHvisor->enqueueBlobReq(blobReq);

    // Wait for a signal from the callback thread
    getHandler->wait();

    if (getHandler->error != ERR_OK) {
        apis::ApiException fdsE;
        if (getHandler->error == ERR_BLOB_NOT_FOUND) {
            fdsE.errorCode = apis::MISSING_RESOURCE;
        } else {
            fdsE.errorCode = apis::BAD_REQUEST;
        }
        throw fdsE;
    }

    boost::shared_ptr<std::string> buf = getHandler->returnBuffer;
    _return = buf->size() > getHandler->returnSize ?
        std::string(*buf, 0, getHandler->returnSize)
        : *buf;
}

void
AmDataApi::updateMetadata(const std::string& domainName,
                          const std::string& volumeName,
                          const std::string& blobName,
                          const apis::TxDescriptor& txDesc,
                          const std::map<std::string, std::string> & metadata) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::updateMetadata(boost::shared_ptr<std::string>& domainName,
                          boost::shared_ptr<std::string>& volumeName,
                          boost::shared_ptr<std::string>& blobName,
                          boost::shared_ptr<apis::TxDescriptor>& txDesc,
                          boost::shared_ptr< std::map<std::string, std::string> >& metadata) {
    if ((true == testUturnAll) ||
        (true == testUturnUpdateMeta)) {
        LOGDEBUG << "Uturn testing update metadata";
        return;
    }
    SimpleResponseHandler::ptr handler(new SimpleResponseHandler(__func__));
    boost::shared_ptr<fpi::FDSP_MetaDataList> metaDataList(new fpi::FDSP_MetaDataList());
    LOGDEBUG << "received updateMetadata cmd";
    fpi::FDSP_MetaDataPair metaPair;
    for (auto const & meta : *metadata) {
        LOGDEBUG << meta.first << ":" << meta.second;
        metaPair.key = meta.first;
        metaPair.value = meta.second;
        metaDataList->push_back(metaPair);
    }
    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
        txDesc->txId));

    AmRequest *blobReq = new SetBlobMetaDataReq(invalid_vol_id,
                                                 *volumeName,
                                                 *blobName,
                                                 blobTxDesc,
                                                 metaDataList,
                                                 SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    LOGDEBUG << "set meta data returned";
    handler->process();
}

void
AmDataApi::updateBlobOnce(const std::string& domainName,
                          const std::string& volumeName,
                          const std::string& blobName,
                          const fds_int32_t blobMode,
                          const std::string& bytes,
                          const int32_t length,
                          const apis::ObjectOffset& objectOffset,
                          const std::map<std::string, std::string>& metadata) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::updateBlobOnce(boost::shared_ptr<std::string>& domainName,
                          boost::shared_ptr<std::string>& volumeName,
                          boost::shared_ptr<std::string>& blobName,
                          boost::shared_ptr<fds_int32_t>& blobMode,
                          boost::shared_ptr<std::string>& bytes,
                          boost::shared_ptr<int32_t>& length,
                          boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                          boost::shared_ptr< std::map<std::string, std::string> >& metadata) {
    if ((true == testUturnAll) ||
        (true == testUturnUpdateBlob)) {
        LOGDEBUG << "Uturn testing update blob";
        return;
    }

    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Create context handler
    UpdateBlobResponseHandler::ptr putHandler(
        boost::make_shared<UpdateBlobResponseHandler>());

    AmRequest *blobReq = new PutBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length,
                                        bytes,
                                        *blobMode,
                                        metadata,
                                        putHandler);
    storHvisor->enqueueBlobReq(blobReq);

    // Wait for a signal from the callback thread
    putHandler->wait();

    ++io_log_counter;
    if ((io_log_counter % io_log_interval) == 0) {
        LOGNORMAL << "Finishing updateBlobOnce for blob " << *blobName
                  << " at object offset " << objectOffset->value
                  << ", log_counter = " << io_log_counter;
    }
    LOGDEBUG << "Finishing updateBlobOnce for blob " << *blobName
             << " at object offset " << objectOffset->value
             << " status:" << putHandler->status;

        // Throw an exception if we didn't get an OK response
    if (putHandler->status != ERR_OK) {
        apis::ApiException fdsE;
        throw fdsE;
    }
}

void
AmDataApi::updateBlob(const std::string& domainName,
                      const std::string& volumeName,
                      const std::string& blobName,
                      const apis::TxDescriptor& txDesc,
                      const std::string& bytes,
                      const int32_t length,
                      const apis::ObjectOffset& objectOffset,
                      const bool isLast) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::updateBlob(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<std::string>& blobName,
                      boost::shared_ptr<apis::TxDescriptor>& txDesc,
                      boost::shared_ptr<std::string>& bytes,
                      boost::shared_ptr<int32_t>& length,
                      boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                      boost::shared_ptr<bool>& isLast) {
    if ((true == testUturnAll) ||
        (true == testUturnUpdateBlob)) {
        LOGDEBUG << "Uturn testing update blob";
        return;
    }

    BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Create context handler
    UpdateBlobResponseHandler::ptr putHandler(
        boost::make_shared<UpdateBlobResponseHandler>());

    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
        txDesc->txId));

    AmRequest *blobReq = new PutBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length,
                                        bytes,
                                        blobTxDesc,
                                        *isLast,
                                        &bucket_ctx,
                                        NULL,
                                        NULL,
                                        putHandler);
    storHvisor->enqueueBlobReq(blobReq);

    // Wait for a signal from the callback thread
    putHandler->wait();

    ++io_log_counter;
    if ((io_log_counter % io_log_interval) == 0) {
        LOGNORMAL << "Finishing updateBlob for blob " << *blobName
                  << " at object offset " << objectOffset->value
                  << ", log_counter = " << io_log_counter;
    }
    LOGDEBUG << "Finishing updateBlob for blob " << *blobName
             << " at object offset " << objectOffset->value;

    // Throw an exception if we didn't get an OK response
    if (putHandler->status != ERR_OK) {
        apis::ApiException fdsE;
        throw fdsE;
    }
}

void
AmDataApi::deleteBlob(const std::string& domainName,
                      const std::string& volumeName,
                      const std::string& blobName) {
    fds_panic("You shouldn't be here.");
}

void
AmDataApi::deleteBlob(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<std::string>& blobName) {
    apis::TxDescriptor txnId;
    boost::shared_ptr<fds_int32_t> bmode(new fds_int32_t(0));
    startBlobTx(txnId, domainName, volumeName, blobName, bmode);
    BlobTxId::ptr blobTxId(new BlobTxId(txnId.txId));

    SimpleResponseHandler::ptr handler(new SimpleResponseHandler(__func__));
    AmRequest *blobReq = new DeleteBlobReq(invalid_vol_id,
                                           *blobName,
                                           *volumeName,
                                           blobTxId,
                                           SHARED_DYN_CAST(Callback, handler));

    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    boost::shared_ptr<apis::TxDescriptor> txnPtr(new apis::TxDescriptor());
    txnPtr->txId = txnId.txId;
    try {
        handler->process();
    } catch(apis::ApiException& e) {
        LOGDEBUG << "delete failed with exception : " << e.what();
        try {
            abortBlobTx(domainName, volumeName, blobName, txnPtr);
        } catch(apis::ApiException& e) {
            LOGERROR << "exception @ abortBlobTx : " << e.what();
        }
        throw e;
    }
    commitBlobTx(domainName, volumeName, blobName, txnPtr);
}

}  // namespace fds
