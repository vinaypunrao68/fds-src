/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <set>
#include <string>
#include <type_traits>
#include <NbdConnector.h>
#include <fds_process.h>

extern "C" {
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/uio.h>
}

#include <ev++.h>

template<typename T>
constexpr auto to_iovec(T* t) -> typename std::remove_cv<T>::type*
{ return const_cast<typename std::remove_cv<T>::type*>(t); }

namespace fds {

template<typename M>
bool get_message_header(int fd, M& message);

template<typename M>
bool get_message_payload(int fd, M& message);

NbdConnector::NbdConnector(AmAsyncDataApi::shared_ptr api,
                           OmConfigApi::shared_ptr omApi)
        : asyncDataApi(api),
          omConfigApi(omApi),
          nbdPort(4444) {
    // Bind to NBD listen port
    nbdSocket = createNbdSocket();
    fds_verify(nbdSocket > 0);

    // Setup event loop
    evIoWatcher = std::unique_ptr<ev::io>(new ev::io());
    evIoWatcher->set<NbdConnector, &NbdConnector::nbdAcceptCb>(this);
    evIoWatcher->start(nbdSocket, ev::READ);

    // Run event loop in thread
    runThread.reset(new boost::thread(
        boost::bind(&NbdConnector::runNbdLoop, this)));
}

NbdConnector::~NbdConnector() {
    shutdown(nbdSocket, SHUT_RDWR);
    close(nbdSocket);
}

void
NbdConnector::nbdAcceptCb(ev::io &watcher, int revents) {
    if (EV_ERROR & revents) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Accept a new NBD client connection
    int clientsd = accept(watcher.fd,
                          (struct sockaddr *)&client_addr,
                          &client_len);

    if (clientsd < 0) {
        GLOGERROR << "Accept error";
        return;
    }

    // Create a handler for this NBD connection
    // Will delete itself when connection dies
    LOGNORMAL << "Creating client connection...";
    NbdConnection *client = new NbdConnection(asyncDataApi, omConfigApi, clientsd);
    LOGNORMAL << "Created client connection...";
}

int
NbdConnector::createNbdSocket() {
    int listenfd;
    struct sockaddr_in serv_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        LOGERROR << "Failed to create NBD socket";
        return listenfd;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(nbdPort);

    fds_verify(bind(listenfd,
                    (struct sockaddr*)&serv_addr,
                    sizeof(serv_addr)) == 0);
    fcntl(listenfd, F_SETFL, fcntl(listenfd, F_GETFL, 0) | O_NONBLOCK);
    listen(listenfd, 10);

    return listenfd;
}

void
NbdConnector::runNbdLoop() {
    LOGNOTIFY << "Accepting NBD connections on port " << nbdPort;
    ev::default_loop loop;
    loop.run(0);
    LOGNOTIFY << "Stopping NBD loop...";
}

NbdConnection::NbdConnection(AmAsyncDataApi::shared_ptr api,
                             OmConfigApi::shared_ptr omApi,
                             int clientsd)
        : asyncDataApi(api),
          omConfigApi(omApi),
          nbdOps(new NbdOperations(asyncDataApi, this)),
          clientSocket(clientsd),
          hsState(PREINIT),
          doUturn(false),
          attach({ { 0x00ull, 0x00u, 0x00u }, 0x00ull, 0x00ull, { 0x00 } }),
          request({ { 0x00u, 0x00u, 0x00ull, 0x00ull, 0x00u }, 0x00ull, 0x00ull, { 0x00 } }),
          maxChunks(0),
          readyHandles(2000),
          readyResponses(4000) {
    fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK);
    FdsConfigAccessor config(g_fdsprocess->get_conf_helper());
    toggleStandAlone = config.get_abs<bool>("fds.am.testing.toggleStandAlone");

    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set<NbdConnection, &NbdConnection::callback>(this);
    ioWatcher->start(clientSocket, ev::READ | ev::WRITE);

    asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
    asyncWatcher->set<NbdConnection, &NbdConnection::wakeupCb>(this);
    asyncWatcher->start();

    LOGNORMAL << "New NBD client connection for " << clientSocket;
}

NbdConnection::~NbdConnection() {
}

constexpr fds_int64_t NbdConnection::NBD_MAGIC;
constexpr char NbdConnection::NBD_MAGIC_PWD[];
constexpr fds_uint16_t NbdConnection::NBD_PROTO_VERSION;
constexpr fds_uint32_t NbdConnection::NBD_ACK;
constexpr fds_int32_t NbdConnection::NBD_OPT_EXPORT;
constexpr fds_int32_t NbdConnection::NBD_FLAG_HAS_FLAGS;
constexpr fds_int32_t NbdConnection::NBD_FLAG_READ_ONLY;
constexpr fds_int32_t NbdConnection::NBD_FLAG_SEND_FLUSH;
constexpr fds_int32_t NbdConnection::NBD_FLAG_SEND_FUA;
constexpr fds_int32_t NbdConnection::NBD_FLAG_ROTATIONAL;
constexpr fds_int32_t NbdConnection::NBD_FLAG_SEND_TRIM;
constexpr char NbdConnection::NBD_PAD_ZERO[];
constexpr fds_int32_t NbdConnection::NBD_REQUEST_MAGIC;
constexpr fds_int32_t NbdConnection::NBD_RESPONSE_MAGIC;
constexpr fds_int32_t NbdConnection::NBD_CMD_READ;
constexpr fds_int32_t NbdConnection::NBD_CMD_WRITE;
constexpr fds_int32_t NbdConnection::NBD_CMD_DISC;
constexpr fds_int32_t NbdConnection::NBD_CMD_FLUSH;
constexpr fds_int32_t NbdConnection::NBD_CMD_TRIM;

constexpr char NbdConnection::fourKayZeros[];

void
NbdConnection::hsPreInit(ev::io &watcher) {
    // Send initial message to client with NBD magic and proto version
    static iovec const vectors[] = {
        { to_iovec(NBD_MAGIC_PWD),       sizeof(NBD_MAGIC_PWD)      },
        { to_iovec(&NBD_MAGIC),          sizeof(NBD_MAGIC)          },
        { to_iovec(&NBD_PROTO_VERSION),  sizeof(NBD_PROTO_VERSION)  },
    };

    ssize_t nwritten = writev(watcher.fd, vectors, std::extent<decltype(vectors)>::value);
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
}

void
NbdConnection::hsPostInit(ev::io &watcher) {
    // Accept client ack
    fds_uint32_t ack;
    ssize_t nread = read(watcher.fd, &ack, sizeof(ack));
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        ack = ntohl(ack);
        fds_verify(NBD_ACK == ack);
        LOGDEBUG << "Received " << nread << " byte ack " << ack;
    }
}

bool
NbdConnection::hsAwaitOpts(ev::io &watcher) {
    if (attach.header_off >= 0) {
        if (!get_message_header(watcher.fd, attach))
            return false;
        attach.header.magic = __builtin_bswap64(attach.header.magic);
        fds_verify(NBD_MAGIC == attach.header.magic);
        attach.header.optSpec = ntohl(attach.header.optSpec);
        fds_verify(NBD_OPT_EXPORT == attach.header.optSpec);
        attach.header.length = ntohl(attach.header.length);

        // Just for sanities sake and protect against bad data
        fds_verify(attach.data.size() >= static_cast<size_t>(attach.header.length));
    }
    if (!get_message_payload(watcher.fd, attach))
        return false;

    // In case volume name is not NULL terminated.
    volumeName = boost::make_shared<std::string>(attach.data.begin(),
                                                 attach.data.begin() + attach.header.length);
    if (toggleStandAlone) {
        volDesc.policy.maxObjectSizeInBytes = 4096;
        volDesc.policy.blockDeviceSizeInBytes = 10737418240;
    } else {
        LOGCRITICAL << "Will stat volume " << *volumeName;
        Error err = omConfigApi->statVolume(volumeName, volDesc);
        fds_verify(ERR_OK == err);
        fds_verify(apis::BLOCK == volDesc.policy.volumeType);
    }
    maxChunks = (2 * 1024 * 1024) / volDesc.policy.maxObjectSizeInBytes;
    LOGNORMAL << "Attaching volume name " << *volumeName << " of size "
              << volDesc.policy.blockDeviceSizeInBytes
              << " max object size " << volDesc.policy.maxObjectSizeInBytes
              << " max number of chunks " << maxChunks;
    return true;
}

void
NbdConnection::hsSendOpts(ev::io &watcher) {
    static fds_int16_t const optFlags = NBD_FLAG_HAS_FLAGS|NBD_FLAG_SEND_FLUSH|NBD_FLAG_SEND_FUA;
    static iovec const vectors[] = {
        { to_iovec(&volDesc.policy.blockDeviceSizeInBytes),
                                         sizeof(volDesc.policy.blockDeviceSizeInBytes) },
        { to_iovec(&optFlags),           sizeof(NBD_MAGIC) },
        { to_iovec(&NBD_PROTO_VERSION),  sizeof(NBD_PROTO_VERSION) },
    };

    ssize_t nwritten = writev(watcher.fd, vectors, std::extent<decltype(vectors)>::value);
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
}

void
NbdConnection::hsReq(ev::io &watcher) {
    if (request.header_off >= 0) {
        if (!get_message_header(watcher.fd, request))
            return;
        request.header.magic = ntohl(request.header.magic);
        fds_verify(NBD_REQUEST_MAGIC == request.header.magic);
        request.header.opType = ntohl(request.header.opType);
        request.header.handle = __builtin_bswap64(request.header.handle);
        request.header.offset = __builtin_bswap64(request.header.offset);
        request.header.length = ntohl(request.header.length);

        LOGTRACE << " magic 0x" << std::hex << request.header.magic << std::dec << std::endl
                 << " op " << request.header.opType << std::endl
                 << " handle 0x" << std::hex << request.header.handle << std::dec << std::endl
                 << " offset " << request.header.offset << std::endl
                 << " length " << request.header.length;
    }

    if (NBD_CMD_WRITE == request.header.opType) {
        if (!get_message_payload(watcher.fd, request))
            return;
    }
    request.header_off = 0;
    request.data_off = -1;

    Error err = dispatchOp(watcher,
                           request.header.opType,
                           request.header.handle,
                           request.header.offset,
                           request.header.length,
                           request.data.data());
    fds_verify(ERR_OK == err);
    return;
}

void
NbdConnection::hsReply(ev::io &watcher) {
    static fds_int32_t magic = htonl(NBD_RESPONSE_MAGIC);
    fds_int32_t error = htonl(0);

    if (doUturn) {
        // Iterate and send each ready response
        while (!readyHandles.empty()) {
            UturnPair rep;
            fds_verify(readyHandles.pop(rep));

            fds_int64_t handle = __builtin_bswap64(rep.handle);

            fds_uint32_t length = rep.length;
            fds_int32_t opType = rep.opType;
            fds_uint32_t chunks = 0;
            if (NBD_CMD_READ == opType) {
                // TODO(Andrew): Remove this verify and handle sub-4K
                fds_verify((length % 4096) == 0);
                chunks = length / 4096;
            }

            // Build iovec for writev call, max size is 3 + 2MiB / 4096 == 515
            iovec vectors[kMaxChunks + 3] = {
                { &magic,  sizeof(magic)  },
                { &error,  sizeof(error)  },
                { &handle, sizeof(handle) },
                { nullptr,              0 },
            };

            for (size_t i = 0; i < chunks; ++i) {
                vectors[3+i].iov_base = to_iovec(fourKayZeros);
                vectors[3+i].iov_len = sizeof(fourKayZeros);
            }

            ssize_t nwritten = writev(watcher.fd, vectors, chunks + 3);
            if (nwritten < 0) {
                LOGERROR << "Socket write error";
                return;
            }
            LOGTRACE << "Wrote " << nwritten << " bytes of zeros";
        }
        LOGTRACE << "No more handles to respond to";
        return;
    }

    // no uturn
    while (!readyResponses.empty()) {
        NbdResponseVector* resp = NULL;
        fds_verify(readyResponses.pop(resp));

        fds_int64_t handle = __builtin_bswap64(resp->getHandle());

        // Build iovec for writev call, max size is 3 + 2MiB / 4096 == 515
        iovec vectors[kMaxChunks + 3] = {
            { &magic,  sizeof(magic)  },
            { &error,  sizeof(error)  },
            { &handle, sizeof(handle) },
            { nullptr,              0 },
        };

        fds_uint32_t context = 0;
        size_t cnt = 0;
        if (resp->isRead()) {
            boost::shared_ptr<std::string> buf = resp->getNextReadBuffer(context);
            while (buf != NULL) {
                GLOGDEBUG << "Handle " << handle << "....Buffer # " << context;
                vectors[3+cnt].iov_base = to_iovec(buf->c_str());
                vectors[3+cnt].iov_len = buf->length();
                ++cnt;
                // get next buffer
                buf = resp->getNextReadBuffer(context);
            }
        }
        ssize_t nwritten = writev(watcher.fd, vectors, cnt + 3);
        if (nwritten < 0) {
            LOGERROR << "Socket write error";
            return;
        }
        LOGTRACE << "Wrote " << nwritten << " bytes of response";

        // cleanup
        delete resp;
    }
    LOGTRACE << "No more reponses to send";
}

Error
NbdConnection::dispatchOp(ev::io &watcher,
                          fds_uint32_t opType,
                          fds_int64_t handle,
                          fds_uint64_t offset,
                          fds_uint32_t length,
                          char* data) {
    switch (opType) {
        UturnPair utPair;
        case NBD_CMD_READ:
            if (doUturn) {
                // TODO(Andrew): Hackey uturn code. Remove.
                utPair.handle = handle;
                utPair.length = length;
                utPair.opType = opType;
                readyHandles.push(utPair);

                // We have something to write, so ask for events
                ioWatcher->set(ev::READ | ev::WRITE);
            } else {
                // do read from AM
                nbdOps->read(volumeName, volDesc.policy.maxObjectSizeInBytes,
                             length, offset, handle);
            }
            break;
        case NBD_CMD_WRITE:
            if (doUturn) {
                // TODO(Andrew): Hackey uturn code. Remove.
                utPair.handle = handle;
                utPair.length = length;
                utPair.opType = opType;
                readyHandles.push(utPair);

                // We have something to write, so ask for events
                ioWatcher->set(ev::READ | ev::WRITE);
            } else {
                 fds_verify(data != NULL);
                 boost::shared_ptr<std::string> buf(new std::string(data, length));
                 nbdOps->write(volumeName, volDesc.policy.maxObjectSizeInBytes,
                               buf, length, offset, handle);
            }
            break;
        case NBD_CMD_FLUSH:
            break;
        case NBD_CMD_DISC:
            LOGNORMAL << "Got a disconnect";
            ioWatcher->stop();
            close(clientSocket);
            // TODO(Andrew): Who's gonna delete me?
            break;
        default:
            fds_panic("Unknown NBD op %d", opType);
    }
    return ERR_OK;
}

void
NbdConnection::wakeupCb(ev::async &watcher, int revents) {
    ioWatcher->set(ev::READ | ev::WRITE);
    ioWatcher->feed_event(EV_WRITE);
}

void
NbdConnection::callback(ev::io &watcher, int revents) {
    if (EV_ERROR & revents) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    if (revents & EV_READ) {
        switch (hsState) {
            case POSTINIT:
                hsPostInit(watcher);
                hsState = AWAITOPTS;
                break;
            case AWAITOPTS:
                if (hsAwaitOpts(watcher)) {
                    hsState = SENDOPTS;
                    ioWatcher->set(ev::READ | ev::WRITE);
                }
                break;
            case DOREQS:
                hsReq(watcher);
                break;
            default:
                fds_panic("Unknown NBD connection state %d", hsState);
        }
    }

    if (revents & EV_WRITE) {
        switch (hsState) {
            case PREINIT:
                hsPreInit(watcher);
                hsState = POSTINIT;
                // Wait for read event from client
                ioWatcher->set(ev::READ);
                break;
            case SENDOPTS:
                hsSendOpts(watcher);
                hsState = DOREQS;
                LOGNORMAL << "Done with NBD handshake";
                // Wait for read events from client
                ioWatcher->set(ev::READ);
                break;
            case DOREQS:
                hsReply(watcher);
                ioWatcher->set(ev::READ);
                break;
            default:
                fds_panic("Unknown NBD connection state %d", hsState);
        }
    }
}

void
NbdConnection::readWriteResp(const Error& error,
                             fds_int64_t handle,
                             NbdResponseVector* response) {
    LOGNORMAL << "Read? " << response->isRead() << " (false is write)"
              << " response from NbdOperations handle " << handle
              << " " << error;

    // add to quueue
    readyResponses.push(response);

    // We have something to write, so poke the loop
    asyncWatcher->send();
}

template<typename M>
bool get_message_header(int fd, M& message) {
    fds_assert(message.header_off >= 0);
    ssize_t to_read = sizeof(typename M::header_type) - message.header_off;
    ssize_t nread = read(fd,
                         reinterpret_cast<uint8_t*>(&message.header) + message.header_off,
                         to_read);
    if (nread < 0) {
        LOGWARN << "Socket read error";
        return false;
    } else if (nread < to_read) {
        LOGDEBUG << "Short read : [ " << std::dec << nread << " of " << to_read << "]";
        message.header_off += nread;
        return false;
    }
    LOGTRACE << "Read " << nread << " bytes of header";
    message.header_off = -1;
    message.data_off = 0;
    return true;
}

template<typename M>
bool get_message_payload(int fd, M& message) {
    fds_assert(message.data_off >= 0);
    ssize_t to_read = message.header.length - message.data_off;
    ssize_t nread = read(fd,
                         message.data.data() + message.data_off,
                         to_read);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return false;
    } else if (nread < to_read) {
        LOGDEBUG << "Short read : [ " << std::dec << nread << " of " << to_read << "]";
        message.data_off += nread;
        return false;
    }
    LOGTRACE << "Read " << nread << " bytes of data";
    return true;
}

}  // namespace fds
