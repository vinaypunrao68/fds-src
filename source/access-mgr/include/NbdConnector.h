/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_

#include <string>
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
    std::string volumeName;

    static constexpr fds_int64_t NBD_MAGIC = 0x49484156454F5054l;
    static constexpr char NBD_MAGIC_PWD[] {'N', 'B', 'D', 'M', 'A', 'G', 'I', 'C'};  // NOLINT
    static constexpr fds_uint16_t NBD_PROTO_VERSION = 1;
    static constexpr fds_uint32_t NBD_ACK = 0;
    static constexpr fds_int32_t NBD_OPT_EXPORT = 1;
    static constexpr fds_int32_t NBD_FLAG_HAS_FLAGS  = 0x1 << 0;
    static constexpr fds_int32_t NBD_FLAG_READ_ONLY  = 0x1 << 1;
    static constexpr fds_int32_t NBD_FLAG_SEND_FLUSH = 0x1 << 2;
    static constexpr fds_int32_t NBD_FLAG_SEND_FUA   = 0x1 << 3;
    static constexpr fds_int32_t NBD_FLAG_ROTATIONAL = 0x1 << 4;
    static constexpr fds_int32_t NBD_FLAG_SEND_TRIM  = 0x1 << 5;
    static constexpr char NBD_PAD_ZERO[124] {0};  // NOLINT

    // TODO(Andrew): This is a total hack. Go ask OM you lazy...
    static constexpr fds_uint64_t volumeSizeInBytes = 10737418240;
    static constexpr fds_uint32_t maxObjectSizeInBytes = 4096;

    std::unique_ptr<ev::io> ioWatcher;

    void callback(ev::io &watcher, int revents);

    enum NbdHandshakeState {
        INVALID   = 0,
        PREINIT   = 1,
        POSTINIT  = 2,
        AWAITOPTS = 3,
        SENDOPTS  = 4,
        DOOPS     = 5
    };
    NbdHandshakeState hsState;

    void hsPreInit(ev::io &watcher);
    void hsPostInit(ev::io &watcher);
    void hsAwaitOpts(ev::io &watcher);
    void hsSendOpts(ev::io &watcher);
    void doOps(ev::io &watcher);

  public:
    explicit NbdConnection(int clientsd);
    ~NbdConnection();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_
