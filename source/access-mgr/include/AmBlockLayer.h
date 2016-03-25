/*
 * Copyright 2016 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMBLOCKLAYER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMBLOCKLAYER_H_

#include <deque>
#include <set>
#include <string>
#include <unordered_map>

#include "AmAsyncDataApi.h"
#include "AmDataProvider.h"

#include <blob/BlobTypes.h>
#include "connector/SectorLockMap.h"
#include <concurrency/RwLock.h>
#include <fdsp/dm_types_types.h>

namespace fds {

struct AmMultiReq;
struct PutBlobReq;

struct BlockUpdate {
    using data_type = std::string;
    using data_buf_type = boost::shared_ptr<data_type>;
    using offset_type = size_t;
    using req_type = AmMultiReq*;
    using req_set_type = std::set<req_type>;

    inline explicit BlockUpdate(offset_type const obj_off);
    inline BlockUpdate(AmMultiReq* root,
                       offset_type const abs_off,
                       offset_type const obj_off,
                       data_buf_type buf,
                       bool const complete = false);
    BlockUpdate(BlockUpdate const& rhs) = default;
    BlockUpdate& operator=(BlockUpdate const& rhs) = default;
    BlockUpdate(BlockUpdate&& rhs) = default;
    BlockUpdate& operator=(BlockUpdate&& rhs) = default;
    ~BlockUpdate() = default;

    inline BlockUpdate& operator>>(BlockUpdate& rhs) const;
    inline BlockUpdate& operator<<(BlockUpdate& rhs);

    data_buf_type data() const
    { return object_data; }

    ObjectID id()
    { rehash(); return object_id; }

    void setId(ObjectID& id)
    { object_id = id; object_data.reset(); dirty = false; }

    void setCached() { cached = true; }

    bool notifyAll(Error const& e);
    
    inline Error result() const;

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

/**
 * Manages performing RMW and amortization of Block IO
 */
struct AmBlockLayer :
    public AmDataProvider
{
    explicit AmBlockLayer(AmDataProvider* prev);
    AmBlockLayer(AmBlockLayer const&) = delete;
    AmBlockLayer& operator=(AmBlockLayer const&) = delete;
    ~AmBlockLayer() override;

    /**
     * These are the Transaction specific DataProvider routines.
     * Everything else is pass-thru.
     */
    void putBlobOnce(AmRequest *amReq) override;

  protected:

    /**
     * These are the response we actually care about seeing the results of
     */
    void getBlobCb(AmRequest * amReq, Error const error) override;
    void putObjectCb(AmRequest * amReq, Error const error) override;
    void updateCatalogCb(AmRequest * amReq, Error const error) override;

  private:
    // These maps build a sector lock for each blob in a volume during a write
    // to create a barrier to do RMW and write amortization.
    using sector_type = SectorLockMap<BlockUpdate>;

    using offset_queue_ptr = std::shared_ptr<sector_type>;

    using blob_map_type = std::unordered_map<BlockUpdate::data_type, offset_queue_ptr>;

    using volume_map_type = std::unordered_map<fds_volid_t, blob_map_type>;

    volume_map_type block_queue;
    std::mutex block_update_lock;

    void dispatchPut(AmRequest* parent_request, BlockUpdate& need_dispatch);
    void dispatchReads(AmRequest* parent_request, std::deque<BlockUpdate>& need_dispatch);
    size_t split_update(PutBlobReq* blobReq, std::deque<BlockUpdate>& needed, std::deque<BlockUpdate>& ready);
    offset_queue_ptr lookup_offset_queue(fds_volid_t const vol_id,
                                         BlockUpdate::data_type const& blob,
                                         bool const create_too = false);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMBLOCKLAYER_H_
