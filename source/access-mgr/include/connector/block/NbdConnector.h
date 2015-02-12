/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_

#include <array>
#include <utility>
#include <string>
#include <queue>
#include <boost/lockfree/queue.hpp>

#include "connector/block/NbdOperations.h"

#include "fds_types.h"
#include "concurrency/Thread.h"
#include "OmConfigService.h"

// Forward declare so we can hide the ev++.h include
// in the cpp file so that it doesn't conflict with
// the libevent headers in Thrift.
namespace ev {
class io;
class async;
}  // namespace ev

namespace fds {

#pragma pack(push)
#pragma pack(1)
struct attach_header {
    uint8_t magic[8];
    fds_int32_t optSpec, length;
};

struct request_header {
    uint8_t magic[4];
    fds_int32_t opType;
    fds_int64_t handle;
    fds_int64_t offset;
    fds_int32_t length;
};
#pragma pack(pop)

template<typename H, typename D>
struct message {
    typedef D data_type;
    typedef H header_type;
    header_type header;
    ssize_t header_off, data_off;
    data_type data;
};

class NbdConnector {
  private:
    fds_uint32_t nbdPort;
    fds_int32_t nbdSocket;

    std::unique_ptr<ev::io> evIoWatcher;
    std::shared_ptr<boost::thread> runThread;
    OmConfigApi::shared_ptr omConfigApi;

    int createNbdSocket();
    void runNbdLoop();
    void nbdAcceptCb(ev::io &watcher, int revents);

  public:
    explicit NbdConnector(OmConfigApi::shared_ptr omApi);
    ~NbdConnector();
    typedef boost::shared_ptr<NbdConnector> shared_ptr;
};

struct NbdConnection : public NbdOperationsResponseIface {
    template<typename T>
    using unique = std::unique_ptr<T>;
    using resp_vector_type = unique<iovec[]>;

    enum Errors {
        connection_closed,
        shutdown_requested,
    };

    NbdConnection(OmConfigApi::shared_ptr omApi, int clientsd);
    ~NbdConnection();

    // implementation of NbdOperationsResponseIface
    void readWriteResp(NbdResponseVector* response);

  private:
    int clientSocket;
    apis::VolumeDescriptor volDesc;
    size_t volume_size;

    OmConfigApi::shared_ptr omConfigApi;
    NbdOperations::shared_ptr nbdOps;
    size_t maxChunks;
    fds_bool_t toggleStandAlone;
    fds_bool_t doUturn;

    std::atomic_uint resp_needed;

    message<attach_header, std::array<char, 1024>> attach;
    message<request_header, boost::shared_ptr<std::string>> request;

    resp_vector_type response;
    size_t total_blocks;
    ssize_t write_offset;

    // Uturn stuff. Remove me.
    struct UturnPair {
        fds_int64_t handle;
        fds_uint32_t length;
        fds_int32_t opType;
    };
    boost::lockfree::queue<UturnPair> readyHandles;
    boost::lockfree::queue<NbdResponseVector*> readyResponses;
    std::unique_ptr<NbdResponseVector> current_response;

    static constexpr uint8_t NBD_MAGIC[] = { 0x49, 0x48, 0x41, 0x56, 0x45, 0x4F, 0x50, 0x54 };
    static constexpr uint8_t NBD_REQUEST_MAGIC[] = { 0x25, 0x60, 0x95, 0x13 };
    static constexpr uint8_t NBD_RESPONSE_MAGIC[] = { 0x67, 0x44, 0x66, 0x98 };
    static constexpr char NBD_MAGIC_PWD[] {'N', 'B', 'D', 'M', 'A', 'G', 'I', 'C'};  // NOLINT
    static constexpr uint8_t NBD_PROTO_VERSION[] = { 0x00, 0x01 };
    static constexpr fds_int32_t NBD_OPT_EXPORT = 1;
    static constexpr fds_int16_t NBD_FLAG_HAS_FLAGS  = 0x1 << 0;
    static constexpr fds_int16_t NBD_FLAG_READ_ONLY  = 0x1 << 1;
    static constexpr fds_int16_t NBD_FLAG_SEND_FLUSH = 0x1 << 2;
    static constexpr fds_int16_t NBD_FLAG_SEND_FUA   = 0x1 << 3;
    static constexpr fds_int16_t NBD_FLAG_ROTATIONAL = 0x1 << 4;
    static constexpr fds_int16_t NBD_FLAG_SEND_TRIM  = 0x1 << 5;
    static constexpr fds_int32_t NBD_CMD_READ = 0;
    static constexpr fds_int32_t NBD_CMD_WRITE = 1;
    static constexpr fds_int32_t NBD_CMD_DISC = 2;
    static constexpr fds_int32_t NBD_CMD_FLUSH = 3;
    static constexpr fds_int32_t NBD_CMD_TRIM = 4;

    // TODO(Andrew): This is a total hack. Go ask OM you lazy...
    static constexpr fds_uint32_t minMaxObjectSizeInBytes = 4096;
    static constexpr char fourKayZeros[4096]{0};  // NOLINT
    static constexpr size_t kMaxChunks = (2 * 1024 * 1024) / minMaxObjectSizeInBytes;

    static void ensure(bool b) { if (!b) throw connection_closed; }

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

    bool hsPreInit(ev::io &watcher);
    void hsPostInit(ev::io &watcher);
    bool hsAwaitOpts(ev::io &watcher);
    bool hsSendOpts(ev::io &watcher);
    void hsReq(ev::io &watcher);
    bool hsReply(ev::io &watcher);
    Error dispatchOp(ev::io &watcher,
                     fds_uint32_t opType,
                     fds_int64_t handle,
                     fds_uint64_t offset,
                     fds_uint32_t length,
                     boost::shared_ptr<std::string> data);

    bool write_response();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_NBDCONNECTOR_H_
