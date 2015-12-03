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

struct NbdConnector
    : public LeaderFollower
{
    ~NbdConnector() = default;

    static void start(std::weak_ptr<AmProcessor> processor);
    static void stop();
    static void volumeAdded(VolumeDesc const& volDesc) {}
    static void volumeRemoved(VolumeDesc const& volDesc) {}
 protected:

    void lead() override;

 private:
    uint32_t nbdPort;
    int32_t nbdSocket {-1};
    bool cfg_no_delay {true};
    uint32_t cfg_keep_alive {0};

    std::shared_ptr<ev::dynamic_loop> evLoop;
    std::unique_ptr<ev::io> evIoWatcher;
    std::unique_ptr<ev::async> asyncWatcher;
    std::weak_ptr<AmProcessor> amProcessor;

    int createNbdSocket();
    void configureSocket(int fd) const;
    void initialize();
    void reset();
    void nbdAcceptCb(ev::io &watcher, int revents);

    NbdConnector(std::weak_ptr<AmProcessor> processor,
                 size_t const followers);
    NbdConnector(NbdConnector const& rhs) = delete;
    NbdConnector& operator=(NbdConnector const& rhs) = delete;

    static std::unique_ptr<NbdConnector> instance_;

  public:
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_NBD_NBDCONNECTOR_H_
