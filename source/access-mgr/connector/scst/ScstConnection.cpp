/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "connector/scst/ScstConnection.h"

#include <cerrno>
#include <string>
#include <type_traits>

extern "C" {
#include <endian.h>
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

#include "connector/scst/ScstTask.h"

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
          volumeName(vol_name),
          volume_size{0},
          resp_needed(0u),
          logical_block_size(512ul),
          readyResponses(4000)
{
    {
        FdsConfigAccessor config(g_fdsprocess->get_conf_helper());
        standalone_mode = config.get_abs<bool>("fds.am.testing.standalone", false);
    }
    {
        FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.scst.");
        logical_block_size = conf.get<uint32_t>("default_block_size", logical_block_size);
    }

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
        logical_block_size,         // Block size
        0,                          // PR cmd Notifications
        {},
        "bare_am",
    };
    snprintf(scst_descriptor.name, SCST_MAX_NAME, "%s", volumeName.c_str());
    auto res = ioctl(scstDev, SCST_USER_REGISTER_DEVICE, &scst_descriptor);
    if (0 > res) {
        LOGERROR << "Failed to register the device! [" << strerror(errno) << "]";
        throw ScstError::connection_closed;
    }

    // TODO(bszmyd): Sat 12 Sep 2015 12:14:19 PM MDT
    // We can support other target-drivers than iSCSI...TBD
    // Create an iSCSI target in the SCST mid-ware for our handler
    auto scstTgtMgmt = open("/sys/kernel/scst_tgt/targets/iscsi/fds.iscsi:tgt/luns/mgmt", O_WRONLY);
    if (0 > scstTgtMgmt) {
        LOGERROR << "Could not map lun, no iSCSI devices will be presented!";
    } else {
        static std::string add_tgt_cmd = "add " + volumeName + " 0";
        auto i = write(scstTgtMgmt, add_tgt_cmd.c_str(), add_tgt_cmd.size());
        close(scstTgtMgmt);
    }

    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set(*loop);
    ioWatcher->set<ScstConnection, &ScstConnection::ioEvent>(this);
    ioWatcher->start(scstDev, ev::READ);

    asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
    asyncWatcher->set(*loop);
    asyncWatcher->set<ScstConnection, &ScstConnection::wakeupCb>(this);
    asyncWatcher->start();

    LOGNORMAL << "New SCST device [" << volumeName
              << "] Tgt [fds.iscsi:tgt"
              << "] LUN [0"
              << "] BlockSize[" << logical_block_size << "]";
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
    if (!readyResponses.empty()) {
        ioEvent(*ioWatcher, ev::WRITE);
    } else {
        ioWatcher->start();
    }
}

void ScstConnection::execAllocCmd() {
    LOGDEBUG << "Allocation command received.";
    fds_panic("Should not be here!");
    auto task = new ScstTask(cmd->cmd_h, SCST_USER_ALLOC_MEM);
    readyResponses.push(task);
}

void ScstConnection::execCompleteCmd() {
    LOGTRACE << "Command responded.";

    auto it = repliedResponses.find(cmd->cmd_h);
    fds_assert(repliedResponses.end() != it);
    repliedResponses.erase(it);
    scst_user_reply_cmd reply { cmd->cmd_h, cmd->subcode, 0ull };
    (void) ioctl(scstDev, SCST_USER_REPLY_CMD, &reply);
}

void ScstConnection::execParseCmd() {
    LOGDEBUG << "Need parsing help.";
    fds_panic("Should not be here!");
    auto task = new ScstTask(cmd->cmd_h, SCST_USER_PARSE);
    readyResponses.push(task);
}

void ScstConnection::execTaskMgmtCmd() {
    LOGDEBUG << "Task Management request.";
    fds_panic("Should not be here!");
    auto task = new ScstTask(cmd->cmd_h, SCST_USER_TASK_MGMT_RECEIVED);
    readyResponses.push(task);
}

void
ScstConnection::execSessionCmd() {
    auto attaching = (SCST_USER_ATTACH_SESS == cmd->subcode) ? true : false;
    auto& sess = cmd->sess;
    LOGDEBUG << "Session "
        << (attaching ? "attachment" : "detachment")
        << " requested," << std::hex
        << " handle [0x" << sess.sess_h
        << "] lun [0x" << sess.lun
        << "] R/O [" << (sess.rd_only == 0 ? "false" : "true")
        << "] Init [" << sess.initiator_name
        << "] Targ [" << sess.target_name << "]";
    auto volName = boost::make_shared<std::string>(volumeName);
    if (attaching) {
        // Attach the volume to AM
        auto task = new ScstTask(cmd->cmd_h, SCST_USER_ATTACH_SESS);
        scstOps->init(volName, amProcessor, task);
    } else {
        auto task = new ScstTask(cmd->cmd_h, SCST_USER_DETACH_SESS);
        readyResponses.push(task);
    }
}

void ScstConnection::execUserCmd() {
    auto& scsi_cmd = cmd->exec_cmd;
    auto task = new ScstTask(cmd->cmd_h, SCST_USER_EXEC);

    switch (scsi_cmd.cdb[0]) {
        case TEST_UNIT_READY:
            LOGTRACE << "Test Unit Ready received.";
            break;
        case INQUIRY:
            {
                auto buffer = std::unique_ptr<uint8_t[]>(new uint8_t[36] {});

                buffer[0] = TYPE_DISK;
                // Check EVPD bit
                if (scsi_cmd.cdb[1] & 0x01) {
                    LOGDEBUG << "Inquiry request for page: [0x" << std::hex << (uint32_t)(scsi_cmd.cdb[2])
                        << "] requested.";
                    buffer[1] = scsi_cmd.cdb[2];
                    switch (scsi_cmd.cdb[2]) {
                        case 0x00: // Supported pages
                            buffer[3] = 3;
                            buffer[4] = 0; // This page
                            buffer[5] = 0x80; // unit serial number
                            buffer[6] = 0x83; // device id
                            break;
                        case 0x80: // Serial Number
                            buffer[3] = 1;
                            buffer[4] = ' ';
                            break;
                        case 0x83: // Device ID
                            buffer[3] = 12;
                            buffer[4] = 0x02; // ASCII
                            buffer[5] = 0x01; // Vendor ID
                            buffer[7] = 8;
                            memcpy(buffer.get() + 8, "FDS     ", 8);
                            break;
                        default:
                            LOGERROR << "Request for unsupported page code.";
                            task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                            return;
                    }
                    task->setResponseBuffer(buffer, buffer[3] + 4);
                } else {
                    LOGDEBUG << "Standard inquiry requested.";
                    buffer[2] = 0x06;
                    buffer[3] = 0x12;
                    buffer[4] = 31;
                    buffer[5] = 0x00;
                    buffer[6] = 0x10;
                    buffer[7] = 0x02;
                    memcpy( &buffer[8], "FDS     ",          8);
                    memcpy(&buffer[16], "SCST_VOL        ", 16);
                    memcpy(&buffer[32], "BETA",              4);
                    task->setResponseBuffer(buffer, buffer[4] + 5);
                }
            }
            break;
        case MODE_SENSE:
            {
                bool dbd = (0x00 != (scsi_cmd.cdb[1] & 0x08));
                uint8_t pc = scsi_cmd.cdb[2] / 0x40;
                uint8_t page_code = scsi_cmd.cdb[2] % 0x40;
                uint8_t& subpage = scsi_cmd.cdb[3];
                LOGIO << "Mode Sense: "
                      << " dbd[" << std::hex << dbd
                      << "] pc[" << std::hex << (uint32_t)pc
                      << "] page_code[" << (uint32_t)page_code
                      << "] subpage[" << (uint32_t)subpage << "]";

                // We do not support any subpages
                if (0x00 != subpage) {
                    task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                }
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_opcode));
            }
            break;
        case READ_10:
        case READ_12:
            {
                uint32_t lba = be32toh(*reinterpret_cast<uint32_t*>(scsi_cmd.cdb + 2));
                uint32_t blocks = (READ_10 == scsi_cmd.cdb[0]) ?
                                    be16toh(*reinterpret_cast<uint16_t*>(scsi_cmd.cdb + 7)) :
                                    be32toh(*reinterpret_cast<uint32_t*>(scsi_cmd.cdb + 6));

                LOGIO << "Read received for "
                      << "LBA[0x" << std::hex << lba
                      << "] Blocks[0x" << blocks
                      << "] Handle[0x" << cmd->cmd_h << "]";
                uint64_t offset = lba * logical_block_size;
                uint32_t read_length = blocks * logical_block_size;
                task->setRead(offset, read_length);
                return scstOps->read(task);
            }
            break;
        case READ_CAPACITY_16:  // READ_CAPACITY(16)
            {
                LOGDEBUG << "Read Capacity received.";
                auto buffer = std::unique_ptr<uint8_t[]>(new uint8_t[32] {});

                uint64_t num_blocks = volume_size / logical_block_size;
                uint32_t blocks_per_object = physical_block_size / logical_block_size;
                *reinterpret_cast<uint64_t*>(&buffer[0]) = htobe64(num_blocks);
                *reinterpret_cast<uint32_t*>(&buffer[8]) = htobe32(logical_block_size);

                // Number of logic blocks per object as a power of 2
                buffer[13] = (uint8_t)__builtin_ctz(blocks_per_object) & 0xFF;
                task->setResponseBuffer(buffer, 32);
            }
            break;
        default:
            LOGNOTIFY << "Unsupported SCSI command received " << std::hex
                << "OPCode [0x" << (uint32_t)(scsi_cmd.cdb[0])
                << "] CDB length [" << std::dec << scsi_cmd.cdb_len
                << "]";
            task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_opcode));
            break;
    }
    readyResponses.push(task);
}

bool
ScstConnection::getAndRespond() {
    if (!cmd) {
        cmd.reset(new scst_user_get_cmd {});
    }

    // If we do not have response then clear reply, otherwise read one into it
    if (readyResponses.empty()) {
        cmd->preply = 0ull;
    } else {
        ScstTask* resp {nullptr};
        ensure(readyResponses.pop(resp));
        cmd->preply = resp->getReply();
        repliedResponses[resp->getHandle()].reset(resp);
    }

    int res = 0;
    cmd->cmd_h = 0x00;
    cmd->subcode = 0x00;
    do {
    auto res = ioctl(scstDev, SCST_USER_REPLY_AND_GET_CMD, cmd.get());
    } while ((0 > res) && (EINTR == errno));

    if (0 != res || 0x00 == cmd->subcode) {
        switch (errno) {
            case ENOTTY:
            case EBADF:
                LOGNOTIFY << "Scst device no longer has a valid handle to the kernel, terminating.";
                throw ScstError::connection_closed;
            case EFAULT:
            case EINVAL:
                fds_panic("Invalid Scst argument!");
            default:
                return false;
        }
    }

    LOGTRACE << "Received SCST command: "
             << "[0x" << std::hex << cmd->cmd_h
             << "] sc [0x" << cmd->subcode << "]";
    switch (cmd->subcode) {
        case SCST_USER_ATTACH_SESS:
        case SCST_USER_DETACH_SESS:
            execSessionCmd();
            break;
        case SCST_USER_ON_FREE_CMD:
            execCompleteCmd();
            break;
        case SCST_USER_TASK_MGMT_RECEIVED:
        case SCST_USER_TASK_MGMT_DONE:
            execTaskMgmtCmd();
            break;
        case SCST_USER_ALLOC_MEM:
        case SCST_USER_ON_CACHED_MEM_FREE:
            execAllocCmd();
            break;
        case SCST_USER_PARSE:
            execParseCmd();
            break;
        case SCST_USER_EXEC:
            execUserCmd();
            break;
        default:
            LOGNOTIFY << "Received unknown Scst subcommand: [" << cmd->subcode << "]";
            return false;
    }
    return true;
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

    // We are guaranteed to be the only thread acting on this file descriptor
    try {
        // Get the next command, and/or reply to any existing finished commands
        while (getAndRespond()) {
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
ScstConnection::respondTask(ScstTask* response) {
    LOGDEBUG << " response from ScstOperations handle: 0x" << std::hex << response->getHandle()
             << " " << response->getError();

    if (response->isRead()) {
        if (!response->getError().ok()) {
            response->checkCondition(SCST_LOAD_SENSE(scst_sense_read_error));
        } else {
            auto buffer = std::unique_ptr<uint8_t[]>(new uint8_t[response->getLength()]);
            fds_uint32_t i = 0, context = 0;
            boost::shared_ptr<std::string> buf = response->getNextReadBuffer(context);
            while (buf != NULL) {
                LOGDEBUG << "Handle 0x" << std::hex << response->getHandle()
                         << "...Size 0x" << buf->length() << "B"
                         << "...Buffer # " << std::dec << context;
                memcpy(buffer.get() + i, buf->c_str(), buf->length());
                i += buf->length();
                buf = response->getNextReadBuffer(context);
            }
            response->setResponseBuffer(buffer, response->getLength());
        }
    } else if (!response->getError().ok()) {
        response->checkCondition(SCST_LOAD_SENSE(scst_sense_write_error));
    }

    // add to quueue
    readyResponses.push(response);

    // We have something to write, so poke the loop
    asyncWatcher->send();
}

void
ScstConnection::attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) {
    // capacity is in MB
    LOGNORMAL << "Attached to volume with capacity: " << volDesc->capacity
        << "MiB and object size: " << volDesc->maxObjSizeInBytes << "B";
    physical_block_size = volDesc->maxObjSizeInBytes;
    volume_size = (volDesc->capacity * Mi);
}

}  // namespace fds
