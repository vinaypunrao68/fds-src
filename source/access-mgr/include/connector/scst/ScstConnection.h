/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTION_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTION_H_

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/queue.hpp>

#include "fds_types.h"
#include "connector/scst/common.h"
#include "connector/scst/ScstOperations.h"

struct scst_user_get_cmd;

namespace fds
{

struct AmProcessor;
struct ScstConnector;
struct ScstTask;

struct ScstConnection : public ScstOperationsResponseIface {
    ScstConnection(std::string const& vol_name,
                   ScstConnector* server,
                   std::shared_ptr<ev::dynamic_loop> loop,
                   std::shared_ptr<AmProcessor> processor);
    ScstConnection(ScstConnection const& rhs) = delete;
    ScstConnection(ScstConnection const&& rhs) = delete;
    ScstConnection operator=(ScstConnection const& rhs) = delete;
    ScstConnection operator=(ScstConnection const&& rhs) = delete;
    ~ScstConnection();

    // implementation of ScstOperationsResponseIface
    void respondTask(ScstTask* response) override;
    void attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) override;
    void terminate() override;

  private:
    template<typename T>
    using unique = std::unique_ptr<T>;
    using cmd_type = unique<scst_user_get_cmd>;

    bool standalone_mode { false };

    enum class ConnectionState { RUNNING,
                                 STOPPING,
                                 STOPPED };

     ConnectionState state_ { ConnectionState::RUNNING };

    std::string volumeName;
    int scstDev {-1};
    size_t volume_size;

    std::shared_ptr<AmProcessor> amProcessor;
    ScstConnector* scst_server;
    ScstOperations::shared_ptr scstOps;

    size_t resp_needed;

    cmd_type cmd;
    uint32_t logical_block_size;
    uint32_t physical_block_size {0};

    boost::lockfree::queue<ScstTask*> readyResponses;
    std::unordered_map<uint32_t, unique<ScstTask>> repliedResponses;

    unique<ev::io> ioWatcher;
    unique<ev::async> asyncWatcher;

    /** Indicates to ev loop if it's safe to handle events on this connection */
    bool processing_ {false};

    int openScst();
    void wakeupCb(ev::async &watcher, int revents);
    void ioEvent(ev::io &watcher, int revents);
    bool getAndRespond();

    void execAllocCmd();
    void execMemFree();
    void execSessionCmd();
    void execUserCmd();
    void execCompleteCmd();
    void execTaskMgmtCmd();
    void execParseCmd();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTION_H_
