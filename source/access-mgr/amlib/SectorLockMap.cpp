/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#include "SectorLockMap.h"

#include "requests/GetBlobReq.h"
#include "requests/PutBlobReq.h"
#include "requests/PutObjectReq.h"
#include "requests/UpdateCatalogReq.h"
#include "ObjectId.h"

namespace fds
{

void
SectorUpdate::rehash() {
    if (!dirty || !object_data) return;
    object_length = object_data->length();
    if (0 == object_length) {
        object_id = NullObjectID;
    } else {
        object_id = ObjectID(ObjIdGen::genObjectId(object_data->c_str(), object_length));
    }
    dirty = false;
}

SectorUpdate&
SectorUpdate::operator>>(SectorUpdate& rhs) const
{
    for (auto& req : notify_set) {
        rhs.notify_set.insert(req);
    }
    return rhs;
}

SectorUpdate&
SectorUpdate::operator<<(SectorUpdate& rhs)
{ 
    // Just swap the buffer if this is a complete write
    if (object_data && !rhs.object_complete) {
        if (cached) {
            // Make a copy of the buffer to merge it may be living in cache too!
            object_data = boost::make_shared<data_type>(*object_data);
            cached = false;
        }
        object_data->replace(rhs.object_offset,
                             rhs.object_data->length(),
                             rhs.object_data->c_str(),
                             rhs.object_data->length());
    } else {
        object_data = rhs.object_data;
        cached = rhs.cached;
        object_offset = rhs.object_offset;
    }
    dirty = true;

    if (!notify_set.empty() && !rhs.notify_set.empty()) {
        (*notify_set.begin())->mergeDependencies(*rhs.notify_set.begin());
    }
    for (auto& req : rhs.notify_set) {
        notify_set.insert(req);
    }
    return *this;
}

bool
SectorUpdate::notifyAll(Error const& e) {
    for (auto req : notify_set) {
        req->notifyResponse(e, absolute_offset);
    }
    return (notify_set.empty() ? true
                                 : (*notify_set.begin())->dependenciesDone());
}

SectorLock::QueueResult
SectorLock::queue_update (entry_type& e) {
    if (!waiting_for_get.empty()) {
        // Queue with others
        waiting_for_get.push_back(e);
        return QueueResult::Delayed;
    } else if (!e.objectComplete() || unstable_data) {
        // Set as first request, may or may not need to get data
        waiting_for_get.push_back(e);
        return (unstable_data ? QueueResult::Delayed : QueueResult::FirstEntry);
    }
    unstable_data.reset(new entry_type(e));
    return QueueResult::MergedEntry;
}

SectorLock::QueueResult
SectorLock::get_resp(entry_type& complete, error_type const error) {
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

SectorLock::QueueResult
SectorLock::write_resp(entry_type& complete, error_type const error) {
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

SectorLock::QueueResult
SectorLock::cat_resp(entry_type& complete, error_type const error) {
    QueueResult result {QueueResult::Missing};
    if (unstable_data) {
        fds_assert(complete.id() == unstable_data->id());
        unstable_data->notifyAll(error);
        *unstable_data >> complete;
        result = QueueResult::Finished;
    }
    return result;
}

bool
SectorLock::data_stable(entry_type& result, entry_deque_type& need_dispatch) {
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

bool
SectorLock::fail(entry_type& failure, entry_deque_type& need_dispatch) {
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

bool
SectorLockMap::queue_update(entry_type& e) {
    auto result = queue_result_type::Delayed;
    fds_assert(1 == e.request_set().size());
    auto req = *e.request_set().begin();
    {
        std::lock_guard<lock_type> g(map_lock);
        request_map[req].insert(e.offset());
        result = sector_map[e.offset()].queue_update(e);
    }
    // Ready to send to SM
    if (queue_result_type::MergedEntry == result) {
        _send_put(e);
    }
    return (queue_result_type::FirstEntry == result);
}

bool
SectorLockMap::read_resp(entry_type& response, error_type const error) {
    queue_result_type result {queue_result_type::Missing};
    entry_deque_type need_dispatch;
    {
        std::lock_guard<lock_type> g(map_lock);
        sector_map_it it = sector_map.find(response.offset());
        if (sector_map.end() != it) {
            auto& sector_lock = it->second;
            result = sector_lock.get_resp(response, error);
            if (queue_result_type::Finished == result) {
                result = _complete(response, need_dispatch);
            } else if (queue_result_type::Delayed == result) {
                if (ERR_OK != error) {
                    _cleanup(response, it);
                }
            }
        }
    }
    if (queue_result_type::MergedEntry == result) {
        _send_put(response);
    } else if (queue_result_type::Finished == result) {
        // These failed, respond
        for (auto req : response.request_set()) {
            _next_in_chain->unknownTypeCb(req, response.result());
        }
        _send_reads(need_dispatch); // These can now be processed normally
    }
    return empty();
}

bool
SectorLockMap::write_resp(entry_type& response, error_type const error) {
    queue_result_type result {queue_result_type::Missing};
    entry_deque_type need_dispatch;
    {
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
    }

    if (queue_result_type::MergedEntry == result) {
        if (ERR_OK != error) {
            // Retry with this rolled up merge
            _send_put(response);
        } else {
            // Send out the catalog update!
            _send_cat(need_dispatch);
        }
    } else if (queue_result_type::Finished == result) {
        for (auto req : response.request_set()) {
            _next_in_chain->unknownTypeCb(req, response.result());
        }
        _send_reads(need_dispatch); // These can now be processed normally
    }
    return empty();
}

bool
SectorLockMap::catalog_resp(entry_type& response, error_type const error) {
    queue_result_type result {queue_result_type::Missing};
    entry_deque_type need_dispatch;
    {
        std::lock_guard<lock_type> g(map_lock);
        sector_map_it it = sector_map.find(response.offset());
        if (sector_map.end() != it) {
            auto& sector_lock = it->second;
            result = sector_lock.cat_resp(response, error);
            if (queue_result_type::Finished == result) {
                result = _complete(response, need_dispatch, true);
            }
        }
    }

    // These are done, respond
    auto err = response.result();
    for (auto req : response.request_set()) {
        _next_in_chain->unknownTypeCb(req, err);
    }

    // If the update to the offset failed, we'll get back some Gets that
    // need getting, otherwise we'll get back puts that need putting
    if (ERR_OK == err) {
        for (auto& needed : need_dispatch) {
            _send_put(needed);
        }
    } else {
        _send_reads(need_dispatch); // These can now be processed normally
    }
    return empty();
}

void
SectorLockMap::_cleanup(entry_type& complete, sector_map_it& it) {
    for (auto req : (*complete.request_set().begin())->allDependencies()) {
        request_map[req].erase(complete.offset());
    }
    sector_map.erase(it);
}

SectorLockMap::queue_result_type
SectorLockMap::_complete(entry_type& complete,
                         entry_deque_type& need_dispatch,
                         bool const cat_update) {
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

void
SectorLockMap::_send_reads(std::deque<entry_type>& need_dispatch) {
    if (!need_dispatch.empty()) {
        fds_assert(0 < need_dispatch.front().request_set().size());
        auto req = *need_dispatch.front().request_set().begin();
        auto begin_off = need_dispatch.front().offset();
        auto end_off = begin_off;
        for (auto dispatch : need_dispatch) {
            begin_off = std::min(begin_off, dispatch.offset());
            end_off = std::max(end_off, dispatch.offset());
        }
        auto get_length = (end_off - begin_off) + req->object_size;
        auto closure = [](GetObjectCallback* cb, fpi::ErrorCode const& e) -> void { };
        auto callback = create_async_handler<GetObjectCallback>(std::move(closure));
        auto getReq = new GetBlobReq(req->io_vol_id,
                                     req->volume_name,
                                     req->getBlobName(),
                                     callback,
                                     begin_off,
                                     get_length);
        getReq->volInfoCopy(req);
        getReq->for_rmw = true;
        AmDataProvider::getBlob(getReq);
    }
}

void
SectorLockMap::_send_put(entry_type& need_dispatch) {
    fds_assert(0 < need_dispatch.request_set().size());
    auto req = *need_dispatch.request_set().begin();
    auto putReq = new PutObjectReq(req->io_vol_id,
                                   req->volume_name,
                                   req->getBlobName(),
                                   need_dispatch.data(),
                                   need_dispatch.id(),
                                   need_dispatch.objectLength());
    putReq->blob_offset = need_dispatch.offset();
    putReq->volInfoCopy(req);
    AmDataProvider::putObject(putReq);
}

void
SectorLockMap::_send_cat(std::deque<entry_type>& need_dispatch) {
    if (!need_dispatch.empty()) {
        fds_assert(0 < need_dispatch.front().request_set().size());
        auto req = static_cast<PutBlobReq*>(*need_dispatch.front().request_set().begin());
        auto catReq = new UpdateCatalogReq(req, true);
        for (auto part_update : need_dispatch) {
            catReq->object_list.emplace(part_update.id(),
                                        std::make_pair(part_update.offset(),
                                                       part_update.objectLength()));
        }
        for (auto req : need_dispatch.front().request_set()) {
            auto pReq = static_cast<PutBlobReq*>(req);
            catReq->metadata->insert(pReq->metadata->begin(), pReq->metadata->end());
        }
        catReq->volInfoCopy(req);
        AmDataProvider::updateCatalog(catReq);
    }
}

}  // namespace fds
