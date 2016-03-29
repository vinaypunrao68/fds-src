/*
 * Copyright 2015-2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_SECTORLOCKMAP_H_
#define SOURCE_ACCESS_MGR_INCLUDE_SECTORLOCKMAP_H_

#include <deque>
#include <mutex>
#include <vector>

#include "fds_error.h"
#include "AmRequest.h"
#include "AmDataProvider.h"

namespace fds
{

struct SectorUpdate {
    using data_type = std::string;
    using data_buf_type = boost::shared_ptr<data_type>;
    using offset_type = size_t;
    using req_type = AmMultiReq*;
    using req_set_type = std::set<req_type>;

    explicit SectorUpdate(offset_type const obj_off) :
        object_data(nullptr),
        object_id(),
        absolute_offset(obj_off),
        notify_set()
    { }

    SectorUpdate(AmMultiReq* root,
                 offset_type const abs_off,
                 offset_type const obj_off,
                 data_buf_type buf,
                 bool const complete = false) :
        dirty(true),
        object_complete(complete),
        object_data(buf),
        object_id(),
        object_offset(obj_off),
        absolute_offset(abs_off),
        notify_set()
    { if (root) notify_set.insert(root); }

    SectorUpdate(SectorUpdate const& rhs) = default;
    SectorUpdate& operator=(SectorUpdate const& rhs) = default;
    SectorUpdate(SectorUpdate&& rhs) = default;
    SectorUpdate& operator=(SectorUpdate&& rhs) = default;
    ~SectorUpdate() = default;

    inline SectorUpdate& operator>>(SectorUpdate& rhs) const;
    inline SectorUpdate& operator<<(SectorUpdate& rhs);

    data_buf_type data() const
    { return object_data; }

    ObjectID id()
    { rehash(); return object_id; }

    void setId(ObjectID& id)
    { object_id = id; object_data.reset(); dirty = false; }

    void setCached() { cached = true; }

    bool notifyAll(Error const& e);
    
    inline Error result() const
    { return (notify_set.empty() ? ERR_OK : (*notify_set.begin())->errorCode()); }

    offset_type offset() const { return absolute_offset; }

    void clearReqs()
    { notify_set.clear(); }

    req_set_type const& request_set() const
    { return notify_set; }    

    bool objectComplete() const { return object_complete; }

    size_t objectLength() const { return object_length; }

 private:
    bool dirty {false};
    bool cached {false};
    bool object_complete {false};
    size_t object_length {0};
    data_buf_type object_data;
    ObjectID object_id;
    offset_type object_offset {0};
    offset_type absolute_offset {0};
    req_set_type notify_set;

    void rehash();
};

struct SectorLock {
    using entry_type = SectorUpdate;
    using error_type = Error;
    using unique_entry = std::unique_ptr<entry_type>;
    using entry_deque_type = std::deque<entry_type>;

    enum class QueueResult { FirstEntry, MergedEntry, Delayed, Finished, Missing };

    QueueResult queue_update(entry_type& e);

    QueueResult get_resp(entry_type& complete, error_type const error);

    QueueResult write_resp(entry_type& complete, error_type const error);

    QueueResult cat_resp(entry_type& complete, error_type const error);

    void add_stable(entry_deque_type& complete)
    {
        // Gather needed Updates so we know the ObjectIDs to write to DM
        if (unstable_data) complete.push_back(*unstable_data);
    }

    bool data_stable(entry_type& result, entry_deque_type& need_dispatch);

    bool fail(entry_type& failure, entry_deque_type& need_dispatch);

 private:
    // Merged writes waiting for underlying data for RMW
    entry_deque_type waiting_for_get;

    // Merged writes waiting for SM responses
    unique_entry unstable_data;
};

// This class offers a way to "lock" a sector of the blob
// and queue operations modifying the same offset to maintain
// consistency for < maxObjectSize writes.
struct SectorLockMap : public AmDataProvider {
    using lock_type = std::mutex;
    using key_type = size_t;
    using key_set_type = std::set<key_type>;
    using entry_type = SectorUpdate;
    using req_type = entry_type::req_type;
    using entry_deque_type = SectorLock::entry_deque_type;
    using sector_map_type = std::unordered_map<key_type, SectorLock>;
    using req_map_type = std::unordered_map<req_type, key_set_type>;
    using sector_map_it = sector_map_type::iterator;

    using error_type = SectorLock::error_type;
    using queue_result_type = SectorLock::QueueResult;

    explicit SectorLockMap(std::shared_ptr<AmDataProvider> const& next) :
        AmDataProvider(nullptr, next),
        map_lock(), sector_map()
    {}
    ~SectorLockMap() = default;

    bool empty() {
        std::lock_guard<std::mutex> g(map_lock);
        return request_map.empty() && sector_map.empty();
    }

    bool queue_update(entry_type& e);

    bool read_resp(entry_type& response, error_type const error);

    bool write_resp(entry_type& response, error_type const error);

    bool catalog_resp(entry_type& response, error_type const error);

 private:
    explicit SectorLockMap(SectorLockMap const& rhs) = delete;  // Non-copyable
    SectorLockMap& operator=(SectorLockMap const& rhs) = delete;  // Non-assignable

    void _cleanup(entry_type& complete, sector_map_it& it);

    queue_result_type _complete(entry_type& complete,
                                entry_deque_type& need_dispatch,
                                bool const cat_update = false);

    void _send_reads(std::deque<entry_type>& need_dispatch);
    void _send_put(entry_type& need_dispatch);
    void _send_cat(std::deque<entry_type>& need_dispatch);

    lock_type map_lock;
    sector_map_type sector_map;
    req_map_type request_map;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_SECTORLOCKMAP_H_
