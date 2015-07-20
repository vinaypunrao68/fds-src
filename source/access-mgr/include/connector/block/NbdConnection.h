/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTION_H_
#define SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTION_H_

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/queue.hpp>

#include "fds_types.h"
#include "connector/block/common.h"
#include "connector/block/NbdOperations.h"

namespace fds
{

struct AmProcessor;
struct NbdConnector;

#pragma pack(push)
#pragma pack(1)
struct attach_header {
    uint8_t magic[8];
    fds_int32_t optSpec, length;
};

struct handshake_header {
    uint32_t ack;
};

struct request_header {
    uint8_t magic[4];
    fds_int32_t opType;
    fds_int64_t handle;
    fds_int64_t offset;
    fds_int32_t length;
};
#pragma pack(pop)

template<typename H, typename D>
struct message {
    typedef D data_type;
    typedef H header_type;
    header_type header;
    ssize_t header_off, data_off;
    data_type data;
};

struct NbdConnection : public NbdOperationsResponseIface {
    NbdConnection(NbdConnector* server,
                  int clientsd,
                  std::shared_ptr<AmProcessor> processor);
    NbdConnection(NbdConnection const& rhs) = delete;
    NbdConnection(NbdConnection const&& rhs) = delete;
    NbdConnection operator=(NbdConnection const& rhs) = delete;
    NbdConnection operator=(NbdConnection const&& rhs) = delete;
    ~NbdConnection();

    // implementation of NbdOperationsResponseIface
    void readWriteResp(NbdResponseVector* response) override;
    void attachResp(Error const& error, boost::shared_ptr<VolumeDesc> const& volDesc) override;
    void terminate() override;

  private:
    template<typename T>
    using unique = std::unique_ptr<T>;
    using resp_vector_type = unique<iovec[]>;

    bool standalone_mode { false };

    bool terminate_ { false };

    int clientSocket;
    size_t volume_size;
    size_t object_size;

    std::shared_ptr<AmProcessor> amProcessor;
    NbdConnector* nbd_server;
    NbdOperations::shared_ptr nbdOps;

    size_t resp_needed;

    message<attach_header, std::array<char, 1024>> attach;
    message<handshake_header, std::nullptr_t> handshake;
    message<request_header, boost::shared_ptr<std::string>> request;

    resp_vector_type response;
    size_t total_blocks;
    ssize_t write_offset;

    boost::lockfree::queue<NbdResponseVector*> readyResponses;
    std::unique_ptr<NbdResponseVector> current_response;

    std::unique_ptr<ev::io> ioWatcher;
    std::unique_ptr<ev::async> asyncWatcher;

    /** Indicates to ev loop if it's safe to handle events on this connection */
    bool processing_ {false};

    void wakeupCb(ev::async &watcher, int revents);
    void ioEvent(ev::io &watcher, int revents);

    enum class NbdProtoState {
        INVALID   = 0,
        PREINIT   = 1,
        POSTINIT  = 2,
        AWAITOPTS = 3,
        SENDOPTS  = 4,
        DOREQS    = 5
    };

    NbdProtoState nbd_state;

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

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTION_H_
