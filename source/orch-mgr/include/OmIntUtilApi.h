/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMINTUTILAPI_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMINTUTILAPI_H_
#include <string>
#include <fds_types.h>
#include <kvstore/kvstore.h>
#include <kvstore/configdb.h>
#include <net/SvcMgr.h>
#include <OmExtUtilApi.h>

typedef std::vector<fpi::SvcInfo>::iterator SvcInfoIter;

namespace fds 
{
    namespace fpi = FDS_ProtocolInterface;

    /**
     * This is case insensitive
     */
    fds_uint64_t             getUuidFromVolumeName(const std::string& name);
    fds_uint64_t             getUuidFromResourceName(const std::string& name);

    fpi::FDSP_Node_Info_Type fromSvcInfo( const fpi::SvcInfo& svcinfo );
    fpi::FDSP_NodeState      fromServiceStatus(fpi::ServiceStatus svcStatus);

    void                     updateSvcInfoList(std::vector<fpi::SvcInfo>& svcInfos,
                                               bool smFlag, bool dmFlag, bool amFlag);

    SvcInfoIter              isServicePresent(std::vector<fpi::SvcInfo>& svcInfos,
                                              FDS_ProtocolInterface::FDSP_MgrIdType type);

    fpi::SvcInfo             getNewSvcInfo(FDS_ProtocolInterface::FDSP_MgrIdType type);

    void                     getServicesToStart(bool start_sm, bool start_dm,
                                                bool start_am,kvstore::ConfigDB* configDB,
                                                NodeUuid nodeUuid,
                                                std::vector<fpi::SvcInfo>& svcInfoList);

    void                     retrieveSvcId(int64_t pmID, fpi::SvcUuid& svcUuid,
                                           FDS_ProtocolInterface::FDSP_MgrIdType type);

    void                     populateAndRemoveSvc(fpi::SvcUuid serviceTypeId,
                                                  fpi::FDSP_MgrIdType type,
                                                  std::vector<fpi::SvcInfo> svcInfos,
                                                  kvstore::ConfigDB* configDB);
    template<class DataStoreT>
    inline void                     updateSvcMaps(DataStoreT*       configDB,
                                           SvcMgr*                  svcMgr,
                                           const fds_uint64_t       svc_uuid,
                                           const fpi::ServiceStatus svc_status,
                                           fpi::FDSP_MgrIdType      svcType,
                                           bool                     handlerUpdate = false,
                                           bool                     registering = false,
                                           fpi::SvcInfo             registeringSvcInfo = fpi::SvcInfo());

    bool                    dbRecordNeedsUpdate(fpi::SvcInfo svcLayerInfo, fpi::SvcInfo dbInfo);
    bool                    areRecordsSame(fpi::SvcInfo svcLayerInfo, fpi::SvcInfo dbInfo );
    void                    broadcast();





}  // namespace fds


/***********************************************************************************
 * Functions related to service state update handling
 ***********************************************************************************/

/*
     * Update the svcMap in both the configDB and svc layer. Because this is a
     * templatized function, the definition needs to be in the .h file to keep
     * the compiler happy
     *
     * @param configDB           pointer to OM's configDB object
     * @param svc_uuid           svc uuid of the service that is being updated
     * @param svc_status         desired status of the service
     * @param handlerUpdate      this update call is coming in through om-svc-handler
     *                           The implication being that a svcInfo was passed in
     * @param registration       true:  if the update is being called from the svc
     *                                    registration path
     *                           false: svc update is coming from any other code path
     * @param incomingSvcInfo    svcInfo of the svc that is either being registered,
     *                           or was received by the OM handler.
     *                           This info is guaranteed to be populated if the
     *                           handlerUpdate/registration flag is true. This svcInfo,
     *                           if valid, should not be modified in any way by this function
     *
    */

template<class DataStoreT>
void fds::updateSvcMaps( DataStoreT*              configDB,
                         SvcMgr*                  svcMgr,
                         const fds_uint64_t       svc_uuid,
                         const fpi::ServiceStatus svc_status,
                         fpi::FDSP_MgrIdType      svcType,
                         bool                     handlerUpdate,   // false by default
                         bool                     registration,   // false by default
                         fpi::SvcInfo             incomingSvcInfo // new, empty object by default
                  )
{
    fpi::SvcUuid    uuid;
    fpi::SvcInfoPtr svcLayerInfoPtr;
    fpi::SvcInfoPtr dbInfoPtr;

    uuid.svc_uuid = svc_uuid;

    bool ret = false;
    bool forceUpdate = false;

    if (handlerUpdate || registration)
    {
        fpi::SvcInfo    svcLayerInfo;
        fpi::SvcInfo    dbInfo;

        if (svcType == fpi::FDSP_CONSOLE)
        {
            // not going to care about checks etc related to this svc.
            // Simply update and return
            fpi::SvcInfoPtr incomingSvcInfoPtr = boost::make_shared<fpi::SvcInfo>(incomingSvcInfo);

            configDB->changeStateSvcMap( incomingSvcInfoPtr );
            svcMgr->updateSvcMap( {incomingSvcInfo} );
            return;
        }
        /*
         * ======================================================
         *  Setting up svcInfo objects from configDB and svcLayer
         * ======================================================
         * */


        LOGNORMAL << "Incoming svcInfo from either OM handler or registration, svc:"
                 << std::hex << incomingSvcInfo.svc_id.svc_uuid.svc_uuid << std::dec;

        bool validUpdate         = false;
        bool svcLayerRecordFound = false;
        bool dbRecordFound       = false;


        //if (incomingSvcInfo.incarnationNo == 0) // Can only be the case if a service is being added
        //{
        //    LOGDEBUG << "Updating zero incarnation number!";
        //    incomingSvcInfo.incarnationNo = util::getTimeStampSeconds();
        //}
        /*
         * ======================================================
         *  Retrieve svcLayer record
         * ======================================================
         * */
        ret = svcMgr->getSvcInfo(uuid, svcLayerInfo);
        if ( ret )
        {
            svcLayerRecordFound = true;
        } else {

            svcLayerInfo = incomingSvcInfo;
        }

        /*
         * ======================================================
         *  Retrieve DB record
         * ======================================================
         * */
        ret = configDB->getSvcInfo(svc_uuid, dbInfo);

        if (ret)
        {
            dbRecordFound = true;
            dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfo);
        } else {

            dbInfo    = incomingSvcInfo;
            dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfo);
        }


        // The only time both incarnationNos can be zero is when a service is getting added/started
        // in the cluster for the very first time. In these cases we will NOT
        // update the svcLayer svcMap because that can end up with broken pipes because of
        // incomplete svcInfos
        bool svcAddition = false;

        if ( (svcLayerInfo.incarnationNo == 0 && dbInfo.incarnationNo == 0) )
        {
            if ( !(svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED) )
            {
                LOGWARN << "No record for svc found in SvcLayer or ConfigDB!! Will not "
                        << "make any updates, returning..";
                return;

            } else {
                svcAddition = true;
            }
        } else if ((svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED)) {
            svcAddition = true;
        }

        LOGNORMAL << "Is this svcAddition for uuid:" << std::hex << svc_uuid << std::dec
                  << " ? " << svcAddition;

        /*
         * ========================================================
         *  Depending on which records were retrieved, take action
         *  incoming > current
         *  Implies: incoming either has a greater inc.# or
         *           incoming has same inc.#, diff & valid state
         *  incoming < current
         *  Implies: incoming had lower inc.# than current or
         *           incoming had state that was a bad transition
         *           from current
         * ========================================================
         * */

//+----------------+---------------------+--------------------------+------------------+---------------------------+
//| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
//|                |RecordFound|                          |                  |                                     |
//+----------------+-------------- -------+--------------------------+------------------+--------------------------+
//     True        |   False   |   incoming > current     |      Yes         | Update DB, svcLayer with incoming   |
//                 |           |   incoming < current     |      No          | Only update svcLayer with DB record |
//                 |           |   incoming = 0           |yes(if ADD/START) | Update DB alone with incoming, ONLY |
//                 |           |                          |else No           | for new rec. no update to 0 incNo   |
//+----------------+-----------+--------------------------+--------------------------------------------------------+
//     False       |   True    |   incoming > current     |      Yes         | Update svcLayer, DB with incoming   |
//                 |           |   incoming = current     | Diff states? Yes | Apply incoming to svcLyr, update db |
//                 |           |                          |   : No           | with svcLayer : No update           |
//                 |           |   incoming < current     |      No          | Only update DB with svcLayer record |
//                 |           |   incoming = 0           |      No          | Ignore incoming                     |
//                 |           |                          |                  |                                     |
//+----------------+-----------+--------------------------+--------------------------------------------------------+
//     False       |   False   |   only incoming is valid |      Yes         | Update DB and svcLayer(if not svcAdd|
//                 |           |                          |                  |  with incoming                      |
//+----------------+-----------+--------------------------+--------------------------------------------------------+
//     True        |   True    |   incoming > svcLayer    |      Yes         | Update svcLayer, DB with incoming   |
//                 |           |   svcLayer > configDB    |                  |                                     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming < svcLayer    |                  | Ignore incoming update              |
//                 |           |   svcLayer != configDB   |      No          |                                     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming < svcLayer    |      No          | svcLayer & DB have most current     |
//                 |           |   svcLayer == configDB   |                  | No updates are necessary            |
//                 |           |                          |                  |                                     |
//                 |           |   incoming > svcLayer    |incoming >        |                                     |
//                 |           |   svcLayer < configDB    |configDB? Yes : No| Update with incoming :              |
//                 |           |                          |                  |     ignore incoming                 |
//                             |                          |                  |                                     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming > svcLayer    |      Yes         | Incoming has the most current       |
//                 |           |   svcLayer == configDB   |                  |                                     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming == svcLayer   |                  |                                     |
//                 |           |   svcLayer == configDB   |   No             | Nothing to update                   |
//                 |           |                          |                  |                                     |
//                 |           |   incoming < svcLayer    |      No          | SvcLayer has most current, update DB|
//                 |           |   svcLayer > configDB    |                  | with svcLyr record, ignore incoming |
//                 |           |                          |                  |                                     |
//                 |           |   incoming = 0           |      No          | Ignore incoming (not a new svc)     |
//+----------------+-----------+--------------------------+------------------+-------------------------------------+


        if (!svcLayerRecordFound && !dbRecordFound)
        {

            if ( !svcAddition )
            {
                LOGNORMAL << "Case: No svcLayer or DB record found, updating both with incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;

                configDB->changeStateSvcMap( dbInfoPtr );
                svcMgr->updateSvcMap( {svcLayerInfo} );

            } else {
                LOGNORMAL << "Case: No svcLayer or DB record found, updating only DB with incoming";

                configDB->changeStateSvcMap( dbInfoPtr );
            }

        } else if ( !svcLayerRecordFound && dbRecordFound ) {

            // DB has a record, check if it is more current than incoming
            validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, dbInfo, "ConfigDB");

            if (validUpdate)
            {
                fpi::SvcInfoPtr incomingSvcInfoPtr = boost::make_shared<fpi::SvcInfo>(incomingSvcInfo);

                if ( !svcAddition )
                {
                    LOGNORMAL << "Case: No svcLayer record, found DB record, updating both with incoming"
                              << ", svc:" << std::hex << svc_uuid << std::dec;

                    configDB->changeStateSvcMap( incomingSvcInfoPtr );
                    svcMgr->updateSvcMap( {incomingSvcInfo} );
                } else {

                    LOGNORMAL << "Case: No svcLayer record, found DB record, updating DB with incoming"
                              << ", svc:" << std::hex << svc_uuid << std::dec;
                    configDB->changeStateSvcMap( incomingSvcInfoPtr );
                }

            } else {

                // If incoming is ACTIVE, and configDB is already ACTIVE make an exception and update

                if ( incomingSvcInfo.svc_status == fpi::SVC_STATUS_ACTIVE &&
                     dbInfo.svc_status == fpi::SVC_STATUS_ACTIVE )
                {
                    LOGNORMAL << "Case: Adding new svcLayer record for ACTIVE svc:"
                              << std::hex << svc_uuid << std::dec;
                    svcMgr->updateSvcMap( {incomingSvcInfo} );
                }
            }

        } else if ( svcLayerRecordFound && !dbRecordFound ) {

            // This should *never* be the case - log a warning, and handle it
            LOGWARN << "No DB record, but svcLayer record present for svc:"
                    << std::hex << svc_uuid << std::dec << " should NEVER be the case!";
            validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, svcLayerInfo, "SvcMgr");

            if (validUpdate)
            {
                LOGNORMAL << "Case: No DB record, found svcLayer record, updating both with incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                svcMgr->updateSvcMap( {incomingSvcInfo} );

            } else {
                LOGNORMAL << "Case: No DB record, found svcLayer record, updating DB with svcLayer"
                          << ", svc:" << std::hex << svc_uuid << std::dec;

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(svcLayerInfo) );
            }
        } else if ( svcLayerRecordFound && dbRecordFound ) {

            svcLayerInfoPtr = boost::make_shared<fpi::SvcInfo>(svcLayerInfo);
            dbInfoPtr       = boost::make_shared<fpi::SvcInfo>(dbInfo);

            validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, svcLayerInfo, "SvcMgr");

            // What we are trying to determine with the below two values is the relationship
            // between the svcLayer and DB record.
            // With the dbHasOlderRecord value, we can trust the return if true, ie dbRecord < svcLyrRecord.
            // However, if we get a false value, there are two possibilities:
            // 1. dbRecord > svcLayerRecord or 2. dbRecord == svcLayerRecord
            // The sameRecords computation helps us determine with certainty which case it is
            // and take action accordingly. Which is also why you will see sameRecords being checked
            // only when this dbHasOlderRecord value is false.
            bool dbHasOlderRecord = dbRecordNeedsUpdate(svcLayerInfo, dbInfo);
            bool sameRecords = areRecordsSame(svcLayerInfo, dbInfo);

            if ( validUpdate && dbHasOlderRecord )
            {
                // incoming > svcLayer
                // svcLayer > configDB

                LOGNORMAL << "Case: Both svcLayer & DB record found, updating both with incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                svcMgr->updateSvcMap( {incomingSvcInfo} );

            }  else if ( validUpdate && !dbHasOlderRecord && !sameRecords ) {
                // incoming > svcLayer
                // svcLayer != configDB

                // Need to establish now that incoming > configDB
                validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, dbInfo, "ConfigDB");

                if ( validUpdate )
                {
                    LOGNORMAL << "Case: Both svcLayer & DB record found, updating both with incoming"
                              << ", svc:" << std::hex << svc_uuid << std::dec;
                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                    svcMgr->updateSvcMap( {incomingSvcInfo} );

                } else {

                    LOGWARN << "Record mismatch! SvcLayer update is valid but configDB"
                              << " update is not, svc:" << std::hex << svc_uuid << std::dec
                              << " ignore incoming update";
                    forceUpdate = true;
                    svcMgr->updateSvcMap( {dbInfo}, forceUpdate);
                }

            } else if ( validUpdate && !dbHasOlderRecord && sameRecords ) {

                LOGNORMAL << "Case: Both svcLayer & DB record found, updating both to incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                // incoming > svcLayer
                // svcLayer == configDB

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                svcMgr->updateSvcMap( {incomingSvcInfo} );

            } else if ( !validUpdate && dbHasOlderRecord ) {

                LOGNORMAL << "Case: Both svcLayer & DB record found, updating DB with svcLayer record"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                // incoming < svcLayer
                // svcLayer > configDB

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(svcLayerInfo) );

            }  else if ( !validUpdate && !dbHasOlderRecord && sameRecords ) {

                LOGNORMAL << "Case: Both svcLayer & DB record found, incoming update not valid"
                          << "nothing to do, svc:" << std::hex << svc_uuid << std::dec;

                // incoming < svcLayer
                // svcLayer == configDB

                // Reuse the svcAddition flag simply to prevent the broadcasting of the map
                svcAddition = true;

            }   else if ( !validUpdate && !dbHasOlderRecord && !sameRecords ) {
                // incoming < svcLayer
                // svcLayer != configDB (could be states or incarnation is different)

                // Determine configDB > incoming
                validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, dbInfo, "ConfigDB");
                if (validUpdate) {
                    LOGWARN << "Record mismatch! ConfigDB update is valid but svcLayer"
                              << " update is not, svc:" << std::hex << svc_uuid << std::dec
                              << " force update";

                    forceUpdate = true;
                    // Force svcMgr to update to the db record
                    svcMgr->updateSvcMap( {dbInfo}, forceUpdate );

                    // Apply incoming update to both now
                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                    svcMgr->updateSvcMap( {incomingSvcInfo} );
                } else {
                    // Get svcLayer record in line with db, since records aren't the same
                    forceUpdate = true;
                    svcMgr->updateSvcMap( {dbInfo}, forceUpdate );
                }
            }
        }

        if ( !svcAddition )
        {
           broadcast();
        }

    } else {

        LOGNORMAL << "Update coming from a non-registration/handler path"
                  << ", svc:" << std::hex << svc_uuid << std::dec;

        // In this path it is fair to assume that the update is coming in
        // for exactly the same incarnationNo but the service status is potentially
        // different, so this path won't have incarnation checks like above
        // because all that is passed in to the function is desired service status

        /*
         * ==========================================
         *  Setting up svcInfo object from configDB
         * ==========================================
         * */

        bool ret                 = false;
        bool svcLayerRecordFound = false;
        bool dbRecordFound       = false;
        fpi::SvcInfo    svcLayerInfoUpdate;
        fpi::SvcInfo    dbInfoUpdate;
        fpi::SvcInfo    initialSvcLayerInfo;
        fpi::SvcInfo    initialDbInfo;
        fpi::SvcInfo    newRecord;

        // Do configDB related work first so the window
        // for svcLayer getting updated while we potentially
        // wait on locks is minimized

        ret = configDB->getSvcInfo(svc_uuid, initialDbInfo);
        if (ret)
        {
            dbRecordFound           = true;
            dbInfoUpdate            = initialDbInfo;
            dbInfoUpdate.svc_status = svc_status;

        } else {

            dbInfoUpdate.svc_id.svc_uuid.svc_uuid = svc_uuid;
            dbInfoUpdate.svc_status    = svc_status;
            dbInfoUpdate.svc_type      = svcType;
            dbInfoUpdate.incarnationNo = 0;
        }

        dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfoUpdate);
        /*
         * ==========================================
         *  Setting up svcInfo object from svcLayer
         * ==========================================
         * */

        ret = svcMgr->getSvcInfo(uuid, initialSvcLayerInfo);

        if (ret)
        {
            svcLayerRecordFound = true;
            svcLayerInfoUpdate = initialSvcLayerInfo;
            svcLayerInfoUpdate.svc_status = svc_status;

        } else {

            svcLayerInfoUpdate.svc_id.svc_uuid.svc_uuid = uuid.svc_uuid;
            svcLayerInfoUpdate.svc_status    = svc_status;
            svcLayerInfoUpdate.svc_type      = svcType;
            svcLayerInfoUpdate.incarnationNo = 0;
         }

        svcLayerInfoPtr = boost::make_shared<fpi::SvcInfo>(svcLayerInfoUpdate);

        // The only time both incarnationNos can be zero is when a service is getting added
        // into the cluster for the very first time. The ADDED, STARTED status is set prior to even
        // sending the start to PM, so the service processes aren't running yet.
        // Also, if the service was added, but now being removed, should be OK (0 incarnation expected)
        bool svcAddition = false;

        if ( (svcLayerInfoUpdate.incarnationNo == 0 && dbInfoUpdate.incarnationNo == 0) )
        {
            if ( !(svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED) )
            {
                if ( !(initialDbInfo.svc_status == fpi::SVC_STATUS_ADDED && svc_status == fpi::SVC_STATUS_REMOVED) )
                {
                    LOGWARN << "No record for svc found in SvcLayer or ConfigDB!! Will not "
                        << "make any updates, returning..";
                    return;
                } else {
                    // meaning an added service is being removed
                    // This bool isn't best phrasing of this scenario, but it does what is necessary
                    svcAddition = true;
                }
            } else {
                svcAddition = true;
            }
        } else if ((svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED)) {
            svcAddition = true;
        }

        LOGNORMAL << "Is this svcAddition for uuid:" << std::hex << svc_uuid << std::dec
                  << " ? " << svcAddition;

        /*
         * ======================================================
         *  (1) Determine which entity has the most current info
         *  using incarnation number
         *  (2) If DB has the more current, determine whether it
         *  needs an update at all
         * ======================================================
         * */

        bool dbHasOlderRecord = dbRecordNeedsUpdate(initialSvcLayerInfo, initialDbInfo);
        bool sameRecords = areRecordsSame(initialSvcLayerInfo, initialDbInfo);

        if ( dbHasOlderRecord )
        {
            LOGNORMAL << "ConfigDB has older record, updating it with svcLayer record(new state)"
                      << ", svc:" << std::hex << svc_uuid << std::dec;

            newRecord = *svcLayerInfoPtr;
            // If the flag is true implies that svcLayer has a more recent
            // record of the service in question, so first update the record
            // in configDB. First check if the transition from the state in svcLayer
            // is one that is allowed
            if ( OmExtUtilApi::isTransitionAllowed( svc_status,
                                                    initialSvcLayerInfo.svc_status,
                                                    true,
                                                    false,
                                                    false,
                                                    svcType) )
            {
                configDB->changeStateSvcMap( svcLayerInfoPtr );
            } else {

                LOGWARN << "Svc state transition check(SvcMgr) failed for svc:"
                        << std::hex << svc_uuid << std::dec << " update DB to svcLayer record"
                        << ", return without applying incoming update";

                // DB has older record ==> either incarnationNo is 0 or is lesser than
                // what svcLayer has, update DB to svcLayer record
                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(initialSvcLayerInfo) );

                return;
            }
        } else {

            // ConfigDB has latest record, go ahead and update it for the new svc status,
            // given that the statuses are different.
            // This record will be used to update svcLayer

            newRecord = *dbInfoPtr;
            if ( initialDbInfo.svc_status != fpi::SVC_STATUS_INVALID )
            {
                if ( OmExtUtilApi::isTransitionAllowed( svc_status,
                                                        initialDbInfo.svc_status,
                                                        true,
                                                        false,
                                                        false,
                                                        svcType) )
                {
                    configDB->changeStateSvcMap( dbInfoPtr );


                } else {
                    LOGWARN << "Svc state transition check(DB) failed, for Svc:"
                            << std::hex << svc_uuid << std::dec << ", return without update";

                    // State transition failed for incoming, but let's update svcLayer to DB
                    // anyway to keep consistent.
                    if (!sameRecords && !svcAddition) {
                        forceUpdate = true;
                        svcMgr->updateSvcMap( {initialDbInfo}, forceUpdate );
                    }
                    return;
                }
            } else {
                // any incoming state is allowed, so need to explicitly check
                configDB->changeStateSvcMap( dbInfoPtr );
            }
        }

        /*
         * ======================================================
         *  Determine whether a change has taken place for this
         *  svc in svc layer while we were doing above checks.
         *  Initiate new update if that is the case
         * ======================================================
         * */

        fpi::SvcInfo svcLayerNewerInfo;
        ret = svcMgr->getSvcInfo(uuid, svcLayerNewerInfo);

        if ( ret )
        {
            if ( initialSvcLayerInfo.incarnationNo != 0 &&
                 svcLayerNewerInfo.incarnationNo != 0 &&
                 svcLayerNewerInfo.svc_status != initialSvcLayerInfo.svc_status &&
                 svcLayerNewerInfo.incarnationNo > svcLayerInfoUpdate.incarnationNo )
            {
                LOGWARN   << "Svc:" << std::hex << uuid.svc_uuid << std::dec
                          << " has already changed in service layer from incoming [incarnation:"
                          << svcLayerInfoUpdate.incarnationNo << ", status:"
                          << OmExtUtilApi::printSvcStatus(svcLayerInfoUpdate.svc_status)
                          << "] to [incarnation:" << svcLayerNewerInfo.incarnationNo << ", status:"
                          << OmExtUtilApi::printSvcStatus(svcLayerNewerInfo.svc_status)
                          << "]. Initiating new update";

                // Something has already changed in the svc layer, current update is out-dated
                // Must take the first path to perform correct comparisons of incarnation numbers
                updateSvcMaps( configDB, MODULEPROVIDER()->getSvcMgr(),
                               uuid.svc_uuid, svcLayerNewerInfo.svc_status,
                               svcLayerNewerInfo.svc_type, true, false, svcLayerNewerInfo );

                return;
            }
        }

        /*
         * ====================================================================
         *  As consistent a view as we can get to, update and broadcast svc map
         * ====================================================================
         * */

        // If the service is in the process of being added/started, and we do not have a record of
        // it in the svcmap already (could be a service/node restart)do not update svcLayer
        if (svcAddition && !svcLayerRecordFound)
        {
            LOGNOTIFY << "No record of svc:" << std::hex << uuid.svc_uuid << std::dec
                      << " incoming status:" << OmExtUtilApi::printSvcStatus(svc_status)
                      << " in SvcMgr, will skip update!";

        } else {

            LOGNOTIFY << "Updating SvcMgr svcMap now for uuid:" << std::hex << uuid.svc_uuid  << std::dec;
            // If we got this far it implies the OM update was valid and successful
            // Verify that the update is going to go through
            if ( !OmExtUtilApi::isTransitionAllowed( svc_status,
                                                     initialSvcLayerInfo.svc_status,
                                                     true,
                                                     false,
                                                     false,
                                                     svcType) )
            {
                // Force update to configDB inital state
                bool forceUpdate = true;
                svcMgr->updateSvcMap( {initialDbInfo}, true );

                // Update to incoming
                svcMgr->updateSvcMap( {newRecord} );

            }

            svcMgr->updateSvcMap( {newRecord} );
        }
    }
}


#endif  // SOURCE_ORCH_MGR_INCLUDE_OMINTUTILAPI_H_
