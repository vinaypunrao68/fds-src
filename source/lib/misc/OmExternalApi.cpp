/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <OmExternalApi.h>
#include <orch-mgr/include/omutils.h>

namespace fds
{

OmExternalApi* OmExternalApi::m_instance = NULL;

OmExternalApi::OmExternalApi()
                      : sendSMMigAbortAfterRestart(false),
                        sendDMMigAbortAfterRestart(false),
                        dltTargetVersionForAbort(0),
                        dmtTargetVersionForAbort(0)
{

}

OmExternalApi* OmExternalApi::getInstance()
{
    if (!m_instance) {
        m_instance = new OmExternalApi();
    }
    return m_instance;
}

/******************************************************************************
 *       Functions related to node remove interruption (SM,DM)
 *****************************************************************************/

void OmExternalApi::addToRemoveList( int64_t uuid,
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

bool OmExternalApi::isMarkedForRemoval( int64_t uuid )
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

void OmExternalApi::clearFromRemoveList( int64_t uuid )
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

int OmExternalApi::getPendingNodeRemoves( fpi::FDSP_MgrIdType svc_type )
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

void OmExternalApi::setSMAbortParams( bool abort,
                                  fds_uint64_t version )
{
    sendSMMigAbortAfterRestart = abort;
    dltTargetVersionForAbort   = version;

}

void OmExternalApi::clearSMAbortParams()
{
    sendSMMigAbortAfterRestart = false;
    dltTargetVersionForAbort   = 0;
}

bool OmExternalApi::isSMAbortAfterRestartTrue()
{
    return sendSMMigAbortAfterRestart;
}

fds_uint64_t OmExternalApi::getSMTargetVersionForAbort()
{
    return dltTargetVersionForAbort;
}


void OmExternalApi::setDMAbortParams( bool abort,
                                  fds_uint64_t version )
{
    sendDMMigAbortAfterRestart = abort;
    dmtTargetVersionForAbort   = version;

}

void OmExternalApi::clearDMAbortParams()
{
    sendDMMigAbortAfterRestart = false;
    dmtTargetVersionForAbort   = 0;
}

bool OmExternalApi::isDMAbortAfterRestartTrue()
{
    return sendDMMigAbortAfterRestart;
}

fds_uint64_t OmExternalApi::getDMTargetVersionForAbort()
{
    return dmtTargetVersionForAbort;
}



/******************************************************************************
 *                   Service status related functions
 *****************************************************************************/

std::string OmExternalApi::printSvcStatus( fpi::ServiceStatus svcStatus )
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
            statusString = "STOPPED";
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
 * is a valid one (following the table allowedStateTransitions
 */
bool OmExternalApi::isTransitionAllowed( fpi::ServiceStatus incoming,
                                         fpi::ServiceStatus current )
{

    // The only situation when this would be, if incarnationNo is newer
    // or was 0 (and now updated), in which case we want to allow
    // the update so that the incarnationNo gets updated
    if (incoming == current)
        return true;

    bool allowed = false;
    std::vector<fpi::ServiceStatus> allowedForThisIncoming = allowedStateTransitions[incoming];

    // If the current state is one of the states allowed for a transition
    // to the incoming state, then return true, otherwise return false
    for (auto s : allowedForThisIncoming)
    {
        if ( s == current)
        {
            allowed = true;
            break;
        }
    }

    if ( !allowed )
    {
        LOGWARN << "!!Will not allow transition from state:" << OmExternalApi::getInstance()->printSvcStatus(current)
                << " to state:" << OmExternalApi::getInstance()->printSvcStatus(incoming) << " !!";
    }

    return allowed;

}


/*
 * This function verifies that an incoming update to svcMaps is valid
 * taking into account (1) incarnationNo (2) valid state transition
 */
bool OmExternalApi::isIncomingUpdateValid( fpi::SvcInfo incomingSvcInfo,
                                           fpi::SvcInfo currentInfo )
{
    bool ret = false;

    LOGNOTIFY << "!!Uuid:" << std::hex << currentInfo.svc_id.svc_uuid.svc_uuid
              << " Incoming [incarnationNo:" << incomingSvcInfo.incarnationNo
              << ", status:" << printSvcStatus(incomingSvcInfo.svc_status)
              << "] VS Current [incarnationNo:" << currentInfo.incarnationNo
              << ", status:" << printSvcStatus(currentInfo.svc_status) << "]";

    if ( incomingSvcInfo.incarnationNo < currentInfo.incarnationNo)
    {
        LOGWARN << "!!Incoming update for svc:"
                //<< fds::logString(incomingSvcInfo)
                << " appears older than what is in svc map, will ignore";
        ret = false;


    } else if ( incomingSvcInfo.incarnationNo == currentInfo.incarnationNo &&
                incomingSvcInfo.svc_status != currentInfo.svc_status ) {
        LOGNOTIFY << "!!Same incarnation number, different status proceeding with update";
        ret = true;

    } else if (incomingSvcInfo.incarnationNo > currentInfo.incarnationNo) {
        LOGNOTIFY << "!!Incoming update has newer incarnationNo";
        ret = true;

    } else if (incomingSvcInfo.incarnationNo == 0) {

        LOGWARN << "!!Zero incarnation number, allow the update nevertheless";
        incomingSvcInfo.incarnationNo = util::getTimeStampSeconds();
        ret = true;
    }

    if (ret)
    {
        // If all the incarnation number checks pass, but the allowed state transition
        // does not pass, set the return back to false
        if ( !isTransitionAllowed(incomingSvcInfo.svc_status, currentInfo.svc_status) )
        {
            ret = false;
        }
    }

    LOGNOTIFY << "isIncomingUpdateValid ? " << ret;
    return ret;
}




} // namespace fds
