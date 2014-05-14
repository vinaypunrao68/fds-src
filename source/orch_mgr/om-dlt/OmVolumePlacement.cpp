/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <new>
#include <OmClusterMap.h>
#include <OmVolumePlacement.h>

namespace fds {

VolumePlacement::VolumePlacement()
        : Module("Volume Placement Engine"),
          dmtMgr(new DMTManager()),
          placeAlgo(NULL),
          placementMutex("Volume Placement mutex")
{
}

VolumePlacement::~VolumePlacement()
{
    if (placeAlgo) {
        delete placeAlgo;
    }
}

int
VolumePlacement::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    // Should we also get DMT width, depth, algo from config?
    curDmtWidth = 3;
    curDmtDepth = 4;

    std::string algo_type_str("RoundRobin");
    VolPlacementAlgorithm::AlgorithmTypes type =
            VolPlacementAlgorithm::AlgorithmTypes::RoundRobin;
    if (algo_type_str.compare("RoundRobinDynamic") == 0) {
        type = VolPlacementAlgorithm::AlgorithmTypes::RoundRobinDynamic;
    } else if (algo_type_str.compare("RoundRobin") == 0) {
        type = VolPlacementAlgorithm::AlgorithmTypes::RoundRobin;
    } else {
        LOGWARN << "Unknown volume placement algorithm type "
                << ", will use Round Robin algorithm";
    }

    LOGNOTIFY << "VolumePlacement: DMT width " << curDmtWidth
              << ", DMT depth " << curDmtDepth
              << ", algorithm " << algo_type_str;

    setAlgorithm(type);

    return 0;
}

void
VolumePlacement::mod_startup() {
}

void
VolumePlacement::mod_shutdown() {
}

void
VolumePlacement::setAlgorithm(VolPlacementAlgorithm::AlgorithmTypes type)
{
    fds_mutex::scoped_lock l(placementMutex);
    if (placeAlgo) {
        delete placeAlgo;
        placeAlgo = NULL;
    }
    switch (type) {
        case VolPlacementAlgorithm::AlgorithmTypes::RoundRobin:
            placeAlgo = new(std::nothrow) RRAlgorithm();
            break;
        case VolPlacementAlgorithm::AlgorithmTypes::RoundRobinDynamic:
            placeAlgo = new(std::nothrow) DynamicRRAlgorithm();
            break;
        default:
            fds_panic("Unknown volume placement algorithm type %u", type);
    }
    fds_verify(placeAlgo != NULL);
}

void
VolumePlacement::computeDMT(const ClusterMap* cmap)
{
    Error err(ERR_OK);
    fds_uint64_t next_version = DMT_VER_INVALID + 1;
    fds_uint32_t depth = curDmtDepth;
    DMT *newDmt = NULL;

    // if we alreay have commited DMT, next version is inc 1
    if (dmtMgr->hasCommittedDMT()) {
        next_version = dmtMgr->getCommittedVersion() + 1;
    }
    if (cmap->getNumMembers(fpi::FDSP_DATA_MGR) < curDmtDepth) {
        depth = cmap->getNumMembers(fpi::FDSP_DATA_MGR);
    }
    fds_verify(depth > 0);

    // allocate and compute new DMT
    newDmt = new(std::nothrow) DMT(curDmtWidth, depth, next_version, true);
    fds_verify(newDmt != NULL);
    {  // compute DMT
        fds_mutex::scoped_lock l(placementMutex);
        if (dmtMgr->hasCommittedDMT()) {
            placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED), newDmt);
        } else {
            placeAlgo->computeDMT(cmap, newDmt);
        }
    }

    // add this DMT as target, we should not have another non-committed
    // target already
    fds_verify(!dmtMgr->hasTargetDMT());
    err = dmtMgr->add(newDmt, DMT_TARGET);
    fds_verify(err.ok());

    LOGDEBUG << "Computed new DMT: " << *newDmt;
}

Error
VolumePlacement::beginRebalance(const ClusterMap* cmap)
{
    Error err(ERR_OK);

    return err;
}

void
VolumePlacement::commitDMT() {
    dmtMgr->commitDMT();
    LOGDEBUG << *dmtMgr;
}


}  // namespace fds
