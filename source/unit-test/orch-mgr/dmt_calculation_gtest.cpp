/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <set>
#include <string>

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <fds_dmt.h>
#include <orch-mgr/om-service.h>
#include <orchMgr.h>
#include <OmClusterMap.h>
#include <OmVolumePlacement.h>

using ::testing::AtLeast;
using ::testing::Return;

bool ignoreFailedSvcs = false;

namespace fds {

// The 7 lines below are to make this unit test compile,
// since everything in OM is global and depends on everything
extern OrchMgr *orchMgr;
OM_Module *omModule;

OM_Module *OM_Module::om_singleton()
{
    return omModule;
}

static std::string logname = "dmt_calculation";
static fds_uint32_t dmtWidth = 4;
static fds_uint32_t dmtDepth = 4;

void expectEqualDmts(const DMTPtr& dmtA, const DMTPtr& dmtB) {
    fds_bool_t bSame = true;
    fds_uint32_t num_cols = dmtA->getNumColumns();
    EXPECT_EQ(num_cols, dmtB->getNumColumns());

    // depth must be equal
    EXPECT_EQ(dmtA->getDepth(), dmtB->getDepth());

    for (fds_uint32_t i = 0; i < num_cols; ++i) {
        DmtColumnPtr colA = dmtA->getNodeGroup(i);
        DmtColumnPtr colB = dmtB->getNodeGroup(i);
        for (fds_uint32_t j = 0; j < dmtA->getDepth(); ++j) {
            EXPECT_EQ(colA->get(j), colB->get(j));
        }
    }
}

void calculateFirstDMT(fds_uint32_t numDMs,
                       ClusterMap* cmap,
                       DMTManagerPtr& dmtMgr,
                       NodeList& addNodes,
                       VolPlacementAlgorithm *placeAlgo,
                       fds_uint32_t numPrimaryDMs) {
    fds_uint64_t version = 1;

    EXPECT_NE(placeAlgo, nullptr);
    EXPECT_NE(cmap, nullptr);

    DMT* dmt = new DMT(dmtWidth, dmtDepth, version);

    // at this point, DMT should be invalid
    Error err = dmt->verify();
    EXPECT_EQ(err, ERR_INVALID_DMT);

    // Create cluster map with 4 nodes
    NodeList rmNodes;
    for (fds_uint32_t i = 0; i < numDMs; ++i) {
        NodeUuid uuid(0xAB00+i);
        OM_NodeAgent::pointer agent(new(std::nothrow) OM_NodeAgent(uuid,
                                                                   fpi::FDSP_DATA_MGR));
        addNodes.push_back(agent);
    }
    cmap->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);

    // calculate DMT
    placeAlgo->computeDMT(cmap, dmt, numPrimaryDMs);
    err = dmt->verify();
    EXPECT_TRUE(err.ok());

    // make this version commited
    EXPECT_FALSE(dmtMgr->hasTargetDMT());
    err = dmtMgr->add(dmt, DMT_TARGET);
    EXPECT_TRUE(err.ok());
    err = dmtMgr->commitDMT(true);
    EXPECT_TRUE(err.ok());

    GLOGNORMAL << *dmtMgr;

    // done with pending services in cluster map
    cmap->resetPendServices(fpi::FDSP_DATA_MGR);
}

TEST(DmtCalculation, dmt_class) {
    fds_uint32_t cols = pow(2, dmtWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numDMs = 4;

    GLOGNORMAL << "Unit testing DMT class. "
               << "Width: " << dmtWidth << ", columns " << cols;

    DMT* dmtA = new DMT(dmtWidth, dmtDepth, version);
    DMT* dmtB = new DMT(dmtWidth, dmtDepth, version+1);

    // at this point, DMT should be invalid
    Error err = dmtA->verify();
    EXPECT_EQ(err, ERR_INVALID_DMT);

    // fill in simple DMT -- both DMTs are the same
    EXPECT_EQ(dmtA->getNumColumns(), cols);
    for (fds_uint32_t i = 0; i < cols; ++i) {
        for (fds_uint32_t j = 0; j < dmtDepth; ++j) {
            NodeUuid uuid(0xaa + j);
            dmtA->setNode(i, j, uuid);
            dmtB->setNode(i, j, uuid);
        }
    }
    // both DMTs must be valid
    err = dmtA->verify();
    EXPECT_TRUE(err.ok());
    err = dmtB->verify();
    EXPECT_TRUE(err.ok());

    // DMTs must be equal
    EXPECT_TRUE(*dmtA == *dmtB);

    // change one cell in dmtB
    NodeUuid newUuid(0xcc00);
    dmtB->setNode(0, 0, newUuid);

    // should be still valid
    err = dmtB->verify();
    EXPECT_TRUE(err.ok());

    // DMTs must be not equal anymore
    EXPECT_FALSE(*dmtA == *dmtB);

    // the first column must have intersection of (depth - 1) uuids
    DmtColumnPtr firstColA = dmtA->getNodeGroup(0);
    DmtColumnPtr firstColB = dmtB->getNodeGroup(0);
    NodeUuidSet sameUuids = firstColB->getIntersection(*firstColA);
    if (dmtDepth > 0) {
        EXPECT_EQ(sameUuids.size(), dmtDepth - 1);
    }

    // new DM in firstColB must be newUuid only
    NodeUuidSet newDms = firstColB->getNewAndNewPrimaryUuids(*firstColA, 0);
    EXPECT_EQ(newDms.size(), 1);
    EXPECT_EQ(newDms.count(newUuid), 1);

    if (dmtDepth > 2) {
        // exchange row 1 and 2 in firstColB
        NodeUuid uuid1 = firstColB->get(1);
        NodeUuid uuid2 = firstColB->get(2);
        firstColB->set(2, uuid1);
        firstColB->set(1, uuid2);
        newDms = firstColB->getNewAndNewPrimaryUuids(*firstColA, 2);
        EXPECT_EQ(newDms.size(), 2);
        // uuid2 moved from secondary to primary
        EXPECT_EQ(newDms.count(uuid2), 1);
        // newUuid must still be a new added uuid
        EXPECT_EQ(newDms.count(newUuid), 1);
    }
}

TEST(DmtCalculation, compute_add) {
    fds_uint32_t cols = pow(2, dmtWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numDMs = 4;

    GLOGNORMAL << "Will calculate DMT with " << numDMs << " DMs."
               << "Width: " << dmtWidth << ", columns " << cols;

    DMTManagerPtr dmtMgr(new fds::DMTManager(2));
    DMT* dmt = new DMT(dmtWidth, dmtDepth, version);

    // at this point, DMT should be invalid
    Error err = dmt->verify();
    EXPECT_EQ(err, ERR_INVALID_DMT);

    // Create cluster map with 4 nodes
    ClusterMap* cmap = new ClusterMap();
    NodeList addNodes, rmNodes;
    for (fds_uint32_t i = 0; i < numDMs; ++i) {
        NodeUuid uuid(0xAB00+i);
        OM_NodeAgent::pointer agent(new(std::nothrow) OM_NodeAgent(uuid,
                                                                   fpi::FDSP_DATA_MGR));
        addNodes.push_back(agent);
    }
    cmap->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);

    // calculate DMT
    VolPlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new(std::nothrow) DynamicRRAlgorithm();
    placeAlgo->computeDMT(cmap, dmt, 0);
    err = dmt->verify();
    EXPECT_TRUE(err.ok());

    // make this version commited
    EXPECT_FALSE(dmtMgr->hasTargetDMT());
    err = dmtMgr->add(dmt, DMT_TARGET);
    EXPECT_TRUE(err.ok());
    err = dmtMgr->commitDMT(true);
    EXPECT_TRUE(err.ok());
    ++version;

    GLOGNORMAL << *dmtMgr;

    // done with pending services in cluster map
    cmap->resetPendServices(fpi::FDSP_DATA_MGR);
    addNodes.clear();
    rmNodes.clear();

    // Add 4 DMs, one DM at a time
    for (fds_uint32_t j = 0; j < 4; ++j) {
        DMT* newDmt = new DMT(dmtWidth, dmtDepth, version);
        NodeUuid newUuid(0xCC01 + j);

        GLOGNORMAL << "Adding one DM service with UUID 0x" <<
                std::hex << newUuid.uuid_get_val() << std::dec;

        OM_NodeAgent::pointer agent(new(std::nothrow) OM_NodeAgent(newUuid,
                                                               fpi::FDSP_DATA_MGR));
        addNodes.push_back(agent);
        cmap->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);

        placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), newDmt, 0);
        err = newDmt->verify();
        EXPECT_TRUE(err.ok());

        // commit DMT
        EXPECT_FALSE(dmtMgr->hasTargetDMT());
        err = dmtMgr->add(newDmt, DMT_TARGET);
        EXPECT_TRUE(err.ok());
        err = dmtMgr->commitDMT(true);
        EXPECT_TRUE(err.ok());
        ++version;

        GLOGNORMAL << *dmtMgr;

        // done with pending services in cluster map
        cmap->resetPendServices(fpi::FDSP_DATA_MGR);
        addNodes.clear();
        rmNodes.clear();
    }

    delete cmap;
    delete placeAlgo;
}

TEST(DmtCalculation, compute_2prim) {
    fds_uint32_t cols = pow(2, dmtWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numPrimaryDMs = 2;

    for (fds_uint32_t numDMs = 1; numDMs <= 8; ++numDMs) {
        for (fds_uint32_t numFailedDms = 0;
             numFailedDms <= numDMs;
             ++numFailedDms) {
            GLOGNORMAL << "Will calculate DMT with " << numDMs << " DMs. "
                       << "Width: " << dmtWidth << ", columns " << cols
                       << ". Number of primary DMs = " << numPrimaryDMs
                       << ", number of failed DMs = " << numFailedDms;

            // create new DMT
            fds_uint32_t depth = dmtDepth;
            if (depth > numDMs) {
                depth = numDMs;
            }
            DMT* dmt = new DMT(dmtWidth, depth, version);

            // at this point, DMT should be invalid
            Error err = dmt->verify();
            EXPECT_EQ(err, ERR_INVALID_DMT);

            // Create cluster map with numDMs nodes
            ClusterMap* cmap = new ClusterMap();
            NodeList addNodes, rmNodes;
            fds_uint32_t failedCnt = 0;
            NodeUuidSet failedDms;
            for (fds_uint32_t i = 0; i < numDMs; ++i) {
                NodeUuid uuid(0xAB00+i);
                OM_NodeAgent::pointer agent(new(std::nothrow) OM_NodeAgent(uuid,
                                                                           fpi::FDSP_DATA_MGR));
                if (failedCnt < numFailedDms) {
                    agent->set_node_state(fpi::FDS_Node_Down);
                    failedDms.insert(uuid);
                    ++failedCnt;
                    LOGNORMAL << "Failed DM " << std::hex << uuid.uuid_get_val() << std::dec;
                } else {
                    LOGNORMAL << "Active DM " << std::hex << uuid.uuid_get_val() << std::dec;
                }
                addNodes.push_back(agent);
            }
            cmap->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);

            // verify number of failed DMs in the cluster map
            EXPECT_EQ(numDMs - numFailedDms, cmap->getNumNonfailedMembers(fpi::FDSP_DATA_MGR));
            EXPECT_EQ(numDMs, cmap->getNumMembers(fpi::FDSP_DATA_MGR));

            // calculate DMT
            VolPlacementAlgorithm *placeAlgo = nullptr;
            placeAlgo = new(std::nothrow) DynamicRRAlgorithm();
            placeAlgo->computeDMT(cmap, dmt, numPrimaryDMs);
            err = dmt->verify();
            EXPECT_TRUE(err.ok());

            LOGNORMAL << *dmt;

            if (numPrimaryDMs <= cmap->getNumNonfailedMembers(fpi::FDSP_DATA_MGR)) {
                // Primary DMs must be non-failed DMs
                for (fds_uint32_t i = 0; i < cols; ++i) {
                    DmtColumnPtr column = dmt->getNodeGroup(i);
                    for (fds_uint32_t j = 0; j < numPrimaryDMs; ++j) {
                    NodeUuid uuid = column->get(j);
                    if (failedDms.count(uuid) != 0 && !ignoreFailedSvcs) {
                        EXPECT_EQ(failedDms.count(uuid), 0);
                    } else {
                        LOGNORMAL << "Feature ignoreFailedSvcs is true, failed DM:" << uuid << " can be primary";
                    }
                    }
                }
            }

            delete dmt;
            delete cmap;
            delete placeAlgo;
        }
    }
}

TEST(DmtCalculation, compute_then_fail_2prim) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dmtWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numDMs = 6;
    fds_uint32_t numPrimaryDMs = 2;

    GLOGNORMAL << "Will calculate DMT with " << numDMs << " DMs."
               << "Width: " << dmtWidth << ", columns " << cols
               <<", number of primary DMS " << numPrimaryDMs
               << ". And fail DMs one at a time";

    fds_uint32_t numFailedDms = 1;
    DMTManagerPtr dmtMgr(new fds::DMTManager(2));
    VolPlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new(std::nothrow) DynamicRRAlgorithm();
    EXPECT_NE(placeAlgo, nullptr);
    ClusterMap* cmap = new(std::nothrow) ClusterMap();
    EXPECT_NE(cmap, nullptr);
    NodeList addNodes;

    // calculate DMT
    calculateFirstDMT(numDMs, cmap, dmtMgr, addNodes, placeAlgo, numPrimaryDMs);
    ++version;

    // Pretend a DM fails, from one to all DMs
    for (fds_uint32_t j = 0; j < numDMs; ++j) {
        DMT* newDmt = new DMT(dmtWidth, dmtDepth, version);
        OM_NodeAgent::pointer agent = addNodes.front();
        agent->set_node_state(fpi::FDS_Node_Down);
        addNodes.push_back(agent);
        addNodes.pop_front();

        GLOGNORMAL << "Failing DM service with UUID 0x" <<
                std::hex << agent->get_uuid().uuid_get_val() << std::dec;

        placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), newDmt, numPrimaryDMs);
        err = newDmt->verify();
        if (!err.ok()) {
            GLOGERROR << "DMT verify failed " << err;
        }
        EXPECT_TRUE(err.ok());

        // commit DMT
        EXPECT_FALSE(dmtMgr->hasTargetDMT());
        err = dmtMgr->add(newDmt, DMT_TARGET);
        EXPECT_TRUE(err.ok());

        if (j >= (numDMs - 2)) {
            expectEqualDmts(dmtMgr->getDMT(DMT_COMMITTED),
                            dmtMgr->getDMT(DMT_TARGET));
        }

        err = dmtMgr->commitDMT(true);
        EXPECT_TRUE(err.ok());
        ++version;

        GLOGNORMAL << *dmtMgr;
    }

    addNodes.clear();
    delete cmap;
    delete placeAlgo;
}

TEST(DmtCalculation, fail_rm_2prim) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dmtWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numDMs = 8;
    fds_uint32_t numPrimaryDMs = 2;

    GLOGNORMAL << "Will calculate DMT with " << numDMs << " DMs."
               << "Width: " << dmtWidth << ", columns " << cols
               <<", number of primary DMS " << numPrimaryDMs
               << ". And fail+rm DMs";

    DMTManagerPtr dmtMgr(new fds::DMTManager(2));
    VolPlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new(std::nothrow) DynamicRRAlgorithm();
    ClusterMap* cmap = new(std::nothrow) ClusterMap();
    EXPECT_NE(cmap, nullptr);
    NodeList firstNodes, rmNodes, addNodes;

    // calculate DMT
    calculateFirstDMT(numDMs, cmap, dmtMgr, firstNodes, placeAlgo, numPrimaryDMs);
    ++version;

    // remove one DM and fail one DM
    DMT* newDmt = new DMT(dmtWidth, dmtDepth, version);
    NodeList::iterator it = firstNodes.begin();
    OM_NodeAgent::pointer failedAgent = *it;
    failedAgent->set_node_state(fpi::FDS_Node_Down);
    GLOGNORMAL << "Failing DM service with UUID 0x" <<
            std::hex << failedAgent->get_uuid().uuid_get_val() << std::dec;
    ++it;
    EXPECT_NE(it, firstNodes.end());   // test with numDMs > 2
    rmNodes.push_back(*it);
    GLOGNORMAL << "Removing DM service with UUID 0x" <<
            std::hex << (*it)->get_uuid().uuid_get_val() << std::dec;

    cmap->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);

    // update DMT
    err = placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), newDmt, numPrimaryDMs);
    if (err == ERR_INVALID_ARG) {
        // this could happen if removed and failed DM is in the same column (at least
        // one column)
        DMTPtr commitedDmt = dmtMgr->getDMT(DMT_COMMITTED);
        fds_uint32_t num_cols = commitedDmt->getNumColumns();
        fds_bool_t found = false;
        for (fds_uint32_t i = 0; i < num_cols; ++i) {
            DmtColumnPtr col = commitedDmt->getNodeGroup(i);
            fds_uint32_t count = 0;
            for (fds_uint32_t j = 0; j < numPrimaryDMs; ++j) {
                NodeUuid uuid = col->get(j);
                if ((uuid == failedAgent->get_uuid()) ||
                    (uuid == (*it)->get_uuid())) {
                    ++count;
                    GLOGNORMAL << "Found column " << i << " prim uuid 0x"
                               << std::hex << uuid.uuid_get_val() << std::dec
                               << " count " << count;
                }
            }
            if (count == numPrimaryDMs) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    } else {
        EXPECT_TRUE(err.ok());
        err = newDmt->verify();
        EXPECT_TRUE(err.ok());

        GLOGNORMAL << *newDmt;
    }

    // cleanup
    rmNodes.clear();
    firstNodes.clear();
    delete newDmt;
    delete cmap;
    delete placeAlgo;
}

TEST(DmtCalculation, rm_add_2prim) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dmtWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numDMs = 8;
    fds_uint32_t numPrimaryDMs = 2;

    GLOGNORMAL << "Will calculate DMT with " << numDMs << " DMs."
               << "Width: " << dmtWidth << ", columns " << cols
               << ", number of primary DMS " << numPrimaryDMs
               << ". And rm then add DMs";

    DMTManagerPtr dmtMgr(new fds::DMTManager(2));
    VolPlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new(std::nothrow) DynamicRRAlgorithm();
    ClusterMap* cmap = new(std::nothrow) ClusterMap();
    EXPECT_NE(cmap, nullptr);
    NodeList firstNodes, rmNodes, addNodes;

    // calculate DMT
    calculateFirstDMT(numDMs, cmap, dmtMgr, firstNodes, placeAlgo, numPrimaryDMs);
    ++version;

    // remove one DM
    DMT* newDmt = new DMT(dmtWidth, dmtDepth, version);
    NodeList::iterator it = firstNodes.begin();
    EXPECT_NE(it, firstNodes.end());   // test with numDMs > 2
    rmNodes.push_back(*it);
    GLOGNORMAL << "Removing DM service with UUID 0x" <<
            std::hex << (*it)->get_uuid().uuid_get_val() << std::dec;

    cmap->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);

    // update DMT
    err = placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), newDmt, numPrimaryDMs);
    EXPECT_TRUE(err.ok());
    err = newDmt->verify();
    EXPECT_TRUE(err.ok());

    // commit DMT
    EXPECT_FALSE(dmtMgr->hasTargetDMT());
    err = dmtMgr->add(newDmt, DMT_TARGET);
    EXPECT_TRUE(err.ok());
    err = dmtMgr->commitDMT(true);
    EXPECT_TRUE(err.ok());
    ++version;

    GLOGNORMAL << *dmtMgr;

    // cleanup
    rmNodes.clear();
    cmap->resetPendServices(fpi::FDSP_DATA_MGR);

    // add one DM
    DMT* thirdDmt = new DMT(dmtWidth, dmtDepth, version);
    NodeUuid addUuid(0xCC0A);
    OM_NodeAgent::pointer addAgent(new(std::nothrow) OM_NodeAgent(addUuid,
                                                                  fpi::FDSP_DATA_MGR));
    addNodes.push_back(addAgent);
    cmap->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);
    GLOGNORMAL << "Adding DM service with UUID 0x" <<
            std::hex << addAgent->get_uuid().uuid_get_val() << std::dec;

    err = placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), thirdDmt, numPrimaryDMs);
    EXPECT_TRUE(err.ok());
    err = thirdDmt->verify();
    EXPECT_TRUE(err.ok());

    // commit DMT
    EXPECT_FALSE(dmtMgr->hasTargetDMT());
    err = dmtMgr->add(thirdDmt, DMT_TARGET);
    EXPECT_TRUE(err.ok());
    err = dmtMgr->commitDMT(true);
    EXPECT_TRUE(err.ok());
    ++version;

    GLOGNORMAL << *dmtMgr;

    firstNodes.clear();
    delete cmap;
    delete placeAlgo;
}

TEST(DmtCalculation, fail_add_2prim) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dmtWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numDMs = 8;
    fds_uint32_t numPrimaryDMs = 2;

    GLOGNORMAL << "Will calculate DMT with " << numDMs << " DMs."
               << "Width: " << dmtWidth << ", columns " << cols
               <<", number of primary DMS " << numPrimaryDMs
               << ". And fail/add DMs";

    DMTManagerPtr dmtMgr(new fds::DMTManager(2));
    VolPlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new(std::nothrow) DynamicRRAlgorithm();
    ClusterMap* cmap = new(std::nothrow) ClusterMap();
    EXPECT_NE(cmap, nullptr);
    NodeList firstNodes, rmNodes, addNodes;

    // calculate DMT
    calculateFirstDMT(numDMs, cmap, dmtMgr, firstNodes, placeAlgo, numPrimaryDMs);
    ++version;

    // fail one DM
    DMT* newDmt = new DMT(dmtWidth, dmtDepth, version);
    NodeList::iterator it = firstNodes.begin();
    EXPECT_NE(it, firstNodes.end());   // we must have numDMs at least 1
    OM_NodeAgent::pointer failedAgent = *it;
    failedAgent->set_node_state(fpi::FDS_Node_Down);
    GLOGNORMAL << "Failing DM service with UUID 0x" <<
            std::hex << failedAgent->get_uuid().uuid_get_val() << std::dec;

    // update DMT
    err = placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), newDmt, numPrimaryDMs);
    EXPECT_TRUE(err.ok());
    err = newDmt->verify();
    EXPECT_TRUE(err.ok());

    // commit DMT
    EXPECT_FALSE(dmtMgr->hasTargetDMT());
    err = dmtMgr->add(newDmt, DMT_TARGET);
    EXPECT_TRUE(err.ok());
    err = dmtMgr->commitDMT(true);
    EXPECT_TRUE(err.ok());
    ++version;

    GLOGNORMAL << *dmtMgr;

    // unfail DM that previously failed
    DMT* thirdDmt = new DMT(dmtWidth, dmtDepth, version);
    failedAgent->set_node_state(fpi::FDS_Node_Up);
    GLOGNORMAL << "Un-failing DM service with UUID 0x" <<
            std::hex << failedAgent->get_uuid().uuid_get_val() << std::dec;

    placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), thirdDmt, numPrimaryDMs);
    err = thirdDmt->verify();
    EXPECT_TRUE(err.ok());

    // commit DMT
    EXPECT_FALSE(dmtMgr->hasTargetDMT());
    err = dmtMgr->add(thirdDmt, DMT_TARGET);
    EXPECT_TRUE(err.ok());
    err = dmtMgr->commitDMT(true);
    EXPECT_TRUE(err.ok());
    ++version;

    GLOGNORMAL << *dmtMgr;

    // fail a different DM
    DMT* fourthDmt = new DMT(dmtWidth, dmtDepth, version);
    it = firstNodes.begin();
    ++it;
    EXPECT_NE(it, firstNodes.end());   // test with numDMs > 2
    (*it)->set_node_state(fpi::FDS_Node_Down);
    GLOGNORMAL << "Failing DM service with UUID 0x" <<
            std::hex << (*it)->get_uuid().uuid_get_val() << std::dec;

    err = placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), fourthDmt, numPrimaryDMs);
    EXPECT_TRUE(err.ok());
    err = fourthDmt->verify();
    EXPECT_TRUE(err.ok());

    // commit DMT
    EXPECT_FALSE(dmtMgr->hasTargetDMT());
    err = dmtMgr->add(fourthDmt, DMT_TARGET);
    EXPECT_TRUE(err.ok());
    err = dmtMgr->commitDMT(true);
    EXPECT_TRUE(err.ok());
    ++version;

    GLOGNORMAL << *dmtMgr;

    // add one more DM
    DMT* fifthDmt = new DMT(dmtWidth, dmtDepth, version);
    NodeUuid addUuid(0xCC0A);
    OM_NodeAgent::pointer addAgent(new(std::nothrow) OM_NodeAgent(addUuid,
                                                                  fpi::FDSP_DATA_MGR));
    addNodes.push_back(addAgent);
    cmap->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);
    GLOGNORMAL << "Adding DM service with UUID 0x" <<
            std::hex << addAgent->get_uuid().uuid_get_val() << std::dec;

    err = placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), fifthDmt, numPrimaryDMs);
    EXPECT_TRUE(err.ok());
    err = fifthDmt->verify();
    EXPECT_TRUE(err.ok());

    // commit DMT
    EXPECT_FALSE(dmtMgr->hasTargetDMT());
    err = dmtMgr->add(fifthDmt, DMT_TARGET);
    EXPECT_TRUE(err.ok());
    err = dmtMgr->commitDMT(true);
    EXPECT_TRUE(err.ok());
    ++version;

    GLOGNORMAL << *dmtMgr;

    firstNodes.clear();
    delete cmap;
    delete placeAlgo;
}

class DmtCalcTest : public FdsProcess {
  public:
    DmtCalcTest(int argc, char *argv[],
                    Module **mod_vec) :
            FdsProcess(argc,
                       argv,
                       "platform.conf",
                       "fds.om.",
                       "dmtcalc-test.log",
                       mod_vec) {
        ignoreFailedSvcs = MODULEPROVIDER()->get_fds_config()->get<bool>("fds.feature_toggle.om.ignore_failed_svcs", true);
    }
    ~DmtCalcTest() {
    }
    int run() override {
        return 0;
    }
};

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);

    fds::DmtCalcTest dltTest(argc, argv, NULL);

    ::testing::InitGoogleMock(&argc, argv);

    namespace po = boost::program_options;
    po::options_description progDesc("DMT Calculation Unit Test options");
    progDesc.add_options()
            ("help,h", "Help Message")
            ("dmt-width",
             po::value<fds_uint32_t>(&fds::dmtWidth)->default_value(4),
             "DMT width")
            ("dmt-depth",
             po::value<fds_uint32_t>(&fds::dmtDepth)->default_value(4),
             "DMT depth");
    po::variables_map varMap;
    po::parsed_options parsedOpt =
            po::command_line_parser(argc, argv).options(progDesc).allow_unregistered().run();
    po::store(parsedOpt, varMap);
    po::notify(varMap);

    std::cout << "DMT config: Width " << fds::dmtWidth
              << ", Depth " << fds::dmtDepth << std::endl;

    return RUN_ALL_TESTS();
}

