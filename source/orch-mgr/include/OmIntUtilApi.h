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
    fpi::SvcInfo    svcLayerInfoUpdate;
    fpi::SvcInfo    dbInfoUpdate;
    fpi::SvcInfoPtr svcLayerInfoPtr;
    fpi::SvcInfoPtr dbInfoPtr;

    uuid.svc_uuid = svc_uuid;

    bool ret = false;

    if (handlerUpdate || registration)
    {
        /*
         * ======================================================
         *  Setting up svcInfo objects from configDB and svcLayer
         * ======================================================
         * */


        LOGNORMAL << "Incoming svcInfo from either OM handler or registration, svc:"
                 << std::hex << incomingSvcInfo.svc_id.svc_uuid.svc_uuid << std::dec;
        bool validUpdate = false;
        bool svcLayerRecordFound = false;
        bool dbRecordFound = false;


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
        ret = svcMgr->getSvcInfo(uuid, svcLayerInfoUpdate);
        if ( ret )
        {
            svcLayerRecordFound = true;
        } else {

            //LOGDEBUG << "Could not find svcInfo for uuid:"
            //          << std::hex << svc_uuid << std::dec
            //          << " in the svcMap";

            svcLayerInfoUpdate = incomingSvcInfo;
        }

        /*
         * ======================================================
         *  Retrieve DB record
         * ======================================================
         * */
        ret = configDB->getSvcInfo(svc_uuid, dbInfoUpdate);

        if (ret)
        {
            dbRecordFound = true;
            dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfoUpdate);
        } else {
            //LOGDEBUG << "Could not find SvcInfo for uuid:"
            //        << std::hex << svc_uuid << std::dec
            //        << " in the OM's configDB";

            dbInfoUpdate    = incomingSvcInfo;
            dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfoUpdate);
        }


        // The only time both incarnationNos can be zero is when a service is getting added
        // into the cluster for the very first time. The ADDED, STARTED status is set prior to even
        // sending the start to PM, so the service processes aren't running yet. In these cases
        // the svcInfo is not fully populated yet so if this condition is true we will NOT
        // update the svcLayer svcMap because that can end up with broken pipes because of
        // invalid port numbers. SvcLayer svcmap will get updated as always when the svc
        // registers.
        bool svcAddition = false;

        if ( (svcLayerInfoUpdate.incarnationNo == 0 && dbInfoUpdate.incarnationNo == 0) )
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
         *  Note: In the table below, >, <, == are used to indicate
         *  results after comparison of incarnation numbers
         * ========================================================
         * */

//+----------------+---------------------+--------------------------+------------------+---------------------------+
//| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
//|                |RecordFound|                          |                  |                                     |
//+----------------+-------------- -------+--------------------------+------------------+--------------------------+
//     True        |   False   |   incoming >= current    |      Yes         | Update DB, svcLayer with incoming   |
//                 |           |   incoming < current     |      No          | Only update svcLayer with DB record |
//                 |           |   incoming = 0           |      Yes         | Update DB, svcLayer with incoming   |
//                 |           |                          |                  | (after updating 0 incarnationNo)    |
//+----------------+-----------+--------------------------+--------------------------------------------------------+
//     False       |   True    |   incoming > current     |      Yes         | Update svcLayer, DB with incoming   |
//                 |           |   incoming = current     | Diff states? Yes | Apply incoming to svcLyr, update db |
//                 |           |                          |   : No           | with svcLayer : No update           |
//                 |           |   incoming < current     |      No          | Only update DB with svcLayer record |
//                 |           |   incoming = 0           |      Yes         | Update svcLayer, DB with incoming   |
//                 |           |                          |                  | (after updating 0 incarnationNo)    |
//+----------------+-----------+--------------------------+--------------------------------------------------------+
//     False       |   False   |   only incoming is valid |      Yes         | Check incoming incarnationNo, if    |
//                 |           |                          |                  | 0, update, then update DB & svcLayer|
//+----------------+-----------+--------------------------+--------------------------------------------------------+
//     True        |   True    |   incoming >= svcLayer   |      Yes         | Update svcLayer, DB with incoming   |
//                 |           |   svcLayer > configDB    |                  |                                     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming < svcLayer    |      No          | DB has most current, update svcLayer|
//                 |           |   svcLayer < configDB    |                  | with DB record, ignore incoming     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming < svcLayer    |      No          | svcLayer & DB have most current     |
//                 |           |   svcLayer == configDB   |                  | No updates are necessary            |
//                 |           |                          |                  |                                     |
//                 |           |   incoming >= svcLayer   |incoming >        |                                     |
//                 |           |   svcLayer < configDB    |configDB? Yes : No| Update with incoming : DB has most  |
//                 |           |                          |                  | current, update svcLayer with DB    |
//                                                                             record ignore incoming              |
//                 |           |                          |                  |                                     |
//                 |           |   incoming > svcLayer    |      Yes         | Incoming has the most current       |
//                 |           |   svcLayer == configDB   |                  |                                     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming == svcLayer   | Diff. states? Yes| Update with incoming :              |
//                 |           |   svcLayer == configDB   | : No             | Nothing to update                   |
//                 |           |                          |                  |                                     |
//                 |           |   incoming < svcLayer    |      No          | SvcLayer has most current, update DB|
//                 |           |   svcLayer > configDB    |                  | with svcLyr record, ignore incoming |
//                 |           |                          |                  |                                     |
//                 |           |   incoming = 0           |      Yes         | Update svcLayer, DB with incoming   |
//+----------------+-----------+--------------------------+------------------+-------------------------------------+


        if (!svcLayerRecordFound && !dbRecordFound)
        {
            // 1. Update DB to incoming
            // 2. Update svcLayer to incoming

            configDB->changeStateSvcMap( dbInfoPtr );

            if ( !svcAddition )
            {
                LOGNORMAL << "Case: No svcLayer or DB record found, updating both with incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                svcMgr->updateSvcMap( {svcLayerInfoUpdate} );
            } else {
                LOGNORMAL << "Case: No svcLayer or DB record found, updating only DB with incoming";
            }

        } else if ( !svcLayerRecordFound && dbRecordFound ) {

            // DB has a record, check if it is more current than incoming
            validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, dbInfoUpdate, "ConfigDB");

            if (validUpdate)
            {
                LOGNORMAL << "Case: No svcLayer record, found DB record, updating both with incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;

                // 1. Update DB to incoming
                // 2. Update svcLayer to incoming

                fpi::SvcInfoPtr incomingSvcInfoPtr = boost::make_shared<fpi::SvcInfo>(incomingSvcInfo);
                configDB->changeStateSvcMap( incomingSvcInfoPtr );

                if ( !svcAddition )
                {
                    svcMgr->updateSvcMap( {incomingSvcInfo} );
                }

            } else {

                // 1. No update to DB
                // 2. Update svcLayer to DB (ONLY if the current state in DB is not
                //    started or added). If incoming is ACTIVE, update svcLayer alone

                if ( !svcAddition &&
                     (dbInfoUpdate.svc_status != fpi::SVC_STATUS_STARTED ||
                      dbInfoUpdate.svc_status != fpi::SVC_STATUS_ADDED) )
                {
                    LOGNORMAL << "Case: No svcLayer record, found DB record, updating svcLayer with DB record"
                              << ", svc:" << std::hex << svc_uuid << std::dec;
                    svcMgr->updateSvcMap( {dbInfoUpdate} );
                } else {

                    if ( incomingSvcInfo.svc_status == fpi::SVC_STATUS_ACTIVE)
                    {
                        LOGNORMAL << "Case: No svcLayer record,updating svcLayer with incoming, "
                                  << " no update to DB, svc:" << std::hex << svc_uuid << std::dec;
                        svcMgr->updateSvcMap( {incomingSvcInfo} );
                    } else {
                    LOGWARN << "Case: No svcLayer record, incoming is NOT valid so no updates"
                              << ", svc:" << std::hex << svc_uuid << std::dec;
                    }
                }
            }

        } else if ( svcLayerRecordFound && !dbRecordFound ) {

            // Only svcLayer has a record, check if it is more current than incoming
            validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, svcLayerInfoUpdate, "SvcMgr");

            if (validUpdate)
            {
                LOGNORMAL << "Case: No DB record, found svcLayer record, updating both with incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                // 1. Update DB to incoming
                // 2. Update svcLayer to incoming

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                svcMgr->updateSvcMap( {incomingSvcInfo} );

            } else {
                LOGNORMAL << "Case: No DB record, found svcLayer record, updating DB with svcLayer"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                // svcLayer has most current info, update the DB since it has no record of this svc
                // 1. Update DB to svcLayer
                // 2. No update to svcLayer
                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(svcLayerInfoUpdate) );
            }
        } else if ( svcLayerRecordFound && dbRecordFound ) {

            svcLayerInfoPtr = boost::make_shared<fpi::SvcInfo>(svcLayerInfoUpdate);
            dbInfoPtr       = boost::make_shared<fpi::SvcInfo>(dbInfoUpdate);

            validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, svcLayerInfoUpdate, "SvcMgr");

            // What we are trying to determine with the below two values is the relationship
            // between the svcLayer and DB record.
            // With the dbHasOlderRecord value, we can trust the return if true, ie dbRecord < svcLyrRecord.
            // However, if we get a false value, there are two possibilities:
            // 1. dbRecord > svcLayerRecord or 2. dbRecord == svcLayerRecord
            // The sameRecords computation helps us determine with certainty which case it is
            // and take action accordingly. Which is also why you will see sameRecords being checked
            // only when this dbHasOlderRecord value is false.
            bool dbHasOlderRecord = dbRecordNeedsUpdate(svcLayerInfoUpdate, dbInfoUpdate);
            bool sameRecords = areRecordsSame(svcLayerInfoUpdate, dbInfoUpdate);

            if ( validUpdate && dbHasOlderRecord )
            {
                LOGNORMAL << "Case: Both svcLayer & DB record found, updating both with incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                // incoming >= svcLayer
                // svcLayer > configDB
                // 1. Update DB to incoming
                // 2. Update svcLayer to incoming

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                svcMgr->updateSvcMap( {incomingSvcInfo} );

            }  else if ( validUpdate && !dbHasOlderRecord && !sameRecords ) {
                // incoming >= svcLayer
                // svcLayer < configDB

                // Need to establish now that incoming > configDB
                validUpdate = OmExtUtilApi::isIncomingUpdateValid(incomingSvcInfo, dbInfoUpdate, "ConfigDB");

                if ( validUpdate )
                {
                    LOGNORMAL << "Case: Both svcLayer & DB record found, updating both with incoming"
                              << ", svc:" << std::hex << svc_uuid << std::dec;
                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                    svcMgr->updateSvcMap( {incomingSvcInfo} );

                } else {

                    // 1. No update to DB
                    // 2. Update svcLayer to DB

                    LOGNORMAL << "Case: Both svcLayer & DB record found, updating svcLayer with DB record"
                              << ", svc:" << std::hex << svc_uuid << std::dec;
                    svcMgr->updateSvcMap( {dbInfoUpdate} );
                }

            } else if ( validUpdate && !dbHasOlderRecord && sameRecords ) {

                LOGNORMAL << "Case: Both svcLayer & DB record found, updating both to incoming"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                // incoming >= svcLayer
                // svcLayer == configDB
                // 1. Update DB to incoming
                // 2. Update svcLayer to incoming

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                svcMgr->updateSvcMap( {incomingSvcInfo} );

            } else if ( !validUpdate && dbHasOlderRecord ) {

                LOGNORMAL << "Case: Both svcLayer & DB record found, updating DB with svcLayer record"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                // incoming < svcLayer
                // svcLayer > configDB
                // 1. Update DB to svcLayer
                // 2. No update to svcLayer

                configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(svcLayerInfoUpdate) );

            }  else if ( !validUpdate && !dbHasOlderRecord && sameRecords ) {

                LOGNORMAL << "Case: Both svcLayer & DB record found, no updates necessary"
                          << ", svc:" << std::hex << svc_uuid << std::dec;

                // Reuse the svcAddition flag simply to prevent the broadcasting of the map
                svcAddition = true;
                // incoming < svcLayer
                // svcLayer == configDB
                // 1. No update to DB
                // 2. No update to svcLayer

                //svcMgr->updateSvcMap( {dbInfoUpdate} );
            }   else if ( !validUpdate && !dbHasOlderRecord && !sameRecords ) {

                LOGNORMAL << "Case: Both svcLayer & DB record found, updating svcLayer with DB record"
                          << ", svc:" << std::hex << svc_uuid << std::dec;
                // incoming < svcLayer
                // svcLayer < configDB
                // 1. No update to DB
                // 2. Update svcLayer to DB

                svcMgr->updateSvcMap( {dbInfoUpdate} );
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

        // Do configDB related work first so the window
        // for svcLayer getting updated while we potentially
        // wait on locks is minimized

        ret = configDB->getSvcInfo(svc_uuid, dbInfoUpdate);

        fpi::ServiceStatus initialDbStatus = fpi::SVC_STATUS_INVALID;
        if (ret)
        {
            dbRecordFound           = true;
            initialDbStatus         = dbInfoUpdate.svc_status;
            dbInfoUpdate.svc_status = svc_status;

        } else {
            //LOGWARN << "Could not find SvcInfo for uuid:"
            //        << std::hex << svc_uuid << std::dec
            //        << " in the OM's configDB";

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


        fpi::SvcInfo initialSvcLayerInfo;
        ret = svcMgr->getSvcInfo(uuid, svcLayerInfoUpdate);

        if (ret)
        {
            svcLayerRecordFound = true;
            initialSvcLayerInfo = svcLayerInfoUpdate;

            // update the status to what we need to update to, since
            // this svcInfoPtr will be treated as "incoming" record
            svcLayerInfoUpdate.svc_status = svc_status;

        } else {
            //LOGWARN << "Could not find svcInfo for uuid:"
            //          << std::hex << svc_uuid << std::dec
            //          << " in the svcMap, generating new";

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
                if ( !(initialDbStatus == fpi::SVC_STATUS_ADDED && svc_status == fpi::SVC_STATUS_REMOVED) )
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

        bool dbHasOlderRecord = dbRecordNeedsUpdate(svcLayerInfoUpdate, dbInfoUpdate);
        bool sameRecords = areRecordsSame(svcLayerInfoUpdate, dbInfoUpdate);

        if ( dbHasOlderRecord )
        {
            LOGNORMAL << "ConfigDB has older record, updating it with svcLayer record"
                      << ", svc:" << std::hex << svc_uuid << std::dec;

            // If the flag is true implies that svcLayer has a more recent
            // record of the service in question, so first update the record
            // in configDB. First check if the transition from the state in svcLayer
            // is one that is allowed
            if ( OmExtUtilApi::isTransitionAllowed( svc_status,
                                                    svcLayerInfoUpdate.svc_status,
                                                    true,
                                                    false,
                                                    false,
                                                    svcType) )
            {
                configDB->changeStateSvcMap( svcLayerInfoPtr );

                // Update the dbInfoUpdate record that is being managed by the dbInfoPtr to the
                // latest (from the update above)
                // we will use this record now to update svcLayer
                ret = configDB->getSvcInfo(svc_uuid, dbInfoUpdate);

                if (!ret)
                {
                    LOGWARN << "Could not retrieve updated svc record from configDB"
                            << ", svc:" << std::hex << svc_uuid << std::dec
                            << " potentially making outdated updates ";

                } else {
                    dbInfoUpdate.svc_status = svc_status;
                }
            } else {

                LOGWARN << "Svc state transition check failed for svc:"
                        << std::hex << svc_uuid << std::dec << " will return without update";

                return;
            }
        } else {

            // ConfigDB has latest record, go ahead and update it for the new svc status,
            // given that the statuses are different.
            // This record will be used to update svcLayer

            if ( initialDbStatus != fpi::SVC_STATUS_INVALID )
            {
                if ( OmExtUtilApi::isTransitionAllowed( svc_status,
                                                        initialDbStatus,
                                                        true,
                                                        false,
                                                        false,
                                                        svcType) )
                {
                    configDB->changeStateSvcMap( dbInfoPtr );

                } else {
                    LOGWARN << "Svc state transition check failed, will return without update"
                            << ", svc:" << std::hex << svc_uuid << std::dec;

                    return;
                }
            } else {
                // any incoming state is allowed, so need to explicitly check
                configDB->changeStateSvcMap( dbInfoPtr );
            }
        }

        /*
         * ======================================================
         *  Determine whether:
         *  (1) a change has taken place for this
         *  svc in svc layer while we were doing above checks.
         *  Initiate new update if that is the case
         *  (2) whether svcLayer svcMap requires an update at all
         * ======================================================
         * */

        fpi::SvcInfo svcLayerNewerInfo;
        ret = svcMgr->getSvcInfo(uuid, svcLayerNewerInfo);

        if ( ret )
        {
            // If we had a valid initialSvcLayerInfo object &&
            // we retrieved a valid record &&
            // The service status in svcLayer has changed from when we began this update &&
            // newer info has a bigger incarnationNo than incoming update
            if ( initialSvcLayerInfo.incarnationNo != 0 &&
                 svcLayerNewerInfo.incarnationNo != 0 &&
                 svcLayerNewerInfo.svc_status != initialSvcLayerInfo.svc_status &&
                 svcLayerNewerInfo.incarnationNo > svcLayerInfoUpdate.incarnationNo )
            {
                LOGNOTIFY << "Svc:" << std::hex << uuid.svc_uuid << std::dec
                          << " has already changed in service layer from incoming [incarnation:"
                          << svcLayerInfoUpdate.incarnationNo << ", status:"
                          << OmExtUtilApi::printSvcStatus(svcLayerInfoUpdate.svc_status)
                          << "] to [incarnation:" << svcLayerNewerInfo.incarnationNo << ", status:"
                          << OmExtUtilApi::printSvcStatus(svcLayerNewerInfo.svc_status)
                          << "]. Initiating new update";

                // Something has already changed in the svc layer, so the current update of
                // status we are proceeding with is already out-dated
                // Call a new update of configDB, and return from the current cycle

                // If the incarnationNo has changed, must take the first path to perform
                // correct comparisons, set handler flag to true
                updateSvcMaps( configDB, MODULEPROVIDER()->getSvcMgr(),
                               uuid.svc_uuid, svcLayerNewerInfo.svc_status,
                               svcLayerNewerInfo.svc_type, true, false, svcLayerNewerInfo );

                return;
            }

            // If svcLayer already had the latest status &&
            // the status has not changed since &&
            // the incarnation number is the same as the DB, we can afford not to do an update and
            // re-broadcast
            if ( (initialSvcLayerInfo.svc_status == svc_status) &&
                 (initialSvcLayerInfo.svc_status == svcLayerNewerInfo.svc_status) &&
                 (svcLayerNewerInfo.incarnationNo == dbInfoPtr->incarnationNo) )
            {
                LOGNOTIFY << "SvcLayer already has the latest [incarnation:"
                          << svcLayerNewerInfo.incarnationNo << ", status:"
                          << OmExtUtilApi::printSvcStatus(svcLayerNewerInfo.svc_status)
                          << "] for service:" << std::hex << uuid.svc_uuid << std::dec
                          << " , no need to update & broadcast";

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

            // The only reason why an update to svcLayer svcMap will be rejected is if
            // in between us updating configDB, svcLayer received an update about this
            // same service and now has a newer incarnation number
            // In this case, we are still OK. A newer incarnation number implies a newer
            // svc process. So OM will definitely receive registration of the svc.
            // The associated update to configDB using this same method should pick up
            // the latest record svcLayer has and perform updates for both
            // (which will be applied under the condition incarnationNo is equal, but status is different)
            // Assumption is that EVERY change to service state of configDB comes through here
            svcMgr->updateSvcMap( {*dbInfoPtr} );
        }
    }
}


#endif  // SOURCE_ORCH_MGR_INCLUDE_OMINTUTILAPI_H_
