/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_

#include <memory>

#include "connector/block/common.h"

namespace std
{
struct thread;
}  // namespace std

namespace fds {

struct AmProcessor;

class NbdConnector {
    uint32_t nbdPort;
    int32_t nbdSocket {-1};
    bool cfg_non_blocking_io {true};
    bool cfg_keep_alive {true};

    std::unique_ptr<ev::io> evIoWatcher;
    std::unique_ptr<std::thread> runThread;
    std::weak_ptr<AmProcessor> amProcessor;

    int createNbdSocket();
    void configureSocket(int fd) const;
    void initialize();
    void runNbdLoop();
    void nbdAcceptCb(ev::io &watcher, int revents);

  public:
    explicit NbdConnector(std::weak_ptr<AmProcessor> processor);
    ~NbdConnector();

    void stop();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_
