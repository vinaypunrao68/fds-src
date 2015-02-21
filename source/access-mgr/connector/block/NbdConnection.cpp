/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#include <cerrno>
#include <string>
#include <type_traits>

extern "C" {
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>
}

#include <ev++.h>

#include "connector/block/NbdConnection.h"
#include "OmConfigService.h"
#include "fds_process.h"

template<typename T>
constexpr auto to_iovec(T* t) -> typename std::remove_cv<T>::type*
{ return const_cast<typename std::remove_cv<T>::type*>(t); }

static std::array<std::string, 5> const io_to_string = {
    { "READ", "WRITE", "DISCONNECT", "FLUSH", "TRIM" }
};

namespace fds
{

template<typename M>
bool get_message_header(int fd, M& message);

template<typename M>
bool get_message_payload(int fd, M& message);

NbdConnection::NbdConnection(OmConfigApi::shared_ptr omApi,
                             int clientsd)
        : omConfigApi(omApi),
          nbdOps(boost::make_shared<NbdOperations>(this)),
          clientSocket(clientsd),
          hsState(PREINIT),
          doUturn(false),
          resp_needed(0u),
          attach({ { 0x00ull, 0x00u, 0x00u }, 0x00ull, 0x00ull, { 0x00 } }),
          request({ { 0x00u, 0x00u, 0x00ull, 0x00ull, 0x00u }, 0x00ull, 0x00ull, nullptr }),
          response(nullptr),
          total_blocks(0ull),
          write_offset(-1ll),
          maxChunks(0ull),
          readyHandles(2000),
          readyResponses(4000),
          current_response(nullptr) {
    FdsConfigAccessor config(g_fdsprocess->get_conf_helper());
    toggleStandAlone = config.get_abs<bool>("fds.am.testing.toggleStandAlone");
    if (config.get_abs<bool>("fds.am.connector.nbd.non_block_io", true)) {
        fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK);
    } else {
        LOGNOTIFY << "Nbd IO in blocking mode.";
    }

    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set<NbdConnection, &NbdConnection::callback>(this);
    ioWatcher->start(clientSocket, ev::READ | ev::WRITE);

    asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
    asyncWatcher->set<NbdConnection, &NbdConnection::wakeupCb>(this);
    asyncWatcher->start();

    LOGNORMAL << "New NBD client connection for " << clientSocket;
}

NbdConnection::~NbdConnection() {
    LOGDEBUG << "NbdConnection going adios!";
    asyncWatcher->stop();
    ioWatcher->stop();
    shutdown(clientSocket, SHUT_RDWR);
    close(clientSocket);
}

constexpr uint8_t NbdConnection::NBD_MAGIC[];
constexpr uint8_t NbdConnection::NBD_REQUEST_MAGIC[];
constexpr uint8_t NbdConnection::NBD_RESPONSE_MAGIC[];
constexpr char NbdConnection::NBD_MAGIC_PWD[];
constexpr uint8_t NbdConnection::NBD_PROTO_VERSION[];
constexpr fds_int32_t NbdConnection::NBD_OPT_EXPORT;
constexpr fds_int16_t NbdConnection::NBD_FLAG_HAS_FLAGS;
constexpr fds_int16_t NbdConnection::NBD_FLAG_READ_ONLY;
constexpr fds_int16_t NbdConnection::NBD_FLAG_SEND_FLUSH;
constexpr fds_int16_t NbdConnection::NBD_FLAG_SEND_FUA;
constexpr fds_int16_t NbdConnection::NBD_FLAG_ROTATIONAL;
constexpr fds_int16_t NbdConnection::NBD_FLAG_SEND_TRIM;
constexpr fds_int32_t NbdConnection::NBD_CMD_READ;
constexpr fds_int32_t NbdConnection::NBD_CMD_WRITE;
constexpr fds_int32_t NbdConnection::NBD_CMD_DISC;
constexpr fds_int32_t NbdConnection::NBD_CMD_FLUSH;
constexpr fds_int32_t NbdConnection::NBD_CMD_TRIM;

constexpr char NbdConnection::fourKayZeros[];

bool
NbdConnection::write_response() {
    static_assert(EAGAIN == EWOULDBLOCK, "EAGAIN != EWOULDBLOCK");
    fds_assert(response);
    fds_assert(total_blocks <= IOV_MAX);

    // Figure out which block we left off on
    size_t current_block = 0ull;
    iovec old_block = response[0];
    if (write_offset > 0) {
        size_t written = write_offset;

        while (written >= response[current_block].iov_len)
            written -= response[current_block++].iov_len;

        // Adjust the block so we don't re-write the same data
        old_block = response[current_block];
        response[current_block].iov_base =
            reinterpret_cast<uint8_t*>(response[current_block].iov_base) + written;
        response[current_block].iov_len -= written;
    }

    // Do the write
    ssize_t nwritten = 0;
    do {
        nwritten = writev(ioWatcher->fd,
                          response.get() + current_block,
                          total_blocks - current_block);
    } while ((0 > nwritten) && (EINTR == errno));

    // Calculate amount we should have written so we can verify the return value
    ssize_t to_write = response[current_block].iov_len;
    // Return the block back to original in case we have to repeat
    response[current_block++] = old_block;
    for (; current_block < total_blocks; ++current_block){
        to_write += response[current_block].iov_len;
    }

    // Check return value
    if (to_write != nwritten) {
        if (nwritten < 0) {
            if (EAGAIN != errno) {
                LOGERROR << "Socket write error: [" << strerror(errno) << "]";
                throw NbdError::connection_closed;
            }
        } else {
            LOGTRACE << "Wrote [" << nwritten << "] of [" << to_write << " bytes";
            write_offset += nwritten;
        }
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
        response = resp_vector_type(new iovec[total_blocks]);
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
    ssize_t nread = 0;

    do {
        nread = read(watcher.fd, &ack, sizeof(ack));
    } while ((0 > nread) && (EINTR == errno));

    ensure(0 < nread);
    ensure(0 == ack);
}

bool
NbdConnection::hsAwaitOpts(ev::io &watcher) {
    if (attach.header_off >= 0) {
        if (!get_message_header(watcher.fd, attach))
            return false;
        ensure(0 == memcmp(NBD_MAGIC, attach.header.magic, sizeof(NBD_MAGIC)));
        attach.header.optSpec = ntohl(attach.header.optSpec);
        ensure(NBD_OPT_EXPORT == attach.header.optSpec);
        attach.header.length = ntohl(attach.header.length);

        // Just for sanities sake and protect against bad data
        ensure(attach.data.size() >= static_cast<size_t>(attach.header.length));
    }
    if (!get_message_payload(watcher.fd, attach))
        return false;

    // In case volume name is not NULL terminated.
    auto volumeName = boost::make_shared<std::string>(attach.data.begin(),
                                                 attach.data.begin() + attach.header.length);
    if (toggleStandAlone) {
        volDesc.policy.maxObjectSizeInBytes = 4096;
        volDesc.policy.blockDeviceSizeInBytes = 10737418240;
    } else {
        LOGNORMAL << "Will stat volume " << *volumeName;
        Error err = omConfigApi->statVolume(volumeName, volDesc);
        if (ERR_OK != err || apis::BLOCK != volDesc.policy.volumeType)
            throw NbdError::connection_closed;
    }

    // Fix endianness
    volume_size = __builtin_bswap64(volDesc.policy.blockDeviceSizeInBytes);
    maxChunks = (2 * 1024 * 1024) / volDesc.policy.maxObjectSizeInBytes;

    LOGNORMAL << "Attaching volume name " << *volumeName << " of size "
              << volDesc.policy.blockDeviceSizeInBytes
              << " max object size " << volDesc.policy.maxObjectSizeInBytes
              << " max number of chunks " << maxChunks;
    nbdOps->init(volumeName, volDesc.policy.maxObjectSizeInBytes);

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
        response = resp_vector_type(new iovec[total_blocks]);
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

bool
NbdConnection::hsReq(ev::io &watcher) {
    if (request.header_off >= 0) {
        if (!get_message_header(watcher.fd, request))
            return false;
        ensure(0 == memcmp(NBD_REQUEST_MAGIC, request.header.magic, sizeof(NBD_REQUEST_MAGIC)));
        request.header.opType = ntohl(request.header.opType);
        request.header.offset = __builtin_bswap64(request.header.offset);
        request.header.length = ntohl(request.header.length);

        // Construct Buffer for Write Payload
        if (NBD_CMD_WRITE == request.header.opType) {
            request.data = boost::make_shared<std::string>(request.header.length, '\0');
        }
    }

    if (NBD_CMD_WRITE == request.header.opType) {
        if (!get_message_payload(watcher.fd, request))
            return false;
    }
    request.header_off = 0;
    request.data_off = -1;

    LOGIO << " op " << io_to_string[request.header.opType]
          << " handle 0x" << std::hex << request.header.handle
          << " offset 0x" << request.header.offset << std::dec
          << " length " << request.header.length
          << " ahead of you: " <<  resp_needed.fetch_add(1, std::memory_order_relaxed);

    Error err = dispatchOp();
    request.data.reset();
    ensure(ERR_OK == err);
    return true;
}

bool
NbdConnection::hsReply(ev::io &watcher) {
    static int32_t const error_ok = htonl(0);
    static int32_t const error_bad = htonl(-1);

    // We can reuse this from now on since we don't go to any state from here
    if (!response) {
        response = resp_vector_type(new iovec[kMaxChunks + 3]);
        response[0].iov_base = to_iovec(NBD_RESPONSE_MAGIC);
        response[0].iov_len = sizeof(NBD_RESPONSE_MAGIC);
        response[1].iov_base = to_iovec(&error_ok); response[1].iov_len = sizeof(error_ok);
        response[2].iov_base = nullptr; response[2].iov_len = 0;
        response[3].iov_base = nullptr; response[3].iov_len = 0ull;
    }

    Error err = ERR_OK;
    if (write_offset == -1) {
        total_blocks = 3;
        if (doUturn) {
            // Iterate and send each ready response
            if (!readyHandles.empty()) {
                UturnPair rep;
                ensure(readyHandles.pop(rep));

                response[2].iov_base = &rep.handle; response[2].iov_len = sizeof(rep.handle);

                fds_uint32_t length = rep.length;
                fds_int32_t opType = rep.opType;
                if (NBD_CMD_READ == opType) {
                    // TODO(Andrew): Remove this verify and handle sub-4K
                    ensure((length % 4096) == 0);
                    total_blocks += length / 4096;
                }

                for (size_t i = 3; i < total_blocks; ++i) {
                    response[i].iov_base = to_iovec(fourKayZeros);
                    response[i].iov_len = sizeof(fourKayZeros);
                }
            }
            return true;
        }

        // no uturn
        if (readyResponses.empty())
            { return false; }

        NbdResponseVector* resp = NULL;
        ensure(readyResponses.pop(resp));
        current_response.reset(resp);

        response[2].iov_base = &current_response->handle;
        response[2].iov_len = sizeof(current_response->handle);
        if (!current_response->getError().ok()) {
            err = current_response->getError();
            response[1].iov_base = to_iovec(&error_bad);
        } else if (current_response->isRead()) {
            fds_uint32_t context = 0;
            boost::shared_ptr<std::string> buf = current_response->getNextReadBuffer(context);
            while (buf != NULL) {
                LOGDEBUG << "Handle 0x" << std::hex << current_response->handle
                         << "...Buffer # " << context
                         << "...Size " << std::dec << buf->length() << "B";
                response[total_blocks].iov_base = to_iovec(buf->c_str());
                response[total_blocks].iov_len = buf->length();
                ++total_blocks;
                // get next buffer
                buf = current_response->getNextReadBuffer(context);
            }
        }
        write_offset = 0;
    }
    fds_assert(response[2].iov_base != nullptr);
    // Try and write the response, if it fails to write ALL
    // the data we'll continue later
    if (!write_response()) {
        return false;
    }
    LOGIO << " handle 0x" << std::hex << current_response->handle << std::dec
          << " done (" << err << ") "
          << resp_needed.fetch_sub(1, std::memory_order_relaxed) - 1 << " requests behind you";

    response[2].iov_base = nullptr;
    current_response.reset();

    return true;
}

Error
NbdConnection::dispatchOp() {
    auto& handle = request.header.handle;
    auto& offset = request.header.offset;
    auto& length = request.header.length;

    switch (request.header.opType) {
        UturnPair utPair;
        case NBD_CMD_READ:
            if (doUturn) {
                // TODO(Andrew): Hackey uturn code. Remove.
                utPair.handle = handle;
                utPair.length = length;
                utPair.opType = NBD_CMD_READ;
                readyHandles.push(utPair);

                // We have something to write, so ask for events
                ioWatcher->set(ev::READ | ev::WRITE);
            } else {
                // do read from AM
                nbdOps->read(length, offset, handle);
            }
            break;
        case NBD_CMD_WRITE:
            if (doUturn) {
                // TODO(Andrew): Hackey uturn code. Remove.
                utPair.handle = handle;
                utPair.length = length;
                utPair.opType = NBD_CMD_WRITE;
                readyHandles.push(utPair);

                // We have something to write, so ask for events
                ioWatcher->set(ev::READ | ev::WRITE);
            } else {
                 fds_assert(request.data);
                 nbdOps->write(request.data, length, offset, handle);
            }
            break;
        case NBD_CMD_FLUSH:
            break;
        case NBD_CMD_DISC:
            LOGNORMAL << "Got a disconnect";
            throw NbdError::shutdown_requested;
            break;
        default:
            fds_panic("Unknown NBD op %d", request.header.opType);
    }
    return ERR_OK;
}

void
NbdConnection::wakeupCb(ev::async &watcher, int revents) {
    // It's ok to keep writing responses if we've been shutdown
    // but don't start watching for requests if we do
    ioWatcher->set(ev::WRITE | (nbdOps ? ev::READ : ev::NONE));
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
                // Read all the requests off the socket
                while (hsReq(watcher))
                    { continue; }
                break;
            default:
                LOGDEBUG << "Asked to read in state: " << hsState;
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
                    LOGDEBUG << "Done with NBD handshake";
                    // Wait for read events from client
                    ioWatcher->set(ev::READ);
                }
                break;
            case DOREQS:
                // Write everything we can
                while (hsReply(watcher))
                    { continue; }

                // If the queue is empty, stop writing. Reading at this
                // point is determined by whether we have an Operations
                // instance to queue to.
                if ((doUturn ? readyHandles.empty() : readyResponses.empty()))
                    { ioWatcher->set(nbdOps ? ev::READ : ev::NONE); }
                break;
            default:
                fds_panic("Unknown NBD connection state %d", hsState);
        }
    }
    } catch(NbdError const& e) {
        // If we had an error, stop the event loop too
        if (e == NbdError::connection_closed) {
            asyncWatcher->stop();
            ioWatcher->stop();
        }

        if (nbdOps) {
            // Tell NbdOperations to delete us once it's handled all outstanding
            // requests. Going to ignore the incoming requests now.
            ioWatcher->set(ev::WRITE);
            nbdOps->shutdown();
            nbdOps.reset();
        }
    }
}

void
NbdConnection::readWriteResp(NbdResponseVector* response) {
    LOGDEBUG << (response->isRead() ? "READ" : "WRITE")
              << " response from NbdOperations handle " << response->handle
              << " " << response->getError();

    // add to quueue
    readyResponses.push(response);

    // We have something to write, so poke the loop
    asyncWatcher->send();
}

ssize_t retry_read(int fd, void* buf, size_t count) {
    ssize_t e = 0;
    do {
        e = read(fd, buf, count);
    } while ((0 > e) && (EINTR == errno));
    return e;
}

template<typename M>
bool get_message_header(int fd, M& message) {
    static_assert(EAGAIN == EWOULDBLOCK, "EAGAIN != EWOULDBLOCK");
    fds_assert(message.header_off >= 0);
    ssize_t to_read = sizeof(typename M::header_type) - message.header_off;
    ssize_t nread = retry_read(fd,
                         reinterpret_cast<uint8_t*>(&message.header) + message.header_off,
                         to_read);
    if (nread <= 0) {
        switch (0 > nread ? errno : EPIPE) {
            case EAGAIN:
                LOGWARN << "Spurious wakeup? Have " << message.header_off << " bytes of header.";
                return false;
            case EPIPE:
                LOGNOTIFY << "Client disconnected";
            default:
                LOGERROR << "Socket read error: [" << strerror(errno) << "]";
                throw NbdError::shutdown_requested;
        }
    } else if (nread < to_read) {
        LOGWARN << "Short read : [ " << std::dec << nread << " of " << to_read << "]";
        message.header_off += nread;
        return false;
    }
    message.header_off = -1;
    message.data_off = 0;
    return true;
}

template<typename M>
ssize_t read_from_socket(int fd, M& buffer, ssize_t off, ssize_t len);

template<>
ssize_t read_from_socket(int fd, std::array<char, 1024>& buffer, ssize_t off, ssize_t len)
{ return retry_read(fd, buffer.data() + off, len); }

template<>
ssize_t read_from_socket(int fd, boost::shared_ptr<std::string>& buffer, ssize_t off, ssize_t len)
{ return retry_read(fd, &(*buffer)[0] + off, len); }

template<typename M>
bool get_message_payload(int fd, M& message) {
    static_assert(EAGAIN == EWOULDBLOCK, "EAGAIN != EWOULDBLOCK");
    fds_assert(message.data_off >= 0);
    ssize_t to_read = message.header.length - message.data_off;
    ssize_t nread = read_from_socket(fd,
                                     message.data,
                                     message.data_off,
                                     to_read);
    if (nread <= 0) {
        switch (0 > nread ? errno : EPIPE) {
            case EAGAIN:
                LOGTRACE << "Payload not there.";
                return false; // We were optimistic the payload was ready.
            case EPIPE:
                LOGNOTIFY << "Client disconnected";
            default:
                LOGERROR << "Socket read error: [" << strerror(errno) << "]";
                throw NbdError::shutdown_requested;
        }
    } else if (nread < to_read) {
        LOGWARN << "Short read : [ " << std::dec << nread << " of " << to_read << "]";
        message.data_off += nread;
        return false;
    }
    return true;
}

}  // namespace fds
