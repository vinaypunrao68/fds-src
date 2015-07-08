/**
 * Copyright 2015 Formation Data Systems, Inc.
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
#include <dlt.h>
#include <orch-mgr/om-service.h>
#include <orchMgr.h>
#include <OmClusterMap.h>
#include <OmDataPlacement.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

// The 7 lines below are to make this unit test compile,
// since everything in OM is global and depends on everything
extern OrchMgr *orchMgr;
OM_Module *omModule;

OM_Module *OM_Module::om_singleton()
{
    return omModule;
}

static std::string logname = "dlt_calculation";
static fds_uint32_t dltWidth = 4;
static fds_uint32_t dltDepth = 4;

DLT* calculateFirstDLT(fds_uint32_t numSMs,
                       ClusterMap* cmap,
                       NodeList& addNodes,
                       PlacementAlgorithm *placeAlgo,
                       fds_uint32_t numPrimarySMs) {
    fds_uint64_t version = 1;

    EXPECT_NE(placeAlgo, nullptr);
    EXPECT_NE(cmap, nullptr);

    // create DLT object
    DLT* newDlt = new DLT(dltWidth,
                          dltDepth,
                          version,
                          true);
    EXPECT_NE(newDlt, nullptr);

    // at this point, DLT should be invalid
    NodeUuidSet expectedUuids;
    NodeUuid nullUuid;
    expectedUuids.insert(nullUuid);
    Error err = newDlt->verify(expectedUuids);
    EXPECT_EQ(err, ERR_INVALID_DLT);

    // Create cluster map with 4 SMs
    NodeList rmNodes;
    for (fds_uint32_t i = 0; i < numSMs; ++i) {
        NodeUuid uuid(i+1);
        OM_NodeAgent::pointer agent(new OM_NodeAgent(uuid,
                                                     fpi::FDSP_STOR_MGR));
        addNodes.push_back(agent);
    }
    cmap->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);

    // calculate DLT
    placeAlgo->computeNewDlt(cmap, nullptr, newDlt, numPrimarySMs);
    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();
    err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
    EXPECT_TRUE(err.ok());

    GLOGNORMAL << *newDlt;

    // done with pending services in cluster map
    cmap->resetPendServices(fpi::FDSP_STOR_MGR);
    return newDlt;
}

void verifyNonFailedPrimaries(fds_uint32_t cols,
                              const DLT* dlt,
                              fds_uint32_t numPrimarySMs,
                              const NodeUuidSet& failedSms) {
    // Primary SMs must be non-failed SMs
    for (fds_uint32_t i = 0; i < cols; ++i) {
        DltTokenGroupPtr column = dlt->getNodes(i);
        for (fds_uint32_t j = 0; j < numPrimarySMs; ++j) {
            NodeUuid uuid = column->get(j);
            EXPECT_EQ(failedSms.count(uuid), 0);
        }
    }
}

TEST(DltCalculation, dlt_class) {
    fds_uint32_t cols = pow(2, dltWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numSMs = 4;

    GLOGNORMAL << "Unit testing DLT class. "
               << "Width: " << dltWidth << ", columns " << cols;

    // create 2 DLT objects
    DLT* dltA = new DLT(dltWidth, dltDepth, version, true);
    DLT* dltB = new DLT(dltWidth, dltDepth, version+1, true);

    // fill in simple DLT -- both DLTs are the same
    EXPECT_EQ(dltA->getNumTokens(), cols);
    NodeUuidSet expectedUuids;
    for (fds_uint32_t i = 0; i < cols; ++i) {
        for (fds_uint32_t j = 0; j < dltDepth; ++j) {
            NodeUuid uuid(0xaa + j);
            dltA->setNode(i, j, uuid);
            dltB->setNode(i, j, uuid);
            expectedUuids.insert(uuid);
        }
    }
    // both DLTs must be valid
    Error err = dltA->verify(expectedUuids);
    EXPECT_TRUE(err.ok());
    err = dltB->verify(expectedUuids);
    EXPECT_TRUE(err.ok());

    // DLTs must be equal
    EXPECT_TRUE(*dltA == *dltB);

    // change one cell in dltB
    NodeUuid newUuid(0xcc00);
    dltB->setNode(0, 0, newUuid);
    expectedUuids.insert(newUuid);

    // should be still valid
    err = dltB->verify(expectedUuids);
    EXPECT_TRUE(err.ok());

    // DLTs must be not equal anymore
    EXPECT_FALSE(*dltA == *dltB);
}

TEST(DltCalculation, compute_add) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dltWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numSMs = 4;
    ClusterMap* cmap = new ClusterMap();
    EXPECT_NE(cmap, nullptr);
    NodeList addNodes, rmNodes;

    GLOGNORMAL << "Will calculate DLT with " << numSMs << " SMs."
               << "Width: " << dltWidth << ", columns " << cols
               << ". Computation with no failed SMs should produce same "
               << " result as original algorithm (ignoring failed state)";

    PlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new ConsistHashAlgorithm();
    DLT* dlt = calculateFirstDLT(numSMs, cmap, addNodes, placeAlgo, 0);
    EXPECT_NE(dlt, nullptr);
    if (dlt == nullptr) {
        // cannot continue this test without dlt
        return;
    }
    ++version;

    // compute with number of primaries = 2, should produce same result
    // because we are not setting any SM state = failed
    ClusterMap* cmap2 = new ClusterMap();
    NodeList addNodes2;
    DLT* dlt2 = calculateFirstDLT(numSMs, cmap2, addNodes2, placeAlgo, 2);
    EXPECT_TRUE(*dlt == *dlt2);

    // done with pending services
    addNodes.clear();
    rmNodes.clear();

    // will not use cmap2 anymore, re-using cmap
    delete cmap2;
    cmap2 = nullptr;

    // Add 4 SMs, one SM at a time
    DLT* newDlt = nullptr;
    DLT* newDlt2 = nullptr;
    for (fds_uint32_t j = 0; j < 4; ++j) {
        newDlt = new DLT(dltWidth, dltDepth, version, true);
        newDlt2 = new DLT(dltWidth, dltDepth, version, true);
        NodeUuid newUuid(numSMs + j + 1);

        GLOGNORMAL << "Adding one SM service with UUID "
                   << newUuid.uuid_get_val();

        OM_NodeAgent::pointer agent(new OM_NodeAgent(newUuid,
                                                     fpi::FDSP_STOR_MGR));
        addNodes.push_back(agent);
        cmap->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);

        // 'dlt' is a commited DLT, update DLT based on an SM addition
        // updated DLT will be computed into 'newDlt'
        placeAlgo->computeNewDlt(cmap, dlt, newDlt, 0);
        placeAlgo->computeNewDlt(cmap, dlt2, newDlt2, 0);
        // Compute DLT's reverse node to token map
        newDlt->generateNodeTokenMap();
        newDlt2->generateNodeTokenMap();
        err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
        EXPECT_TRUE(err.ok());
        // both DLTs must be the same
        EXPECT_TRUE(*newDlt == *newDlt2);

        // commit DLT
        delete dlt;
        delete dlt2;
        dlt = newDlt;
        newDlt = nullptr;
        dlt2 = newDlt2;
        newDlt2 = nullptr;
        ++version;

        GLOGNORMAL << *dlt;

        // done with pending services in cluster map
        cmap->resetPendServices(fpi::FDSP_STOR_MGR);
        addNodes.clear();
        rmNodes.clear();
    }

    delete dlt;
    delete dlt2;
    delete cmap;
    delete placeAlgo;
}

TEST(DltCalculation, compute_2prim) {
    fds_uint32_t cols = pow(2, dltWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numPrimarySMs = 2;

    for (fds_uint32_t numSMs = 1; numSMs <= 8; ++numSMs) {
        for (fds_uint32_t numFailedSms = 0;
             numFailedSms <= numSMs;
             ++numFailedSms) {
            GLOGNORMAL << "Will calculate DLT with " << numSMs << " SMs. "
                       << "Width: " << dltWidth << ", columns " << cols
                       << ". Number of primary SMs = " << numPrimarySMs
                       << ", number of failed SMs = " << numFailedSms;

            // create new DLT
            fds_uint32_t depth = dltDepth;
            if (depth > numSMs) {
                depth = numSMs;
            }
            DLT* dlt = new DLT(dltWidth, depth, version, true);

            // Create cluster map with numSMs SM services
            ClusterMap* cmap = new ClusterMap();
            NodeList addNodes, rmNodes;
            fds_uint32_t failedCnt = 0;
            NodeUuidSet failedSms;
            for (fds_uint32_t i = 0; i < numSMs; ++i) {
                NodeUuid uuid(i+1);
                OM_NodeAgent::pointer agent(new OM_NodeAgent(uuid,
                                                             fpi::FDSP_STOR_MGR));
                if (failedCnt < numFailedSms) {
                    agent->set_node_state(fpi::FDS_Node_Down);
                    failedSms.insert(uuid);
                    ++failedCnt;
                    LOGNORMAL << "Failed SM " << uuid.uuid_get_val();
                } else {
                    LOGNORMAL << "Active SM " << uuid.uuid_get_val();
                }
                addNodes.push_back(agent);
            }
            cmap->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);

            // verify number of failed SMs in the cluster map
            EXPECT_EQ(numSMs - numFailedSms, cmap->getNumNonfailedMembers(fpi::FDSP_STOR_MGR));
            EXPECT_EQ(numSMs, cmap->getNumMembers(fpi::FDSP_STOR_MGR));

            // calculate DLT
            PlacementAlgorithm *placeAlgo = nullptr;
            placeAlgo = new ConsistHashAlgorithm();
            placeAlgo->computeNewDlt(cmap, nullptr, dlt, numPrimarySMs);
            // Compute DLT's reverse node to token map
            dlt->generateNodeTokenMap();
            Error err = dlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
            EXPECT_TRUE(err.ok());

            LOGNORMAL << *dlt;

            fds_uint32_t nonFailedSmCount = cmap->getNumNonfailedMembers(fpi::FDSP_STOR_MGR);
            if (numPrimarySMs <= nonFailedSmCount) {
                // Primary SMs must be non-failed SMs
                GLOGNORMAL << "Primary SMs must be non-failed SMs. Num of primary: "
                           << numPrimarySMs << ", Non failed SMs count = "
                           << nonFailedSmCount;
                verifyNonFailedPrimaries(cols, dlt, numPrimarySMs, failedSms);
            }

            delete dlt;
            delete cmap;
            delete placeAlgo;
        }
    }
}

TEST(DltCalculation, compute_then_fail_2prim) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dltWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numSMs = 6;
    fds_uint32_t numPrimarySMs = 2;

    GLOGNORMAL << "Will calculate DLT with " << numSMs << " SMs."
               << "Width: " << dltWidth << ", columns " << cols
               <<", number of primary SMS " << numPrimarySMs
               << ". And fail SMs one at a time";

    fds_uint32_t numFailedSms = 1;

    PlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new ConsistHashAlgorithm();
    ClusterMap* cmap = new ClusterMap();
    NodeList addNodes;

    // calculate DLT
    DLT* dlt = calculateFirstDLT(numSMs, cmap, addNodes, placeAlgo, numPrimarySMs);
    EXPECT_NE(dlt, nullptr);
    if (dlt == nullptr) {
        // cannot continue this test without dlt
        return;
    }
    ++version;

    // Pretend a SM fails, from one to all SMs
    DLT* newDlt = nullptr;
    NodeUuidSet failedSms;
    for (fds_uint32_t j = 0; j < numSMs; ++j) {
        newDlt = new DLT(dltWidth, dltDepth, version, true);
        OM_NodeAgent::pointer agent = addNodes.front();
        agent->set_node_state(fpi::FDS_Node_Down);
        failedSms.insert(agent->get_uuid());
        addNodes.push_back(agent);
        addNodes.pop_front();

        GLOGNORMAL << "Failing SM service with UUID "
                   << agent->get_uuid().uuid_get_val();

        // 'dlt' is a commited DLT, update DLT based on an SM failure
        // updated DLT will be computed into 'newDlt'
        placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
        // Compute DLT's reverse node to token map
        newDlt->generateNodeTokenMap();
        err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
        EXPECT_TRUE(err.ok());

        // 'dlt' is commited DLT and 'newDlt' is a target DLT
        // TODO(Anna) what else do we need to check here?
        fds_uint32_t nonFailedCnt = numSMs - failedSms.size();
        if (numPrimarySMs <= nonFailedCnt) {
            // it is possible that the whole column failed, even if there are enough
            // non-failed SMs, check this first
            fds_bool_t atLeastOneColumnFailed = false;
            for (fds_uint32_t i = 0; i < cols; ++i) {
                DltTokenGroupPtr column = newDlt->getNodes(i);
                fds_uint32_t failedCnt = 0;
                for (fds_uint32_t j = 0; j < newDlt->getDepth(); ++j) {
                    NodeUuid uuid = column->get(j);
                    if (failedSms.count(uuid) > 0) {
                        ++failedCnt;
                    }
                }
                if (failedCnt == newDlt->getDepth()) {
                    atLeastOneColumnFailed = true;
                    break;
                }
            }
            if (!atLeastOneColumnFailed) {
                GLOGNORMAL << "Primary SMs must be non-failed SMs. Verifying... "
                           << " num prim " << numPrimarySMs << ", non failed " << nonFailedCnt;
                verifyNonFailedPrimaries(cols, newDlt, numPrimarySMs, failedSms);
            }
        } else {
            EXPECT_TRUE(*dlt == *newDlt);
        }

        // commit DLT
        delete dlt;
        dlt = newDlt;
        newDlt = nullptr;
        ++version;

        GLOGNORMAL << *dlt;
    }

    addNodes.clear();
    delete dlt;
    delete cmap;
    delete placeAlgo;
}

TEST(DltCalculation, fail_rm_2prim) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dltWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numSMs = 8;
    fds_uint32_t numPrimarySMs = 2;

    GLOGNORMAL << "Will calculate DLT with " << numSMs << " SMs."
               << "Width: " << dltWidth << ", columns " << cols
               <<", number of primary SMS " << numPrimarySMs
               << ". And fail+rm SMs";

    PlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new ConsistHashAlgorithm();
    ClusterMap* cmap = new ClusterMap();
    NodeList firstNodes, rmNodes, addNodes;
    NodeUuidSet failedSms;

    // calculate DLT
    DLT* dlt = calculateFirstDLT(numSMs, cmap, firstNodes, placeAlgo, numPrimarySMs);
    EXPECT_NE(dlt, nullptr);
    if (dlt == nullptr) {
        // cannot continue this test without dlt
        return;
    }
    ++version;

    // remove one SM and fail one SM
    DLT* newDlt = new DLT(dltWidth, dltDepth, version, true);
    NodeList::iterator it = firstNodes.begin();
    OM_NodeAgent::pointer failedAgent = *it;
    failedAgent->set_node_state(fpi::FDS_Node_Down);
    failedSms.insert(failedAgent->get_uuid());
    GLOGNORMAL << "Failing SM service with UUID 0x" <<
            std::hex << failedAgent->get_uuid().uuid_get_val() << std::dec;
    ++it;
    EXPECT_NE(it, firstNodes.end());   // test with numSMs > 2
    rmNodes.push_back(*it);
    GLOGNORMAL << "Removing SM service with UUID 0x" <<
            std::hex << (*it)->get_uuid().uuid_get_val() << std::dec;

    cmap->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);

    // update DLT
    // 'dlt' is a commited DLT, update DLT based on an SM addition
    // updated DLT will be computed into 'newDlt'
    err = placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
    EXPECT_TRUE(err.ok());
    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();
    err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
    EXPECT_TRUE(err.ok());

    if (numSMs > (numPrimarySMs + 1)) {
        // primaries should be 'non-failed'
        verifyNonFailedPrimaries(cols, newDlt, numPrimarySMs, failedSms);
    }

    // commit DLT
    delete dlt;
    dlt = newDlt;
    newDlt = nullptr;
    ++version;
    GLOGNORMAL << *dlt;

    cmap->resetPendServices(fpi::FDSP_STOR_MGR);
    rmNodes.clear();

    // if we don't change the cluster map and recompute DLT, there must be
    // no change in the DLT
    GLOGNORMAL << "Recomputing DLT without cluster map change";
    newDlt = new DLT(dltWidth, dltDepth, version, true);
    err = placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
    EXPECT_TRUE(*newDlt == *dlt);

    // cleanup
    rmNodes.clear();
    firstNodes.clear();
    delete newDlt;
    delete dlt;
    delete cmap;
    delete placeAlgo;
}

TEST(DltCalculation, rm_add_2prim) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dltWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numSMs = 8;
    fds_uint32_t numPrimarySMs = 2;

    GLOGNORMAL << "Will calculate DLT with " << numSMs << " SMs."
               << "Width: " << dltWidth << ", columns " << cols
               << ", number of primary SMS " << numPrimarySMs
               << ". And rm then add SMs";

    PlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new ConsistHashAlgorithm();
    ClusterMap* cmap = new ClusterMap();
    NodeList firstNodes, rmNodes, addNodes;

    // calculate DLT
    DLT* dlt = calculateFirstDLT(numSMs, cmap, firstNodes, placeAlgo, numPrimarySMs);
    EXPECT_NE(dlt, nullptr);
    if (dlt == nullptr) {
        // cannot continue this test without dlt
        return;
    }
    ++version;

    // remove one SM
    DLT* newDlt = new DLT(dltWidth, dltDepth, version, true);
    NodeList::iterator it = firstNodes.begin();
    EXPECT_NE(it, firstNodes.end());   // test with numSMs > 2
    rmNodes.push_back(*it);
    GLOGNORMAL << "Removing SM service with UUID 0x" <<
            std::hex << (*it)->get_uuid().uuid_get_val() << std::dec;

    cmap->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);

    // update DLT
    // 'dlt' is a commited DLT, update DLT based on an SM addition
    // updated DLT will be computed into 'newDlt'
    placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();
    err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
    EXPECT_TRUE(err.ok());

    // commit DLT
    delete dlt;
    dlt = newDlt;
    newDlt = nullptr;
    ++version;

    GLOGNORMAL << *dlt;

    // cleanup
    rmNodes.clear();
    cmap->resetPendServices(fpi::FDSP_STOR_MGR);

    // add one SM
    newDlt = new DLT(dltWidth, dltDepth, version, true);
    NodeUuid addUuid(numSMs + 1);
    OM_NodeAgent::pointer addAgent(new OM_NodeAgent(addUuid,
                                                    fpi::FDSP_STOR_MGR));
    addNodes.push_back(addAgent);
    cmap->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);
    GLOGNORMAL << "Adding SM service with UUID 0x" <<
            std::hex << addAgent->get_uuid().uuid_get_val() << std::dec;

    // 'dlt' is a commited DLT, update DLT based on an SM addition
    // updated DLT will be computed into 'newDlt'
    placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();
    err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
    EXPECT_TRUE(err.ok());

    GLOGNORMAL << *newDlt;

    firstNodes.clear();
    delete newDlt;
    delete dlt;
    delete cmap;
    delete placeAlgo;
}

TEST(DltCalculation, fail_add_2prim) {
    Error err(ERR_OK);
    fds_uint32_t cols = pow(2, dltWidth);
    fds_uint64_t version = 1;
    fds_uint32_t numSMs = 8;
    fds_uint32_t numPrimarySMs = 2;

    GLOGNORMAL << "Will calculate DLT with " << numSMs << " SMs."
               << "Width: " << dltWidth << ", columns " << cols
               <<", number of primary SMs " << numPrimarySMs
               << ". And fail/add SMs";

    PlacementAlgorithm *placeAlgo = nullptr;
    placeAlgo = new ConsistHashAlgorithm();
    ClusterMap* cmap = new ClusterMap();
    NodeList firstNodes, rmNodes, addNodes;
    NodeUuidSet failedSms;

    // calculate DLT
    DLT* dlt = calculateFirstDLT(numSMs, cmap, firstNodes, placeAlgo, numPrimarySMs);
    EXPECT_NE(dlt, nullptr);
    if (dlt == nullptr) {
        // cannot continue this test without dlt
        return;
    }
    ++version;

    // fail one SM
    DLT* newDlt = new DLT(dltWidth, dltDepth, version, true);
    NodeList::iterator it = firstNodes.begin();
    EXPECT_NE(it, firstNodes.end());   // we must have numSMs at least 1
    OM_NodeAgent::pointer failedAgent = *it;
    failedAgent->set_node_state(fpi::FDS_Node_Down);
    failedSms.insert(failedAgent->get_uuid());
    GLOGNORMAL << "Failing SM service with UUID "
               << failedAgent->get_uuid().uuid_get_val();

    // update DLT
    // 'dlt' is a commited DLT, update DLT based on an SM addition
    // updated DLT will be computed into 'newDlt'
    placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();
    err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
    EXPECT_TRUE(err.ok());

    if (numSMs > numPrimarySMs) {
        // primaries should be 'non-failed'
        verifyNonFailedPrimaries(cols, newDlt, numPrimarySMs, failedSms);
    }

    // commit DLT
    delete dlt;
    dlt = newDlt;
    newDlt = nullptr;
    ++version;

    GLOGNORMAL << *dlt;

    // unfail SM that previously failed
    newDlt = new DLT(dltWidth, dltDepth, version, true);
    failedAgent->set_node_state(fpi::FDS_Node_Up);
    failedSms.clear();
    GLOGNORMAL << "Un-failing SM service with UUID "
               << failedAgent->get_uuid().uuid_get_val();

    // update DLT
    // 'dlt' is a commited DLT, update DLT based on an SM addition
    // updated DLT will be computed into 'newDlt'
    placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();
    err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
    EXPECT_TRUE(err.ok());

    // commit DLT
    delete dlt;
    dlt = newDlt;
    newDlt = nullptr;
    ++version;

    GLOGNORMAL << *dlt;

    // fail a different SM
    newDlt = new DLT(dltWidth, dltDepth, version, true);
    it = firstNodes.begin();
    ++it;
    EXPECT_NE(it, firstNodes.end());   // test with numSMs > 2
    (*it)->set_node_state(fpi::FDS_Node_Down);
    failedSms.insert((*it)->get_uuid());
    GLOGNORMAL << "Failing SM service with UUID "
               << (*it)->get_uuid().uuid_get_val();

    // update DLT
    // 'dlt' is a commited DLT, update DLT based on an SM addition
    // updated DLT will be computed into 'newDlt'
    placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();
    err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
    EXPECT_TRUE(err.ok());

    if (numSMs > numPrimarySMs) {
        // primaries should be 'non-failed'
        verifyNonFailedPrimaries(cols, newDlt, numPrimarySMs, failedSms);
    }

    // commit DLT
    delete dlt;
    dlt = newDlt;
    newDlt = nullptr;
    ++version;

    GLOGNORMAL << *dlt;

    // add one more SM
    newDlt = new DLT(dltWidth, dltDepth, version, true);
    NodeUuid addUuid(numSMs + 2);
    OM_NodeAgent::pointer addAgent(new OM_NodeAgent(addUuid,
                                                    fpi::FDSP_STOR_MGR));
    addNodes.push_back(addAgent);
    cmap->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);
    GLOGNORMAL << "Adding SM service with UUID "
               << addAgent->get_uuid().uuid_get_val();

    // update DLT
    // 'dlt' is a commited DLT, update DLT based on an SM addition
    // updated DLT will be computed into 'newDlt'
    placeAlgo->computeNewDlt(cmap, dlt, newDlt, numPrimarySMs);
    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();
    err = newDlt->verify(cmap->getServiceUuids(fpi::FDSP_STOR_MGR));
    EXPECT_TRUE(err.ok());

    if (numSMs > numPrimarySMs) {
        // primaries should be 'non-failed'
        verifyNonFailedPrimaries(cols, newDlt, numPrimarySMs, failedSms);
    }

    // commit DLT
    delete dlt;
    dlt = newDlt;
    newDlt = nullptr;
    ++version;

    GLOGNORMAL << *dlt;

    firstNodes.clear();
    delete dlt;
    delete cmap;
    delete placeAlgo;
}

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);

    namespace po = boost::program_options;
    po::options_description progDesc("DLT Calculation Unit Test options");
    progDesc.add_options()
            ("help,h", "Help Message")
            ("dlt-width",
             po::value<fds_uint32_t>(&fds::dltWidth)->default_value(4),
             "DLT width")
            ("dlt-depth",
             po::value<fds_uint32_t>(&fds::dltDepth)->default_value(4),
             "DLT depth");
    po::variables_map varMap;
    po::parsed_options parsedOpt =
            po::command_line_parser(argc, argv).options(progDesc).allow_unregistered().run();
    po::store(parsedOpt, varMap);
    po::notify(varMap);

    std::cout << "DLT config: Width " << fds::dltWidth
              << ", Depth " << fds::dltDepth << std::endl;

    return RUN_ALL_TESTS();
}

