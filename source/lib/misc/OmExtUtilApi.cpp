/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <OmExtUtilApi.h>

namespace fds
{

OmExtUtilApi* OmExtUtilApi::m_instance = NULL;


//+--------------------------+---------------------------------------------------+
//|     Current State        |    Valid IncomingState                            |
//+--------------------------+---------------------------------------------------+
//      ACTIVE               |  STANDBY, STOPPING, INACTIVE_FAILED, STARTED
//      INACTIVE_STOPPED     |  ACTIVE, STARTED, REMOVED, DISCOVERED(for PM only)
//      DISCOVERED           |  ACTIVE
//      STANDBY              |  ACTIVE
//      ADDED                |  STARTED, REMOVED
//      STARTED              |  ACTIVE, STOPPING, INACTIVE_FAILED
//      STOPPING             |  INACTIVE_STOPPED, STARTED, REMOVED
//      REMOVED              |  - (non-PM svcs transition to this state post which
//                           |     the svc is deleted from the DB, so no incoming
//                           |     state is valid) But for PMs discovered is valid
//                           |  DISCOVERED
//      INACTIVE_FAILED      |  ACTIVE, STOPPING, STARTED
//+--------------------------+----------------------------------------------------+

const std::vector<std::vector<fpi::ServiceStatus>> OmExtUtilApi::allowedStateTransitions =
{
        // valid incoming for state: INVALID(0)
        { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_INACTIVE_STOPPED,
          fpi::SVC_STATUS_DISCOVERED, fpi::SVC_STATUS_STANDBY, fpi::SVC_STATUS_ADDED,
          fpi::SVC_STATUS_STARTED, fpi::SVC_STATUS_STOPPING,
          fpi::SVC_STATUS_REMOVED, fpi::SVC_STATUS_INACTIVE_FAILED },
        // valid incoming for state: ACTIVE (1)
        { fpi::SVC_STATUS_STANDBY, fpi::SVC_STATUS_STOPPING,
          fpi::SVC_STATUS_INACTIVE_FAILED, fpi::SVC_STATUS_STARTED },
        // valid incoming for state: INACTIVE_STOPPED(2)
        { fpi::SVC_STATUS_ACTIVE,fpi::SVC_STATUS_STARTED, fpi::SVC_STATUS_REMOVED,
          fpi::SVC_STATUS_DISCOVERED /* This state for PM ONLY */ },
        // valid incoming for state: DISCOVERED(3)
        { fpi::SVC_STATUS_ACTIVE },
        // valid incoming for state: STANDBY(4)
        { fpi::SVC_STATUS_ACTIVE },
        // valid incoming for state: ADDED(5)
        { fpi::SVC_STATUS_STARTED, fpi::SVC_STATUS_REMOVED },
        // valid incoming for state: STARTED(6)
        { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_STOPPING,
          fpi::SVC_STATUS_INACTIVE_FAILED },
        // valid incoming for state: STOPPING(7)
        { fpi::SVC_STATUS_INACTIVE_STOPPED,fpi::SVC_STATUS_STARTED,
          fpi::SVC_STATUS_REMOVED, fpi::SVC_STATUS_DISCOVERED /*this ONLY for PM */ },
        // valid incoming for state: REMOVED(8)
        { fpi::SVC_STATUS_DISCOVERED },
        // valid incoming for state: INACTIVE_FAILED(9)
        { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_STOPPING, fpi::SVC_STATUS_STARTED }
};

OmExtUtilApi::OmExtUtilApi()
                      : sendSMMigAbortAfterRestart(false),
                        sendDMMigAbortAfterRestart(false),
                        dltTargetVersionForAbort(0),
                        dmtTargetVersionForAbort(0)
{

}

OmExtUtilApi* OmExtUtilApi::getInstance()
{
    if (!m_instance) {
        m_instance = new OmExtUtilApi();
    }
    return m_instance;
}

/******************************************************************************
 *       Functions related to node remove interruption (SM,DM)
 *****************************************************************************/

void OmExtUtilApi::addToRemoveList( int64_t uuid,
                                 fpi::FDSP_MgrIdType type )
{
    if (!isMarkedForRemoval(uuid))
    {
        removedNodes.push_back(std::make_pair(uuid, type));
    } else {
        LOGNOTIFY << "Svc:" << std::hex << uuid << std::dec
                  << " already in toRemove list";
    }
}

bool OmExtUtilApi::isMarkedForRemoval( int64_t uuid )
{
    bool found = false;
    for (const std::pair<int64_t, fpi::FDSP_MgrIdType> elem : removedNodes)
    {
        if (elem.first == uuid)
        {
            found = true;
            break;
        }
    }

    if (found)
        return true;

    return false;
}

void OmExtUtilApi::clearFromRemoveList( int64_t uuid )
{
    std::vector<std::pair<int64_t, fpi::FDSP_MgrIdType>>::iterator iter;
    iter = std::find_if(removedNodes.begin(), removedNodes.end(),
                        [uuid](std::pair<int64_t,fpi::FDSP_MgrIdType> mem)->bool
                        {
                        return uuid == mem.first;
                       });

    if (iter != removedNodes.end()) {
        LOGDEBUG << "Erasing SM/DM:" << std::hex << uuid << std::dec
                 << " from the remove list";
        removedNodes.erase(iter);
    }
}

int OmExtUtilApi::getPendingNodeRemoves( fpi::FDSP_MgrIdType svc_type )
{
    int size = 0;
    for (auto item : removedNodes)
    {
        if (item.second == svc_type) {
            ++size;
        }
    }
    return size;
}


/***********************************************************************************
 * Functions to help recover from interruption of DLT/DMT propagation by OM restart
 ***********************************************************************************/

void OmExtUtilApi::setSMAbortParams( bool abort,
                                     fds_uint64_t version )
{
    sendSMMigAbortAfterRestart = abort;
    dltTargetVersionForAbort   = version;

}

void OmExtUtilApi::clearSMAbortParams()
{
    sendSMMigAbortAfterRestart = false;
    dltTargetVersionForAbort   = 0;
}

bool OmExtUtilApi::isSMAbortAfterRestartTrue()
{
    return sendSMMigAbortAfterRestart;
}

fds_uint64_t OmExtUtilApi::getSMTargetVersionForAbort()
{
    return dltTargetVersionForAbort;
}


void OmExtUtilApi::setDMAbortParams( bool abort,
                                  fds_uint64_t version )
{
    sendDMMigAbortAfterRestart = abort;
    dmtTargetVersionForAbort   = version;

}

void OmExtUtilApi::clearDMAbortParams()
{
    sendDMMigAbortAfterRestart = false;
    dmtTargetVersionForAbort   = 0;
}

bool OmExtUtilApi::isDMAbortAfterRestartTrue()
{
    return sendDMMigAbortAfterRestart;
}

fds_uint64_t OmExtUtilApi::getDMTargetVersionForAbort()
{
    return dmtTargetVersionForAbort;
}



/******************************************************************************
 *                   Service status related functions
 *****************************************************************************/

std::string OmExtUtilApi::printSvcStatus( fpi::ServiceStatus svcStatus )
{
    int32_t status = svcStatus;
    std::string statusString;

    switch ( status )
    {
        case 0:
            statusString = "INVALID";
            break;
        case 1:
            statusString = "ACTIVE";
            break;
        case 2:
            statusString = "INACTIVE_STOPPED";
            break;
        case 3:
            statusString = "DISCOVERED";
            break;
        case 4:
            statusString = "STANDBY";
            break;
        case 5:
            statusString = "ADDED";
            break;
        case 6:
            statusString = "STARTED";
            break;
        case 7:
            statusString = "STOPPING";
            break;
        case 8:
            statusString = "REMOVED";
            break;
        case 9:
            statusString = "INACTIVE_FAILED";
            break;
        default:
            statusString = "Invalid entry";
            break;
    }

    return statusString;
}


/*
 * This function ensures that an incoming transition from a given state to another
 * is a valid one (following the table allowedStateTransitions)
 *
 * This function should not get called if/when a service is being added for the first
 * time to the domain
 */
bool OmExtUtilApi::isTransitionAllowed( fpi::ServiceStatus incoming,
                                        fpi::ServiceStatus current,
                                        bool sameIncNo,
                                        bool greaterIncNo,
                                        bool zeroIncNo,
                                        fpi::FDSP_MgrIdType type,
                                        bool pingUpdate )
{

    // The only situation when this would be, if incarnationNo is newer
    // or was 0 (and now updated), in which case we want to allow
    // the update so that the incarnationNo gets updated
    if ( incoming == current )
    {
        if ( zeroIncNo || greaterIncNo ) {
            return true;
        } else {
            LOGDEBUG << "Current state:"
                     << OmExtUtilApi::getInstance()->printSvcStatus(current)
                     << " is the same as incoming, same incarnation, nothing to do";
            return false;
        }
    }

    // special case: INACTIVE_FAILED -> ACTIVE should be allowed ONLY if the
    // incarnation # is newer
    if ( incoming == fpi::SVC_STATUS_ACTIVE && current == fpi::SVC_STATUS_INACTIVE_FAILED)
    {
        if (greaterIncNo) {
            return true;
        } else if ( pingUpdate ) {
            LOGNOTIFY << "Svc is being marked ACTIVE after successful ping, will allow";
            return true;
        } else if (type == fpi::FDSP_ORCH_MGR) {
            LOGWARN << "Warning: Performing update FAILED to ACTIVE for OM svc for"
                    << " *same* incarnation number, should never happen!!";
            return true;

        } else {
            LOGDEBUG << "Will not allow transition from state:"
                    << OmExtUtilApi::getInstance()->printSvcStatus(current)
                    << " to state:" << OmExtUtilApi::getInstance()->printSvcStatus(incoming)
                    << " for the same incarnation number!!";
            return false;
        }
    }


    bool allowed = false;
    std::vector<fpi::ServiceStatus> allowedStates = OmExtUtilApi::allowedStateTransitions[current];

    // If the current state is one of the states allowed for a transition
    // to the incoming state, then return true, otherwise return false
    for (auto s : allowedStates)
    {
        if ( s == incoming)
        {
            allowed = true;
            break;
        }
    }

    if ( !allowed )
    {
        LOGWARN << "Will not allow transition from state:"
                << OmExtUtilApi::getInstance()->printSvcStatus(current)
                << " to state:" << OmExtUtilApi::getInstance()->printSvcStatus(incoming) << " !!";
    }

    return allowed;

}


/*
 * This function verifies that an incoming update to svcMaps is valid
 * taking into account (1) incarnationNo (2) valid state transition
 */
bool OmExtUtilApi::isIncomingUpdateValid( fpi::SvcInfo& incomingSvcInfo,
                                          fpi::SvcInfo currentInfo,
                                          std::string source,
                                          bool pingUpdate )
{
    bool ret          = false;
    bool sameIncNo    = false;
    bool greaterIncNo = false;
    bool zeroIncNo    = false;

    LOGNOTIFY << "Uuid:" << std::hex << currentInfo.svc_id.svc_uuid.svc_uuid << std::dec
              << " Incoming [incarnationNo:" << incomingSvcInfo.incarnationNo
              << ", status:" << OmExtUtilApi::printSvcStatus(incomingSvcInfo.svc_status)
              << "] VS " << source << " Current [incarnationNo:" << currentInfo.incarnationNo
              << ", status:" << OmExtUtilApi::printSvcStatus(currentInfo.svc_status) << "]";

    if ( incomingSvcInfo.incarnationNo < currentInfo.incarnationNo)
    {
        LOGDEBUG << "Incoming update for svc:"
                << " appears older than what is in svc map, will ignore";
        ret = false;


    } else if ( incomingSvcInfo.incarnationNo == currentInfo.incarnationNo &&
                incomingSvcInfo.svc_status != currentInfo.svc_status ) {
        sameIncNo = true;
        ret       = true;

    } else if (incomingSvcInfo.incarnationNo > currentInfo.incarnationNo) {
        greaterIncNo = true;
        ret          = true;

    } else if (incomingSvcInfo.incarnationNo == 0) {
        //incomingSvcInfo.incarnationNo = util::getTimeStampSeconds();

        // This really should ONLY be the case if incoming status is STARTED or ADDED.
        // (given a STARTED service successfully transitioned to ACTIVE at some point)
        // if not, let's allow it but put a warning, best we can do considering all the
        // flakiness in behavior (and unpredictable user actions)
        if ( incomingSvcInfo.svc_status != fpi::SVC_STATUS_ADDED ||
            incomingSvcInfo.svc_status != fpi::SVC_STATUS_STARTED ) {

            LOGWARN << "Update for svc:" << std::hex << incomingSvcInfo.svc_id.svc_uuid.svc_uuid << std::dec
                    << " coming in with ZERO incarnation for status:"
                    << OmExtUtilApi::printSvcStatus(incomingSvcInfo.svc_status);
        }


        zeroIncNo = true;
        ret       = true;
    }

    if (ret)
    {
        // If all the incarnation number checks pass, but the allowed state transition
        // does not pass, set the return back to false
        if ( !OmExtUtilApi::isTransitionAllowed(incomingSvcInfo.svc_status,
                                                currentInfo.svc_status,
                                                sameIncNo,
                                                greaterIncNo,
                                                zeroIncNo,
                                                incomingSvcInfo.svc_type,
                                                pingUpdate) )
        {
            ret = false;
        }
    }

    //LOGNOTIFY << "IsIncomingUpdateValid for uuid:"
    //          << std::hex << currentInfo.svc_id.svc_uuid.svc_uuid
    //          << std::dec << " ? " << ret
    //           << " SameIncarnationNo? " << sameIncNo
    //          << " GreaterIncomingIncNo? " << greaterIncNo
    //          << " ZeroIncarnationNo? " << zeroIncNo;
    return ret;
}

std::vector<std::vector<fpi::ServiceStatus>> OmExtUtilApi::getAllowedTransitions()
{
    return OmExtUtilApi::allowedStateTransitions;
}

} // namespace fds
