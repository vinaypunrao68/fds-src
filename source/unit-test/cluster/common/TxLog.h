/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef _TXLOG_H_
#define _TXLOG_H_

#include <list>
#include <memory>

namespace fds {

template<class EntryT>
struct TxLog {
    using Iterator = typename std::list<SHPTR<EntryT>>::iterator;
    TxLog()
    {
        init(0, 0);
    }
    TxLog(uint32_t maxEntries, int64_t lastCommitId) 
    {
        init(maxEntries, lastCommitId);
    }
    void init(uint32_t maxEntries, int64_t lastCommitId)
    {
        maxEntries_ = maxEntries;
        lastCommitId_ = lastCommitId;
    }
    Error append(int64_t commitId, const SHPTR<EntryT> &entry)
    {
        fds_assert(commitId == lastCommitId_ + 1);
        fds_assert(entries_.size() <= maxEntries_);

        entries_.push_back(entry);
        lastCommitId_++;
        if (entries_.size() > maxEntries_) {
            entries_.pop_front();
        }
        return ERR_OK;
    }
    Iterator iterator(int64_t commitId)
    {
        auto dist = distFromStart(commitId);
        if (dist < 0 || dist >= entries_.size()) {
            return entries_.end();
        }
        return std::next(entries_.begin(), dist);
    }
    SHPTR<EntryT> get(const Iterator &itr)
    {
        return *itr;
    }
    bool isValid(const Iterator &itr)
    {
        return itr != entries_.end();
    }
    int32_t distFromStart(int64_t commitId)
    {
        return entries_.size() - (lastCommitId_ - commitId - 1);
    }

 protected:
    uint32_t                                maxEntries_;
    std::list<SHPTR<EntryT>>                entries_;
    int64_t                                 lastCommitId_;
};
}
#endif
