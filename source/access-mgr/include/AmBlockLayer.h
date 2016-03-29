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
#include "SectorLockMap.h"
#include <concurrency/RwLock.h>
#include <fdsp/dm_types_types.h>

namespace fds {

struct AmMultiReq;
struct PutBlobReq;

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
    using sector_type = SectorLockMap::entry_type;

    using offset_queue_type = SectorLockMap;
    using offset_queue_ptr = std::shared_ptr<SectorLockMap>;

    using blob_map_type = std::unordered_map<sector_type::data_type, offset_queue_ptr>;

    using volume_map_type = std::unordered_map<fds_volid_t, blob_map_type>;

    volume_map_type block_queue;
    std::mutex block_update_lock;

    void dispatchPut(AmRequest* parent_request, sector_type& need_dispatch);
    void dispatchReads(AmRequest* parent_request, std::deque<sector_type>& need_dispatch);
    size_t split_update(PutBlobReq* blobReq, std::deque<sector_type>& needed, std::deque<sector_type>& ready);
    offset_queue_ptr lookup_offset_queue(fds_volid_t const vol_id,
                                         sector_type::data_type const& blob,
                                         bool const create_too = false);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMBLOCKLAYER_H_
