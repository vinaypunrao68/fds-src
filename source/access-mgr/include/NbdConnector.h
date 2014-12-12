/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_

#include <utility>
#include <string>
#include <fds_types.h>
#include <concurrency/Thread.h>
#include <AmAsyncDataApi.h>
#include <OmConfigService.h>
#include <NbdOperations.h>
#include <queue>
#include <boost/lockfree/queue.hpp>

// Forward declare so we can hide the ev++.h include
// in the cpp file so that it doesn't conflict with
// the libevent headers in Thrift.
namespace ev {
class io;
class async;
}  // namespace ev

namespace fds {

class NbdConnector {
  private:
    fds_uint32_t nbdPort;
    fds_int32_t nbdSocket;

    std::unique_ptr<ev::io> evIoWatcher;
    std::shared_ptr<boost::thread> runThread;
    AmAsyncDataApi::shared_ptr asyncDataApi;
    OmConfigApi::shared_ptr omConfigApi;

    int createNbdSocket();
    void runNbdLoop();
    void nbdAcceptCb(ev::io &watcher, int revents);

  public:
    NbdConnector(AmAsyncDataApi::shared_ptr api,
                 OmConfigApi::shared_ptr omApi);
    ~NbdConnector();
    typedef boost::shared_ptr<NbdConnector> shared_ptr;
};

class NbdConnection : public NbdOperationsResponseIface {
  private:
    int clientSocket;
    boost::shared_ptr<std::string> volumeName;
    apis::VolumeDescriptor volDesc;
    AmAsyncDataApi::shared_ptr asyncDataApi;
    OmConfigApi::shared_ptr omConfigApi;
    NbdOperations::shared_ptr nbdOps;
    fds_bool_t doUturn;

    // Uturn stuff. Remove me.
    struct UturnPair {
        fds_int64_t handle;
        fds_uint32_t length;
        fds_int32_t opType;
    };
    boost::lockfree::queue<UturnPair> readyHandles;
    boost::lockfree::queue<NbdResponseVector*> readyResponses;

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
    static constexpr fds_int32_t NBD_REQUEST_MAGIC = 0x25609513;
    static constexpr fds_int32_t NBD_RESPONSE_MAGIC = 0x67446698;
    static constexpr fds_int32_t NBD_CMD_READ = 0;
    static constexpr fds_int32_t NBD_CMD_WRITE = 1;
    static constexpr fds_int32_t NBD_CMD_DISC = 2;
    static constexpr fds_int32_t NBD_CMD_FLUSH = 3;
    static constexpr fds_int32_t NBD_CMD_TRIM = 4;

    // TODO(Andrew): This is a total hack. Go ask OM you lazy...
    static constexpr fds_uint32_t maxObjectSizeInBytes = 4096;
    static constexpr char fourKayZeros[4096]{0};  // NOLINT
    static constexpr size_t kMaxChunks = (2 * 1024 * 1024) / maxObjectSizeInBytes;

    std::unique_ptr<ev::io> ioWatcher;
    std::unique_ptr<ev::async> asyncWatcher;

    void wakeupCb(ev::async &watcher, int revents);
    void callback(ev::io &watcher, int revents);

    enum NbdHandshakeState {
        INVALID   = 0,
        PREINIT   = 1,
        POSTINIT  = 2,
        AWAITOPTS = 3,
        SENDOPTS  = 4,
        DOREQS    = 5
    };
    NbdHandshakeState hsState;

    void hsPreInit(ev::io &watcher);
    void hsPostInit(ev::io &watcher);
    void hsAwaitOpts(ev::io &watcher);
    void hsSendOpts(ev::io &watcher);
    void hsReq(ev::io &watcher);
    void hsReply(ev::io &watcher);
    Error dispatchOp(ev::io &watcher,
                     fds_uint32_t opType,
                     fds_int64_t handle,
                     fds_uint64_t offset,
                     fds_uint32_t length);

  public:
    NbdConnection(AmAsyncDataApi::shared_ptr api,
                  OmConfigApi::shared_ptr omApi,
                  int clientsd);
    ~NbdConnection();

    // implementation of NbdOperationsResponseIface
    void readResp(const Error& error,
                  fds_int64_t handle,
                  NbdResponseVector* response);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_
