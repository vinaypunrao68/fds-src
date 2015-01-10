/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <limits>
#include <set>

#include <fds_assert.h>
#include <util/Log.h>

#include <MigrationUtility.h>

namespace fds {

MigrationSeqNumReceiver::MigrationSeqNumReceiver()
{
    seqNum = 0;
    lastSeqNum = std::numeric_limits<uint64_t>::max();
}

MigrationSeqNumReceiver::~MigrationSeqNumReceiver()
{
    fds_verify(seqNum == lastSeqNum);
    fds_verify(seqNumList.empty());
}

bool
MigrationSeqNumReceiver::setSeqNum(uint64_t latestSeqNum, bool isLastSeqNum)
{
    std::lock_guard<std::mutex> lock(seqNumLock);
    std::set<uint64_t>::iterator cit;

    if (isLastSeqNum) {
        lastSeqNum = latestSeqNum;
    }

    if (latestSeqNum == seqNum) {
        if (seqNum == lastSeqNum) {
            return true;
        }
        ++seqNum;
        while ((cit = seqNumList.begin()) != seqNumList.end()) {
            if (*cit == seqNum) {
                seqNumList.erase(cit);
                if (seqNum == lastSeqNum) {
                    return true;
                }
                ++seqNum;
            } else {
                break;
            }
        }

    } else {
        seqNumList.insert(latestSeqNum);
    }

    return false;
}

std::ostream& operator<< (std::ostream& out,
                          const MigrationSeqNumReceiver& seqNumRecv)
{
    out << "seqNum: " << seqNumRecv.seqNum << std::endl;
    out << "lastSeqNum: " << seqNumRecv.lastSeqNum << std::endl;

    std::cout << "set size: " << seqNumRecv.seqNumList.size() << std::endl;

    for (auto cit = seqNumRecv.seqNumList.begin();
         cit != seqNumRecv.seqNumList.end();
         ++cit) {
        out << "[" << *cit << "] ";
    }
    out << std::endl;

    return out;
}

/**
 * prints out histogram data to log.
 */
boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                            const MigrationSeqNumReceiver& seqNumRecv)
{
    out << "seqNum: " << seqNumRecv.seqNum << std::endl;
    out << "lastSeqNum: " << seqNumRecv.lastSeqNum << std::endl;

    for (auto cit = seqNumRecv.seqNumList.begin();
         cit != seqNumRecv.seqNumList.end();
         ++cit) {
        out << "[" << *cit << "] ";
    }
    out << std::endl;

    return out;
}


}  // namespace fds
