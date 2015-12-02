/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <unordered_map>
#include <map>
#include <utility>
#include <vector>
#include <set>
#include <string>
#include <fdsp_utils.h>
#include <fiu-control.h>
#include <util/fiu_util.h>
#include <orch-mgr/om-service.h>
#include <fds_process.h>
#include <OmDataPlacement.h>
#include <net/net-service.h>
#include <fdsp/svc_types_types.h>
#include <net/SvcRequestPool.h>
#include <list>
#include "fdsp/sm_api_types.h"

namespace fds {

/**********
 * Functions definitions for data
 * placement
 **********/
DataPlacement::DataPlacement()
        : Module("Data Placement Engine"),
          placeAlgo(NULL),
          placementMutex("Data Placement mutex"),
          prevDlt(NULL),
          commitedDlt(NULL),
          newDlt(NULL),
          sendMigAbortAfterRestart(false){
    curClusterMap = &gl_OMClusMapMod;
    numOfPrimarySMs = 0;
}

DataPlacement::~DataPlacement() {
    // delete curClusterMap;
    if (commitedDlt != NULL) {
        delete commitedDlt;
    }
    if (newDlt != NULL) {
        delete newDlt;
    }
}

void
DataPlacement::setAlgorithm(PlacementAlgorithm::AlgorithmTypes type) {
    fds_mutex::scoped_lock l(placementMutex);
    delete placeAlgo;
    switch (type) {
        case PlacementAlgorithm::AlgorithmTypes::RoundRobin:
            placeAlgo = new RoundRobinAlgorithm();
            break;
        case PlacementAlgorithm::AlgorithmTypes::ConsistHash:
            placeAlgo = new ConsistHashAlgorithm();
            break;

        default:
            fds_panic("Received unknown data placement algorithm type %u",
                      type);
    }
    algoType = type;
}

Error
DataPlacement::updateMembers(const NodeList &addNodes,
                             const NodeList &rmNodes) {
    fds_mutex::scoped_lock l(placementMutex);
    LOGDEBUG << "Updating OM Cluster Map "
             << " Add Nodes: " << addNodes.size()
             << " Remove Nodes: " << rmNodes.size();
    
    Error err = curClusterMap->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);
    // TODO(Andrew): We should be recomputing the DLT here.

    return err;
}

Error
DataPlacement::computeDlt() {
    // Currently always create a new empty DLT.
    // Will change to be relative to the current.

    fds_mutex::scoped_lock l(placementMutex);

    fds_uint64_t version;
    Error err(ERR_OK);

    /**
     * as of 11/24/2015, this should be a no harm fds_verify, so removing it, because we are now setting the
     * newDlt on all restart use cases.
     */
//    fds_verify( newDlt == NULL );

    if (commitedDlt == NULL) 
    {
        version = DLT_VER_INVALID + 1;
    } 
    else 
    {
        version = commitedDlt->getVersion() + 1;
    }
    
    // If we have fewer members than total replicas
    // use the number of members as the replica count
    fds_uint32_t depth = curDltDepth;
    if (curClusterMap->getNumMembers(fpi::FDSP_STOR_MGR) < curDltDepth) 
    {
        depth = curClusterMap->getNumMembers(fpi::FDSP_STOR_MGR);
    }
    if (depth == 0) {
        // there are no SMs in the domain, nothing to compute
        return ERR_NOT_FOUND;
    }

    // Allocate and compute new DLT
    newDlt = new DLT(curDltWidth,
                     depth,
                     version,
                     true);
    err = placeAlgo->computeNewDlt(curClusterMap,
                                   commitedDlt,
                                   newDlt, getNumOfPrimarySMs());

    // Compute DLT's reverse node to token map
    if (err.ok()) {
        newDlt->generateNodeTokenMap();

        // it is possible that this method is called
        // when there are no changes in cluster map that require
        // a change in the DLT, check that newDlt is different from commited
        if (commitedDlt && (*commitedDlt == *newDlt)) {
            LOGDEBUG << "Newly computed DLT is the same as committed DLT."
                     << " Not going to commit";
            LOGDEBUG << *commitedDlt;
            LOGDEBUG << "Computed DLT (same as commited)" << *newDlt;
            err = ERR_NOT_READY;
        }
    }

    if (err.ok()) {
        // we computed new DLT which we want to commit
        // store the dlt to config db
        fds_verify(configDB != NULL);
        if (!configDB->storeDlt(*newDlt, "next")) 
        {
            GLOGERROR << "unable to store dlt to config db "
                      << "[" << newDlt->getVersion() << "]";
            err = ERR_DISK_WRITE_FAILED;
        }
    } else {
        LOGNORMAL << "Did not update DLT " << err;
        delete newDlt;
        newDlt = nullptr;
    }

    // TODO(Andrew): We should version the (now) old DLT
    // before we delete it and replace it with the
    // new DLT. We should also update the DLT's
    // internal version.
    return err;
}

/**
 * Begins a rebalance command between the nodes affect
 * in the current DLT change.
 */
Error
DataPlacement::beginRebalance() {
    Error err(ERR_OK);

    fds_mutex::scoped_lock l(placementMutex);
    // find all nodes which need to do migration (=have new tokens)
    rebalanceNodes.clear();

    if (!commitedDlt) 
    {
        LOGNOTIFY << "Not going to rebalance data, because this is "
                  << " the first DLT we computed";
        return err;
    }

    // at this point we have committed DLT
    // find all SMs that will either get a new responsibility for a DLT token
    // or need re-sync because an SM became a primary
    NodeUuidSet rmSMs = curClusterMap->getRemovedServices(fpi::FDSP_STOR_MGR);
    NodeUuidSet failedSMs = curClusterMap->getFailedServices(fpi::FDSP_STOR_MGR);
    // NodeUuid.uuid_get_val() for source SM to CtrlNotifySMStartMigrationPtr msg
    std::unordered_map<fds_uint64_t, fpi::CtrlNotifySMStartMigrationPtr> startMigrMsgs;
    for (fds_token_id tokId = 0; tokId < newDlt->getNumTokens(); ++tokId) {
        DltTokenGroupPtr cmtCol = commitedDlt->getNodes(tokId);
        DltTokenGroupPtr tgtCol = newDlt->getNodes(tokId);

        // find all SMs in target column that need re-sync: either they
        // got a new responsibility for a DLT token or became a primary
        NodeUuidSet destSms = tgtCol->getNewAndNewPrimaryUuids(*cmtCol, getNumOfPrimarySMs());
        if (destSms.size() == 0) continue;
        LOGDEBUG << "Found " << destSms.size() << " SMs that need to sync token " << tokId;

        // get list of candidates to sync from
        fds_uint32_t rows = (getNumOfPrimarySMs() > 0) ? getNumOfPrimarySMs() : cmtCol->getLength();
        NodeUuidSet srcCandidates = cmtCol->getUuids(rows);

        // exclude SMs that were removed
        for (auto sm : rmSMs) {
            srcCandidates.erase(sm);
        }
        // exclude SMs that failed
        for (auto sm : failedSMs) {
            srcCandidates.erase(sm);
        }

        // if all SMs need re-sync, this means that we moved a secondary to be
        // a primary (both primaries failed)
        if (destSms.size() == tgtCol->getLength()) {
            // this should not happen if number of primary SMs == 0 (this config
            // means original implementation where we do not re-sync when promoting secondaries)
            // If that happens, it means that DLT calculation algorithm managed to
            // place all new SMs into the same column -- will need to fix that!
            fds_verify(getNumOfPrimarySMs() > 0);

            // there must be one SM that is in committed and target column
            // and in a primary row. Otherwise all SMs failed in that column
            NodeUuidSet intersectSMs = tgtCol->getIntersection(*cmtCol);
            NodeUuid nosyncSm;
            for (auto sm: intersectSMs) {
                int index = tgtCol->find(sm);
                if ((index >= 0) && (index < (int)getNumOfPrimarySMs())) {
                    if (nosyncSm.uuid_get_val() == 0) {
                        nosyncSm = sm;
                    } else {
                        // if we are here, means the whole column fail
                        // and we just skip doing anything for that column
                        nosyncSm.uuid_set_val(0);
                        break;
                    }
                }
            }
            if (nosyncSm.uuid_get_val() > 0) {
                // secondary SM survived, but both primaries didn't
                // do not sync to this SM
                destSms.erase(nosyncSm);
                // but everyone should sync from this SM
                srcCandidates.clear();
                srcCandidates.insert(nosyncSm);
            } else {
                LOGWARN << "Looks like the whole column for DLT token "
                        << tokId << " failed; no SM to sync from";
                continue;
            }
        }

        // there need to be at least one candidate to be a source
        // otherwise we need to revisit DLT computation algorithm
        LOGMIGRATE << "Found " << srcCandidates.size() << " candidates for a source "
                   << " for DLT token " << tokId;

//        fds_verify(srcCandidates.size() > 0);

        for (auto smUuid: destSms) {
            // see if we already have a startMigration msg prepared for this source
            fpi::CtrlNotifySMStartMigrationPtr startMigrMsg;
            if (startMigrMsgs.count(smUuid.uuid_get_val()) == 0) {
                // create start migration msg for this source SM
                fpi::CtrlNotifySMStartMigrationPtr msg(new fpi::CtrlNotifySMStartMigration());
                startMigrMsgs[smUuid.uuid_get_val()] = msg;
            }
            startMigrMsg = startMigrMsgs[smUuid.uuid_get_val()];

            // find a source for this destination SM
            NodeUuid sourceUuid;
            for (auto srcCandidate: srcCandidates) {
                // cannot be myself
                if (srcCandidate != smUuid) {
                    sourceUuid = srcCandidate;
                    break;
                }
            }

//            fds_verify(sourceUuid.uuid_get_val() !=0);
            fds_bool_t found = false;
            if ( sourceUuid.uuid_get_val() > 0 ) {
                LOGMIGRATE << "Destination " << std::hex << smUuid.uuid_get_val()
                << " Source " << sourceUuid.uuid_get_val() << std::dec
                << " token " << tokId;

                // find if there is already a migration group created for this src SM

                for (fds_uint32_t index = 0; index < (startMigrMsg->migrations).size(); ++index) {
                    if ((startMigrMsg->migrations)[index].source == sourceUuid.toSvcUuid()) {
                        found = true;
                        (startMigrMsg->migrations)[index].tokens.push_back(tokId);
                        LOGTRACE << "Found group for destination: " << std::hex << smUuid.uuid_get_val()
                        << ", source: " << sourceUuid.uuid_get_val() << std::dec
                        << ", adding token " << tokId;
                        break;
                    }
                }
            }

            if (!found) {
                fpi::SMTokenMigrationGroup grp;
                grp.source = sourceUuid.toSvcUuid();
                grp.tokens.push_back(tokId);
                startMigrMsg->migrations.push_back(grp);
                LOGTRACE << "Starting new group for destination: " << std::hex << smUuid.uuid_get_val()
                         << ", source: " << std::hex<< sourceUuid.uuid_get_val()
                         << ", adding token " << tokId;
            }
        }
    }

    // actually send start migration messages
    for (auto entry: startMigrMsgs) {
        NodeUuid uuid(entry.first);
        fpi::CtrlNotifySMStartMigrationPtr msg = entry.second;

        auto om_req =  gSvcRequestPool->newEPSvcRequest(uuid.toSvcUuid());

        msg->DLT_version = newDlt->getVersion();

        om_req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifySMStartMigration), msg);
        om_req->onResponseCb(std::bind(&DataPlacement::startMigrationResp, this, uuid,
                                       newDlt->getVersion(),
                                       std::placeholders::_1, std::placeholders::_2,
                                       std::placeholders::_3));
        // really long timeout for migration = 10 hours
        om_req->setTimeoutMs(10*60*60*1000);
        om_req->invoke();

        LOGNOTIFY << "Sending the DLT migration request to node 0x"
                  << std::hex << uuid.uuid_get_val() << std::dec;

        rebalanceNodes.insert(uuid);
    }

    LOGNOTIFY << "Sent DLT migration event to " << rebalanceNodes.size() << " nodes";
    newDlt->dump();
    return err;
}

/**
* Response handler for startMigration message.
*/
void DataPlacement::startMigrationResp(NodeUuid uuid,
                                       fds_uint64_t dltVersion,
                                       EPSvcRequest* svcReq,
                                       const Error& error,
                                       boost::shared_ptr<std::string> payload) {
    Error err(ERR_OK);
    try {
        LOGNOTIFY << "Received migration done notification from node "
                  << std::hex << uuid << std::dec
                  << " for target DLT version " << dltVersion << " " << error;

        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        err = domain->om_recv_migration_done(uuid, dltVersion, error);
    }
    catch(...) {
        LOGERROR << "Orch Mgr encountered exception while "
                    << "processing migration done request";
    }
}

/**
 * Returns true if there is no target DLT computed or committed
 * as official version (but not to persistent store)
 */
fds_bool_t DataPlacement::hasNoTargetDlt() const {
    return (newDlt == NULL);
}

/**
 * Returns true if there is target DLT computed, but not yet commited
 * as official version
 */
fds_bool_t DataPlacement::hasNonCommitedTarget() const {
    if (newDlt != NULL) {
        if ((commitedDlt == NULL) ||
            (commitedDlt->getVersion() != newDlt->getVersion())) {
            return true;
        }
    }
    return false;
}

/**
 * Returns true if there is target DLT that is commited as official
 * version but not persisted yet
 */
fds_bool_t DataPlacement::hasCommitedNotPersistedTarget() const {
    if (newDlt && (newDlt->getVersion() == commitedDlt->getVersion())) {
        return true;
    }
    return false;
}

/**
 * Commits the current DLT as an 'official'
 * copy. The commit stores the DLT to the
 * permanent DLT history.
 */
void
DataPlacement::commitDlt() {
    commitDlt( false );
}
void
DataPlacement::commitDlt( const bool unsetTarget ) {

    fds_mutex::scoped_lock l(placementMutex);
    
    fds_verify(newDlt != NULL);
    fds_uint64_t oldVersion = -1;
    if (commitedDlt) {
        oldVersion = commitedDlt->getVersion();
        if (prevDlt) {
            delete prevDlt;
            prevDlt = NULL;
        }
        prevDlt = commitedDlt;
    }

    commitedDlt = newDlt;

    if ( unsetTarget )
    {
        if (!configDB->setDltType(0, "next")) {
            LOGWARN << "Failed to unset target DLT in config db";
        }
        newDlt = NULL;
    }
}

///  only reverts if we did not persist it..
void
DataPlacement::undoTargetDltCommit() {
    fds_mutex::scoped_lock l(placementMutex);
    if (newDlt) {
        if (!hasNonCommitedTarget()) {
            // we already assigned commitedDlt target, revert back
            commitedDlt = prevDlt;
            prevDlt = NULL;
        }

        // forget about target DLT
        fds_uint64_t targetDltVersion = newDlt->getVersion();
        delete newDlt;
        newDlt = NULL;

        // also forget target DLT in persistent store
        if (!configDB->setDltType(0, "next")) {
            LOGWARN << "Failed to unset target DLT in config db "
                    << "[" << targetDltVersion << "]";
        }
    }
}

void
DataPlacement::persistCommitedTargetDlt() {
    fds_mutex::scoped_lock l(placementMutex);
    fds_verify(newDlt != NULL);
    fds_verify(commitedDlt != NULL);
    fds_verify(newDlt->getVersion() == commitedDlt->getVersion());

    // both the new & the old dlts should already be in the config db
    // just set their types

    if (!configDB->setDltType(commitedDlt->getVersion(), "current")) {
        LOGWARN << "unable to store dlt type to config db "
                << "[" << commitedDlt->getVersion() << "]";
    }

    if (!configDB->setDltType(0, "next")) {
        LOGWARN << "unable to store dlt type to config db "
                << "[" << commitedDlt->getVersion() << "]";
    }

    // TODO(prem) do we need to remove the old dlt from the config db ??
    // oldVersion ?

    newDlt = NULL;
}

const DLT*
DataPlacement::getCommitedDlt() const {
    return commitedDlt;
}

const DLT*
DataPlacement::getTargetDlt() const {
    return newDlt;
}

fds_uint64_t
DataPlacement::getLatestDltVersion() const {
    if (newDlt) {
        return newDlt->getVersion();
    } else if (commitedDlt) {
        return commitedDlt->getVersion();
    }
    return DLT_VER_INVALID;
}

fds_uint64_t
DataPlacement::getCommitedDltVersion() const {
    if (commitedDlt) {
        return commitedDlt->getVersion();
    }
    return DLT_VER_INVALID;
}

fds_uint64_t
DataPlacement::getTargetDltVersion() const {
    if (newDlt) {
        return newDlt->getVersion();
    }
    return DLT_VER_INVALID;
}

const ClusterMap*
DataPlacement::getCurClustMap() const {
    return curClusterMap;
}

NodeUuidSet
DataPlacement::getRebalanceNodes() const {
    return rebalanceNodes;
}

int
DataPlacement::mod_init(SysParams const *const param) {
    Module::mod_init(param);

    FdsConfigAccessor conf_helper(g_fdsprocess->get_conf_helper());
    std::string algo_type_str = conf_helper.get<std::string>("placement_algo");
    curDltWidth = conf_helper.get<int>("token_factor");
    curDltDepth = conf_helper.get<int>("replica_factor");

    PlacementAlgorithm::AlgorithmTypes type =
            PlacementAlgorithm::AlgorithmTypes::ConsistHash;
    if (algo_type_str.compare("ConsistHash") == 0) {
        type = PlacementAlgorithm::AlgorithmTypes::ConsistHash;
    } else if (algo_type_str.compare("RoundRobin") == 0) {
        type = PlacementAlgorithm::AlgorithmTypes::RoundRobin;
    } else {
        LOGWARN << "DataPlacement: unknown placement algorithm type in "
                << "config file, will use Consistent Hashing algorithm";
    }

    setAlgorithm(type);

    curClusterMap = OM_Module::om_singleton()->om_clusmap_mod();

    numOfPrimarySMs = MODULEPROVIDER()->get_fds_config()->
            get<int>("fds.sm.number_of_primary");

    LOGNOTIFY << "DataPlacement: DLT width " << curDltWidth
              << ", dlt depth " << curDltDepth
              << ", algorithm " << algo_type_str
              << ", number of primary SMs " << numOfPrimarySMs;

    return 0;
}

void
DataPlacement::mod_startup() {
}

void
DataPlacement::mod_shutdown() {
}

Error
DataPlacement::validateDltOnDomainActivate(const NodeUuidSet& sm_services) {
    Error err(ERR_OK);

    if (commitedDlt) {
        err = commitedDlt->verify(sm_services);
        if (err.ok()) {
            LOGDEBUG << "DLT is valid!";
            // but we must not have any target DLT at this point
            // when we shutdown domain, we should have aborted any ongoing migrations
            if (newDlt) {
                LOGERROR << "Unexpected target DLT exists! We should have aborted "
                         << " DLT update during domain shutdown!";
                return ERR_NOT_IMPLEMENTED;
            }
        }
    } else {
        // there must be no SM services
        if (sm_services.size() > 0) {
            LOGERROR << "No DLT but " << sm_services.size() << " known SM services";
            return ERR_PERSIST_STATE_MISMATCH;
        }
    }

    return err;
}

Error DataPlacement::loadDltsFromConfigDB(const NodeUuidSet& sm_services,
                                          const NodeUuidSet& deployed_sm_services) {
    Error err(ERR_OK);

    // this function should be called only during init..
    fds_verify(commitedDlt == NULL);
    fds_verify(newDlt == NULL);

    // if there are no nodes, we are not going to load any dlts
    if (sm_services.size() == 0) {
        LOGNOTIFY << "Not loading DLTs from configDB since there are "
                  << "no persisted SMs";
        return err;
    }

    fds_uint64_t currentVersion = configDB->getDltVersionForType("current");
    if (currentVersion > 0) {
        DLT* dlt = new DLT(0, 0, 0, false);
        if (!configDB->getDlt(*dlt, currentVersion)) {
            LOGCRITICAL << "unable to load (current) dlt version "
                        <<"["<< currentVersion << "] from configDB";
            err = Error(ERR_PERSIST_STATE_MISMATCH);
        } else {
            // check if DLT is valid with respect to nodes
            // i.e. only contains node uuis that are in deployed_sm_services
            // set and does not contain any zeroes, etc.
            err = dlt->verify(deployed_sm_services);
            if (err.ok()) {
                LOGNOTIFY << "Current DLT in config DB is valid!";
                // we will set newDLT because we don't know yet if
                // the nodes in DLT will actually come up...
                newDlt = dlt;
            }
        }

        if (!err.ok()) {
            delete dlt;
            return err;
        }
    } else {
        // we got > 0 nodes from configDB but there is no DLT
        // going to throw away persistent state, because there is a mismatch
        LOGWARN << "No current DLT even though we persisted "
                << sm_services.size() << " nodes";
        return Error(ERR_PERSIST_STATE_MISMATCH);
    }

    // If we saved target DLT version, we were in the middle of migration
    // when domain shutdown or OM went down. We are going to restart migration
    // and re-compute the DLT again, so throw away this DLT
    // Maybe in the future, we can re-start migration with saved target DLT
    // but this is an optimization, may do later
    fds_uint64_t nextVersion = configDB->getDltVersionForType("next");
    if (nextVersion > 0 && nextVersion != currentVersion) {
        LOGNOTIFY << "OM went down in the middle of migration. Will throw away "
                  << "persisted  target DLT and re-compute it again if discovered "
                  << "SMs re-register";
        if (!configDB->setDltType(0, "next")) {
            LOGWARN << "unable to reset DLT target version in configDB";
        }

        setAbortParams(true, nextVersion);

    } else {
        if (0 == nextVersion) {
            LOGDEBUG << "There is only commited DLT in configDB (OK)";
        } else {
            LOGDEBUG << "Both current and target DLT are of same version (OK): "
                     << nextVersion;
        }
    }

    return err;
}

void DataPlacement::setConfigDB(kvstore::ConfigDB* configDB)
{
    this->configDB = configDB;
}


void DataPlacement::setAbortParams(bool abort, fds_int64_t version)
{
    sendMigAbortAfterRestart = abort;
    targetVersionForAbort    = version;

}

void DataPlacement::clearAbortParams()
{
    sendMigAbortAfterRestart = false;
    targetVersionForAbort    = 0;

    // We prevented this from being cleared out during om_load_state
    // so do it now
    if (!configDB->setDltType(0, "next")) {
        LOGWARN << "Failed to unset target DLT in config db";
    }
    newDlt = NULL;
}

bool DataPlacement::isAbortAfterRestartTrue()
{
    return sendMigAbortAfterRestart;
}

fds_uint64_t DataPlacement::getTargetVersionForAbort()
{
    return targetVersionForAbort;
}

}  // namespace fds
