/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <dm_mod_gtest.h>
static fds_uint32_t MAX_OBJECT_SIZE = 1024 * 1024 * 2;    // 2MB
static fds_uint32_t NUM_OBJTS = 1;    // 2MB
static fds_uint64_t BLOB_SIZE = 1 * 1024 * 1024 * 1024;   // 1GB


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
    auto startBlobTx = SvcMsgFactory::newStartBlobTxMsg(volId_, blobName);
    auto asyncBlobTxReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    startBlobTx->txId = 1;
    startBlobTx->dmt_version = 1;
    startBlobTx->blob_mode = 0;
    asyncBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg), startBlobTx);
    asyncBlobTxReq->onResponseCb(std::bind(&DMApi::startBlobTxCb, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));
    startBlobTxIssued_++;
    asyncBlobTxReq->invoke();
     //  wait for the request to complete
    POLL_MS((startBlobTxIssued_ == startBlobTxFailed_ + startBlobTxSuccess_), 500, 1000);
    // update request
    auto updateCatMsg = SvcMsgFactory::newUpdateCatalogMsg(volId_, blobName);
         updateCatMsg->obj_list.clear();
    fds::UpdateBlobInfoNoData(updateCatMsg, MAX_OBJECT_SIZE, BLOB_SIZE);
    updateCatMsg->txId = 1;

    auto asyncUpdCatReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    asyncUpdCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg), updateCatMsg);
    asyncUpdCatReq->onResponseCb(std::bind(&DMApi::updateCatCb, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));
    updateCatIssued_++;
    asyncUpdCatReq->invoke();
     //  wait for the request to complete
    POLL_MS((updateCatIssued_ == updateCatFailed_ + updateCatSuccess_), 500, 1000);
    // Commit  request
    auto commitBlobMsg = SvcMsgFactory::newCommitBlobTxMsg(volId_, blobName);
    auto asyncCommitBlobReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    commitBlobMsg->txId  = 1;
    commitBlobMsg->dmt_version = 1;
    asyncCommitBlobReq->setPayload(FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg), commitBlobMsg);
    asyncCommitBlobReq->onResponseCb(std::bind(&DMApi::updateCatCb, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));
    commitBlobTxIssued_++;
    asyncCommitBlobReq->invoke();
       //  wait for the request to complete
    POLL_MS((commitBlobTxIssued_ == commitBlobTxFailed_ + commitBlobTxSuccess_), 500, 1000);
}


TEST_F(DMApi, getBlobMetaTest)
{
    std::string blobName("testBlob");
    fpi::SvcUuid svcUuid;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    // start transaction
    auto getBlobMeta = SvcMsgFactory::newGetBlobMetaDataMsg(volId_, blobName);
    auto asyncGetBlobMetaReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    asyncGetBlobMetaReq->setPayload(FDSP_MSG_TYPEID(fpi::GetBlobMetaDataMsg), getBlobMeta);
    asyncGetBlobMetaReq->onResponseCb(std::bind(&DMApi::getBlobMetaCb, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2, std::placeholders::_3));
    getBlobMetaIssued_++;
    asyncGetBlobMetaReq->invoke();
     //  wait for the request to complete
    POLL_MS((getBlobMetaIssued_ == getBlobMetaFailed_ + getBlobMetaSuccess_), 500, 1000);
}

TEST_F(DMApi, qryCatTest)
{
    std::string blobName("testBlob");
    fpi::SvcUuid svcUuid;
    uint64_t blobOffset = 0;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    // start transaction
    auto qryCat = SvcMsgFactory::newQueryCatalogMsg(volId_, blobName, blobOffset);
    auto asyncQryCatReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
    asyncQryCatReq->setPayload(FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), qryCat);
    asyncQryCatReq->onResponseCb(std::bind(&DMApi::getBlobMetaCb, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2, std::placeholders::_3));
    queryCatIssued_++;
    asyncQryCatReq->invoke();
     //  wait for the request to complete
    POLL_MS((queryCatIssued_ == queryCatFailed_ + queryCatSuccess_), 500, 1000);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>(), "puts count");
    DMApi::init(argc, argv, opts, "vol1");
    return RUN_ALL_TESTS();
}
