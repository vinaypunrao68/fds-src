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

    std::string targetName() const { return target_name; }

 protected:
    void lead() override;

 private:
    std::shared_ptr<ev::dynamic_loop> evLoop;
    std::weak_ptr<AmProcessor> amProcessor;

    ScstConnector(std::string const& name,
                  std::weak_ptr<AmProcessor> processor,
                  size_t const followers);
    ScstConnector(ScstConnector const& rhs) = delete;
    ScstConnector& operator=(ScstConnector const& rhs) = delete;

    bool initializeTarget();

    static std::unique_ptr<ScstConnector> instance_;

    std::string target_name;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_
