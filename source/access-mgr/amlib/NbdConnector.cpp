/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <set>
#include <string>
#include <NbdConnector.h>
#include <ev.h>
#include <fcntl.h>
#include <ev++.h>
#include <arpa/inet.h>

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
          clientSocket(clientsd),
          hsState(PREINIT) {
    fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK);

    totalConns++;

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
    ssize_t nwritten = write(watcher.fd, NBD_MAGIC_PWD, sizeof(NBD_MAGIC_PWD));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
    nwritten = write(watcher.fd, &NBD_MAGIC, sizeof(NBD_MAGIC));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
    nwritten = write(watcher.fd, &NBD_PROTO_VERSION, sizeof(NBD_PROTO_VERSION));
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
        LOGDEBUG << "Read " << nread << " bytes, magic 0x"
                 << std::hex << magic << std::dec;
    }

    fds_int32_t optSpec;
    nread = recv(watcher.fd, &optSpec, sizeof(optSpec), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    } else {
        optSpec = ntohl(optSpec);
        fds_verify(NBD_OPT_EXPORT == optSpec);
        LOGDEBUG << "Read " << nread << " bytes, optSpec " << optSpec;
    }

    fds_int32_t length;
    nread = recv(watcher.fd, &length, sizeof(length), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    } else {
        length = ntohl(length);
        LOGDEBUG << "Read " << nread << " bytes, length " << length;
    }

    char exportName[length];  // NOLINT
    nread = recv(watcher.fd, exportName, length, 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
        return;
    } else {
        volumeName = exportName;
        LOGDEBUG << "Read " << nread << " bytes, name " << volumeName;
    }
}

void
NbdConnection::hsSendOpts(ev::io &watcher) {
    ssize_t nwritten = write(watcher.fd, &volumeSizeInBytes, sizeof(volumeSizeInBytes));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
    fds_int16_t optFlags = NBD_FLAG_HAS_FLAGS | NBD_FLAG_SEND_FLUSH | NBD_FLAG_SEND_FUA;
    nwritten = write(watcher.fd, &optFlags, sizeof(optFlags));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
    nwritten = write(watcher.fd, NBD_PAD_ZERO, sizeof(NBD_PAD_ZERO));
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
        LOGDEBUG << "Read " << nread << " bytes, magic 0x"
                 << std::hex << magic << std::dec;
    }

    fds_int32_t opType;
    nread = recv(watcher.fd, &opType, sizeof(opType), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        opType = ntohl(opType);
        // TODO(Andrew): Need to verify this magic #...
        LOGDEBUG << "Read " << nread << " bytes, op " << opType;
    }

    fds_int64_t handle;
    nread = recv(watcher.fd, &handle, sizeof(handle), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        handle = __builtin_bswap64(handle);
        // TODO(Andrew): Need to verify this magic #...
        LOGDEBUG << "Read " << nread << " bytes, handle 0x"
                 << std::hex << handle << std::dec;
    }

    fds_uint64_t offset;
    nread = recv(watcher.fd, &offset, sizeof(offset), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        offset = __builtin_bswap64(offset);
        // TODO(Andrew): Need to verify this magic #...
        LOGDEBUG << "Read " << nread << " bytes, offset " << offset;
    }

    fds_uint32_t length;
    nread = recv(watcher.fd, &length, sizeof(length), 0);
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        length = ntohl(length);
        // TODO(Andrew): Need to verify this magic #...
        LOGDEBUG << "Read " << nread << " bytes, length " << length;
    }

    // TODO(Andrew): We should check if we have something to write
    ioWatcher->set(ev::READ | ev::WRITE);
    Error err = dispatchOp(watcher, opType, handle, offset, length);
    fds_verify(ERR_OK == err);
}

void
NbdConnection::hsReply(ev::io &watcher,
                       fds_int64_t handle) {
    fds_int32_t magic = htonl(NBD_RESPONSE_MAGIC);
    ssize_t nwritten = write(watcher.fd, &magic, sizeof(magic));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
    LOGDEBUG << "Sent " << nwritten << " bytes, magic 0x" << std::hex << magic << std::dec;
    fds_int32_t error = 0;
    nwritten = write(watcher.fd, &error, sizeof(error));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
    LOGDEBUG << "Sent " << nwritten << " bytes";
    handle = __builtin_bswap64(handle);
    nwritten = write(watcher.fd, &handle, sizeof(handle));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
    LOGDEBUG << "Sent " << nwritten << " bytes, handle 0x" << std::hex << handle << std::dec;
    LOGDEBUG << "Sent " << nwritten << " bytes";
    nwritten = write(watcher.fd, fourKayZeros, sizeof(fourKayZeros));
    if (nwritten < 0) {
        LOGERROR << "Socket write error";
        return;
    }
    LOGDEBUG << "Sent " << nwritten << " bytes";
    LOGDEBUG << "Sent data";
}

Error
NbdConnection::dispatchOp(ev::io &watcher,
                          fds_uint32_t opType,
                          fds_int64_t handle,
                          fds_uint64_t offset,
                          fds_uint32_t length) {
    switch (opType) {
        case NBD_CMD_READ:
            LOGNORMAL << "Got a read";
            curHandle = handle;
            // TODO(Andrew): Hack for uturn tests
            // hsReply(watcher, opType, handle);
            break;
        case NBD_CMD_WRITE:
            LOGNORMAL << "Got a write";
            break;
        case NBD_CMD_FLUSH:
            LOGNORMAL << "Got a flush";
            break;
        default:
            fds_panic("Unknown NBD op %d", opType);
    }
    return ERR_OK;
}

void
NbdConnection::callback(ev::io &watcher, int revents) {
    LOGNORMAL << "NBD connection callback";

    if (EV_ERROR & revents) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    if (revents & EV_READ) {
        LOGTRACE << "Read event";
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
                fds_panic("Unknown NBD connection state");
        }
    }

    if (revents & EV_WRITE) {
        LOGNORMAL << "Write event";
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
                // Wait for read events from client
                ioWatcher->set(ev::READ);
                break;
            case DOREQS:
                LOGWARN << "Got a write event in DOREQS";
                hsReply(watcher, curHandle);
                ioWatcher->set(ev::READ);
                break;
            default:
                fds_panic("Unknown NBD connection state!");
        }
    }
}

int NbdConnection::totalConns = 0;

}  // namespace fds
