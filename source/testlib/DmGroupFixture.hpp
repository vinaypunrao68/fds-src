/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef TESTLIB_DMGROUPFIXTURE_HPP_
#define TESTLIB_DMGROUPFIXTURE_HPP_

#include <testlib/TestFixtures.h>
#include <testlib/TestOm.hpp>
#include <testlib/TestDm.hpp>
#include <testlib/TestAm.hpp>
#include <testlib/TestUtils.h>
#include <net/VolumeGroupHandle.h>
#include <path.h>

#define MAX_OBJECT_SIZE 1024 * 1024 * 2

using namespace fds;
using namespace fds::TestUtils;

struct DmGroupFixture : BaseTestFixture {
    using DmHandle = ProcessHandle<TestDm>;
    using OmHandle = ProcessHandle<TestOm>;
    using AmHandle = ProcessHandle<TestAm>;

    DmGroupFixture() :
        sequenceId(0),
        numOfNodes(0)
    {}

    std::vector<std::string> &getRootDirectories() {
        if (roots.size() == 0) {
            std::string fdsSrcPath;
            auto findRet = findFdsSrcPath(fdsSrcPath);
            fds_verify(findRet);

            std::string homedir = boost::filesystem::path(getenv("HOME")).string();
            std::string baseDir =  homedir + "/temp";

            fds::util::populateTempRootDirectories(roots, numOfNodes);
            setupDmClusterEnv(fdsSrcPath, baseDir);
        }
        return roots;
    }

    int getPlatformUuid(int idx)
    {
        return 2048 + (256 * idx);
    }
    int getPlatformPort(int idx)
    {
        return 9850 + (10 * idx);
    }

    void createCluster(int numDms) {
        numOfNodes = numDms;

        auto roots = getRootDirectories();
        omHandle.start({"am",
                       roots[0],
                       "--fds.pm.platform_uuid=1024",
                       "--fds.pm.platform_port=7000",
                       "--fds.om.threadpool.num_threads=2"
                       });
        g_fdsprocess = omHandle.proc;
        g_cntrs_mgr = omHandle.proc->get_cntrs_mgr();

        amHandle.start({"am",
                       roots[0],
                       util::strformat("--fds.pm.platform_uuid=%d", getPlatformUuid(0)),
                       util::strformat("--fds.pm.platform_port=%d", getPlatformPort(0)),
                       "--fds.am.threadpool.num_threads=3"
                       });

        dmGroup.resize(numOfNodes);
        for (int i = 0; i < numOfNodes; i++) {
            dmGroup[i].reset(new DmHandle);
            dmGroup[i]->start({"dm",
                              roots[i],
                              util::strformat("--fds.pm.platform_uuid=%d", getPlatformUuid(i)),
                              util::strformat("--fds.pm.platform_port=%d", getPlatformPort(i)),
                              "--fds.dm.threadpool.num_threads=3",
                              "--fds.dm.qos.default_qos_threads=3",
                              "--fds.feature_toggle.common.enable_volumegrouping=true",
                              "--fds.feature_toggle.common.enable_timeline=false"
                              });
        }

        VolumeGroupHandle::GROUPCHECK_INTERVAL_SEC = 2;
    }

    void createVolumeV1()
    {
        /* Add DMT to AM with the DM group */
        std::vector<fpi::SvcUuid> dms;
        for (const auto &dm : dmGroup) {
            dms.push_back(dm->proc->getSvcMgr()->getSelfSvcUuid());
        }

        /* Add the DMT to om and am */
        dmt = DMT::newDMT(dms);
        dmt->getSerialized(dmtData);
        omHandle.proc->addDmt(dmt);

        /* Add volume to DM group */
        v1Desc = generateVolume(v1Id);
        omHandle.proc->addVolume(v1Id, v1Desc);
        Error e;
        for (uint32_t i = 0; i < dmGroup.size(); i++) {
            e = dmGroup[i]->proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                                 nullptr,
                                                                                 DMT_COMMITTED);
            ASSERT_TRUE(e == ERR_OK);
            e = dmGroup[i]->proc->getDataMgr()->addVolume("test1", v1Id, v1Desc.get());
            ASSERT_TRUE(e == ERR_OK);
            ASSERT_TRUE(dmGroup[i]->proc->getDataMgr()->\
                        getVolumeMeta(v1Id)->getState() == fpi::Offline);
        }
    }

    void setupVolumeGroupHandleOnAm1(uint32_t quorumCnt)
    {
        /* Set up the volume on om and dms */
        if (!v1Desc) {
            createVolumeV1();
        }
        Error e = amHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                                nullptr,
                                                                                DMT_COMMITTED);
        ASSERT_TRUE(e == ERR_OK);

        /* Create a coordinator with quorum of quorumCnt */
        v1 = MAKE_SHARED<VolumeGroupHandle>(amHandle.proc, v1Id, quorumCnt);
        amHandle.proc->setVolumeHandle(v1.get());

        /* open should succeed */
        openVolume(*v1, waiter);
        ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
    }

    DMTPtr getOmDMT(DMTType type) {
        return omHandle.proc->getSvcMgr()->getDmtManager()->getDMT(type);
    }

    SHPTR<VolumeDesc> generateVolume(const fds_volid_t &volId) {
        std::string name = "test" + std::to_string(volId.get());

        SHPTR<VolumeDesc> vdesc(new VolumeDesc(name, volId));

        vdesc->tennantId = volId.get();
        vdesc->localDomainId = volId.get();
        vdesc->globDomainId = volId.get();

        vdesc->volType = fpi::FDSP_VOL_S3_TYPE;
        // vdesc->volType = fpi::FDSP_VOL_BLKDEV_TYPE;
        vdesc->capacity = 10 * 1024;  // 10GB
        vdesc->maxQuota = 90;
        vdesc->redundancyCnt = 1;
        vdesc->maxObjSizeInBytes = MAX_OBJECT_SIZE;

        vdesc->writeQuorum = 1;
        vdesc->readQuorum = 1;
        vdesc->consisProtocol = fpi::FDSP_CONS_PROTO_EVENTUAL;

        vdesc->volPolicyId = 50;
        vdesc->archivePolicyId = 1;
        vdesc->mediaPolicy = fpi::FDSP_MEDIA_POLICY_HYBRID;
        vdesc->placementPolicy = 1;
        vdesc->appWorkload = fpi::FDSP_APP_S3_OBJS;
        vdesc->backupVolume = invalid_vol_id.get();

        vdesc->iops_assured = 1000;
        vdesc->iops_throttle = 5000;
        vdesc->relativePrio = 1;

        vdesc->fSnapshot = false;
        vdesc->srcVolumeId = invalid_vol_id;
        vdesc->lookupVolumeId = invalid_vol_id;
        vdesc->qosQueueId = invalid_vol_id;
        return vdesc;
    }

    /* Issues open volume with write mode */
    void openVolume(VolumeGroupHandle &v, Waiter &w)
    {
        w.reset(1);
        auto msg = MAKE_SHARED<fpi::OpenVolumeMsg>();
        msg->mode.can_write = true;
        v.open(msg,
               [this, &w](const Error &e, const fpi::OpenVolumeRspMsgPtr& resp) {
                   if (e == ERR_OK) {
                       sequenceId = resp->sequence_id;
                   }
                   w.doneWith(e);
               });
    }

    void openVolumeReadonly(VolumeGroupHandle &v, Waiter &w)
    {
        w.reset(1);
        auto msg = MAKE_SHARED<fpi::OpenVolumeMsg>();
        msg->mode.can_write = false;
        v.open(msg,
               [this, &w](const Error &e, const fpi::OpenVolumeRspMsgPtr& resp) {
                   w.doneWith(e);
               });
    }

    void doIo(uint32_t count)
    {
        /* Do more IO.  IO should succeed */
        for (uint32_t i = 0; i < count; i++) {
            sendUpdateOnceMsg(*v1, blobName, waiter);
            ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
            sendQueryCatalogMsg(*v1, blobName, waiter);
            ASSERT_TRUE(waiter.awaitResult() == ERR_OK);
        }
    }

    void sendUpdateOnceMsg(VolumeGroupHandle &v,
                           const std::string &blobName,
                           Waiter &w)
    {
        auto updateMsg = SvcMsgFactory::newUpdateCatalogOnceMsg(v.getGroupId(), blobName);
        updateMsg->txId = txId++;
        updateMsg->sequence_id = ++sequenceId;
        w.reset(1);
        v.sendCommitMsg<fpi::UpdateCatalogOnceMsg>(
            FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg),
            updateMsg,
            [&w](const Error &e, StringPtr) {
                w.doneWith(e);
            });
    }
    void sendStartBlobTxMsg(VolumeGroupHandle &v,
                            const std::string &blobName,
                            Waiter &w)
    {
        auto startMsg = SvcMsgFactory::newStartBlobTxMsg(v.getGroupId(), blobName);
        startMsg->txId = txId++;
        w.reset(1);
        v.sendModifyMsg<fpi::StartBlobTxMsg>(
            FDSP_MSG_TYPEID(fpi::StartBlobTxMsg),
            startMsg,
            [&w](const Error &e, StringPtr) {
                w.doneWith(e);
            });
    }
    void sendQueryCatalogMsg(VolumeGroupHandle &v,
                             const std::string &blobName,
                             Waiter &w)
    {
        auto queryMsg= SvcMsgFactory::newQueryCatalogMsg(v.getGroupId(), blobName, 0);
        w.reset(1);
        v.sendReadMsg<fpi::QueryCatalogMsg>(
            FDSP_MSG_TYPEID(fpi::QueryCatalogMsg),
            queryMsg,
            [&w](const Error &e, StringPtr) {
                w.doneWith(e);
            });
    }
    void doGroupStateCheck(fds_volid_t vId)
    {
        for (uint32_t i = 0; i < dmGroup.size(); i++) {
            ASSERT_TRUE(dmGroup[i]->proc->getDataMgr()->\
                        getVolumeMeta(vId)->getState() == fpi::Active);
        }
    }

    void doVolumeStateCheck(int idx,
                            fds_volid_t volId,
                            fpi::ResourceState desiredState,
                            int32_t step = 500,
                            int32_t total = 8000)
    {
        POLL_MS(dmGroup[idx]->proc->getDataMgr()->getVolumeMeta(volId)->getState() == desiredState, step, total);
        ASSERT_TRUE(dmGroup[idx]->proc->getDataMgr()->getVolumeMeta(volId)->getState() == desiredState);
    }

    void doMigrationCheck(int idx, fds_volid_t volId)
    {
        ASSERT_TRUE(dmGroup[idx]->proc->getDataMgr()->counters->totalVolumesReceivedMigration.value() > 0);
        ASSERT_TRUE(dmGroup[idx]->proc->getDataMgr()->counters->totalMigrationsAborted.value() == 0);
    }

    
    OmHandle                                omHandle;
    AmHandle                                amHandle;
    std::vector<std::unique_ptr<DmHandle>>  dmGroup;
    int64_t                                 sequenceId;
    int64_t                                 txId {0};
    Waiter                                  waiter{0};
    std::string                             blobName{"blob1"};
    fds_volid_t                             v1Id{10};
    SHPTR<VolumeGroupHandle>                v1;
    SHPTR<VolumeDesc>                       v1Desc;
    DMTPtr                                  dmt;
    std::string                             dmtData;
    int                                     numOfNodes;
    std::vector<std::string>                roots;
};


#endif /* TESTLIB_DMGROUPFIXTURE_HPP_ */
