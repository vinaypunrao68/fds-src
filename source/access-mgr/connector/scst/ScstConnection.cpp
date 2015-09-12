/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "connector/scst/ScstConnection.h"

#include <cerrno>
#include <string>
#include <type_traits>

extern "C" {
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
}

#include <ev++.h>

#include "connector/scst/ScstConnector.h"
#include "fds_process.h"
#include "fds_volume.h"
#include "fdsp/config_types_types.h"
extern "C" {
#include "connector/scst/scst_user.h"
}

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
{ return (!b ? throw fds::ScstError::connection_closed : true); }

namespace fds
{

ScstConnection::ScstConnection(std::string const& vol_name,
                               ScstConnector* server,
                               std::shared_ptr<ev::dynamic_loop> loop,
                               std::shared_ptr<AmProcessor> processor)
        : amProcessor(processor),
          scst_server(server),
          scstOps(boost::make_shared<ScstOperations>(this)),
          volume_size{0},
          object_size{0},
          scst_state(ScstProtoState::PREINIT),
          resp_needed(0u),
          response(nullptr),
          total_blocks(0ull),
          write_offset(-1ll),
          readyResponses(4000),
          current_response(nullptr)
{
    FdsConfigAccessor config(g_fdsprocess->get_conf_helper());
    standalone_mode = config.get_abs<bool>("fds.am.testing.standalone", false);

    // Open the SCST user device
    if (0 > (scstDev = openScst())) {
        return;
    }

    // XXX(bszmyd): Fri 11 Sep 2015 08:25:48 AM MDT
    // REGISTER a device
    static scst_user_dev_desc scst_descriptor {
        (unsigned long)DEV_USER_VERSION, // Constant
        (unsigned long)"GPL",       // License string
        TYPE_DISK,                  // Device type
        0, 0, 0, 0,                 // SGV options
        {                           // SCST options
                SCST_USER_PARSE_STANDARD,                   // parse type
                SCST_USER_ON_FREE_CMD_CALL,                 // command on-free type
                SCST_USER_MEM_NO_REUSE,                     // command reuse type
                SCST_USER_PARTIAL_TRANSFERS_NOT_SUPPORTED,  // partial transfer type
                0,                                          // partial transfer length
                SCST_TST_1_SEP_TASK_SETS,                   // task set sharing
                0,                                          // task mgmt only (on fault)
                SCST_QUEUE_ALG_1_UNRESTRICTED_REORDER,      // reordering however
                SCST_QERR_0_ALL_RESUME,                     // fault does not abort all cmds
                0, 0, 0, 0                                  // TAS/SWAP/DSENSE/ORDERING
        },
        4096,                       // Block size
        0,                          // PR cmd Notifications
        {},
        "bare_am",
    };
    snprintf(scst_descriptor.name, SCST_MAX_NAME, "%s", vol_name.c_str());
    auto res = ioctl(scstDev, SCST_USER_REGISTER_DEVICE, &scst_descriptor);
    if (0 > res) {
        LOGERROR << "Failed to register the device! [" << strerror(errno) << "]";
        throw ScstError::connection_closed;
    }

    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set(*loop);
    ioWatcher->set<ScstConnection, &ScstConnection::ioEvent>(this);
    ioWatcher->start(scstDev, ev::READ | ev::WRITE);

    asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
    asyncWatcher->set(*loop);
    asyncWatcher->set<ScstConnection, &ScstConnection::wakeupCb>(this);
    asyncWatcher->start();

    LOGNORMAL << "New SCST devcie " << scstDev;
}

ScstConnection::~ScstConnection() {
    LOGNORMAL << "SCST client disconnected for " << scstDev;
}

void
ScstConnection::terminate() {
    state_ = ConnectionState::STOPPING;
    asyncWatcher->send();
}

int
ScstConnection::openScst() {
    int dev = open(DEV_USER_PATH DEV_USER_NAME, O_RDWR | O_NONBLOCK);
    if (0 > dev) {
        LOGERROR << "Opening the SCST device failed: " << strerror(errno);
    }
    return dev;
}

void
ScstConnection::wakeupCb(ev::async &watcher, int revents) {
    if (processing_) return;
    if (ConnectionState::STOPPED == state_ || ConnectionState::STOPPING == state_) {
        scstOps->shutdown();
        if (ConnectionState::STOPPED == state_) {
            asyncWatcher->stop();
            ioWatcher->stop();
            scstOps.reset();
            return;
        }
    }

    // It's ok to keep writing responses if we've been shutdown
    auto writting = (scst_state == ScstProtoState::SENDOPTS ||
                     current_response ||
                     !readyResponses.empty()) ? ev::WRITE : ev::NONE;

    ioWatcher->set(writting | ev::READ);
    ioWatcher->start();
}

void
ScstConnection::ioEvent(ev::io &watcher, int revents) {
    if (processing_ || (EV_ERROR & revents)) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    ioWatcher->stop();
    processing_ = true;
    // Unblocks the next thread to listen on the ev loop
    scst_server->process();

    try {
    if (revents & EV_READ) {
        LOGDEBUG << "Got Read event from SCST!";
    }

    if (revents & EV_WRITE) {
        LOGDEBUG << "Got Write event from SCST!";
    }
    } catch(ScstError const& e) {
        state_ = ConnectionState::STOPPING;
        if (e == ScstError::connection_closed) {
            // If we had an error, stop the event loop too
            state_ = ConnectionState::STOPPED;
        }
    }
    // Unblocks the ev loop to handle events again on this connection
    processing_ = false;
    asyncWatcher->send();
    scst_server->follow();
}

void
ScstConnection::readWriteResp(ScstResponseVector* response) {
    LOGDEBUG << (response->isRead() ? "READ" : "WRITE")
              << " response from ScstOperations handle: 0x" << std::hex << response->handle
              << " " << response->getError();

    // add to quueue
    readyResponses.push(response);

    // We have something to write, so poke the loop
    asyncWatcher->send();
}

void
ScstConnection::attachResp(Error const& error, boost::shared_ptr<VolumeDesc> const& volDesc) {
    if (ERR_OK == error) {
        // capacity is in MB
        LOGNORMAL << "Attached to volume with capacity: " << volDesc->capacity
                  << "MiB and object size: " << volDesc->maxObjSizeInBytes << "B";
        object_size = volDesc->maxObjSizeInBytes;
        volume_size = __builtin_bswap64(volDesc->capacity * Mi);
    }
    scst_state = ScstProtoState::SENDOPTS;
    asyncWatcher->send();
}

}  // namespace fds
