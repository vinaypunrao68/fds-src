/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_

#include <memory>

#include "connector/block/common.h"

namespace fds {

struct AmProcessor;

class NbdConnector {
    uint32_t nbdPort;
    int32_t nbdSocket {-1};
    bool cfg_no_delay {true};
    uint32_t cfg_keep_alive {0};

    std::unique_ptr<ev::io> evIoWatcher;
    std::unique_ptr<ev::async> asyncWatcher;
    std::weak_ptr<AmProcessor> amProcessor;

    int createNbdSocket();
    void configureSocket(int fd) const;
    void initialize();
    void reset();
    void runNbdLoop();
    void nbdAcceptCb(ev::io &watcher, int revents);

    explicit NbdConnector(std::weak_ptr<AmProcessor> processor);
    NbdConnector(NbdConnector const& rhs) = delete;
    NbdConnector& operator=(NbdConnector const& rhs) = delete;

    static std::unique_ptr<NbdConnector> instance_;

  public:
    ~NbdConnector() = default;

    static void start(std::weak_ptr<AmProcessor> processor);
    static void stop();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_
