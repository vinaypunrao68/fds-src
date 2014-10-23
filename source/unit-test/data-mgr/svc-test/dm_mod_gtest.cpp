/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <dm_mod_gtest.h>
#include <net/SvcRequest.h>
#include <net/net-service-tmpl.hpp>
static fds_uint32_t MAX_OBJECT_SIZE = 1024 * 1024 * 2;    // 2MB
static fds_uint32_t NUM_OBJTS = 1;    // 2MB
static fds_uint64_t BLOB_SIZE = static_cast<fds_uint64_t>(10) * 1024 * 1024 * 1024;   // 1GB

boost::shared_ptr<LatencyCounter> startTxCounter(new LatencyCounter("startBLobTx", 0, 0));
boost::shared_ptr<LatencyCounter> updateTxCounter(new LatencyCounter("updateBlobTx", 0, 0));
boost::shared_ptr<LatencyCounter> commitTxCounter(new LatencyCounter("commitBlobTx", 0, 0));
boost::shared_ptr<LatencyCounter> getBlobMetaCounter(new LatencyCounter("getBlobMeta", 0, 0));
boost::shared_ptr<LatencyCounter> qryCatCounter(new LatencyCounter("getBlobMeta", 0, 0));
boost::shared_ptr<LatencyCounter> setBlobMetaCounter(new LatencyCounter("getBlobMeta", 0, 0));
boost::shared_ptr<LatencyCounter> delCatObjCounter(new LatencyCounter("getBlobMeta", 0, 0));
boost::shared_ptr<LatencyCounter> getBucketCounter(new LatencyCounter("getBlobMeta", 0, 0));
boost::shared_ptr<LatencyCounter> txCounter(new LatencyCounter("tx", 0, 0));

static fds_uint64_t txStartTs = 0;

/**
* @brief basic  DM tests- startBlob, UpdateBlob and Commit  
*
*/

TEST_F(DMApi, metaDataTest)
{
    std::string blobName("testBlob");

    fpi::SvcUuid svcUuid;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    // start transaction
    SvcRequestCbTask<EPSvcRequest, fpi::StartBlobTxRspMsg> waiter;
    auto startBlobTx = SvcMsgFactory::newStartBlobTxMsg(volId_, blobName);
    auto asyncBlobTxReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    startBlobTx->txId = 1;
    startBlobTx->dmt_version = 1;
    startBlobTx->blob_mode = 0;
    asyncBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg), startBlobTx);
    asyncBlobTxReq->onResponseCb(waiter.cb);
    startBlobTxIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    txStartTs = startTs;
    asyncBlobTxReq->invoke();
    waiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    startTxCounter->update(endTs - startTs);
    }
    ASSERT_EQ(waiter.error, ERR_OK) << "Error: " << waiter.error;

    SvcRequestCbTask<EPSvcRequest, fpi::UpdateCatalogRspMsg> updWaiter;
    auto updateCatMsg = SvcMsgFactory::newUpdateCatalogMsg(volId_, blobName);
    auto asyncUpdCatReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
         updateCatMsg->obj_list.clear();
    fds::UpdateBlobInfoNoData(updateCatMsg, MAX_OBJECT_SIZE, BLOB_SIZE);
    updateCatMsg->txId = 1;

    asyncUpdCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg), updateCatMsg);
    asyncUpdCatReq->onResponseCb(updWaiter.cb);
    updateCatIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    asyncUpdCatReq->invoke();
    updWaiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    updateTxCounter->update(endTs - startTs);
    }
    ASSERT_EQ(updWaiter.error, ERR_OK) << "Error: " << updWaiter.error;

    SvcRequestCbTask<EPSvcRequest, fpi::CommitBlobTxRspMsg> commitWaiter;
    auto commitBlobMsg = SvcMsgFactory::newCommitBlobTxMsg(volId_, blobName);
    auto asyncCommitBlobReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    commitBlobMsg->txId  = 1;
    commitBlobMsg->dmt_version = 1;
    asyncCommitBlobReq->setPayload(FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg), commitBlobMsg);
    asyncCommitBlobReq->onResponseCb(commitWaiter.cb);
    commitBlobTxIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    asyncCommitBlobReq->invoke();
    commitWaiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    commitTxCounter->update(endTs - startTs);
    txCounter->update(endTs - txStartTs);
    }
    ASSERT_EQ(commitWaiter.error, ERR_OK) << "Error: " << commitWaiter.error;

    std::cout << "\033[33m[startTx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (startTxCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << startTxCounter->count() << std::endl;
    std::cout << "\033[33m[updateBlobTx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (updateTxCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << updateTxCounter->count() << std::endl;
    std::cout << "\033[33m[CommitBlobTx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (commitTxCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << commitTxCounter->count() << std::endl;
    std::cout << "\033[33m[Tx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (txCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << txCounter->count() << std::endl;
}


TEST_F(DMApi, getBlobMetaTest)
{
    std::string blobName("testBlob");
    fpi::SvcUuid svcUuid;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    SvcRequestCbTask<EPSvcRequest, fpi::GetBlobMetaDataMsg> getMetaWaiter;
    auto getBlobMeta = SvcMsgFactory::newGetBlobMetaDataMsg(volId_, blobName);
    auto asyncGetBlobMetaReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    asyncGetBlobMetaReq->setPayload(FDSP_MSG_TYPEID(fpi::GetBlobMetaDataMsg), getBlobMeta);
    asyncGetBlobMetaReq->onResponseCb(getMetaWaiter.cb);
    getBlobMetaIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    asyncGetBlobMetaReq->invoke();
    getMetaWaiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    getBlobMetaCounter->update(endTs - startTs);
    }
    ASSERT_EQ(getMetaWaiter.error, ERR_OK) << "Error: " << getMetaWaiter.error;
    std::cout << "\033[33m[Tx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (getBlobMetaCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << getBlobMetaCounter->count() << std::endl;
}

TEST_F(DMApi, qryCatTest)
{
    std::string blobName("testBlob");
    fpi::SvcUuid svcUuid;
    uint64_t blobOffset = 0;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);


    SvcRequestCbTask<EPSvcRequest, fpi::QueryCatalogRspMsg> qryCatWaiter;
    auto qryCat = SvcMsgFactory::newQueryCatalogMsg(volId_, blobName, blobOffset);
    auto asyncQryCatReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    asyncQryCatReq->setPayload(FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), qryCat);
    asyncQryCatReq->onResponseCb(qryCatWaiter.cb);
    queryCatIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    asyncQryCatReq->invoke();
    qryCatWaiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    qryCatCounter->update(endTs - startTs);
    }
    ASSERT_EQ(qryCatWaiter.error, ERR_OK) << "Error: " << qryCatWaiter.error;
    std::cout << "\033[33m[Tx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (qryCatCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << qryCatCounter->count() << std::endl;
}

TEST_F(DMApi, setBlobMeta)
{
    std::string blobName("testBlob");
    fpi::SvcUuid svcUuid;
    uint64_t blobOffset = 0;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    // start transaction
    SvcRequestCbTask<EPSvcRequest, fpi::StartBlobTxRspMsg> waiter;
    auto startBlobTx = SvcMsgFactory::newStartBlobTxMsg(volId_, blobName);
    auto asyncBlobTxReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    startBlobTx->txId = 2;
    startBlobTx->dmt_version = 1;
    startBlobTx->blob_mode = 0;
    asyncBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg), startBlobTx);
    asyncBlobTxReq->onResponseCb(waiter.cb);
    startBlobTxIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    txStartTs = startTs;
    asyncBlobTxReq->invoke();
    waiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    startTxCounter->update(endTs - startTs);
    }
    ASSERT_EQ(waiter.error, ERR_OK) << "Error: " << waiter.error;


    SvcRequestCbTask<EPSvcRequest, fpi::SetBlobMetaDataRspMsg> setBlobWaiter;
    auto setBlobMeta = SvcMsgFactory::newSetBlobMetaDataMsg(volId_, blobName);
    auto setBlobMetaReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    setBlobMeta->txId  = 2;
    setBlobMetaReq->setPayload(FDSP_MSG_TYPEID(fpi::SetBlobMetaDataMsg), setBlobMeta);
    setBlobMetaReq->onResponseCb(setBlobWaiter.cb);
    setBlobMetaIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    setBlobMetaReq->invoke();
    setBlobWaiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    setBlobMetaCounter->update(endTs - startTs);
    }
    ASSERT_EQ(setBlobWaiter.error, ERR_OK) << "Error: " << setBlobWaiter.error;

    SvcRequestCbTask<EPSvcRequest, fpi::CommitBlobTxRspMsg> commitWaiter;
    auto commitBlobMsg = SvcMsgFactory::newCommitBlobTxMsg(volId_, blobName);
    auto asyncCommitBlobReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    commitBlobMsg->txId  = 2;
    commitBlobMsg->dmt_version = 1;
    asyncCommitBlobReq->setPayload(FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg), commitBlobMsg);
    asyncCommitBlobReq->onResponseCb(commitWaiter.cb);
    commitBlobTxIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    asyncCommitBlobReq->invoke();
    commitWaiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    commitTxCounter->update(endTs - startTs);
    txCounter->update(endTs - txStartTs);
    }
    ASSERT_EQ(commitWaiter.error, ERR_OK) << "Error: " << commitWaiter.error;

    std::cout << "\033[33m[startBlobTx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (startTxCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << startTxCounter->count() << std::endl;
    std::cout << "\033[33m[setBlobMeta latency]\033[39m " << std::fixed << std::setprecision(3)
            << (setBlobMetaCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << setBlobMetaCounter->count() << std::endl;
    std::cout << "\033[33m[commitBlobTx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (commitTxCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << commitTxCounter->count() << std::endl;
    std::cout << "\033[33m[Tx latency]\033[39m " << std::fixed << std::setprecision(3)
            << (txCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << txCounter->count() << std::endl;
}


#if 0
TEST_F(DMApi, deleteCatObject)
{
    std::string blobName("testBlob");
    fpi::SvcUuid svcUuid;
    uint64_t blobOffset = 0;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);


    SvcRequestCbTask<EPSvcRequest, fpi::DeleteCatalogObjectRspMsg> delCatObjWaiter;
    auto delCatObj = SvcMsgFactory::newDeleteCatalogObjectMsg(volId_, blobName);
    auto delCatObjReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    delCatObjReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteCatalogObjectMsg), delCatObj);
    delCatObjReq->onResponseCb(delCatObjWaiter.cb);
    setBlobMetaIssued_++;
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    delCatObjReq->invoke();
    delCatObjWaiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    delCatObjCounter->update(endTs - startTs);
    }
    ASSERT_EQ(delCatObjWaiter.error, ERR_OK) << "Error: " << delCatObjWaiter.error;
    std::cout << "\033[33m[deleteBlobMeta latency]\033[39m " << std::fixed << std::setprecision(3)
            << (delCatObjCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << delCatObjCounter->count() << std::endl;
}


TEST_F(DMApi, getBucket)
{
    std::string blobName("testBlob");
    fpi::SvcUuid svcUuid;
    uint64_t blobOffset = 0;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    SvcRequestCbTask<EPSvcRequest, fpi::GetBucketMsg> getBucketWaiter;
    auto getBucket = SvcMsgFactory::newGetBucketMsg(volId_, 0);
    auto getBucketReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    getBucketReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteCatalogObjectMsg), getBucket);
    getBucketReq->onResponseCb(getBucketWaiter.cb);
    {
    fds_uint64_t startTs = util::getTimeStampNanos();
    getBucketReq->invoke();
    getBucketWaiter.await();
    fds_uint64_t endTs = util::getTimeStampNanos();
    getBucketCounter->update(endTs - startTs);
    }
    ASSERT_EQ(getBucketWaiter.error, ERR_OK) << "Error: " << getBucketWaiter.error;
    std::cout << "\033[33m[getBucket latency]\033[39m " << std::fixed << std::setprecision(3)
            << (getBucketCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << getBucketCounter->count() << std::endl;
}
#endif

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>(), "puts count");
    DMApi::init(argc, argv, opts, "vol1");
    return RUN_ALL_TESTS();
}
