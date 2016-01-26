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
		  numOfPrimaryDMs(1),
		  numOfFailures(0)
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

    // TODO(Andrew): Should we also get DMT width and algo from config?
    curDmtWidth = 4;
    curDmtDepth = MODULEPROVIDER()->get_fds_config()->
            get<int>("fds.om.replica_factor");;

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

Error
VolumePlacement::computeDMT(const ClusterMap* cmap)
{
    Error err(ERR_OK);
    fds_uint64_t next_version = startDmtVersion;
    fds_uint32_t depth = curDmtDepth;
    DMT *newDmt = NULL;
    fds_bool_t dmResync = cmap->getDmResyncServices().size() ? true : false;

    if (dmResync) {
    	fds_assert(dmtMgr->hasCommittedDMT());
    	fds_assert(depth > 0);
    	LOGDEBUG << "DM Resync in progress. DMT calculation should remain the same.";
    }

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

    if (depth == 0) {
        // there are no DMs in the domain, nothing to compute
        return ERR_NOT_FOUND;
    }

    // allocate and compute new DMT
    newDmt = new(std::nothrow) DMT(curDmtWidth, depth, next_version, true);
    fds_verify(newDmt != NULL);
    {  // compute DMT
        fds_mutex::scoped_lock l(placementMutex);
        if (dmtMgr->hasCommittedDMT()) {
            err = placeAlgo->updateDMT(cmap, dmtMgr->getDMT(DMT_COMMITTED),
                                       newDmt, getNumOfPrimaryDMs());
            if (err == ERR_INVALID_ARG) {
                LOGWARN << "Couldn't update DMT most likely because we tried to "
                        << " remove all primary DMs from at least one DMT column";
                // DMT was not updated, because we are not supporting the update
                // with some combination of removed DMs. Returning an error
                delete newDmt;  // delete since not adding to dmtMgr
                return err;
            }
            fds_verify(err.ok());
            // if we ended up computing exactly the same DMT, do not
            // make it a target, just return an error so that state machine
            // knows not to proceed with commiting it, etc...
            DMTPtr commitedDmt = dmtMgr->getDMT(DMT_COMMITTED);
            if (!dmResync && (*commitedDmt == *newDmt)) {
                LOGDEBUG << "Newly computed DMT is the same as committed DMT."
                         << " Not going to commit";
                LOGDEBUG << *dmtMgr;
                LOGDEBUG << "Computed DMT (same as commited)" << *newDmt;
                delete newDmt;  // delete since not adding to dmtMgr
                return ERR_NOT_READY;
            }
        } else {
            placeAlgo->computeDMT(cmap, newDmt, getNumOfPrimaryDMs());
        }
    }

    // add this DMT as target, we should not have another non-committed
    // target already
//    fds_verify(!dmtMgr->hasTargetDMT());
    err = dmtMgr->add(newDmt, DMT_TARGET);
    LOGDEBUG << "Adding new DMT target, result: " << err;
    if ( err.ok() )
    {
//    fds_verify(err.ok());
//    fds_verify(configDB != NULL);
        if ( !configDB->storeDmt( *newDmt, "target" ) )
        {
            GLOGWARN << "unable to store dmt to config db "
                     << "[ " << newDmt->getVersion() << " ]";
        }

        LOGNORMAL << "Version: " << newDmt->getVersion();
        LOGDEBUG << "Computed new DMT: " << *newDmt;
    }

    return err;
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
    NodeUuidSet nonFailedDms = cmap->getNonfailedServices(fpi::FDSP_DATA_MGR);
    NodeUuidSet resyncNodes = cmap->getDmResyncServices();
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
        auto volid = fds_volid_t(((*it)->rs_get_uuid()).uuid_get_val());
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
        // also skip snapshot volumes
        if ( volinfo->vol_get_properties()->fSnapshot ) continue;

        // Populate volume descriptor
        volinfo->vol_fmt_desc_pkt(&vol_desc);

        // for each DM in target column, find all DMs that will need to
        // sync volume metadata
        // new_dms will contain new DMs that are in the target DMT but not in the committed DMT
        NodeUuidSet new_dms = target_col->getNewAndNewPrimaryUuids(*cmt_col, getNumOfPrimaryDMs());
        // Now put nodes that need to undergo resync as part of the "new" Dms
        for (const auto cit : resyncNodes) {
            new_dms.insert(cit);
        }
        LOGDEBUG << "Found " << new_dms.size() << " DMs " << " (" << resyncNodes.size()
        << " resyncing DM's) that need to get"
        << " meta for vol " << volid;

        // get list of candidates to resync from
        // if number of primary DMs == 0, means can resync from anyone
        fds_uint32_t rows = (getNumOfPrimaryDMs() > 0) ? getNumOfPrimaryDMs() : cmt_col->getLength();
        NodeUuidSet srcCandidates = cmt_col->getUuids(rows);
        // exclude DMs that were removed
        for (const auto dm : rmNodes) {
            srcCandidates.erase(dm);
        }
        // exclude DMs that need to undergo resync
        for (const auto dm : resyncNodes) {
            srcCandidates.erase(dm);
        }

        // if all DMs need resync, this means that we moved a secondary to be
        // a primary (both primaries failed)
        if (new_dms.size() == target_col->getLength()) {
            // this should not happen if num primary DMs = 0 (if we did not
            // care about resyncing secondaries). If that happen, it means
            // that DMT calculation algorithm managed to put all new DMs into
            // the same column.
            fds_verify(getNumOfPrimaryDMs() > 0);  // FIX DMT computation algorithm

            // there must be one DM that is in committed and target column
            // and in a primary row. Otherwise, all DMs failed in that column
            NodeUuidSet intersectDMs = target_col->getIntersection(*cmt_col);
            NodeUuid nosyncDm;
            for (auto dm: intersectDMs) {
                int index = target_col->find(dm);
                if ((index >= 0) && (index < (int) getNumOfPrimaryDMs())) {
                    if (nosyncDm.uuid_get_val() == 0) {
                        nosyncDm = dm;
                    } else {
                        // if we are here, means the whole column fail
                        // and we just skip doing anything for that column
                        nosyncDm.uuid_set_val(0);
                        break;
                    }
                }
            }
            if (nosyncDm.uuid_get_val() > 0) {
                // secondary DM survived, but both primaries didn't
                // do not sync to this DM
                new_dms.erase(nosyncDm);
                // but everyone should sync from this DM
                /**
                 * WARNING: This is a very edge case and is a last resort.
                 * There's no other pretty solution. We try to keep a number
                 * of primaries > 1 so that we don't get into this situation.
                 * The secondary DMs is most likely to be behind the primaries
                 * so we could potentially have data loss in this scenario.
                 * But the pro is that we at least can have a functional
                 * volume.
                 */
                srcCandidates.clear();
                srcCandidates.insert(nosyncDm);
            } else {
                LOGWARN << "Looks like the whole column for volume "
                << volid << " failed; no DM to sync from";
                continue;
            }
        }

        // if no new DMs, then this volume does not need to be
        // rebalanced, start working on next volume
        if (new_dms.size() == 0) continue;

        // there must be at least one DM candidate to be a source
        // otherwise we need to revisit DMT calculation algorithm
        LOGDEBUG << "Found " << srcCandidates.size()
                 << " candidates for a source for volume " << volid;

        if (srcCandidates.size() == 0)
        {
            LOGERROR << "MUST BE AT LEAST ONE DM CANDIDATE TO A SOURCE."
                     << " OTHERWISE, DMT CALCULATION ALGORITHM NEEDS CHECKING!";
            continue;
            // fds_verify(srcCandidates.size() > 0);
        }

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
                bool grp_found = false;
                for (MiGrIter migrationGrpIter = srcDMMigrateMsg->migrations.begin();
                     migrationGrpIter != srcDMMigrateMsg->migrations.end();
                     migrationGrpIter++) {
                    /**
                     * Go through this CtrlNotifyDMStartMigrationMsg's migrations
                     * and see if we can find an existing source to append this volume
                     * to that source.
                     */
                    for (auto uuid : srcCandidates) {
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
                    for (auto uuid : srcCandidates) {
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
        if (pmiter->second.migrations.size() == 0) {
            /* Nothing to migrate */
            continue;
        }
    	OM_DmAgent::pointer agent = loc_domain->om_dm_agent(NodeUuid(pmiter->first));
    	fds_verify(agent != nullptr);

    	// Making a copy because boost pointer will try to take ownership of the map value.
    	fpi::CtrlNotifyDMStartMigrationMsgPtr message(new fpi::CtrlNotifyDMStartMigrationMsg(pmiter->second));
    	NodeUuid node (pmiter->first);
    	message->DMT_version = dmtMgr->getTargetVersion();

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
void VolumePlacement::commitDMT() {
    commitDMT( false );
}

void VolumePlacement::commitDMT( const bool unsetTarget )
{   
    // commit DMT without removing target
    prevDmtVersion = dmtMgr->getCommittedVersion();
    dmtMgr->commitDMT(false);
    LOGNORMAL << "DMT version: " << dmtMgr->getCommittedVersion()
              << " Previous committed DMT version " << prevDmtVersion;
    LOGDEBUG << *dmtMgr;
    
    if ( unsetTarget )
    {
        dmtMgr->unsetTarget( false );
    }
}

fds_bool_t
VolumePlacement::undoTargetDmtCommit() {
    fds_bool_t rollBackNeeded = false;
    if (dmtMgr->hasTargetDMT()) {
        if (!hasNonCommitedTarget()) {
            LOGDEBUG << "Failure occured after commit - reverting committed DMT version to " << prevDmtVersion;
            // we already assigned committed DMT target, revert.
            // Note: the false here is to ensure that "target_version" is not wiped... so the unsetTarget below will work.
            Error err = dmtMgr->commitDMT(prevDmtVersion, false);
            fds_verify(err.ok());
            prevDmtVersion = DMT_VER_INVALID;

            LOGDEBUG << "Clearing target DMT";

            // forget about target DMT and remove it from DMT manager
            dmtMgr->unsetTarget(true);

            // also forget target DMT in persistent store
            if (!configDB->setDmtType(0, "target")) {
                LOGWARN << "unable to store target dmt type to config db";
            }
            rollBackNeeded = true;
        } else {
            OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();

            if (domain->isDomainShuttingDown()) {
                LOGDEBUG << "Domain is shutting down, unset target DMT for correct state on startup";
                dmtMgr->unsetTarget(false);
            }
        }
    }
    return rollBackNeeded;
}

void
VolumePlacement::clearTargetDmt()
{
    LOGDEBUG << "Clearing out target DMT in configDB & dmtMgr";
    if (!configDB->setDmtType(0, "target")) {
        LOGWARN << "unable to store target dmt type to config db";
    }

    dmtMgr->unsetTarget(false);

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

Error VolumePlacement::loadDmtsFromConfigDB(const NodeUuidSet& dm_services,
                                            const NodeUuidSet& deployed_dm_services)
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
    if (committedVersion > 0) {
        DMT* dmt = new DMT(0, 0, 0, false);
        if (!configDB->getDmt(*dmt, committedVersion)) {
            LOGCRITICAL << "unable to load (committed) dmt version "
                        <<"["<< committedVersion << "] from configDB";
            err = Error(ERR_PERSIST_STATE_MISMATCH);
        } else {
            // check if DMT is valid with respect to nodes
            // i.e. only contains node uuids that are in deployed_dm_services
            // set and does not contain any zeroes, etc.
            err = dmt->verify(deployed_dm_services);
            if (err.ok()) {
                LOGNOTIFY << "Current DMT in config DB is valid!";
                // we will set dmt because we don't know yet if
                // the nodes in DMT will actually come up...
                dmtMgr->add(dmt, DMT_TARGET);
                LOGNOTIFY << "Loaded DMT version: " << committedVersion << ".  Setting as target";
            }
        }

        if (!err.ok()) {
            delete dmt;
            return err;
        }
    } else {
        // ok if there are no deployed DMs
        if (deployed_dm_services.size() == 0) {
            LOGNOTIFY << "There is no persisted DMT and no deployed DMs -- OK";
            return ERR_OK;
        }
        // we got > 0 deployed DMs from configDB but there is no DMT
        // going to throw away persistent state, because there is a mismatch
        LOGWARN << "No current DMT even though we persisted "
                << dm_services.size() << " DMs, "
                << deployed_dm_services.size() << " deployed DMs";
        return Error(ERR_PERSIST_STATE_MISMATCH);
    }


    // If we saved target DMT version, we were in the middle of migration
    // when domain shutdown or OM went down. We are going to restart migration
    // and re-compute the DMT again, so throw away this DMT
    // Maybe in the future, we can re-start migration with saved target DMT
    // but this is an optimization, may do later
    fds_uint64_t targetVersion = configDB->getDmtVersionForType("target");
    if (targetVersion > 0 && targetVersion != committedVersion) {
        LOGNOTIFY << "OM went down in the middle of migration. Will throw away "
                  << "persisted  target DMT and re-compute it again if discovered "
                  << "DMs re-register";
        if (!configDB->setDmtType(0, "next")) {
            LOGWARN << "unable to reset target DMT version to config db ";
        }

        DltDmtUtil::getInstance()->setDMAbortParams(true, targetVersion);
    } else {
        if (0 == targetVersion) {
            LOGDEBUG << "There is only commited DMT in configDB (OK)";
        } else {
            LOGDEBUG << "Both current and target DMT are of same version (OK): "
                     << targetVersion;
        }
    }

    return err;
}

fds_bool_t VolumePlacement::canRetryMigration() {
    fds_bool_t ret = false;

    // For now, we maximize at 4. Perhaps to be made into a configurable var
    // next time?
    if (numOfFailures < 4) {
        ret = true;
    }

    return ret;
}
}  // namespace fds
