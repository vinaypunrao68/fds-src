/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <limits>
#include <set>

#include <fds_assert.h>
#include <util/Log.h>

#include <MigrationUtility.h>

namespace fds {

MigrationSeqNum::MigrationSeqNum()
{
    curSeqNum = 0;
    lastSeqNum = std::numeric_limits<uint64_t>::max();
    seqComplete = false;
}

MigrationSeqNum::~MigrationSeqNum()
{
    if (std::numeric_limits<uint64_t>::max() != lastSeqNum) {
        fds_verify(curSeqNum == lastSeqNum);
    }
    fds_verify(seqNumList.empty());
}

bool
MigrationSeqNum::isSeqNumComplete()
{
    return seqComplete;
}

void
MigrationSeqNum::resetSeqNum()
{
    curSeqNum = 0;
    lastSeqNum = std::numeric_limits<uint64_t>::max();
    seqComplete = false;
    seqNumList.clear();
}

bool
MigrationSeqNum::setSeqNum(uint64_t latestSeqNum, bool isLastSeqNum)
{
    std::lock_guard<std::mutex> lock(seqNumLock);
    std::set<uint64_t>::iterator cit;

    if (isLastSeqNum) {
        lastSeqNum = latestSeqNum;
    }

    if (latestSeqNum == curSeqNum) {
        if (curSeqNum == lastSeqNum) {
            seqComplete = true;
            return true;
        }
        ++curSeqNum;
        while ((cit = seqNumList.begin()) != seqNumList.end()) {
            if (*cit == curSeqNum) {
                seqNumList.erase(cit);
                if (curSeqNum == lastSeqNum) {
                    seqComplete = true;
                    return true;
                }
                ++curSeqNum;
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
                          const MigrationSeqNum& seqNumRecv)
{
    out << "curSeqNum: " << seqNumRecv.curSeqNum << std::endl;
    out << "lastSeqNum: " << seqNumRecv.lastSeqNum << std::endl;

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
                                            const MigrationSeqNum& seqNumRecv)
{
    out << "curSeqNum: " << seqNumRecv.curSeqNum << std::endl;
    out << "lastSeqNum: " << seqNumRecv.lastSeqNum << std::endl;

    for (auto cit = seqNumRecv.seqNumList.begin();
         cit != seqNumRecv.seqNumList.end();
         ++cit) {
        out << "[" << *cit << "] ";
    }
    out << std::endl;

    return out;
}

MigrationDoubleSeqNum::MigrationDoubleSeqNum()
{
    curSeqNum1 = 0;
    lastSeqNum1 = std::numeric_limits<uint64_t>::max();
    completeSeqNum1 = false;
}

MigrationDoubleSeqNum::~MigrationDoubleSeqNum()
{
    if (std::numeric_limits<uint64_t>::max() != lastSeqNum1) {
        fds_verify(curSeqNum1 == lastSeqNum1);
    }
    fds_verify(mapSeqNum2.empty());
}

bool
MigrationDoubleSeqNum::isDoubleSeqNumComplete()
{
    return completeSeqNum1;
}
void
MigrationDoubleSeqNum::resetDoubleSeqNum()
{
    curSeqNum1 = 0;
    lastSeqNum1 = std::numeric_limits<uint64_t>::max();
    completeSeqNum1 = false;
}

bool
MigrationDoubleSeqNum::setDoubleSeqNum(uint64_t seqNum1, bool isLastSeqNum1,
                                       uint64_t seqNum2, bool isLastSeqNum2)
{
    std::lock_guard<std::mutex> lock(seqNumLock);

    // std::cout << "seq1(" << seqNum1 << "," << isLastSeqNum1 << ") ";
    // std::cout << "seq2(" << seqNum2 << "," << isLastSeqNum2 << ") ";

    if (isLastSeqNum1) {
        lastSeqNum1 = seqNum1;
    }

    /* We shouldn't see this.  This means that last set flag for the seqNum2
     * was specified more than once or are seeing seqNum1 after all
     * seqNum2 associated with it has completed.  This is likely issue with
     * programming.
     */
    fds_verify(seqNum1 >= curSeqNum1);

    if (nullptr == mapSeqNum2[seqNum1]) {
        mapSeqNum2[seqNum1] = MigrationSeqNum::ptr(new MigrationSeqNum());
    }

    fds_verify(nullptr != mapSeqNum2[seqNum1]);
    bool completeSeqNum2 = mapSeqNum2[seqNum1]->setSeqNum(seqNum2, isLastSeqNum2);
    // std::cout << "=> seqNum2 complete=" << completeSeqNum2;

    if ((seqNum1 == curSeqNum1) && mapSeqNum2[curSeqNum1]->isSeqNumComplete()) {
        if (seqNum1 == lastSeqNum1) {
            // std::cout << " " << __LINE__ << " Removing map[" << curSeqNum1 << "] ";
            mapSeqNum2.erase(curSeqNum1);
            completeSeqNum1 = true;
            // std::cout << std::endl;
            return true;
        }
        // std::cout << " " << __LINE__ << " Removing map[" << curSeqNum1 << "] ";
        mapSeqNum2.erase(curSeqNum1);
        ++curSeqNum1;

        // loop to check if there are out of sequence numbers.
        while ((nullptr != mapSeqNum2[curSeqNum1]) && mapSeqNum2[curSeqNum1]->isSeqNumComplete()) {
            // std::cout << " " << __LINE__ << " Removing map[" << curSeqNum1 << "] ";
            mapSeqNum2.erase(curSeqNum1);
            if (curSeqNum1 == lastSeqNum1) {
                // std::cout << " " << __LINE__ << "Last sequence" << std::endl;
                completeSeqNum1 = true;
                return true;
            }
            ++curSeqNum1;
        }
    }

    // std::cout << std::endl;

    return false;
}

std::ostream& operator<< (std::ostream& out,
                          const MigrationDoubleSeqNum& doubleSeqNum)
{
    out << "curSeqNum: " << doubleSeqNum.curSeqNum1 << std::endl;
    out << "lastSeqNum: " << doubleSeqNum.lastSeqNum1 << std::endl;
    out << "complete: " << doubleSeqNum.completeSeqNum1 << std::endl;

    for (auto cit = doubleSeqNum.mapSeqNum2.begin();
         cit != doubleSeqNum.mapSeqNum2.end();
         ++cit) {
        out << *cit->second;
    }
    out << std::endl;

    return out;
}

boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                            const MigrationDoubleSeqNum& doubleSeqNum)
{
    out << "curSeqNum: " << doubleSeqNum.curSeqNum1 << std::endl;
    out << "lastSeqNum: " << doubleSeqNum.lastSeqNum1 << std::endl;
    out << "complete: " << doubleSeqNum.completeSeqNum1 << std::endl;

    for (auto cit = doubleSeqNum.mapSeqNum2.begin();
         cit != doubleSeqNum.mapSeqNum2.end();
         ++cit) {
        out << *cit->second;
    }
    out << std::endl;

    return out;
}

}  // namespace fds
