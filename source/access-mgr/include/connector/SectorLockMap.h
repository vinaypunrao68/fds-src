/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SECTORLOCKMAP_H_
#define SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SECTORLOCKMAP_H_

#include <deque>
#include <mutex>

namespace fds
{

// This class offers a way to "lock" a sector of the blob
// and queue operations modifying the same offset to maintain
// consistency for < maxObjectSize writes.
template <typename E>
struct SectorLockMap {
    typedef E entry_type;
    typedef std::mutex lock_type;
    typedef uint64_t key_type;
    typedef std::deque<entry_type> queue_type;
    typedef std::unordered_map<key_type, std::unique_ptr<queue_type>> map_type;
    typedef typename map_type::iterator map_it;

    enum class QueueResult { FirstEntry, AddedEntry, Failure };

    SectorLockMap() :
        map_lock(), sector_map()
    {}
    ~SectorLockMap() {}

    QueueResult queue_update(key_type const& k, entry_type e) {
        QueueResult result = QueueResult::AddedEntry;
        std::lock_guard<lock_type> g(map_lock);
        map_it it = sector_map.find(k);
        if (sector_map.end() != it) {
            (*it).second->push_front(e);
        } else {
            auto r = sector_map.insert(
                std::make_pair(k, std::move(std::unique_ptr<queue_type>(new queue_type()))));
            result = QueueResult::FirstEntry;
        }
        return result;
    }

    std::pair<bool, entry_type> pop(key_type const& k, bool and_delete = false)
    {
        static std::pair<bool, entry_type> const no = {false, entry_type()};
        std::lock_guard<lock_type> g(map_lock);
        auto it = sector_map.find(k);
        if (sector_map.end() != it) {
            if (!(*it).second->empty()) {
                auto entry = (*it).second->back();
                (*it).second->pop_back();
                return std::make_pair(true, entry);
            } else {
                // No more queued requests, return nullptr
                if (and_delete) {
                    sector_map.erase(it);
                }
            }
        }
        return no;
    }

 private:
    explicit SectorLockMap(SectorLockMap const& rhs) = delete;  // Non-copyable
    SectorLockMap& operator=(SectorLockMap const& rhs) = delete;  // Non-assignable

    lock_type map_lock;
    map_type sector_map;
};

}  // namespace fds

#endif  // SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SECTORLOCKMAP_H_
