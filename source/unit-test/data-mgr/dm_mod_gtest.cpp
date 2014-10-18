/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <dm_mod_gtest.h>
static fds_uint32_t MAX_OBJECT_SIZE = 1024 * 1024 * 2;    // 2MB
static fds_uint32_t NUM_OBJTS = 1024;    // 2MB
static fds_uint64_t BLOB_SIZE = 1 * 1024 * 1024 * 1024;   // 1GB


TEST_F(DMApi, qryCatTest)
{
    std::string blobName("testBlob");
}

TEST_F(DMApi, getBlobMetaTest)
{
    std::string blobName("testBlob");
}

/**
* @brief Tests basic puts and gets
*
*/
TEST_F(DMApi, metaDataTest)
{
    // std::string blobName = this->getArg<const char* >("blob-name");
    std::string blobName("testBlob");

    fpi::SvcUuid svcUuid;
    svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    // start transaction
      auto startBlobTx = SvcMsgFactory::newStartBlobTxMsg(volId_, blobName);
      auto asyncBlobTxReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
      startBlobTx->txId = 1;
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
      auto dataGen = boost::make_shared<RandDataGenerator<>>(BLOB_SIZE,
                                         MAX_OBJECT_SIZE, MAX_OBJECT_SIZE);

      fds::UpdateBlobInfo(updateCatMsg, dataGen, BLOB_SIZE);
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
      asyncCommitBlobReq->setPayload(FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg), commitBlobMsg);
      asyncCommitBlobReq->onResponseCb(std::bind(&DMApi::updateCatCb, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));
      commitBlobTxIssued_++;
      asyncCommitBlobReq->invoke();
       //  wait for the request to complete
      POLL_MS((commitBlobTxIssued_ == commitBlobTxFailed_ + commitBlobTxSuccess_), 500, 1000);
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
