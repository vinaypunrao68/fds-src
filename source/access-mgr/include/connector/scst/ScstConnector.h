/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_

#include <memory>

#include "connector/scst/common.h"

namespace fds {

struct AmProcessor;
struct ScstTarget;

struct ScstConnector
{
    ScstConnector(ScstConnector const& rhs) = delete;
    ScstConnector& operator=(ScstConnector const& rhs) = delete;
    ~ScstConnector() = default;

    static void start(std::weak_ptr<AmProcessor> processor);
    static void stop();

    std::string targetPrefix() const { return target_prefix; }

 private:
    template<typename T>
    using unique = std::unique_ptr<T>;

    static unique<ScstConnector> instance_;

    ScstConnector(std::string const& prefix,
                  std::weak_ptr<AmProcessor> processor);

    std::weak_ptr<AmProcessor> amProcessor;

    std::string target_prefix;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_
