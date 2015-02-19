/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_

#include <memory>
#include <boost/shared_ptr.hpp>

#include "connector/block/common.h"

namespace boost
{
struct thread;
}  // namespace boost

namespace fds {

struct OmConfigApi;

class NbdConnector {
  private:
    uint32_t nbdPort;
    int32_t nbdSocket;

    std::unique_ptr<ev::io> evIoWatcher;
    std::shared_ptr<boost::thread> runThread;
    boost::shared_ptr<OmConfigApi> omConfigApi;

    int createNbdSocket();
    void runNbdLoop();
    void nbdAcceptCb(ev::io &watcher, int revents);

  public:
    explicit NbdConnector(boost::shared_ptr<OmConfigApi> omApi);
    ~NbdConnector();
    typedef boost::shared_ptr<NbdConnector> shared_ptr;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_BLOCK_H_
