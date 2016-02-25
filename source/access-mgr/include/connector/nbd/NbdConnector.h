/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_NBD_NBDCONNECTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_NBD_NBDCONNECTOR_H_

#include <memory>

#include "connector/nbd/common.h"
#include "concurrency/LeaderFollower.h"

#include "fds_volume.h"

namespace fds {

struct AmProcessor;
struct NbdConnection;

struct NbdConnector
    : public LeaderFollower
{
    NbdConnector(NbdConnector const& rhs) = delete;
    NbdConnector& operator=(NbdConnector const& rhs) = delete;
    ~NbdConnector() = default;

    static void start(std::weak_ptr<AmProcessor> processor);
    static void stop();
    static void volumeAdded(VolumeDesc const& volDesc) {}
    static void volumeRemoved(VolumeDesc const& volDesc) {}

    void deviceDone(int const socket);

 protected:
    void lead() override;

 private:
    uint32_t nbdPort;
    int32_t nbdSocket {-1};
    bool cfg_no_delay {true};
    uint32_t cfg_keep_alive {0};

    template<typename T>
    using unique = std::unique_ptr<T>;
    using connection_ptr = unique<NbdConnection>;

    using map_type = std::map<int, connection_ptr>;

    static std::unique_ptr<NbdConnector> instance_;

    bool stopping {false};

    std::mutex connection_lock;
    map_type connection_map;

    std::shared_ptr<ev::dynamic_loop> evLoop;
    std::unique_ptr<ev::io> evIoWatcher;
    std::unique_ptr<ev::async> asyncWatcher;
    std::weak_ptr<AmProcessor> amProcessor;

    NbdConnector(std::weak_ptr<AmProcessor> processor,
                 size_t const followers);

    int createNbdSocket();
    void configureSocket(int fd) const;
    void initialize();
    void reset();
    void nbdAcceptCb(ev::io &watcher, int revents);
    void startShutdown();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_NBD_NBDCONNECTOR_H_
