/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_

#include <memory>
#include <thread>
#include <boost/shared_ptr.hpp>

#include "connector/block/common.h"

namespace std
{
struct thread;
}  // namespace boost

namespace fds {

struct OmConfigApi;

class NbdConnector {
    uint32_t nbdPort;
    int32_t nbdSocket {-1};

    std::unique_ptr<ev::io> evIoWatcher;
    std::unique_ptr<std::thread> runThread;
    boost::shared_ptr<OmConfigApi> omConfigApi;

    int createNbdSocket();
    void initialize();
    void deinit();
    void runNbdLoop();
    void nbdAcceptCb(ev::io &watcher, int revents);

  public:
    explicit NbdConnector(boost::shared_ptr<OmConfigApi> omApi);
    ~NbdConnector();
    typedef boost::shared_ptr<NbdConnector> shared_ptr;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_
