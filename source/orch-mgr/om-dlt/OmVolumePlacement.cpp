/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <new>
#include <map>
#include <set>
#include <OmClusterMap.h>
#include <OmVolumePlacement.h>
#include "fdsp/dm_api_types.h"
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>

namespace fds {

VolumePlacement::VolumePlacement()
        : Module("Volume Placement Engine"),
          dmtMgr(new DMTManager(1)),
          prevDmtVersion(DMT_VER_INVALID),
          startDmtVersion(DMT_VER_INVALID + 1),
          placeAlgo(NULL),
          placementMutex("Volume Placement mutex"),
		  numOfPrimaryDMs(1)
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
    setNumOfPrimaryDMs(MODULEPROVIDER()->get_fds_config()->
				  get<int>("fds.dm.number_of_primary"));

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
    fds_uint64_t next_version = startDmtVersion;
    fds_uint32_t depth = curDmtDepth;
    DMT *newDmt = NULL;

    // if we alreay have commited DMT, next version is inc 1
    if (dmtMgr->hasCommittedDMT()) {
        next_version = dmtMgr->getCommittedVersion() + 1;
    } else {
        // will use startDmtVersion, but increment startDmtVersion
        // in case this DMT will be reverted (due to error)
        ++startDmtVersion;
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
 * Finds the new DMs joining the domain, figure out the source DMs
 * and the volumes belonging to those source DMs from which the new
 * DMs can pull.
 * Returns a set of uuids of DMs to which Pull message was sent
 */
Error
VolumePlacement::beginRebalance(const ClusterMap* cmap,
                                NodeUuidSet* dm_set)
{
    Error err(ERR_OK);
    fds_bool_t expect_rebal = false;
    fds_uint64_t dmt_columns = pow(2, curDmtWidth);
    fds_verify(dm_set != NULL);
    RsArray vol_ary;

    // If we do not have any committed DMT, means that this is
    // the first DM(s) added to the domain, so no need to rebalance
    if (!dmtMgr->hasCommittedDMT()) {
        LOGNOTIFY << "Not going to rebalance volume meta, because "
                  << " this is the first DMT we computed";
        return err;
    }
    fds_verify(dmtMgr->hasTargetDMT());

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
    // List used for sending DMVolumeMigrationGroup
    std::list<fpi::FDSP_VolumeDescType> volDescList;
    /**
     * What we have here:
     * 1. A unordered_map of NewDMs -> CtrlNotifyDMStartMigrationMsgs
     * 2. Each CtrlNotifyMDStartMsgs contains a list of migrations.
     * 3. Each migration contains a map of srcNodes -> list of volumes to pull.
     * The for loop below:
     * a. Goes through each volume.
     * b. Finds the new DMs that need to pull this volume.
     * c. Look through the map of new DMs, appends if it's not in the list.
     * d. Goes through the NewDM's of ctrlNotifymsgs, finds if a source
     * 		exists there that matches this volume's DMT primary DM. If so, adds
     * 		the volume to that list. Otherwise, create a new source-> vol mapping.
     */
    using migrateMsgs = fpi::CtrlNotifyDMStartMigrationMsg;
    using pull_msgs = std::unordered_map<fds_uint64_t, migrateMsgs>;
    pull_msgs pull_msg;

    // for each volume, we will get column from committed and
    // target DMT (getting column is cheap operation, so ok to
    // re-do if several volumes fall into the same column
    for (RsContainer::const_iterator it = vol_ary.cbegin();
         it != vol_ary.cend();
         ++it) {
        fds_uint64_t volid = ((*it)->rs_get_uuid()).uuid_get_val();
        DmtColumnPtr cmt_col = dmtMgr->getCommittedNodeGroup(volid);
        DmtColumnPtr target_col = dmtMgr->getTargetNodeGroup(volid);
        fpi::FDSP_VolumeDescType vol_desc; // to be added to list

        // skip rebalancing of volumes in 'inactive' state -- those
        // are volumes that got delayed because we already set our
        // bRebalancing flag to true
        VolumeInfo::pointer volinfo = VolumeInfo::vol_cast_ptr(*it);
        if (volinfo->isVolumeInactive()) continue;
        // also skip rebalancing volumes which are in the process of
        // deleting (passed delete check)
        if (volinfo->isDeletePending()) continue;

        // Populate volume descriptor
        volinfo->vol_fmt_desc_pkt(&vol_desc);

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

        for (NodeUuidSet::const_iterator cit = new_dms.cbegin();
             cit != new_dms.cend();
             ++cit) {

        	migrateMsgs *srcDMMigrateMsg = nullptr;
        	/**
        	 * First go through the existing pull_msg. See if this new DM
        	 * exists in the map already. If not, create an entry.
        	 */
        	pull_msgs::iterator got = pull_msg.find(cit->uuid_get_val());
        	if (got == pull_msg.end()) {
        		/**
        		 * Not found. First DestinationDM instance in map.
        		 * Store this new (destination) DM UUID as the key,
        		 * and the actual msg as the value.
        		 */
        		srcDMMigrateMsg = new migrateMsgs();
        		pull_msg[cit->uuid_get_val()] = *srcDMMigrateMsg;
        		srcDMMigrateMsg->DMT_version = dmtMgr->getTargetVersion();
        	} else {
        		srcDMMigrateMsg = &(got->second);
        	}

        	/**
        	 * At this time, the volume descriptor is what the DM doesn't have.
        	 * Look through DM Migration groups to see if any of the source DMs
        	 * contain this volume. If so, add this volume to that list.
        	 * If not, create a new group.
			 * If a node in the migration group exists that matches a node
			 * in the cmt_col, that means this this current committed node
			 * can be a candidate for the dest node to pull from.
			 * Do a lookup only from the primary DMs.
			 */
			using MiGrIter = std::vector<fpi::DMVolumeMigrationGroup>::iterator;
			NodeUuid uuid;
			fds_verify(uuid.uuid_get_val() == 0); // 0 means not found yet.
			bool grp_found = false;
			for (MiGrIter migrationGrpIter = srcDMMigrateMsg->migrations.begin();
					migrationGrpIter != srcDMMigrateMsg->migrations.end();
					migrationGrpIter++) {
				/**
				 * Go through this CtrlNotifyDMStartMigrationMsg's migrations
				 * and see if we can find an existing source to append this volume
				 * to that source.
				 */
				for (fds_uint32_t j = 0; j < getNumOfPrimaryDMs(); j++) {
					if (j >= cmt_col->getLength()) break;
					// Go through the commit DMT column to find a primary
					uuid = cmt_col->get(j);
					if (rmNodes.count(uuid) > 0) {
						LOGDEBUG << "Node is being removed: " << uuid.uuid_get_val();
						// This primary is being removed. Look for another primary.
						continue;
					}
					if ((unsigned)migrationGrpIter->source.svc_uuid ==
							uuid.uuid_get_val()) {
						grp_found = true;
						migrationGrpIter->VolDescriptors.push_back(vol_desc);
						LOGDEBUG << "DM: " << cit->uuid_get_val() << " will pull from: "
								<< migrationGrpIter->source.svc_uuid << " for volume ("
								<< vol_desc.volUUID << ") " << vol_desc.vol_name;
						break;
					}
				}
				if (grp_found) {
					break;
				}
			}
        	if (!grp_found) {
        		/**
        		 * Currently, in the migration msg, there's no source node that
        		 * contains this volume. So we need to find a primary DM node
        		 * as the source to pull from, since primary DM(s) will be the
        		 * only ones that are trustworthy.
        		 */
				for (fds_uint32_t j = 0; j < getNumOfPrimaryDMs(); j++) {
					if (j >= cmt_col->getLength()) break;
					// Go through the commit DMT column to find a primary
					uuid = cmt_col->get(j);
					if (rmNodes.count(uuid) > 0) {
						LOGDEBUG << "Node is being removed: " << uuid.uuid_get_val();
						// This primary is being removed. Look for another primary.
						continue;
					}
					fpi::DMVolumeMigrationGroup newGroup;
					newGroup.source.svc_uuid = uuid.uuid_get_val();
					newGroup.VolDescriptors.push_back(vol_desc);
					srcDMMigrateMsg->migrations.push_back(newGroup);
					LOGDEBUG << "DM: " << cit->uuid_get_val() << " will pull from: "
							<< newGroup.source.svc_uuid << " for volume ("
							<< vol_desc.volUUID << ") " << vol_desc.vol_name;
					grp_found = true;
					break;
				}
        	}
        	if (!grp_found) {
        		/**
        		 * Primaries are all being removed. Unable to rebalance.
        		 * This is really bad and should be addressed in future error
        		 * handling cases.
        		 */
        		LOGERROR << "Primary DM(s) are being removed in target DMT";
        		err = ERR_DM_MIGRATION_ABORTED;
        		return (err);
        	}
        } // End for(NodeUuidSet)
    }

    // Send pull messages to the new DMs
    for (pull_msgs::iterator pmiter = pull_msg.begin();
    		pmiter != pull_msg.end(); pmiter++) {
    	OM_DmAgent::pointer agent = loc_domain->om_dm_agent(NodeUuid(pmiter->first));
    	fds_verify(agent != nullptr);

    	// Making a copy because boost pointer will try to take ownership of the map value.
    	fpi::CtrlNotifyDMStartMigrationMsgPtr message(new fpi::CtrlNotifyDMStartMigrationMsg(pmiter->second));
    	NodeUuid node (pmiter->first);

    	err = agent->om_send_pullmeta(message);
    	if (err.ok()) {
    		(*dm_set).insert(node);
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
    // commit DMT without removing target
    prevDmtVersion = dmtMgr->getCommittedVersion();
    dmtMgr->commitDMT(false);
    LOGNORMAL << "DMT version: " << dmtMgr->getCommittedVersion()
              << " Previous committed DMT version " << prevDmtVersion;
    LOGDEBUG << *dmtMgr;
}

void
VolumePlacement::undoTargetDmtCommit() {
    if (dmtMgr->hasTargetDMT()) {
        if (!hasNonCommitedTarget()) {
            // we already assigned committed DMT target, revert
            Error err = dmtMgr->commitDMT(prevDmtVersion);
            fds_verify(err.ok());
            prevDmtVersion = DMT_VER_INVALID;
        }

        // forget about target DMT and remove it from DMT manager
        dmtMgr->unsetTarget(true);

        // also forget target DMT in persistent store
        if (!configDB->setDmtType(0, "target")) {
            LOGWARN << "unable to store target dmt type to config db";
        }
    }
}

/**
 * Returns true if there is no target DMT computed or committed
 * as an official version
 */
fds_bool_t
VolumePlacement::hasNoTargetDmt() const {
    return (dmtMgr->hasTargetDMT() == false);
}

fds_bool_t
VolumePlacement::hasNonCommitedTarget() const {
    if ((dmtMgr->hasCommittedDMT() == false) ||
        (dmtMgr->getCommittedVersion() != dmtMgr->getTargetVersion())) {
        return true;
    }
    return false;
}

Error
VolumePlacement::persistCommitedTargetDmt() {
    if (!dmtMgr->hasTargetDMT() ||
        !dmtMgr->hasCommittedDMT()) {
        return ERR_NOT_FOUND;
    } else if (dmtMgr->getCommittedVersion() != dmtMgr->getTargetVersion()) {
        return ERR_NOT_READY;
    }

    // both the new & the old dlts should already be in the config db
    // just set their types
    if (!configDB->setDmtType(dmtMgr->getCommittedVersion(), "committed")) {
        LOGWARN << "unable to store committed dmt type to config db "
                << "[" << dmtMgr->getCommittedVersion() << "]";
    }

    if (!configDB->setDmtType(0, "target")) {
        LOGWARN << "unable to store target dmt type to config db";
    }
    LOGNORMAL << "DMT version: " << dmtMgr->getCommittedVersion();
    LOGDEBUG << *dmtMgr;

    // unset target DMT in DMT Mgr
    Error err = dmtMgr->unsetTarget(false);
    if (!err.ok()) {
        LOGERROR << "Failed to unset DMT target " << err;
    }
    return err;
}

Error
VolumePlacement::validateDmtOnDomainActivate(const NodeUuidSet& dm_services) {
    Error err(ERR_OK);

    if (dmtMgr->hasCommittedDMT()) {
        std::set<fds_uint64_t> uniqueNodes;
        dmtMgr->getDMT(DMT_COMMITTED)->getUniqueNodes(&uniqueNodes);
        if (dm_services.size() == uniqueNodes.size()) {
            LOGDEBUG << "DMT is valid!";
            // but we must not have any target DMT at this point
            // when we shutdown domain, we should have aborted any ongoing migrations
            if (dmtMgr->hasTargetDMT()) {
                LOGERROR << "Unexpected target DMT exists! We should have aborted "
                         << " DMT update during domain shutdown!";
                return ERR_NOT_IMPLEMENTED;
            }
        }
    } else {
        // there must be no DM services
        if (dm_services.size() > 0) {
            LOGERROR << "No DMT but " << dm_services.size() << " known DM services";
            return ERR_PERSIST_STATE_MISMATCH;
        }
    }

    return err;
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
