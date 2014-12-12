/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <set>
#include <string>
#include <type_traits>
#include <NbdConnector.h>

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
          maxChunks(0),
          readyHandles(2000),
          readyResponses(4000) {
    fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK);

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

void
NbdConnection::hsAwaitOpts(ev::io &watcher) {
    fds_int64_t magic;
    fds_int32_t optSpec, length;

    iovec const vectors[] = {
        { &magic,   sizeof(magic)   },
        { &optSpec, sizeof(optSpec) },
        { &length,  sizeof(length)  },
    };

    // Read
    ssize_t nread = readv(watcher.fd, vectors, std::extent<decltype(vectors)>::value);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    }

    magic = __builtin_bswap64(magic);
    optSpec = ntohl(optSpec);
    fds_verify(NBD_MAGIC == magic);
    fds_verify(NBD_OPT_EXPORT == optSpec);
    length = ntohl(length);
    fds_verify(length < 4096);  // Just for sanities sake and protect against bad data

    char exportName[length+1];  // NOLINT
    nread = read(watcher.fd, exportName, length);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    }

    exportName[length] = '\0';  // In case volume name is not NULL terminated.
    volumeName = boost::make_shared<std::string>(&exportName[0]);
    Error err = omConfigApi->statVolume(volumeName, volDesc);
    fds_verify(ERR_OK == err);
    fds_verify(apis::BLOCK == volDesc.policy.volumeType);
    maxChunks = (2 * 1024 * 1024) / volDesc.policy.maxObjectSizeInBytes;
    LOGNORMAL << "Attaching volume name " << *volumeName << " of size "
              << volDesc.policy.blockDeviceSizeInBytes
              << " max object size " << volDesc.policy.maxObjectSizeInBytes
              << " max number of chunks " << maxChunks;
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
    fds_int32_t magic, opType, length;
    fds_int64_t handle, offset;

    iovec const vectors[] = {
        { &magic,  sizeof(magic)  },
        { &opType, sizeof(opType) },
        { &handle, sizeof(handle) },
        { &offset, sizeof(offset) },
        { &length, sizeof(length) },
    };

    // Read
    ssize_t nread = readv(watcher.fd, vectors, std::extent<decltype(vectors)>::value);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    }

    magic = ntohl(magic);
    fds_verify(NBD_REQUEST_MAGIC == magic);
    opType = ntohl(opType);
    handle = __builtin_bswap64(handle);
    offset = __builtin_bswap64(offset);
    length = ntohl(length);

    LOGTRACE << "Read " << nread << " bytes," << std::endl
             << " magic 0x" << std::hex << magic << std::dec << std::endl
             << " op " << opType << std::endl
             << " handle 0x" << std::hex << handle << std::dec << std::endl
             << " offset " << offset << std::endl
             << " length " << length;

    if (NBD_CMD_WRITE == opType) {
        char data[length];  // NOLINT
        nread = read(watcher.fd, data, length);
        LOGTRACE << "Read " << nread << " bytes of data";
    }

    Error err = dispatchOp(watcher, opType, handle, offset, length);
    fds_verify(ERR_OK == err);
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
        boost::shared_ptr<std::string> buf = resp->getNextReadBuffer(context);
        while (buf != NULL) {
            GLOGDEBUG << "Handle " << handle << "....Buffer # " << context;
            vectors[3+cnt].iov_base = to_iovec(buf->c_str());
            vectors[3+cnt].iov_len = buf->length();
            ++cnt;
            // get next buffer
            buf = resp->getNextReadBuffer(context);
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
                          fds_uint32_t length) {
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
            // TODO(Andrew): Hackey uturn code. Remove.
            utPair.handle = handle;
            utPair.length = length;
            utPair.opType = opType;
            readyHandles.push(utPair);

            // We have something to write, so ask for events
            ioWatcher->set(ev::READ | ev::WRITE);
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
                hsAwaitOpts(watcher);
                hsState = SENDOPTS;
                ioWatcher->set(ev::READ | ev::WRITE);
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
NbdConnection::readResp(const Error& error,
                        fds_int64_t handle,
                        NbdResponseVector* response) {
    LOGNORMAL << "Read response from NbdOperations handle " << handle
              << " " << error;

    // add to quueue
    readyResponses.push(response);

    // We have something to write, so poke the loop
    asyncWatcher->send();
}

}  // namespace fds
