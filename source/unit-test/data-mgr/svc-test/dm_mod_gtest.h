/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_DATA_MGR_SVC_TEST_DM_MOD_GTEST_H_
#define SOURCE_UNIT_TEST_DATA_MGR_SVC_TEST_DM_MOD_GTEST_H_

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>
#include <boost/make_shared.hpp>
#include <platform/platform-lib.h>
#include <net/SvcRequestPool.h>
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include <apis/ConfigurationService.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT


struct DMApi : SingleNodeTest
{
    DMApi() {
        queryCatIssued_ = 0;
        queryCatFailed_ = 0;
        queryCatSuccess_ = 0;

        updateCatIssued_ = 0;
        updateCatFailed_ = 0;
        updateCatSuccess_ = 0;

        updateCatOnceIssued_ = 0;
        updateCatOnceFailed_ = 0;
        updateCatOnceSuccess_ = 0;

        startBlobTxIssued_ = 0;
        startBlobTxFailed_ = 0;
        startBlobTxSuccess_ = 0;

        commitBlobTxIssued_ = 0;
        commitBlobTxFailed_ = 0;
        commitBlobTxSuccess_ = 0;

        deleteCatIssued_ = 0;
        deleteCatFailed_ = 0;
        deleteCatSuccess_ = 0;

        abortBlobTxIssued_ = 0;
        abortBlobTxFailed_ = 0;
        abortBlobTxSuccess_ = 0;

        setBlobMetaIssued_ = 0;
        setBlobMetaFailed_ = 0;
        setBlobMetaSuccess_ = 0;

        getBlobMetaIssued_ = 0;
        getBlobMetaFailed_ = 0;
        getBlobMetaSuccess_ = 0;

        getVolumeMetaIssued_ = 0;
        getVolumeMetaFailed_ = 0;
        getVolumeMetaSuccess_ = 0;

        delBlobIssued_ = 0;
        delBlobFailed_ = 0;
        delBlobSuccess_ = 0;
    }

    void queryCatCb(EPSvcRequest* svcReq, const Error& error,
                             boost::shared_ptr<std::string> payload)
    {
        std::cout << "queryCatCb  successfull";
        if (error != ERR_OK) {
            queryCatFailed_++;
        } else {
            queryCatSuccess_++;
        }
        // std::cout << "pubcb: " << error << std::endl;
    }

    void updateCatCb(EPSvcRequest* svcReq, const Error& error,
                             boost::shared_ptr<std::string> payload)
    {
        if (error != ERR_OK) {
            updateCatFailed_++;
        } else {
            updateCatSuccess_++;
        }
        // std::cout << "pubcb: " << error << std::endl;
    }

    void updateCatOnceCb(EPSvcRequest* svcReq, const Error& error,
                             boost::shared_ptr<std::string> payload)
    {
        if (error != ERR_OK) {
            updateCatOnceFailed_++;
        } else {
            updateCatOnceSuccess_++;
        }
        // std::cout << "pubcb: " << error << std::endl;
    }

    void startBlobTxCb(EPSvcRequest* svcReq, const Error& error,
                             boost::shared_ptr<std::string> payload)
    {
        if (error != ERR_OK) {
            startBlobTxFailed_++;
        } else {
            startBlobTxSuccess_++;
        }
        // std::cout << "pubcb: " << error << std::endl;
    }

    void commitBlobTxCb(EPSvcRequest* svcReq, const Error& error,
                             boost::shared_ptr<std::string> payload)
    {
        if (error != ERR_OK) {
            commitBlobTxFailed_++;
        } else {
            commitBlobTxSuccess_++;
        }
        // std::cout << "pubcb: " << error << std::endl;
    }

    void getBlobMetaCb(EPSvcRequest* svcReq, const Error& error,
                             boost::shared_ptr<std::string> payload)
    {
        if (error != ERR_OK) {
            getBlobMetaFailed_++;
        } else {
            getBlobMetaSuccess_++;
        }
        // std::cout << "pubcb: " << error << std::endl;
    }

 protected:
      std::atomic<uint32_t> queryCatIssued_;
      std::atomic<uint32_t> queryCatFailed_;
      std::atomic<uint32_t> queryCatSuccess_;

      std::atomic<uint32_t> updateCatIssued_;
      std::atomic<uint32_t> updateCatFailed_;
      std::atomic<uint32_t> updateCatSuccess_;

      std::atomic<uint32_t> updateCatOnceIssued_;
      std::atomic<uint32_t> updateCatOnceFailed_;
      std::atomic<uint32_t> updateCatOnceSuccess_;

      std::atomic<uint32_t> startBlobTxIssued_;
      std::atomic<uint32_t> startBlobTxFailed_;
      std::atomic<uint32_t> startBlobTxSuccess_;

      std::atomic<uint32_t> commitBlobTxIssued_;
      std::atomic<uint32_t> commitBlobTxFailed_;
      std::atomic<uint32_t> commitBlobTxSuccess_;

      std::atomic<uint32_t> deleteCatIssued_;
      std::atomic<uint32_t> deleteCatFailed_;
      std::atomic<uint32_t>  deleteCatSuccess_;

      std::atomic<uint32_t> abortBlobTxIssued_;
      std::atomic<uint32_t> abortBlobTxFailed_;
      std::atomic<uint32_t> abortBlobTxSuccess_;

      std::atomic<uint32_t> setBlobMetaIssued_;
      std::atomic<uint32_t> setBlobMetaFailed_;
      std::atomic<uint32_t> setBlobMetaSuccess_;

      std::atomic<uint32_t> getBlobMetaIssued_;
      std::atomic<uint32_t> getBlobMetaFailed_;
      std::atomic<uint32_t> getBlobMetaSuccess_;

      std::atomic<uint32_t> getVolumeMetaIssued_;
      std::atomic<uint32_t> getVolumeMetaFailed_;
      std::atomic<uint32_t> getVolumeMetaSuccess_;

      std::atomic<uint32_t> delBlobIssued_;
      std::atomic<uint32_t> delBlobFailed_;
      std::atomic<uint32_t> delBlobSuccess_;
};

#endif  // SOURCE_UNIT_TEST_DATA_MGR_SVC_TEST_DM_MOD_GTEST_H_
