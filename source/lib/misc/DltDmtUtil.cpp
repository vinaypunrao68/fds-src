/*
 * Copyright 2014 Formation Data Systems, Inc.
 */



#include <DltDmtUtil.h>
/**
 *  Implementationg for DLT ...
 */
namespace fds {

DltDmtUtil* DltDmtUtil::m_instance = NULL;

DltDmtUtil::DltDmtUtil()
                      : sendSMMigAbortAfterRestart(false),
                        sendDMMigAbortAfterRestart(false),
                        dltTargetVersionForAbort(0),
                        dmtTargetVersionForAbort(0)
{

}
DltDmtUtil* DltDmtUtil::getInstance() {
    if (!m_instance) {
        m_instance = new DltDmtUtil();
    }
    return m_instance;
}

void DltDmtUtil::addToRemoveList(int64_t uuid) {
    removedNodes.push_back(uuid);
}

bool DltDmtUtil::isMarkedForRemoval(int64_t nodeUuid) {
    std::vector<fds_int64_t>::iterator iter;
    iter = std::find_if(removedNodes.begin(), removedNodes.end(),
                        [nodeUuid](fds_int64_t id)->bool
                        {
                        return nodeUuid == id;
                       });
    if (iter == removedNodes.end())
        return true;

    return false;
}

void DltDmtUtil::clearFromRemoveList(int64_t nodeUuid)
{
    std::vector<fds_int64_t>::iterator iter;
    iter = std::find_if(removedNodes.begin(), removedNodes.end(),
                        [nodeUuid](fds_int64_t id)->bool
                        {
                        return nodeUuid == id;
                       });
    if (iter != removedNodes.end()) {
        //LOGDEBUG << "Erased SM:" << std::hex << nodeUuid << std::dec
        //         << "from the remove list";
        removedNodes.erase(iter);
    } else {
        //LOGWARN << "Did not delete SM:" << std::hex
        //        << nodeUuid << std::dec << "from the dlt remove list";
    }
}

// double check the use of this
bool DltDmtUtil::isAnyRemovalPending(fds_int64_t& nodeUuid)
{
    if (removedNodes.size() != 0) {
        nodeUuid = removedNodes.back();
        return true;
    }

    return false;

}

void DltDmtUtil::setSMAbortParams(bool abort, fds_int64_t version)
{
    sendSMMigAbortAfterRestart = abort;
    dltTargetVersionForAbort   = version;

}

void DltDmtUtil::clearSMAbortParams()
{
    sendSMMigAbortAfterRestart = false;
    dltTargetVersionForAbort   = 0;
}

bool DltDmtUtil::isSMAbortAfterRestartTrue()
{
    return sendSMMigAbortAfterRestart;
}

int64_t DltDmtUtil::getSMTargetVersionForAbort()
{
    return dltTargetVersionForAbort;
}


void DltDmtUtil::setDMAbortParams(bool abort, fds_int64_t version)
{
    sendDMMigAbortAfterRestart = abort;
    dmtTargetVersionForAbort   = version;

}

void DltDmtUtil::clearDMAbortParams()
{
    sendDMMigAbortAfterRestart = false;
    dmtTargetVersionForAbort   = 0;
}

bool DltDmtUtil::isDMAbortAfterRestartTrue()
{
    return sendDMMigAbortAfterRestart;
}

int64_t DltDmtUtil::getDMTargetVersionForAbort()
{
    return dmtTargetVersionForAbort;
}

} // namespace fds
