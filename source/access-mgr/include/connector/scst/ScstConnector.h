/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_

#include <memory>

#include "connector/scst/common.h"
#include "concurrency/LeaderFollower.h"

namespace fds {

struct AmProcessor;

struct ScstConnector
    : public LeaderFollower
{
    ~ScstConnector() = default;

    static void start(std::weak_ptr<AmProcessor> processor);
    static void stop();
 protected:

    void lead() override;

 private:
    int32_t scstDev {-1};
    bool cfg_no_delay {true};
    uint32_t cfg_keep_alive {0};

    std::unique_ptr<ev::dynamic_loop> evLoop;
    std::unique_ptr<ev::io> evIoWatcher;
    std::unique_ptr<ev::async> asyncWatcher;
    std::weak_ptr<AmProcessor> amProcessor;

    int createScstSocket();
    void configureSocket(int fd) const;
    void initialize();
    void reset();
    void scstAcceptCb(ev::io &watcher, int revents);

    ScstConnector(std::weak_ptr<AmProcessor> processor,
                 size_t const followers);
    ScstConnector(ScstConnector const& rhs) = delete;
    ScstConnector& operator=(ScstConnector const& rhs) = delete;

    static std::unique_ptr<ScstConnector> instance_;

  public:
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_
