/*
 * Copyright 2013-2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_

#include <atomic>
#include <set>
#include <string>

#include "fds_volume.h"
#include "PerfTrace.h"
#include "AsyncResponseHandlers.h"

namespace fds
{

struct AmRequest : public FDS_IOType {
    // Performance
    PerfContext    e2e_req_perf_ctx;
    PerfContext    qos_perf_ctx;
    PerfContext    hash_perf_ctx;
    PerfContext    dm_perf_ctx;
    PerfContext    sm_perf_ctx;

    // Table version used to message Catalog Service
    fds_uint64_t   dmt_version;

    // Table version used to message Object Service
    fds_uint64_t   dlt_version;

    std::size_t    object_size;
    std::size_t    data_len;
    std::size_t    blob_offset;
    std::size_t    blob_offset_end;
    std::string    volume_name;

    bool           absolute_offset {false};
    bool           forced_unit_access {true};
    bool           page_out_cache {false};

    // Flag to indicate when a request has been responded to
    std::atomic<bool> completed;

    CallbackPtr cb;

    AmRequest(fds_io_op_t         _op,
              fds_volid_t         _vol_id,
              const std::string&  _vol_name,
              const std::string&  _blob_name,
              CallbackPtr         _cb,
              size_t              _blob_offset = 0,
              size_t              _data_len = 0)
        : FDS_IOType(),
        volume_name(_vol_name),
        completed(false),
        blob_name(_blob_name),
        blob_offset(_blob_offset),
        data_len(_data_len),
        cb(_cb)
    {
        io_module = ACCESS_MGR_IO;
        io_req_id = 0;
        io_type   = _op;
        setVolId(_vol_id);
    }

    void setVolId(fds_volid_t const vol_id) {
        io_vol_id = vol_id;
        e2e_req_perf_ctx.reset_volid(io_vol_id);
        qos_perf_ctx.reset_volid(io_vol_id);
        hash_perf_ctx.reset_volid(io_vol_id);
        dm_perf_ctx.reset_volid(io_vol_id);
        sm_perf_ctx.reset_volid(io_vol_id);
    }

    void volInfoCopy(AmRequest* const req) {
        object_size = req->object_size;
        page_out_cache = req->page_out_cache;
        forced_unit_access = req->forced_unit_access;
    }

    virtual ~AmRequest()
    { fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx); }

    bool isCompleted()
    { return !completed.load(std::memory_order_relaxed); }

    bool testAndSetComplete()
    { return completed.exchange(true, std::memory_order_relaxed); }

    const std::string& getBlobName() const
    { return blob_name; }

 protected:
    std::string        blob_name;
};

/**
 * A requests that dispatches multiple messages, and therefore expects multiple
 * responses. The responses need to be kept so that the *most relevant* error
 * code is returned instead of whichever just happens to be the last.
 */
struct AmMultiReq : public AmRequest {
    using AmRequest::AmRequest;

    void setResponseCount(size_t const cnt) {
        std::lock_guard<std::mutex> g(resp_lock);
        resp_acks = cnt;
    }

    void expectResponseOn(size_t const offset) {
        expected_offset.insert(offset);
    }

    bool dependenciesDone() const {
        if (dependencies) {
            for (auto req : *dependencies) {
                if (!req->expected_offset.empty()) return false;
            }
        } else if (!expected_offset.empty()) {
            return false;
        }
        return true;
    }

    std::pair<bool, Error> notifyResponse(const Error &e, size_t const offset) {
        bool done {false};
        if (0 < expected_offset.erase(offset) || !e.ok()) {
            op_err = e.ok() ? op_err : e;
            done = expected_offset.empty();

            if (!op_err.ok() && dependencies) {
                for (auto req : *dependencies) {
                    req->op_err = op_err;
                }
            }
        }
        return std::make_pair(done, (done ? op_err : ERR_OK));
    }

    std::pair<bool, Error> notifyResponse(const Error &e) {
        size_t acks_left = 0;
        {
            std::lock_guard<std::mutex> g(resp_lock);
            op_err = e.ok() ? op_err : e;
            acks_left = --resp_acks;
        }
        if (0 == acks_left) {
            return std::make_pair(true, op_err);
        }
        return std::make_pair(false, ERR_OK);
    }

    Error errorCode() const { return op_err; }

    void mergeDependencies(AmMultiReq* req)
    {
        // Create a set with ourselves if we don't have one yet
        if (!dependencies) {
            dependencies = std::make_shared<std::set<AmMultiReq*>>();
            dependencies->insert(this);
        }

            // If this is a new dependency add it and reset set
        if (dependencies->insert(req).second) {
            // If the other request had dependencies
            if (req->dependencies) {
                for (auto request : *req->dependencies) {
                    // If not already a dependency, add and reset set
                    if (dependencies->insert(request).second) {
                        request->dependencies = dependencies;
                    }
                }
            }
            // Now reset parent
            req->dependencies = dependencies;
        }
    }

    std::set<AmMultiReq*> const& allDependencies() {
        if (!dependencies) {
            dependencies = std::make_shared<std::set<AmMultiReq*>>();
            dependencies->insert(this);
        }
        return *dependencies;
    }

 protected:
    /* ack cnt for responses, decremented when response from SM and DM come back */
    mutable std::mutex resp_lock;
    size_t resp_acks {1};

    std::set<size_t> expected_offset;
    std::shared_ptr<std::set<AmMultiReq*>> dependencies;
    Error op_err {ERR_OK};
};

struct AmTxReq {
    AmTxReq() : tx_desc(nullptr) {}
    explicit AmTxReq(BlobTxId::ptr _tx_desc) : tx_desc(_tx_desc) {}
    BlobTxId::ptr tx_desc;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
