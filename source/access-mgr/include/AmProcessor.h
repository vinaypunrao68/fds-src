/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_

#include <memory>
#include <string>

#include "fds_volume.h"

namespace fds {

/**
 * Forward declarations
 */
struct AmProcessor_impl;
struct AmRequest;
struct AccessMgr;
struct CommonModuleProviderIf;

class AmProcessor : public std::enable_shared_from_this<AmProcessor>
{
    using shutdown_cb_type = std::function<void(void)>;
  public:
    AmProcessor(AccessMgr* parent, CommonModuleProviderIf *modProvider);
    AmProcessor(AmProcessor const&) = delete;
    AmProcessor& operator=(AmProcessor const&) = delete;
    ~AmProcessor();

    /**
     * Start AmProcessor and register a callback that will
     * indicate a shutdown sequence and all I/O has drained
     */
    void start();

    /**
     * Asynchronous shutdown initiation.
     */
    void stop();

    /**
     * Synchronous shutdown initiation.
     */
    void stopAndWait();

    void prepareForShutdownMsgRespBindCb(shutdown_cb_type&& cb);

    /**
     * Enqueue a connector request
     */
    void enqueueRequest(AmRequest* amReq);

    void getVolumes(std::vector<VolumeDesc>& volumes);

    bool isShuttingDown() const;

    /**
     * Update volume description
     */
    Error modifyVolumePolicy(const VolumeDesc& vdesc);


    /**
     * Create object/metadata/offset caches for the given volume
     */
    void registerVolume(const VolumeDesc& volDesc);

    /**
     * Remove object/metadata/offset caches for the given volume
     */
    void removeVolume(const VolumeDesc& volDesc);

    /**
     * DMT/DLT table updates
     */
    Error updateDlt(bool dlt_type, std::string& dlt_data, std::function<void (const Error&)> const& cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data, std::function<void (const Error&)> const& cb);

    /**
     * True when DLT and DMT have been received
     */
    bool haveTables();

    /**
     * Update QoS' rate and throttle
     */
    Error updateQoS(long int const* rate, float const* throttle);

  private:
    std::unique_ptr<AmProcessor_impl> _impl;
};


}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
