/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_

#include <fds_types.h>
#include <concurrency/Thread.h>

// Forward declare so we can hide the ev++.h include
// in the cpp file so that it doesn't conflict with
// the libevent headers in Thrift.
namespace ev {
class io;
}  // namespace ev

namespace fds {

class NbdConnector {
  private:
    fds_uint32_t nbdPort;
    fds_int32_t nbdSocket;

    std::unique_ptr<ev::io> evIoWatcher;
    std::shared_ptr<boost::thread> runThread;

    int createNbdSocket();
    void runNbdLoop();
    void nbdAcceptCb(ev::io &watcher, int revents);

  public:
    NbdConnector();
    ~NbdConnector();
    typedef boost::shared_ptr<NbdConnector> shared_ptr;
};

class NbdConnection {
  private:
    int clientSocket;
    static int totalConns;

    std::unique_ptr<ev::io> ioWatcher;

    void callback(ev::io &watcher, int revents);

  public:
    explicit NbdConnection(int clientsd);
    ~NbdConnection();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_
