/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTOPERATIONS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTOPERATIONS_H_

#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/enable_shared_from_this.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "fds_types.h"
#include "concurrency/RwLock.h"
#include "fdsp/xdi_types.h"
#include "concurrency/Mutex.h"
#include "AmAsyncResponseApi.h"
#include "AmAsyncDataApi.h"
#include "connector/scst/scst_user.h"
#include "connector/SectorLockMap.h"

namespace fds {

struct ScstTask;

// Response interface for ScstOperations
struct ScstOperationsResponseIface {
    ScstOperationsResponseIface() = default;
    ScstOperationsResponseIface(ScstOperationsResponseIface const&) = delete;
    ScstOperationsResponseIface& operator=(ScstOperationsResponseIface const&) = delete;
    ScstOperationsResponseIface(ScstOperationsResponseIface const&&) = delete;
    ScstOperationsResponseIface& operator=(ScstOperationsResponseIface const&&) = delete;
    virtual ~ScstOperationsResponseIface() = default;

    virtual void respondTask(ScstTask* response) = 0;
    virtual void attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) = 0;
    virtual void terminate() = 0;
};

struct ScstSequencePair {
    uint32_t handle;
    uint32_t seq;
};

class ScstOperations
    :   public boost::enable_shared_from_this<ScstOperations>,
        public AmAsyncResponseApi<ScstSequencePair>
{
    using req_api_type = AmAsyncDataApi<ScstSequencePair>;
    using resp_api_type = AmAsyncResponseApi<ScstSequencePair>;

    typedef resp_api_type::handle_type handle_type;
    typedef SectorLockMap<handle_type, 1024> sector_type;
    typedef std::unordered_map<uint32_t, ScstTask*> response_map_type;
  public:
    explicit ScstOperations(ScstOperationsResponseIface* respIface);
    explicit ScstOperations(ScstOperations const& rhs) = delete;
    ScstOperations& operator=(ScstOperations const& rhs) = delete;
    ~ScstOperations() = default;

    typedef boost::shared_ptr<ScstOperations> shared_ptr;
    void init(req_api_type::shared_string_type vol_name,
              std::shared_ptr<AmProcessor> processor,
              ScstTask* resp);

    void read(ScstTask* resp);

    void write(char* bytes, ScstTask* resp);

    void attachVolumeResp(const resp_api_type::error_type &error,
                          handle_type& requestId,
                          resp_api_type::shared_vol_descriptor_type& volDesc,
                          resp_api_type::shared_vol_mode_type& mode) override;

    void detachVolumeResp(const resp_api_type::error_type &error,
                          handle_type& requestId) override;

    // The two response types we do support
    void getBlobResp(const resp_api_type::error_type &error,
                     handle_type& requestId,
                     const resp_api_type::shared_buffer_array_type& bufs,
                     resp_api_type::size_type& length) override;

    void updateBlobResp(const resp_api_type::error_type &error, handle_type& requestId) override;

    void shutdown();

  private:
    void finishResponse(ScstTask* response);

    void drainUpdateChain(uint64_t const offset,
                          boost::shared_ptr<std::string> buf,
                          handle_type* queued_handle_ptr,
                          Error const error);

    uint32_t getObjectCount(uint32_t length,
                                uint64_t offset);

    void detachVolume();

    // api we've built
    std::unique_ptr<req_api_type> amAsyncDataApi;
    boost::shared_ptr<std::string> volumeName;
    uint32_t maxObjectSizeInBytes;

    // interface to respond to scst passed down in constructor
    std::unique_ptr<ScstOperationsResponseIface> scstResp;
    bool shutting_down {false};

    // for all reads/writes to AM
    boost::shared_ptr<std::string> blobName;
    boost::shared_ptr<std::string> domainName;
    boost::shared_ptr<fds_int32_t> blobMode;
    boost::shared_ptr< std::map<std::string, std::string> > emptyMeta;

    // for now we are supporting <=4K requests
    // so keep current handles for which we are waiting responses
    fds_mutex respLock;
    response_map_type responses;

    sector_type sector_map;

    // AmAsyncResponseApi un-implemented responses
    void abortBlobTxResp       (const resp_api_type::error_type &, handle_type&) override {}
    void commitBlobTxResp      (const resp_api_type::error_type &, handle_type&) override {}
    void deleteBlobResp        (const resp_api_type::error_type &, handle_type&) override {}
    void getBlobWithMetaResp   (const resp_api_type::error_type &, handle_type&, const resp_api_type::shared_buffer_array_type&, resp_api_type::size_type&, resp_api_type::shared_descriptor_type&) override {}  // NOLINT
    void startBlobTxResp       (const resp_api_type::error_type &, handle_type&, resp_api_type::shared_tx_ctx_type&) override {}  // NOLINT
    void statBlobResp          (const resp_api_type::error_type &, handle_type&, shared_descriptor_type&) override {}  // NOLINT
    void updateBlobOnceResp    (const resp_api_type::error_type &, handle_type&) override {}
    void updateMetadataResp    (const resp_api_type::error_type &, handle_type&) override {}
    void renameBlobResp        (const resp_api_type::error_type &, handle_type&, shared_descriptor_type&) override {}
    void volumeContentsResp    (const resp_api_type::error_type &, handle_type&, shared_descriptor_vec_type&) override {}  // NOLINT
    void volumeStatusResp      (const resp_api_type::error_type &, handle_type&, shared_status_type&) override {}  // NOLINT
    void setVolumeMetadataResp (const resp_api_type::error_type &, handle_type&) override {}  // NOLINT
    void getVolumeMetadataResp (const resp_api_type::error_type &, handle_type&, shared_meta_type&) override {}  // NOLINT
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTOPERATIONS_H_
