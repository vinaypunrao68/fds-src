/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DltDmtUtil.h>

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

void DltDmtUtil::addToRemoveList(int64_t uuid, fpi::FDSP_MgrIdType type) {

    if (!isMarkedForRemoval(uuid))
    {
        removedNodes.push_back(std::make_pair(uuid, type));
    } else {
        LOGNOTIFY << "Svc:" << std::hex << uuid << std::dec
                  << " already in toRemove list";
    }
}

bool DltDmtUtil::isMarkedForRemoval(int64_t uuid) {
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

void DltDmtUtil::clearFromRemoveList(int64_t uuid)
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

int DltDmtUtil::getPendingNodeRemoves(fpi::FDSP_MgrIdType svc_type)
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

void DltDmtUtil::setSMAbortParams(bool abort, fds_uint64_t version)
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

fds_uint64_t DltDmtUtil::getSMTargetVersionForAbort()
{
    return dltTargetVersionForAbort;
}


void DltDmtUtil::setDMAbortParams(bool abort, fds_uint64_t version)
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

fds_uint64_t DltDmtUtil::getDMTargetVersionForAbort()
{
    return dmtTargetVersionForAbort;
}

} // namespace fds
