/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <set>
#include <string>
#include <NbdConnector.h>

extern "C" {
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/uio.h>
}

#include <ev++.h>

namespace fds {

NbdConnector::NbdConnector(AmAsyncDataApi::shared_ptr api)
        : asyncDataApi(api),
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
    NbdConnection *client = new NbdConnection(asyncDataApi, clientsd);
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
                             int clientsd)
        : asyncDataApi(api),
          nbdOps(new NbdOperations(asyncDataApi, this)),
          clientSocket(clientsd),
          hsState(PREINIT),
          doUturn(false),
          readyHandles(2000),
          readyResponses(4000) {
    fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK);

    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set<NbdConnection, &NbdConnection::callback>(this);
    ioWatcher->start(clientSocket, ev::READ | ev::WRITE);

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

constexpr fds_uint64_t NbdConnection::volumeSizeInBytes;
constexpr fds_uint32_t NbdConnection::maxObjectSizeInBytes;
constexpr char NbdConnection::fourKayZeros[];

void
NbdConnection::hsPreInit(ev::io &watcher) {
    // Send initial message to client with NBD magic and proto version
    static iovec const vectors[] = {
        { const_cast<void*>(reinterpret_cast<void const*>(NBD_MAGIC_PWD)),
            sizeof(NBD_MAGIC_PWD) },
        { const_cast<void*>(reinterpret_cast<void const*>(&NBD_MAGIC)),
            sizeof(NBD_MAGIC) },
        { const_cast<void*>(reinterpret_cast<void const*>(&NBD_PROTO_VERSION)),
            sizeof(NBD_PROTO_VERSION) },
    };

    ssize_t nwritten = writev(watcher.fd, vectors, sizeof(vectors) / sizeof(iovec));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
}

void
NbdConnection::hsPostInit(ev::io &watcher) {
    // Accept client ack
    fds_uint32_t ack;
    ssize_t nread = recv(watcher.fd, &ack, sizeof(ack), 0);
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
    // Read magic number
    fds_int64_t magic;
    ssize_t nread = recv(watcher.fd, &magic, sizeof(magic), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    } else {
        magic = __builtin_bswap64(magic);
        fds_verify(NBD_MAGIC == magic);
    }

    fds_int32_t optSpec;
    nread = recv(watcher.fd, &optSpec, sizeof(optSpec), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    } else {
        optSpec = ntohl(optSpec);
        fds_verify(NBD_OPT_EXPORT == optSpec);
    }

    fds_int32_t length;
    nread = recv(watcher.fd, &length, sizeof(length), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    } else {
        length = ntohl(length);
    }

    char exportName[length];  // NOLINT
    nread = recv(watcher.fd, exportName, length, 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    } else {
        volumeName = boost::shared_ptr<std::string>(new std::string(exportName));
        LOGNORMAL << "Volume name " << *volumeName;
    }
}

void
NbdConnection::hsSendOpts(ev::io &watcher) {
    static fds_int16_t const optFlags = NBD_FLAG_HAS_FLAGS|NBD_FLAG_SEND_FLUSH|NBD_FLAG_SEND_FUA;
    static iovec const vectors[] = {
        { const_cast<void*>(reinterpret_cast<void const*>(&volumeSizeInBytes)),
            sizeof(volumeSizeInBytes) },
        { const_cast<void*>(reinterpret_cast<void const*>(&optFlags)),
            sizeof(NBD_MAGIC) },
        { const_cast<void*>(reinterpret_cast<void const*>(&NBD_PROTO_VERSION)),
            sizeof(NBD_PROTO_VERSION) },
    };

    ssize_t nwritten = writev(watcher.fd, vectors, sizeof(vectors) / sizeof(iovec));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
}

void
NbdConnection::hsReq(ev::io &watcher) {
    fds_int32_t magic;
    ssize_t nread = recv(watcher.fd, &magic, sizeof(magic), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        magic = ntohl(magic);
        fds_verify(NBD_REQUEST_MAGIC == magic);
        LOGTRACE << "Read " << nread << " bytes, magic 0x"
                 << std::hex << magic << std::dec;
    }

    fds_int32_t opType;
    nread = recv(watcher.fd, &opType, sizeof(opType), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        opType = ntohl(opType);
        // TODO(Andrew): Need to verify this magic #...
        LOGTRACE << "Read " << nread << " bytes, op " << opType;
    }

    fds_int64_t handle;
    nread = recv(watcher.fd, &handle, sizeof(handle), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        handle = __builtin_bswap64(handle);
        // TODO(Andrew): Need to verify this magic #...
        LOGTRACE << "Read " << nread << " bytes, handle 0x"
                 << std::hex << handle << std::dec;
    }

    fds_uint64_t offset;
    nread = recv(watcher.fd, &offset, sizeof(offset), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        offset = __builtin_bswap64(offset);
        // TODO(Andrew): Need to verify this magic #...
        LOGTRACE << "Read " << nread << " bytes, offset " << offset;
    }

    fds_uint32_t length;
    nread = recv(watcher.fd, &length, sizeof(length), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        length = ntohl(length);
        // TODO(Andrew): Need to verify this magic #...
        LOGTRACE << "Read " << nread << " bytes, length " << length;
    }

    if (NBD_CMD_WRITE == opType) {
        char data[length];  // NOLINT
        nread = recv(watcher.fd, data, length, 0);
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
            iovec vectors[kMaxChunks + 3];
            vectors[0] = {reinterpret_cast<void*>(&magic), sizeof(magic)};
            vectors[1] = {const_cast<void*>(reinterpret_cast<void const*>(&error)), sizeof(error)};
            vectors[2] = {reinterpret_cast<void*>(&handle), sizeof(handle)};

            for (size_t i = 0; i < chunks; ++i) {
                vectors[3+i].iov_base = const_cast<void*>(
                    reinterpret_cast<void const*>(fourKayZeros));
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
        iovec vectors[kMaxChunks + 3];
        vectors[0] = {reinterpret_cast<void*>(&magic), sizeof(magic)};
        vectors[1] = {const_cast<void*>(reinterpret_cast<void const*>(&error)), sizeof(error)};
        vectors[2] = {reinterpret_cast<void*>(&handle), sizeof(handle)};

        fds_uint32_t context = 0;
        size_t cnt = 0;
        boost::shared_ptr<std::string> buf = resp->getNextReadBuffer(context);
        while (buf != NULL) {
            GLOGDEBUG << "Handle " << handle << "....Buffer # " << context;
            vectors[3+cnt].iov_base = const_cast<void*>(
                reinterpret_cast<void const*>(buf->c_str()));
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
                nbdOps->read(volumeName, length, offset, handle);
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

    // We have something to write, so ask for events
    ioWatcher->set(ev::READ | ev::WRITE);
    // ioWatcher->feed_event(EV_WRITE);
}

}  // namespace fds
