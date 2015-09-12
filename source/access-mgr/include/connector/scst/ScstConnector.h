/*
 * Copyright 2015 by Formation Data Systems, Inc.
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

    std::shared_ptr<ev::dynamic_loop> evLoop;
    std::weak_ptr<AmProcessor> amProcessor;

    ScstConnector(std::weak_ptr<AmProcessor> processor,
                 size_t const followers);
    ScstConnector(ScstConnector const& rhs) = delete;
    ScstConnector& operator=(ScstConnector const& rhs) = delete;

    static std::unique_ptr<ScstConnector> instance_;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_
