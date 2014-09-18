/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <new>
#include <map>
#include <set>
#include <OmClusterMap.h>
#include <OmVolumePlacement.h>

namespace fds {

VolumePlacement::VolumePlacement()
        : Module("Volume Placement Engine"),
          dmtMgr(new DMTManager()),
          placeAlgo(NULL),
          placementMutex("Volume Placement mutex")
{
    bRebalancing = ATOMIC_VAR_INIT(false);
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
    curDmtWidth = 4;
    curDmtDepth = 4;

    std::string algo_type_str("RoundRobinDynamic");
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

void VolumePlacement::setConfigDB(kvstore::ConfigDB* configDB)
{
    this->configDB = configDB;
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

    // TODO(Rao): Check with Anna if a lock is required
    // store the dmt to config db
    fds_verify(configDB != NULL);
    if (!configDB->storeDmt(*newDmt, "target")) {
        GLOGWARN << "unable to store dmt to config db "
                << "[" << newDmt->getVersion() << "]";
    }

    LOGNORMAL << "Version: " << newDmt->getVersion();
    LOGDEBUG << "Computed new DMT: " << *newDmt;
}

/**
 * Finds which nodes to send PushMeta message and which volumes
 * to push. Returns a set of uuids of DMs to which PushMeta was sent
 */
Error
VolumePlacement::beginRebalance(const ClusterMap* cmap,
                                NodeUuidSet* dm_set)
{
    Error err(ERR_OK);
    fds_bool_t expect_rebal = false;
    fds_uint64_t dmt_columns = pow(2, curDmtWidth);
    fds_verify(dm_set != NULL);

    // node to send the push meta msg -> push_meta message
    typedef std::map<fds_uint64_t, fpi::FDSP_PushMetaPtr> pm_msgs_t;
    pm_msgs_t push_meta_msgs;
    RsArray vol_ary;

    // If we do not have any committed DMT, means that this is
    // the first DM(s) added to the domain, so no need to rebalance
    if (!dmtMgr->hasCommittedDMT()) {
        LOGNOTIFY << "Not going to rebalance volume meta, because "
                  << " this is the first DMT we computed";
        return err;
    }
    fds_verify(dmtMgr->hasTargetDMT());

    // find the list of DMT columns whose volumes will be rebalanced
    // = columns that have at least one different DM uuid
    rebalColumns.clear();
    for (fds_uint32_t cix = 0; cix < dmt_columns; ++cix) {
        DmtColumnPtr cmt_col = dmtMgr->getDMT(DMT_COMMITTED)->getNodeGroup(cix);
        DmtColumnPtr target_col = dmtMgr->getDMT(DMT_TARGET)->getNodeGroup(cix);
        for (fds_uint32_t j = 0; j < target_col->getLength(); ++j) {
            NodeUuid uuid = target_col->get(j);
            if (cmt_col->find(uuid) < 0) {
                // we have a new DM uuid in this column in target DMT
                rebalColumns.insert(cix);
                break;  // at least one, so no need to search remaining in col
            }
        }
    }
    // set rebalancing flag
    if (!std::atomic_compare_exchange_strong(&bRebalancing, &expect_rebal, true)) {
        fds_panic("VolumePlacement must not be in balancing state");
        // fix DMT state machine!
    }

    // get snapshot of current volumes
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = loc_domain->om_vol_mgr();
    fds_uint32_t vol_count = volumes->rs_container_snapshot(&vol_ary);
    NodeUuidSet rmNodes = cmap->getRemovedServices(fpi::FDSP_DATA_MGR);

    // for each volume, we will get column from committed and
    // target DMT (getting column is cheap operation, so ok to
    // re-do if several volumes fall into the same column
    for (RsContainer::const_iterator it = vol_ary.cbegin();
         it != vol_ary.cend();
         ++it) {
        fds_uint64_t volid = ((*it)->rs_get_uuid()).uuid_get_val();
        DmtColumnPtr cmt_col = dmtMgr->getCommittedNodeGroup(volid);
        DmtColumnPtr target_col = dmtMgr->getTargetNodeGroup(volid);

        // skip rebalancing of volumes in 'inactive' state -- those
        // are volumes that got delayed because we already set our
        // bRebalancing flag to true
        VolumeInfo::pointer volinfo = VolumeInfo::vol_cast_ptr(*it);
        if (volinfo->isVolumeInactive()) continue;
        // also skip rebalancing volumes which are in the process of
        // deleting (passed delete check)
        if (volinfo->isDeletePending()) continue;

        // for each DM in target column, find all DMs that
        // that got added to that column
        NodeUuidSet new_dms;
        for (fds_uint32_t j = 0; j < target_col->getLength(); ++j) {
            NodeUuid uuid = target_col->get(j);
            if (cmt_col->find(uuid) < 0) {
                new_dms.insert(uuid);
            }
        }

        LOGDEBUG << "Found " << new_dms.size() << " DMs that need to get"
                 << " meta for vol " << std::hex << volid << std::dec;

        // if no new DMs, then this volume does not need to be
        // rebalanced, start working on next volume
        if (new_dms.size() == 0) continue;

        // use the first node in the committed column which is not
        // deleted (primary, or if primary was deleted secondary, etc).
        // TODO(xxx) we may improve performance if we sync from
        // multiple sources (i.e. find > 1 node)
        fds_uint64_t src_dm;
        for (fds_uint32_t j = 0; j < cmt_col->getLength(); ++j) {
            if (rmNodes.count(cmt_col->get(j)) == 0) {
                src_dm = (cmt_col->get(j)).uuid_get_val();
                break;
            }
        }
        LOGDEBUG << "We will rsync from " << std::hex << src_dm << std::dec;
        // we must have at least one node that we can push meta from!
        fds_verify(src_dm != 0);

        if (push_meta_msgs.count(src_dm) == 0) {
            fpi::FDSP_PushMetaPtr meta_msg(new fpi::FDSP_PushMeta());
            push_meta_msgs[src_dm] = meta_msg;
        }
        for (NodeUuidSet::const_iterator cit = new_dms.cbegin();
             cit != new_dms.cend();
             ++cit) {
            // search meta list if we already have item with same
            // destination uuid
            fds_bool_t found = false;
            for (fds_uint32_t idx = 0;
                 idx < ((push_meta_msgs[src_dm])->metaVol).size();
                 ++idx) {
                fds_uint64_t uuid_val;
                uuid_val = ((push_meta_msgs[src_dm])->metaVol)[idx].node_uuid.uuid;
                if (uuid_val == (*cit).uuid_get_val()) {
                    ((push_meta_msgs[src_dm])->metaVol)[idx].volList.push_back(volid);
                    found = true;
                    break;
                }
            }
            if (!found) {
                fpi::FDSP_metaData meta;
                meta.node_uuid.uuid = (*cit).uuid_get_val();
                meta.volList.push_back(volid);
                ((push_meta_msgs[src_dm])->metaVol).push_back(meta);
            }
        }
    }

    // send msgs
    for (pm_msgs_t::iterator it = push_meta_msgs.begin();
         it != push_meta_msgs.end();
         ++it) {
        for (auto metavol : (it->second)->metaVol) {
            std::string volid_str;
            for (auto vol : metavol.volList) {
                volid_str.append(std::to_string(vol));
                volid_str.append(",");
            }
            LOGDEBUG << "Src node " << std::hex << it->first << ", Dst node "
                     << metavol.node_uuid.uuid << std::dec << " volumes " << volid_str;
        }
        if (OM_NodeDomainMod::om_in_test_mode()) {
            LOGDEBUG << "IN TEST MODE: not sending push meta messages";
            continue;
        }
        // send message
        OM_DmAgent::pointer agent = loc_domain->om_dm_agent(NodeUuid(it->first));
        fds_verify(agent != NULL);
        err = agent->om_send_pushmeta(it->second);
        if (err.ok()) {
            (*dm_set).insert(it->first);
        }
    }

    return err;
}

/**
 * Return true if 2 conditions are true:
 *  1. In rebalancing mode (between rebalancing started and
 *     we got notified rebalancing is finished): bRebalancing == true.
 *  2. Corresponding DMT column for volume 'volume id' is
 *     getting rebalanced.
 */
fds_bool_t VolumePlacement::isRebalancing(fds_volid_t volume_id) const {
    fds_uint64_t cols = pow(2, curDmtWidth);
    fds_uint32_t col_ix = DMT::getNodeGroupIndex(volume_id, cols);
    fds_bool_t b_rebal = std::atomic_load(&bRebalancing);
    return (b_rebal && (rebalColumns.count(col_ix) > 0));
}

/**
 * Notifies that rebalancing is finished. VolumePlacement may not be in
 * rebalancing mode (e.g. was nothing to rebalance), which is ok
 */
void VolumePlacement::notifyEndOfRebalancing() {
    std::atomic_exchange(&bRebalancing, false);
    LOGNORMAL << "Notified VolumePlacement about end of vol meta rebalance";
}

void
VolumePlacement::commitDMT() {
    dmtMgr->commitDMT();
    // both the new & the old dlts should already be in the config db
    // just set their types
    if (!configDB->setDmtType(dmtMgr->getCommittedVersion(), "committed")) {
        LOGWARN << "unable to store committed dmt type to config db "
                << "[" << dmtMgr->getCommittedVersion() << "]";
    }

    if (!configDB->setDmtType(0, "target")) {
        LOGWARN << "unable to store target dmt type to config db";
    }
    LOGNORMAL << "version: " << dmtMgr->getCommittedVersion();
    LOGDEBUG << *dmtMgr;
}

Error VolumePlacement::loadDmtsFromConfigDB(const NodeUuidSet& dm_services)
{
    Error err(ERR_OK);
    // NOTE: Copy paste code from DLT.  May need different handling
    // in the future

    // this function should be called only during init..
    fds_verify(dmtMgr->hasTargetDMT() == false);
    fds_verify(dmtMgr->hasCommittedDMT() == false);

    // if there are no nodes, we are not going to load any dmts
    if (dm_services.size() == 0) {
        LOGNOTIFY << "Not loading DMTs from configDB since there are "
                  << "no persisted DMs";
        return err;
    }

    fds_uint64_t committedVersion = configDB->getDmtVersionForType("committed");
    fds_uint64_t targetVersion = configDB->getDmtVersionForType("target");
    // We only support graceful restart.  We shouldn't have any cruft.
    fds_verify(targetVersion == 0);

    if (committedVersion > 0) {
        DMT* dmt = new DMT(0, 0, 0, false);
        if (!configDB->getDmt(*dmt, committedVersion)) {
            LOGCRITICAL << "unable to load (committed) dmt version "
                        <<"["<< committedVersion << "] from configDB";
            err = Error(ERR_PERSIST_STATE_MISMATCH);
        } else {
            // check if DMT is valid with respect to nodes
            // i.e. only contains node uuis that are in nodes' set
            // does not contain any zeroes, etc.
            std::set<fds_uint64_t> uniqueNodes;
            dmt->getUniqueNodes(&uniqueNodes);
            fds_verify(dm_services.size() == uniqueNodes.size())
            for (auto dm_uuid : dm_services) {
                fds_verify(uniqueNodes.count(dm_uuid.uuid_get_val()) == 1);
            }
            // we will set as target and distribute this dmt around
            dmtMgr->add(dmt, DMT_TARGET);
            LOGNOTIFY << "Loaded DMT version: " << committedVersion << ".  Setting as target";
        }

        if (!err.ok()) {
            delete dmt;
            return err;
        }
    } else {
        // we got > 0 nodes from configDB but there is no DMT
        // going to throw away persistent state, because there is a mismatch
        LOGWARN << "No current DMT even though we persisted "
                << dm_services.size() << " nodes";
        fds_verify(!"No persisted dmt");
        return Error(ERR_PERSIST_STATE_MISMATCH);
    }

    return err;
}

}  // namespace fds
