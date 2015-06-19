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
    placeAlgo->computeDMT(cmap, dmt);
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

        placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), newDmt);
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

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
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

