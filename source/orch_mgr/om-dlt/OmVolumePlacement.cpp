/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
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
            placeAlgo = new RRAlgorithm();
            break;
        case VolPlacementAlgorithm::AlgorithmTypes::RoundRobinDynamic:
            placeAlgo = new DynamicRRAlgorithm();
            break;
        default:
            fds_panic("Unknown volume placement algorithm type %u", type);
    }
}

void
VolumePlacement::computeDMT(const ClusterMap* cmap)
{
}

Error
VolumePlacement::beginRebalance(const ClusterMap* cmap)
{
    Error err(ERR_OK);

    return err;
}

void
VolumePlacement::commitDMT() {
}


}  // namespace fds
