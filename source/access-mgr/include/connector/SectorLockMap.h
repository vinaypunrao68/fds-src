/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SECTORLOCKMAP_H_
#define SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SECTORLOCKMAP_H_

#include <deque>
#include <mutex>
#include <vector>

#include "fds_error.h"

namespace fds
{

template <typename E>
struct SectorLock {
    using entry_type = E;
    using error_type = Error;
    using unique_entry = std::unique_ptr<entry_type>;
    using entry_deque_type = std::deque<entry_type>;

    enum class QueueResult { FirstEntry, MergedEntry, Delayed, Finished, Missing };

    QueueResult queue_update(entry_type& e, bool const complete) {
        if (!waiting_for_get.empty()) {
            // Queue with others
            waiting_for_get.push_back(e);
            return QueueResult::Delayed;
        } else if (!complete || unstable_data) {
            // Set as first request, may or may not need to get data
            waiting_for_get.push_back(e);
            return (unstable_data ? QueueResult::Delayed : QueueResult::FirstEntry);
        }
        unstable_data.reset(new entry_type(e));
        return QueueResult::MergedEntry;
    }

    QueueResult get_resp(entry_type& complete, error_type const error) {
        QueueResult result {QueueResult::Missing};
        if (!unstable_data && !waiting_for_get.empty()) {
            for (auto update : waiting_for_get) {
                complete << update;
            }
            waiting_for_get.clear();
            if (ERR_OK != error) {
                auto done = complete.notifyAll(error);
                result = (done ? QueueResult::Finished : QueueResult::Delayed);
            } else {
                unstable_data.reset(new entry_type(complete));
                result = QueueResult::MergedEntry;
            }
        }
        return result;
    }

    QueueResult write_resp(entry_type& complete, error_type const error) {
        QueueResult result {QueueResult::Missing};
        if (unstable_data && complete.id() == unstable_data->id()) {
            if (ERR_OK != error) {
                if (!waiting_for_get.empty()) {
                    // Roll ourselves into the next command who may succeed
                    for (auto update : waiting_for_get) {
                        *unstable_data << update;
                    }
                    waiting_for_get.clear();
                    complete = *unstable_data;
                    return QueueResult::MergedEntry;
                } 
                // This command failed, remove from queue and notify parents
                complete = std::move(*unstable_data);
                unstable_data.reset();
            } else {
                // The cache may have stashed this successful put
                unstable_data->setCached();
                *unstable_data >> complete;
            }
            auto done = complete.notifyAll(error);
            result = (done ? QueueResult::Finished : QueueResult::Delayed);
        }
        return result;
    }

    QueueResult cat_resp(entry_type& complete, error_type const error) {
        QueueResult result {QueueResult::Missing};
        if (unstable_data) {
            fds_assert(complete.id() == unstable_data->id());
            unstable_data->notifyAll(error);
            *unstable_data >> complete;
            result = QueueResult::Finished;
        }
        return result;
    }

    void add_stable(entry_deque_type& complete) {
        // Gather needed Updates so we know the ObjectIDs to write to DM
        if (unstable_data) {
            complete.push_back(*unstable_data);
        }
    }

    bool data_stable(entry_type& result, entry_deque_type& need_dispatch) {
        if (unstable_data) {
            *unstable_data >> result;  // Bundle into result
            if (!waiting_for_get.empty()) {  // Merge any pending writes
                unstable_data->clearReqs();
                for (auto update : waiting_for_get) {
                    *unstable_data << update;
                }
                waiting_for_get.clear();  // We can now write these to SM
                need_dispatch.push_back(*unstable_data);
                return false;
            } else {
                unstable_data.reset();
            }
        }
        return true;
    }

    bool fail(entry_type& failure, entry_deque_type& need_dispatch) {
        if (unstable_data) {
            *unstable_data >> failure;  // Bundle into result
            unstable_data.reset();
            // Result pending writes, we can't use the data from the previous
            // write since it failed, so we need to redo the GET
            if (!waiting_for_get.empty()) {
                need_dispatch.push_back(waiting_for_get.front());
                return false;
            }
        }
        return true;
    }

 private:
    // Merged writes waiting for underlying data for RMW
    entry_deque_type waiting_for_get;

    // Merged writes waiting for SM responses
    unique_entry unstable_data;
};

// This class offers a way to "lock" a sector of the blob
// and queue operations modifying the same offset to maintain
// consistency for < maxObjectSize writes.
template <typename E>
struct SectorLockMap {
    typedef E entry_type;
    typedef typename entry_type::req_type req_type;
    typedef std::mutex lock_type;
    typedef uint64_t key_type;
    typedef std::set<key_type> key_set_type;
    typedef SectorLock<entry_type> sector_lock_type;
    typedef typename sector_lock_type::entry_deque_type entry_deque_type;
    typedef std::unordered_map<key_type, sector_lock_type> sector_map_type;
    typedef std::unordered_map<req_type, key_set_type> req_map_type;
    typedef typename sector_map_type::iterator sector_map_it;

    typedef typename sector_lock_type::error_type error_type;
    typedef typename sector_lock_type::QueueResult queue_result_type;

    SectorLockMap() :
        map_lock(), sector_map()
    {}
    ~SectorLockMap() {}

    queue_result_type queue_update(entry_type& e, bool const complete) {
        queue_result_type result = queue_result_type::Delayed;
        std::lock_guard<lock_type> g(map_lock);
        for (auto req : e.request_set()) {
            request_map[req].insert(e.offset());
        }
        return sector_map[e.offset()].queue_update(e, complete);
    }

    queue_result_type read_resp(entry_type& response,
                                entry_deque_type& need_dispatch,
                                error_type const error) {
        queue_result_type result {queue_result_type::Missing};
        std::lock_guard<lock_type> g(map_lock);
        sector_map_it it = sector_map.find(response.offset());
        if (sector_map.end() != it) {
            auto& sector_lock = it->second;
            result = sector_lock.get_resp(response, error);
            if (queue_result_type::Finished == result) {
                return _complete(response, need_dispatch);
            } else if (queue_result_type::Delayed == result) {
                if (ERR_OK != error) {
                    _cleanup(response, it);
                }
            }
        }
        return result;
    }

    queue_result_type write_resp(entry_type& response,
                                 entry_deque_type& need_dispatch,
                                 error_type const error) {
        queue_result_type result {queue_result_type::Missing};
        std::lock_guard<lock_type> g(map_lock);
        sector_map_it it = sector_map.find(response.offset());
        if (sector_map.end() != it) {
            auto& sector_lock = it->second;
            result = sector_lock.write_resp(response, error);
            if (queue_result_type::Finished == result) {
                result = _complete(response, need_dispatch);
            } else if (queue_result_type::Delayed == result && ERR_OK != error) {
                _cleanup(response, it);
            }
        }
        return result;
    }

    queue_result_type catalog_resp(entry_type& response,
                                   entry_deque_type& need_dispatch,
                                   error_type const error) {
        queue_result_type result {queue_result_type::Missing};
        std::lock_guard<lock_type> g(map_lock);
        sector_map_it it = sector_map.find(response.offset());
        if (sector_map.end() != it) {
            auto& sector_lock = it->second;
            result = sector_lock.cat_resp(response, error);
            if (queue_result_type::Finished == result) {
                return _complete(response, need_dispatch, true);
            }
        }
        return result;
    }

 private:
    explicit SectorLockMap(SectorLockMap const& rhs) = delete;  // Non-copyable
    SectorLockMap& operator=(SectorLockMap const& rhs) = delete;  // Non-assignable

    void _cleanup(entry_type& complete, sector_map_it& it) {
        for (auto req : (*complete.request_set().begin())->allDependencies()) {
            request_map[req].erase(complete.offset());
        }
        sector_map.erase(it);
    }

    queue_result_type _complete(entry_type& complete, entry_deque_type& need_dispatch, bool cat_update = false) {
        // Get complete set of offsets requests (all dependencies)
        key_set_type done_keys;
        if (!complete.request_set().empty()) {
            for (auto req : (*complete.request_set().begin())->allDependencies()) {
                auto req_it = request_map.find(req);
                if (request_map.end() != req_it) {
                    auto& offset_set = req_it->second;
                    done_keys.insert(offset_set.begin(), offset_set.end());
                }
            }
        }

        // For each done key, gather all done updates into complete, and any
        // Put/Get requests we need to send out in need_dispatch depending on
        // if there was an error.
        if (!done_keys.empty()) {
            auto err = complete.result();
            for (auto key : done_keys) {
                if (ERR_OK == err) {
                    if (cat_update) {
                        for (auto req : (*complete.request_set().begin())->allDependencies()) {
                            request_map.erase(req);
                        }
                        // Data is stable if this was a DM write
                        if (sector_map[key].data_stable(complete, need_dispatch)) {
                            sector_map.erase(key);
                        }
                    } else {
                        // Gather Object for DM write
                        sector_map[key].add_stable(need_dispatch);
                    }
                } else {
                    for (auto req : (*complete.request_set().begin())->allDependencies()) {
                        request_map.erase(req);
                    }
                    if (sector_map[key].fail(complete, need_dispatch)) {
                        sector_map.erase(key);
                    }
                }
            }
            if (ERR_OK == err) {
                return queue_result_type::MergedEntry;
            }
        }
        return queue_result_type::Finished;
    }

    lock_type map_lock;
    sector_map_type sector_map;
    req_map_type request_map;
};

}  // namespace fds

#endif  // SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SECTORLOCKMAP_H_
