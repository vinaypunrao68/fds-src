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

namespace fds
{

struct AmProcessor;
struct ScstConnector;

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
    void readWriteResp(ScstResponseVector* response) override;
    void attachResp(Error const& error, boost::shared_ptr<VolumeDesc> const& volDesc) override;
    void terminate() override;

  private:
    template<typename T>
    using unique = std::unique_ptr<T>;
    using resp_vector_type = unique<iovec[]>;

    bool standalone_mode { false };

    enum class ConnectionState { RUNNING,
                                 STOPPING,
                                 STOPPED };

     ConnectionState state_ { ConnectionState::RUNNING };

    int scstDev {-1};
    size_t volume_size;
    size_t object_size;

    std::shared_ptr<AmProcessor> amProcessor;
    ScstConnector* scst_server;
    ScstOperations::shared_ptr scstOps;

    size_t resp_needed;

    resp_vector_type response;
    size_t total_blocks;
    ssize_t write_offset;

    boost::lockfree::queue<ScstResponseVector*> readyResponses;
    std::unique_ptr<ScstResponseVector> current_response;

    std::unique_ptr<ev::io> ioWatcher;
    std::unique_ptr<ev::async> asyncWatcher;

    /** Indicates to ev loop if it's safe to handle events on this connection */
    bool processing_ {false};

    int openScst();
    void wakeupCb(ev::async &watcher, int revents);
    void ioEvent(ev::io &watcher, int revents);

    enum class ScstProtoState {
        INVALID   = 0,
        PREINIT   = 1,
        POSTINIT  = 2,
        AWAITOPTS = 3,
        SENDOPTS  = 4,
        DOREQS    = 5
    };

    ScstProtoState scst_state;

    // Handshake State
    bool handshake_start(ev::io &watcher);
    bool handshake_complete(ev::io &watcher);

    // Option Negotiation State
    bool option_request(ev::io &watcher);
    bool option_reply(ev::io &watcher);

    // Data IO State
    bool io_request(ev::io &watcher);
    bool io_reply(ev::io &watcher);

    Error dispatchOp();
    bool write_response();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTION_H_
