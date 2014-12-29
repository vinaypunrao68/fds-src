/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <cerrno>
#include <set>
#include <string>
#include <type_traits>
#include <NbdConnector.h>
#include <fds_process.h>

extern "C" {
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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

NbdConnector::NbdConnector(OmConfigApi::shared_ptr omApi)
        : omConfigApi(omApi),
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
    NbdConnection *client = new NbdConnection(omConfigApi, clientsd);
    LOGNORMAL << "Created client connection...";
}

int
NbdConnector::createNbdSocket() {
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(nbdPort);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        LOGERROR << "Failed to create NBD socket";
        return listenfd;
    }

    // If we crash this allows us to reuse the socket before it's fully closed
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        LOGWARN << "Failed to set REUSEADDR on NBD socket";
    }

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
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    if (0 != pthread_sigmask(SIG_BLOCK, &set, nullptr)) {
        LOGWARN << "Failed to enable SIGPIPE mask on NBD server.";
    }

    ev::default_loop loop;
    loop.run(0);
    LOGNOTIFY << "Stopping NBD loop...";
}

NbdConnection::NbdConnection(OmConfigApi::shared_ptr omApi,
                             int clientsd)
        : omConfigApi(omApi),
          nbdOps(boost::make_shared<NbdOperations>(this)),
          clientSocket(clientsd),
          hsState(PREINIT),
          doUturn(false),
          attach({ { 0x00ull, 0x00u, 0x00u }, 0x00ull, 0x00ull, { 0x00 } }),
          request({ { 0x00u, 0x00u, 0x00ull, 0x00ull, 0x00u }, 0x00ull, 0x00ull, nullptr }),
          response(nullptr),
          total_blocks(0ull),
          write_offset(-1ll),
          maxChunks(0ull),
          readyHandles(2000),
          readyResponses(4000),
          current_response(nullptr) {
    fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK);
    FdsConfigAccessor config(g_fdsprocess->get_conf_helper());
    toggleStandAlone = config.get_abs<bool>("fds.am.testing.toggleStandAlone");

    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set<NbdConnection, &NbdConnection::callback>(this);
    ioWatcher->start(clientSocket, ev::READ | ev::WRITE);

    asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
    asyncWatcher->set<NbdConnection, &NbdConnection::wakeupCb>(this);
    asyncWatcher->start();

    nbdOps->init();
    LOGNORMAL << "New NBD client connection for " << clientSocket;
}

NbdConnection::~NbdConnection() {
    LOGTRACE << "NbdConnection going adios!";
    ioWatcher->stop();
    shutdown(clientSocket, SHUT_RDWR);
    close(clientSocket);
}

constexpr uint8_t NbdConnection::NBD_MAGIC[];
constexpr char NbdConnection::NBD_MAGIC_PWD[];
constexpr uint8_t NbdConnection::NBD_PROTO_VERSION[];
constexpr fds_int32_t NbdConnection::NBD_OPT_EXPORT;
constexpr fds_int16_t NbdConnection::NBD_FLAG_HAS_FLAGS;
constexpr fds_int16_t NbdConnection::NBD_FLAG_READ_ONLY;
constexpr fds_int16_t NbdConnection::NBD_FLAG_SEND_FLUSH;
constexpr fds_int16_t NbdConnection::NBD_FLAG_SEND_FUA;
constexpr fds_int16_t NbdConnection::NBD_FLAG_ROTATIONAL;
constexpr fds_int16_t NbdConnection::NBD_FLAG_SEND_TRIM;
constexpr fds_int32_t NbdConnection::NBD_REQUEST_MAGIC;
constexpr fds_int32_t NbdConnection::NBD_RESPONSE_MAGIC;
constexpr fds_int32_t NbdConnection::NBD_CMD_READ;
constexpr fds_int32_t NbdConnection::NBD_CMD_WRITE;
constexpr fds_int32_t NbdConnection::NBD_CMD_DISC;
constexpr fds_int32_t NbdConnection::NBD_CMD_FLUSH;
constexpr fds_int32_t NbdConnection::NBD_CMD_TRIM;

constexpr char NbdConnection::fourKayZeros[];

bool
NbdConnection::write_response() {
    fds_verify(response);
    size_t current_block = 0ull;
    if (write_offset > 0)
    {  // Figure out which block we left off on
        size_t written = write_offset;

        while (written >= response[current_block].iov_len)
            written -= response[current_block++].iov_len;

        // Adjust the io vector so we don't re-write the same data
        response[current_block].iov_base = reinterpret_cast<uint8_t*>(
            response[current_block].iov_base) + written;
        response[current_block].iov_len -= written;
    }

    ssize_t nwritten = writev(ioWatcher->fd,
                              response.get() + current_block,
                              total_blocks - current_block);
    if (nwritten < 0) {
        LOGERROR << "Socket write error: " << errno;
        switch (errno) {
            case EINVAL: fds_assert(false);  // Indicates logic bug
                         break;
            case EBADF:
            case EPIPE: throw connection_closed;
                        break;
        }
        return false;
    } else if (nwritten == 0) {
        return false;
    }
    write_offset += nwritten;

    ssize_t to_write = 0;
    for (; current_block < total_blocks; ++current_block)
        to_write += response[current_block].iov_len;

    if (to_write != nwritten) {
        LOGTRACE << "Wrote [" << nwritten << "] of [" << to_write << " bytes";
        return false;
    }
    total_blocks = 0;
    write_offset = -1;
    return true;
}

// Send initial message to client with NBD magic and proto version
bool
NbdConnection::hsPreInit(ev::io &watcher) {
    // Vector always starts from this state
    static iovec const vectors[] = {
        { to_iovec(NBD_MAGIC_PWD),       sizeof(NBD_MAGIC_PWD)      },
        { to_iovec(NBD_MAGIC),           sizeof(NBD_MAGIC)          },
        { to_iovec(NBD_PROTO_VERSION),   sizeof(NBD_PROTO_VERSION)  },
    };

    if (!response) {
        // First pass (fingers crossed, the only), std::default_deleter
        // is good enough for a unique_ptr (no custom deleter needed).
        write_offset = 0;
        total_blocks = std::extent<decltype(vectors)>::value;
        response = decltype(response)(new iovec[total_blocks]);
        memcpy(response.get(), vectors, sizeof(vectors));
    }

    // Try and write the response, if it fails to write ALL
    // the data we'll continue later
    if (!write_response()) {
        return false;
    }

    response.reset();
    return true;
}

void
NbdConnection::hsPostInit(ev::io &watcher) {
    // Accept client ack
    fds_uint32_t ack;
    ssize_t nread = read(watcher.fd, &ack, sizeof(ack));
    if (nread < 0) {
        LOGERROR << "Socket read error";
    } else {
        fds_verify(0 == ack);
        LOGDEBUG << "Received " << nread << " byte ack " << ack;
    }
}

bool
NbdConnection::hsAwaitOpts(ev::io &watcher) {
    if (attach.header_off >= 0) {
        if (!get_message_header(watcher.fd, attach))
            return false;
        fds_verify(0 == memcmp(NBD_MAGIC, attach.header.magic, sizeof(NBD_MAGIC)));
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
        LOGNORMAL << "Will stat volume " << *volumeName;
        Error err = omConfigApi->statVolume(volumeName, volDesc);
        if (ERR_OK != err || apis::BLOCK != volDesc.policy.volumeType)
            throw connection_closed;
    }

    // Fix endianness
    volume_size = __builtin_bswap64(volDesc.policy.blockDeviceSizeInBytes);
    maxChunks = (2 * 1024 * 1024) / volDesc.policy.maxObjectSizeInBytes;

    LOGNORMAL << "Attaching volume name " << *volumeName << " of size "
              << volDesc.policy.blockDeviceSizeInBytes
              << " max object size " << volDesc.policy.maxObjectSizeInBytes
              << " max number of chunks " << maxChunks;

    return true;
}

bool
NbdConnection::hsSendOpts(ev::io &watcher) {
    static fds_int16_t const optFlags =
        ntohs(NBD_FLAG_HAS_FLAGS);
    static iovec const vectors[] = {
        { nullptr,                  sizeof(volume_size) },
        { to_iovec(&optFlags),      sizeof(optFlags)    },
        { to_iovec(fourKayZeros),   124                 },
    };

    if (!response) {
        write_offset = 0;
        total_blocks = std::extent<decltype(vectors)>::value;
        response = decltype(response)(new iovec[total_blocks]);
        memcpy(response.get(), vectors, sizeof(vectors));
        response[0].iov_base = &volume_size;
    }

    // Try and write the response, if it fails to write ALL
    // the data we'll continue later
    if (!write_response()) {
        return false;
    }

    response.reset();
    return true;
}

void
NbdConnection::hsReq(ev::io &watcher) {
    if (request.header_off >= 0) {
        if (!get_message_header(watcher.fd, request))
            return;
        request.header.magic = ntohl(request.header.magic);
        fds_verify(NBD_REQUEST_MAGIC == request.header.magic);
        request.header.opType = ntohl(request.header.opType);
        request.header.offset = __builtin_bswap64(request.header.offset);
        request.header.length = ntohl(request.header.length);

        LOGTRACE << " magic 0x" << std::hex << request.header.magic << std::dec << std::endl
                 << " op " << request.header.opType << std::endl
                 << " handle 0x" << std::hex << request.header.handle << std::dec << std::endl
                 << " offset " << request.header.offset << std::endl
                 << " length " << request.header.length;
        // Construct Buffer for Payload
        request.data = boost::make_shared<std::string>(request.header.length, '\0');
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
                           request.data);
    fds_verify(ERR_OK == err);
    return;
}

bool
NbdConnection::hsReply(ev::io &watcher) {
    static fds_int32_t magic = htonl(NBD_RESPONSE_MAGIC);
    fds_int32_t error = htonl(0);

    // We can reuse this from now on since we don't go to any state from here
    if (!response) {
        response = decltype(response)(new iovec[kMaxChunks + 3]);
        response[0].iov_base = &magic; response[0].iov_len = sizeof(magic);
        response[1].iov_base = &error; response[1].iov_len = sizeof(error);
        response[2].iov_base = nullptr; response[2].iov_len = 0;
        response[3].iov_base = nullptr; response[3].iov_len = 0ull;
    }

    if (write_offset == -1) {
        total_blocks = 3;
        if (doUturn) {
            // Iterate and send each ready response
            if (!readyHandles.empty()) {
                UturnPair rep;
                fds_verify(readyHandles.pop(rep));

                response[2].iov_base = &rep.handle; response[2].iov_len = sizeof(rep.handle);

                fds_uint32_t length = rep.length;
                fds_int32_t opType = rep.opType;
                if (NBD_CMD_READ == opType) {
                    // TODO(Andrew): Remove this verify and handle sub-4K
                    fds_verify((length % 4096) == 0);
                    total_blocks += length / 4096;
                }

                for (size_t i = 3; i < total_blocks; ++i) {
                    response[i].iov_base = to_iovec(fourKayZeros);
                    response[i].iov_len = sizeof(fourKayZeros);
                }
            }
        }

        // no uturn
        if (!readyResponses.empty()) {
            NbdResponseVector* resp = NULL;
            fds_verify(readyResponses.pop(resp));
            current_response.reset(resp);

            response[2].iov_base = &current_response->handle;
            response[2].iov_len = sizeof(current_response->handle);
            Error opError = current_response->getError();
            if (!opError.ok()) {
                error = htonl(-1);
            }

            fds_uint32_t context = 0;
            if (current_response->isRead() && (opError.ok())) {
                boost::shared_ptr<std::string> buf = current_response->getNextReadBuffer(context);
                while (buf != NULL) {
                    GLOGDEBUG << "Handle " << current_response->handle << "...Buffer # " << context;
                    response[total_blocks].iov_base = to_iovec(buf->c_str());
                    response[total_blocks].iov_len = buf->length();
                    ++total_blocks;
                    // get next buffer
                    buf = current_response->getNextReadBuffer(context);
                }
            }
        } else {
            return true;
        }
        write_offset = 0;
    }
    fds_assert(response[2].iov_base != nullptr);
    // Try and write the response, if it fails to write ALL
    // the data we'll continue later
    if (!write_response()) {
        return false;
    }

    response[2].iov_base = nullptr;
    current_response.reset();

    LOGTRACE << "No more reponses to send";
    return true;
}

Error
NbdConnection::dispatchOp(ev::io &watcher,
                          fds_uint32_t opType,
                          fds_int64_t handle,
                          fds_uint64_t offset,
                          fds_uint32_t length,
                          boost::shared_ptr<std::string> data) {
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
                 nbdOps->write(volumeName, volDesc.policy.maxObjectSizeInBytes,
                               data, length, offset, handle);
            }
            break;
        case NBD_CMD_FLUSH:
            break;
        case NBD_CMD_DISC:
            LOGNORMAL << "Got a disconnect";
            throw connection_closed;
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

    try {
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
                LOGTRACE << "Asked to read in state: " << hsState;
                // We could have read and writes waiting and are not in the
                // correct state to handle more requests...yet
                break;
        }
    }

    if (revents & EV_WRITE) {
        switch (hsState) {
            case PREINIT:
                if (hsPreInit(watcher)) {
                    hsState = POSTINIT;
                    // Wait for read event from client
                    ioWatcher->set(ev::READ);
                }
                break;
            case SENDOPTS:
                if (hsSendOpts(watcher)) {
                    hsState = DOREQS;
                    LOGNORMAL << "Done with NBD handshake";
                    // Wait for read events from client
                    ioWatcher->set(ev::READ);
                }
                break;
            case DOREQS:
                if (hsReply(watcher)) {
                    bool more_to_write = doUturn ? readyHandles.empty()
                                                 : readyResponses.empty();
                    ioWatcher->set(more_to_write ? ev::READ
                                                 : ev::READ | ev::WRITE);
                }
                break;
            default:
                fds_panic("Unknown NBD connection state %d", hsState);
        }
    }
    } catch(Errors e) {
        delete this;
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
        LOGERROR << "Socket read error" << errno;
        switch (errno) {
            case EINVAL: fds_assert(false);  // Indicates logic bug
                         break;
            case EBADF:
            case EPIPE: throw NbdConnection::connection_closed;
                        break;
        }
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
ssize_t read_from_socket(int fd, M& buffer, ssize_t off, ssize_t len);

template<>
ssize_t read_from_socket(int fd, std::array<char, 1024>& buffer, ssize_t off, ssize_t len)
{ return read(fd, buffer.data() + off, len); }

template<>
ssize_t read_from_socket(int fd, boost::shared_ptr<std::string>& buffer, ssize_t off, ssize_t len)
{ return read(fd, &(*buffer)[0] + off, len); }

template<typename M>
bool get_message_payload(int fd, M& message) {
    fds_assert(message.data_off >= 0);
    ssize_t to_read = message.header.length - message.data_off;
    ssize_t nread = read_from_socket(fd,
                                     message.data,
                                     message.data_off,
                                     to_read);
    if (nread < 0) {
        LOGERROR << "Socket read error" << errno;
        switch (errno) {
            case EINVAL: fds_assert(false);  // Indicates logic bug
                         break;
            case EBADF:
            case EPIPE: throw NbdConnection::connection_closed;
                        break;
        }
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
