/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCKOPERATIONS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCKOPERATIONS_H_

#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/enable_shared_from_this.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "fdsp/common_types.h"
#include "fdsp/xdi_types.h"
#include "AmAsyncResponseApi.h"
#include "AmAsyncDataApi.h"
#include "connector/BlockTask.h"
#include "connector/SectorLockMap.h"

namespace fds {

enum class BlockError : uint8_t {
    connection_closed,
    shutdown_requested,
};

struct BlockHandle {
    uint64_t handle;
    uint32_t seq;
};

/**
 * The BlockOperations class provides a simple interface to a dynamic connector
 * allowing block like semantics. The interface consists of three main calls to
 * attach the volume, read data and write data. The RMW logic and operation
 * rollup all happens in here allowing block connectors to issue their requests
 * as fast as possible without having to deal with consistency themselves and
 * map I/O to AmAsyncDataApi calls.
 */
class BlockOperations
    :   public boost::enable_shared_from_this<BlockOperations>,
        public AmAsyncResponseApi<BlockHandle>
{
    using req_api_type = AmAsyncDataApi<BlockHandle>;
    using resp_api_type = AmAsyncResponseApi<BlockHandle>;
    using task_type = BlockTask;

    using handle_type = resp_api_type::handle_type;
    using error_type = resp_api_type::error_type;
    using size_type = resp_api_type::size_type;


    typedef SectorLockMap<handle_type, 1024> sector_type;
    typedef std::unordered_map<int64_t, task_type*> response_map_type;
  public:

    // Response interface for BlockOperations
    struct ResponseIFace {
        ResponseIFace() = default;
        ResponseIFace(ResponseIFace const&) = delete;
        ResponseIFace& operator=(ResponseIFace const&) = delete;
        ResponseIFace(ResponseIFace const&&) = delete;
        ResponseIFace& operator=(ResponseIFace const&&) = delete;
        virtual ~ResponseIFace() = default;

        virtual void respondTask(task_type* response) = 0;
        virtual void attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) = 0;
        virtual void terminate() = 0;
    };

    explicit BlockOperations(ResponseIFace* respIface);
    explicit BlockOperations(BlockOperations const& rhs) = delete;
    BlockOperations& operator=(BlockOperations const& rhs) = delete;
    ~BlockOperations() = default;

    typedef boost::shared_ptr<BlockOperations> shared_ptr;
    void init(req_api_type::shared_string_type vol_name,
              std::shared_ptr<AmProcessor> processor,
              task_type* resp);

    void read(task_type* resp);
    void read(uint32_t length, uint64_t offset, int64_t handle);

    void write(req_api_type::shared_buffer_type& bytes, task_type* resp);

    void attachVolumeResp(const error_type &error,
                          handle_type& requestId,
                          resp_api_type::shared_vol_descriptor_type& volDesc,
                          resp_api_type::shared_vol_mode_type& mode) override;

    void detachVolumeResp(const error_type &error,
                          handle_type& requestId) override;

    // The two response types we do support
    void getBlobResp(const error_type &error,
                     handle_type& requestId,
                     const resp_api_type::shared_buffer_array_type& bufs,
                     size_type& length) override;

    void updateBlobResp(const error_type &error, handle_type& requestId) override;

    void detachVolume();

    void shutdown();

  private:
    void finishResponse(task_type* response);

    void drainUpdateChain(uint64_t const offset,
                          boost::shared_ptr<std::string> buf,
                          handle_type* queued_handle_ptr,
                          fpi::ErrorCode const error);

    uint32_t getObjectCount(uint32_t length, uint64_t offset);

    // api we've built
    std::unique_ptr<req_api_type> amAsyncDataApi;
    boost::shared_ptr<std::string> volumeName;
    uint32_t maxObjectSizeInBytes;

    // interface to respond to block passed down in constructor
    std::unique_ptr<ResponseIFace> blockResp;
    bool shutting_down {false};

    // for all reads/writes to AM
    boost::shared_ptr<std::string> blobName;
    boost::shared_ptr<std::string> domainName;
    boost::shared_ptr<int32_t> blobMode;
    boost::shared_ptr< std::map<std::string, std::string> > emptyMeta;

    // for now we are supporting <=4K requests
    // so keep current handles for which we are waiting responses
    std::mutex respLock;
    response_map_type responses;

    sector_type sector_map;

    // AmAsyncResponseApi un-implemented responses
    void abortBlobTxResp       (const error_type &, handle_type&) override {}
    void commitBlobTxResp      (const error_type &, handle_type&) override {}
    void deleteBlobResp        (const error_type &, handle_type&) override {}
    void getBlobWithMetaResp   (const error_type &, handle_type&, const resp_api_type::shared_buffer_array_type&, size_type&, resp_api_type::shared_descriptor_type&) override {}  // NOLINT
    void startBlobTxResp       (const error_type &, handle_type&, resp_api_type::shared_tx_ctx_type&) override {}  // NOLINT
    void statBlobResp          (const error_type &, handle_type&, resp_api_type::shared_descriptor_type&) override {}  // NOLINT
    void updateBlobOnceResp    (const error_type &, handle_type&) override {}
    void updateMetadataResp    (const error_type &, handle_type&) override {}
    void renameBlobResp        (const error_type &, handle_type&, resp_api_type::shared_descriptor_type&) override {}
    void volumeContentsResp    (const error_type &, handle_type&, resp_api_type::shared_descriptor_vec_type&, resp_api_type::shared_string_vec_type&) override {}  // NOLINT
    void volumeStatusResp      (const error_type &, handle_type&, resp_api_type::shared_status_type&) override {}  // NOLINT
    void setVolumeMetadataResp (const error_type &, handle_type&) override {}  // NOLINT
    void getVolumeMetadataResp (const error_type &, handle_type&, resp_api_type::shared_meta_type&) override {}  // NOLINT
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCKOPERATIONS_H_
