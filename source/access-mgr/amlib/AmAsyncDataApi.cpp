/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <map>
#include <string>
#include <vector>
#include <AmAsyncDataApi.h>
#include <handlermappings.h>
#include <responsehandler.h>
#include <StorHvisorNet.h>

namespace fds {

AmAsyncDataApi::AmAsyncDataApi() {
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

    responseApi = boost::make_shared<AmAsyncXdiResponse>();
}

AmAsyncDataApi::~AmAsyncDataApi() {
}

void
AmAsyncDataApi::setResponseApi(AmAsyncResponseApi::shared_ptr respApi) {
    responseApi = respApi;
}

void
AmAsyncDataApi::volumeStatus(const apis::RequestId& requestId,
                             const std::string& domainName,
                             const std::string& volumeName) {
    fds_panic("You shouldn't be here.");
}

void
AmAsyncDataApi::volumeStatus(boost::shared_ptr<apis::RequestId>& requestId,
                             boost::shared_ptr<std::string>& domainName,
                             boost::shared_ptr<std::string>& volumeName) {
}

void
AmAsyncDataApi::statBlob(const apis::RequestId& requestId,
                         const std::string& domainName,
                         const std::string& volumeName,
                         const std::string& blobName) {
    fds_panic("You shouldn't be here.");
}

void
AmAsyncDataApi::statBlob(boost::shared_ptr<apis::RequestId>& requestId,
                         boost::shared_ptr<std::string>& domainName,
                         boost::shared_ptr<std::string>& volumeName,
                         boost::shared_ptr<std::string>& blobName) {
}

void
AmAsyncDataApi::startBlobTx(const apis::RequestId& requestId,
                            const std::string& domainName,
                            const std::string& volumeName,
                            const std::string& blobName,
                            const fds_int32_t blobMode) {
    fds_panic("You shouldn't be here.");
}

void
AmAsyncDataApi::startBlobTx(boost::shared_ptr<apis::RequestId>& requestId,
                            boost::shared_ptr<std::string>& domainName,
                            boost::shared_ptr<std::string>& volumeName,
                            boost::shared_ptr<std::string>& blobName,
                            boost::shared_ptr<fds_int32_t>& blobMode) {
    if ((true == testUturnAll) ||
        (true == testUturnStartTx)) {
        LOGDEBUG << "Uturn testing start blob tx";
        return;
    }

    AsyncStartBlobTxResponseHandler::ptr handler(
        new AsyncStartBlobTxResponseHandler(responseApi,
                                            requestId));

    FdsBlobReq *blobReq = new StartBlobTxReq(invalid_vol_id,
                                             *volumeName,
                                             *blobName,
                                             *blobMode,
                                             SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);
}

void
AmAsyncDataApi::commitBlobTx(const apis::RequestId& requestId,
                             const std::string& domainName,
                             const std::string& volumeName,
                             const std::string& blobName,
                             const apis::TxDescriptor& txDesc) {
    fds_panic("You shouldn't be here.");
}

void
AmAsyncDataApi::commitBlobTx(boost::shared_ptr<apis::RequestId>& requestId,
                             boost::shared_ptr<std::string>& domainName,
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

    FdsBlobReq *blobReq = new CommitBlobTxReq(invalid_vol_id,
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
AmAsyncDataApi::abortBlobTx(const apis::RequestId& requestId,
                            const std::string& domainName,
                            const std::string& volumeName,
                            const std::string& blobName,
                            const apis::TxDescriptor& txDesc) {
    fds_panic("You shouldn't be here.");
}

void
AmAsyncDataApi::abortBlobTx(boost::shared_ptr<apis::RequestId>& requestId,
                            boost::shared_ptr<std::string>& domainName,
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

    FdsBlobReq *blobReq = new AbortBlobTxReq(invalid_vol_id,
                                             *volumeName,
                                             *blobName,
                                             blobTxDesc,
                                             SHARED_DYN_CAST(Callback, handler));
    storHvisor->enqueueBlobReq(blobReq);

    handler->wait();
    handler->process();
}

void
AmAsyncDataApi::getBlob(const apis::RequestId& requestId,
                        const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const int32_t length,
                        const apis::ObjectOffset& offset) {
    fds_panic("You shouldn't be here.");
}

void
AmAsyncDataApi::getBlob(boost::shared_ptr<apis::RequestId>& requestId,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<int32_t>& length,
                        boost::shared_ptr<apis::ObjectOffset>& objectOffset) {
}

void
AmAsyncDataApi::updateMetadata(const apis::RequestId& requestId,
                               const std::string& domainName,
                               const std::string& volumeName,
                               const std::string& blobName,
                               const apis::TxDescriptor& txDesc,
                               const std::map<std::string, std::string> & metadata) {
    fds_panic("You shouldn't be here.");
}

void
AmAsyncDataApi::updateMetadata(boost::shared_ptr<apis::RequestId>& requestId,
                               boost::shared_ptr<std::string>& domainName,
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

    FdsBlobReq *blobReq = new SetBlobMetaDataReq(invalid_vol_id,
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
AmAsyncDataApi::updateBlobOnce(const apis::RequestId& requestId,
                               const std::string& domainName,
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
AmAsyncDataApi::updateBlobOnce(boost::shared_ptr<apis::RequestId>& requestId,
                               boost::shared_ptr<std::string>& domainName,
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

    FdsBlobReq *blobReq = new PutBlobReq(invalid_vol_id,
                                         *volumeName,
                                         *blobName,
                                         static_cast<fds_uint64_t>(objectOffset->value),
                                         *length,
                                         const_cast<char *>(bytes->c_str()),
                                         *blobMode,
                                         metadata,
                                         putHandler);
    storHvisor->enqueueBlobReq(blobReq);

    // Wait for a signal from the callback thread
    putHandler->wait();

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
AmAsyncDataApi::updateBlob(const apis::RequestId& requestId,
                           const std::string& domainName,
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
AmAsyncDataApi::updateBlob(boost::shared_ptr<apis::RequestId>& requestId,
                           boost::shared_ptr<std::string>& domainName,
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

    FdsBlobReq *blobReq = new PutBlobReq(invalid_vol_id,
                                         *volumeName,
                                         *blobName,
                                         static_cast<fds_uint64_t>(objectOffset->value),
                                         *length,
                                         const_cast<char *>(bytes->c_str()),
                                         blobTxDesc,
                                         *isLast,
                                         &bucket_ctx,
                                         NULL,
                                         NULL,
                                         putHandler);
    storHvisor->enqueueBlobReq(blobReq);

    // Wait for a signal from the callback thread
    putHandler->wait();

    LOGDEBUG << "Finishing updateBlob for blob " << *blobName
             << " at object offset " << objectOffset->value;

    // Throw an exception if we didn't get an OK response
    if (putHandler->status != ERR_OK) {
        apis::ApiException fdsE;
        throw fdsE;
    }
}

void
AmAsyncDataApi::deleteBlob(const apis::RequestId& requestId,
                           const std::string& domainName,
                           const std::string& volumeName,
                           const std::string& blobName) {
    fds_panic("You shouldn't be here.");
}

void
AmAsyncDataApi::deleteBlob(boost::shared_ptr<apis::RequestId>& requestId,
                           boost::shared_ptr<std::string>& domainName,
                           boost::shared_ptr<std::string>& volumeName,
                           boost::shared_ptr<std::string>& blobName) {
}
}  // namespace fds
