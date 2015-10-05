/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#include "connector/nbd/NbdConnection.h"

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

#include "connector/nbd/NbdConnector.h"
#include "fds_process.h"
#include "fds_volume.h"
#include "fdsp/config_types_types.h"


/// These constants come from the Nbd Protocol
/// ******************************************
static constexpr uint8_t NBD_MAGIC[]    = { 0x49, 0x48, 0x41, 0x56, 0x45, 0x4F, 0x50, 0x54 };
static constexpr char    NBD_MAGIC_PWD[]  {'N', 'B', 'D', 'M', 'A', 'G', 'I', 'C'};  // NOLINT
static constexpr uint8_t NBD_REQUEST_MAGIC[]    = { 0x25, 0x60, 0x95, 0x13 };
static constexpr uint8_t NBD_RESPONSE_MAGIC[]   = { 0x67, 0x44, 0x66, 0x98 };
static constexpr uint8_t NBD_PROTO_VERSION[]    = { 0x00, 0x01 };
static constexpr int32_t NBD_OPT_EXPORT         = 1;
static constexpr int16_t NBD_FLAG_HAS_FLAGS     = 0b000001;
static constexpr int16_t NBD_FLAG_READ_ONLY     = 0b000010;
static constexpr int16_t NBD_FLAG_SEND_FLUSH    = 0b000100;
static constexpr int16_t NBD_FLAG_SEND_FUA      = 0b001000;
static constexpr int16_t NBD_FLAG_ROTATIONAL    = 0b010000;
static constexpr int16_t NBD_FLAG_SEND_TRIM     = 0b100000;
static constexpr int32_t NBD_CMD_READ           = 0;
static constexpr int32_t NBD_CMD_WRITE          = 1;
static constexpr int32_t NBD_CMD_DISC           = 2;
static constexpr int32_t NBD_CMD_FLUSH          = 3;
static constexpr int32_t NBD_CMD_TRIM           = 4;
/// ******************************************


/// Some useful constants for us
/// ******************************************
static constexpr size_t Ki = 1024;
static constexpr size_t Mi = Ki * Ki;
static constexpr size_t Gi = Ki * Mi;
static constexpr ssize_t max_block_size = 8 * Mi;
/// ******************************************

template<typename T>
constexpr auto to_iovec(T* t) -> typename std::remove_cv<T>::type*
{ return const_cast<typename std::remove_cv<T>::type*>(t); }

static constexpr bool ensure(bool b)
{ return (!b ? throw fds::BlockError::connection_closed : true); }

static std::array<std::string, 5> const io_to_string = {
    { "READ", "WRITE", "DISCONNECT", "FLUSH", "TRIM" }
};

static std::array<std::string, 6> const state_to_string = {
    { "INVALID", "PREINIT", "POSTINIT", "AWAITOPTS", "SENDOPTS", "DOREQS" }
};

namespace fds
{

template<typename M>
bool get_message_header(int fd, M& message);

template<typename M>
bool get_message_payload(int fd, M& message);

NbdConnection::NbdConnection(NbdConnector* server,
                             std::shared_ptr<ev::dynamic_loop> loop,
                             int clientsd,
                             std::shared_ptr<AmProcessor> processor)
        : amProcessor(processor),
          nbd_server(server),
          nbdOps(boost::make_shared<BlockOperations>(this)),
          clientSocket(clientsd),
          volume_size{0},
          object_size{0},
          nbd_state(NbdProtoState::PREINIT),
          resp_needed(0u),
          handshake({ { 0x01u },  0x00ull, 0x00ull, nullptr }),
          response(nullptr),
          total_blocks(0ull),
          write_offset(-1ll),
          readyResponses(4000),
          current_response(nullptr)
{
    memset(&attach, '\0', sizeof(attach));
    memset(&request, '\0', sizeof(request));

    FdsConfigAccessor config(g_fdsprocess->get_conf_helper());
    standalone_mode = config.get_abs<bool>("fds.am.testing.standalone", false);

    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set(*loop);
    ioWatcher->set<NbdConnection, &NbdConnection::ioEvent>(this);
    ioWatcher->start(clientSocket, ev::READ | ev::WRITE);

    asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
    asyncWatcher->set(*loop);
    asyncWatcher->set<NbdConnection, &NbdConnection::wakeupCb>(this);
    asyncWatcher->start();

    LOGNORMAL << "New NBD client connection for " << clientSocket;
}

NbdConnection::~NbdConnection() {
    LOGNORMAL << "NBD client disconnected for " << clientSocket;
    shutdown(clientSocket, SHUT_RDWR);
    close(clientSocket);
}

void
NbdConnection::terminate() {
    state_ = ConnectionState::STOPPING;
    asyncWatcher->send();
}

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
                throw BlockError::connection_closed;
            }
        } else {
            // Didn't write all the data yet, update offset
            write_offset += nwritten;
        }
        return false;
    }
    total_blocks = 0;
    write_offset = -1;
    return true;
}

// Send initial message to client with NBD magic and proto version
bool NbdConnection::handshake_start(ev::io &watcher) {
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

bool NbdConnection::handshake_complete(ev::io &watcher) {
    if (!get_message_header(watcher.fd, handshake))
        return false;
    ensure(0 == handshake.header.ack);
    return true;
}

bool NbdConnection::option_request(ev::io &watcher) {
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
    LOGNORMAL << "Attaching volume name " << *volumeName;
    if (standalone_mode) {
        object_size = 4 * Ki;
        volume_size = 1 * Gi;
        nbd_state = NbdProtoState::SENDOPTS;
        asyncWatcher->send();
    } else {
        auto task = new BlockTask(0ull);
        nbdOps->init(volumeName, amProcessor, task);
    }

    return true;
}

bool
NbdConnection::option_reply(ev::io &watcher) {
    static char const zeros[124]{0};  // NOLINT
    static int16_t const optFlags =
        ntohs(NBD_FLAG_HAS_FLAGS);
    static iovec const vectors[] = {
        { nullptr,             sizeof(volume_size) },
        { to_iovec(&optFlags), sizeof(optFlags)    },
        { to_iovec(zeros),     sizeof(zeros)       },
    };

    // Verify we got a valid volume size from attach
    if (0 == volume_size) {
        throw BlockError::connection_closed;
    }

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

bool NbdConnection::io_request(ev::io &watcher) {
    if (request.header_off >= 0) {
        if (!get_message_header(watcher.fd, request))
            return false;
        ensure(0 == memcmp(NBD_REQUEST_MAGIC, request.header.magic, sizeof(NBD_REQUEST_MAGIC)));
        request.header.opType = ntohl(request.header.opType);
        request.header.offset = __builtin_bswap64(request.header.offset);
        request.header.length = ntohl(request.header.length);

        if (max_block_size < request.header.length) {
            LOGWARN << "Client used a block size, "
                    << request.header.length << "B, "
                    << "larger than the supported " << max_block_size << "B.";
            throw BlockError::shutdown_requested;
        }

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
          << " offset 0x" << request.header.offset
          << " length 0x" << request.header.length << std::dec
          << " ahead of you: " <<  resp_needed++;

    Error err = dispatchOp();
    request.data.reset();
    ensure(ERR_OK == err);
    return true;
}

bool
NbdConnection::io_reply(ev::io &watcher) {
    static int32_t const error_ok = htonl(0);
    static int32_t const error_bad = htonl(-1);

    // We can reuse this from now on since we don't go to any state from here
    if (!response) {
        response = resp_vector_type(new iovec[(max_block_size / object_size) + 3]);
        response[0].iov_base = to_iovec(NBD_RESPONSE_MAGIC);
        response[0].iov_len = sizeof(NBD_RESPONSE_MAGIC);
        response[1].iov_base = to_iovec(&error_ok); response[1].iov_len = sizeof(error_ok);
        response[2].iov_base = nullptr; response[2].iov_len = 0;
        response[3].iov_base = nullptr; response[3].iov_len = 0ull;
    }

    Error err = ERR_OK;
    if (write_offset == -1) {
        if (readyResponses.empty())
            { return false; }

        total_blocks = 3;

        BlockTask* resp = nullptr;
        ensure(readyResponses.pop(resp));
        current_response.reset(resp);

        response[2].iov_base = &current_response->handle;
        response[2].iov_len = sizeof(current_response->handle);
        response[1].iov_base = to_iovec(&error_ok);
        if (!current_response->getError().ok()) {
            err = current_response->getError();
            response[1].iov_base = to_iovec(&error_bad);
            LOGERROR << "Returning error: " << err;
        } else if (current_response->isRead()) {
            fds_uint32_t context = 0;
            boost::shared_ptr<std::string> buf = current_response->getNextReadBuffer(context);
            while (buf != NULL) {
                LOGDEBUG << "Handle 0x" << std::hex << current_response->handle
                         << "...Size 0x" << buf->length() << "B"
                         << "...Buffer # " << std::dec << context;
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
          << --resp_needed << " requests behind you";

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
        case NBD_CMD_READ:
            {
                auto task = new BlockTask(handle);
                task->setRead(offset, length);
                nbdOps->read(task);
            }
            break;
        case NBD_CMD_WRITE:
            {
                fds_assert(request.data);
                auto task = new BlockTask(handle);
                task->setWrite(offset, length);
                nbdOps->write(request.data, task);
            }
            break;
        case NBD_CMD_FLUSH:
            break;
        case NBD_CMD_DISC:
            LOGNORMAL << "Got a disconnect";
        default:
            throw BlockError::shutdown_requested;
            break;
    }
    return ERR_OK;
}

void
NbdConnection::wakeupCb(ev::async &watcher, int revents) {
    if (processing_) return;
    if (ConnectionState::STOPPED == state_ || ConnectionState::STOPPING == state_) {
        nbdOps->shutdown();
        if (ConnectionState::STOPPED == state_) {
            asyncWatcher->stop();
            ioWatcher->stop();
            nbdOps.reset();
            return;
        }
    }

    // It's ok to keep writing responses if we've been shutdown
    auto writting = (nbd_state == NbdProtoState::SENDOPTS ||
                     current_response ||
                     !readyResponses.empty()) ? ev::WRITE : ev::NONE;

    ioWatcher->set(writting | ev::READ);
    ioWatcher->start();
}

void
NbdConnection::ioEvent(ev::io &watcher, int revents) {
    if (processing_ || (EV_ERROR & revents)) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    ioWatcher->stop();
    processing_ = true;
    // Unblocks the next thread to listen on the ev loop
    nbd_server->process();

    try {
    if (revents & EV_READ) {
        switch (nbd_state) {
            case NbdProtoState::POSTINIT:
                if (handshake_complete(watcher))
                    { nbd_state = NbdProtoState::AWAITOPTS; }
                break;
            case NbdProtoState::AWAITOPTS:
                option_request(watcher);
                break;
            case NbdProtoState::DOREQS:
                // Read all the requests off the socket
                while (io_request(watcher))
                    { continue; }
                break;
            default:
                LOGDEBUG << "Asked to read in state: "
                         << state_to_string[static_cast<uint32_t>(nbd_state)];
                // We could have read and writes waiting and are not in the
                // correct state to handle more requests...yet
                break;
        }
    }

    if (revents & EV_WRITE) {
        switch (nbd_state) {
            case NbdProtoState::PREINIT:
                if (handshake_start(watcher)) {
                    nbd_state = NbdProtoState::POSTINIT;
                }
                break;
            case NbdProtoState::SENDOPTS:
                if (option_reply(watcher)) {
                    nbd_state = NbdProtoState::DOREQS;
                    LOGDEBUG << "Done with NBD handshake";
                }
                break;
            case NbdProtoState::DOREQS:
                // Write everything we can
                while (io_reply(watcher))
                    { continue; }
                break;
            default:
                fds_panic("Unknown NBD connection state %d", nbd_state);
        }
    }
    } catch(BlockError const& e) {
        state_ = ConnectionState::STOPPING;
        if (e == BlockError::connection_closed) {
            // If we had an error, stop the event loop too
            state_ = ConnectionState::STOPPED;
        }
    }
    // Unblocks the ev loop to handle events again on this connection
    processing_ = false;
    asyncWatcher->send();
    nbd_server->follow();
}

void
NbdConnection::respondTask(BlockTask* response) {
    LOGDEBUG << " response from BlockOperations handle: 0x" << std::hex << response->getHandle()
             << " " << response->getError();

    // add to quueue
    if (response->isRead() || response->isWrite()) {
        readyResponses.push(response);
    } else {
        delete response;
    }

    // We have something to write, so poke the loop
    asyncWatcher->send();
}

void
NbdConnection::attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) {
    if (volDesc) {
        // capacity is in MB
        LOGNORMAL << "Attached to volume with capacity: " << volDesc->capacity
                  << "MiB and object size: " << volDesc->maxObjSizeInBytes << "B";
        object_size = volDesc->maxObjSizeInBytes;
        volume_size = __builtin_bswap64(volDesc->capacity * Mi);
    }
    nbd_state = NbdProtoState::SENDOPTS;
}

ssize_t retry_read(int fd, void* buf, size_t count) {
    ssize_t e = 0;
    do {
        e = read(fd, buf, count);
    } while ((0 > e) && (EINTR == errno));
    return e;
}

template<typename M>
ssize_t read_from_socket(int fd, M& buffer, ssize_t off, ssize_t len);

template<>
ssize_t read_from_socket(int fd, uint8_t*& buffer, ssize_t off, ssize_t len)
{ return retry_read(fd, buffer + off, len); }

template<>
ssize_t read_from_socket(int fd, std::array<char, 1024>& buffer, ssize_t off, ssize_t len)
{ return retry_read(fd, buffer.data() + off, len); }

template<>
ssize_t read_from_socket(int fd, boost::shared_ptr<std::string>& buffer, ssize_t off, ssize_t len)
{ return retry_read(fd, &(*buffer)[0] + off, len); }

template<typename D>
bool nbd_read(int fd, D& data, ssize_t& off, ssize_t const len)
{
    static_assert(EAGAIN == EWOULDBLOCK, "EAGAIN != EWOULDBLOCK");
    // If we've nothing to read, done
    fds_assert(0 != len); // Know about this in DEBUG...logic error?
    if (0 == len) return true;

    ssize_t nread = read_from_socket(fd, data, off, len);
    if (0 > nread) {
        switch (0 > nread ? errno : EPIPE) {
            case EAGAIN:
                LOGTRACE << "Payload not there.";
                return false; // We were optimistic the payload was ready.
            case EPIPE:
                LOGNOTIFY << "Client disconnected";
            default:
                LOGERROR << "Socket read error: [" << strerror(errno) << "]";
                throw BlockError::shutdown_requested;
        }
    } else if (0 == nread) {
        // Orderly shutdown of the TCP connection
        LOGNORMAL << "Client disconnected.";
        throw BlockError::connection_closed;
    } else if (nread < len) {
        // Only received some of the data so far
        off += nread;
        return false;
    }
    return true;
}

template<typename M>
bool get_message_header(int fd, M& message) {
    fds_assert(message.header_off >= 0);
    ssize_t to_read = sizeof(typename M::header_type) - message.header_off;

    auto buffer = reinterpret_cast<uint8_t*>(&message.header);
    if (nbd_read(fd, buffer, message.header_off, to_read))
    {
        message.header_off = -1;
        message.data_off = 0;
        return true;
    }
    return false;
}

template<typename M>
bool get_message_payload(int fd, M& message) {
    fds_assert(message.data_off >= 0);
    ssize_t to_read = message.header.length - message.data_off;

    return nbd_read(fd, message.data, message.data_off, to_read);
}

}  // namespace fds
