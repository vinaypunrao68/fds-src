/*
 * ScstDevice.cpp
 *
 * Copyright (c) 2015, Brian Szmyd <szmyd@formationds.com>
 * Copyright (c) 2015, Formation Data Systems
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "connector/scst/ScstDevice.h"

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
#include <boost/make_shared.hpp>

// TODO(bszmyd): Sun 18 Oct 2015 04:44:55 PM MDT
// REMOVE THIS!!!!
// =================
#include "fds_process.h"
#include "fds_config.hpp"
#include "util/Log.h"
// =================

#include "connector/scst/ScstTarget.h"
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

static constexpr bool ensure(bool b)
{ return (!b ? throw fds::BlockError::connection_closed : true); }

namespace fds
{

ScstDevice::ScstDevice(std::string const& vol_name,
                       ScstTarget* target,
                       std::shared_ptr<AmProcessor> processor)
        : amProcessor(processor),
          scst_target(target),
          scstOps(boost::make_shared<BlockOperations>(this)),
          volumeName(vol_name),
          logical_block_size(512ul),
          readyResponses(4000)
{
    // When we are first up, the serial number is just spaces.
    snprintf(serial_number, sizeof(serial_number), "%32.0lX", 0ul);
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
        throw ScstError::scst_not_found;
    }

    // XXX(bszmyd): Fri 11 Sep 2015 08:25:48 AM MDT
    // REGISTER a device
    scst_user_dev_desc scst_descriptor {
        (unsigned long)DEV_USER_VERSION, // Constant
        (unsigned long)"GPL",       // License string
        TYPE_DISK,                  // Device type
        1, 0, 0, 0,                 // SGV enabled
        {                           // SCST options
                SCST_USER_PARSE_STANDARD,                   // parse type
                SCST_USER_ON_FREE_CMD_CALL,                 // command on-free type
                SCST_USER_MEM_REUSE_ALL,                    // buffer reuse type
                SCST_USER_PARTIAL_TRANSFERS_NOT_SUPPORTED,  // partial transfer type
                0,                                          // partial transfer length
                SCST_TST_0_SINGLE_TASK_SET,                 // task set sharing
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
        GLOGERROR << "Failed to register the device! [" << strerror(errno) << "]";
        throw ScstError::scst_error;
    }

    GLOGNORMAL << "New SCST device [" << volumeName
               << "] BlockSize[" << logical_block_size << "]";
}

void
ScstDevice::start(std::shared_ptr<ev::dynamic_loop> loop) {
    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set(*loop);
    ioWatcher->set<ScstDevice, &ScstDevice::ioEvent>(this);
    ioWatcher->start(scstDev, ev::READ);

    asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
    asyncWatcher->set(*loop);
    asyncWatcher->set<ScstDevice, &ScstDevice::wakeupCb>(this);
    asyncWatcher->start();
}

ScstDevice::~ScstDevice() {
    GLOGNORMAL << "SCST client disconnected for " << scstDev;
}

void
ScstDevice::terminate() {
    state_ = ConnectionState::STOPPING;
    asyncWatcher->send();
}

int
ScstDevice::openScst() {
    int dev = open(DEV_USER_PATH DEV_USER_NAME, O_RDWR | O_NONBLOCK);
    if (0 > dev) {
        LOGERROR << "Opening the SCST device failed: " << strerror(errno);
    }
    return dev;
}

void
ScstDevice::wakeupCb(ev::async &watcher, int revents) {
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

void ScstDevice::execAllocCmd() {
    auto& length = cmd.alloc_cmd.alloc_len;
    LOGTRACE << "Allocation of [0x" << std::hex << length << "] bytes requested.";

    // Allocate a page aligned memory buffer for Scst usage
    ensure(0 == posix_memalign((void**)&fast_reply.alloc_reply.pbuf, sysconf(_SC_PAGESIZE), length));
    fastReply();
}

void ScstDevice::execMemFree() {
    LOGTRACE << "Deallocation requested.";
    free((void*)cmd.on_cached_mem_free.pbuf);
    fastReply(); // Setup the reply for the next ioctl
}

void ScstDevice::execCompleteCmd() {
    LOGTRACE << "Command complete: [0x" << std::hex << cmd.cmd_h << "]";

    auto it = repliedResponses.find(cmd.cmd_h);
    fds_assert(repliedResponses.end() != it);
    if (!cmd.on_free_cmd.buffer_cached && 0 < cmd.on_free_cmd.pbuf) {
        free((void*)cmd.on_free_cmd.pbuf);
    }
    repliedResponses.erase(it);
    fastReply(); // Setup the reply for the next ioctl
}

void ScstDevice::execParseCmd() {
    LOGDEBUG << "Need parsing help.";
    fds_panic("Should not be here!");
    auto task = new ScstTask(cmd.cmd_h, SCST_USER_PARSE);
    readyResponses.push(task);
}

void ScstDevice::execTaskMgmtCmd() {
    LOGDEBUG << "Task Management request.";
    // We do not support Aborts/Resets/etc. at this point, so do nothing.
    fastReply(); // Setup the reply for the next ioctl
}

void
ScstDevice::execSessionCmd() {
    auto attaching = (SCST_USER_ATTACH_SESS == cmd.subcode) ? true : false;
    auto& sess = cmd.sess;
    LOGDEBUG << "Session "
        << (attaching ? "attachment" : "detachment")
        << " requested," << std::hex
        << " handle [0x" << sess.sess_h
        << "] lun [0x" << sess.lun
        << "] R/O [" << (sess.rd_only == 0 ? "false" : "true")
        << "] Init [" << sess.initiator_name
        << "] Targ [" << sess.target_name << "]";
    auto volName = boost::make_shared<std::string>(volumeName);
    if (attaching && (0 == sessions++ || 0 == volume_size)) {
        // Attach the volume to AM if we haven't already
        auto task = new ScstTask(cmd.cmd_h, SCST_USER_ATTACH_SESS);
        return scstOps->init(volName, amProcessor, task); // Defer
    } else {
        if (0 > sessions && 0 == --sessions) {
            scstOps->detachVolume();
        }
    }
    fastReply(); // Setup the reply for the next ioctl
}

void ScstDevice::execUserCmd() {
    auto& scsi_cmd = cmd.exec_cmd;
    auto& op_code = scsi_cmd.cdb[0];
    auto task = new ScstTask(cmd.cmd_h, SCST_USER_EXEC);

    auto buffer = (uint8_t*)scsi_cmd.pbuf;
    if (!buffer && 0 < scsi_cmd.alloc_len) {
        ensure(0 == posix_memalign((void**)&buffer, sysconf(_SC_PAGESIZE), scsi_cmd.alloc_len));
    }

    // Poor man's goto
    do {
    if (0 == volume_size) {
        task->checkCondition(SCST_LOAD_SENSE(scst_sense_not_ready));
        continue;
    }

    switch (op_code) {
    case TEST_UNIT_READY:
        LOGTRACE << "Test Unit Ready received.";
        break;
    case FORMAT_UNIT:
        {
            LOGTRACE << "Format Unit received.";
            bool fmtpinfo = (0x00 != (scsi_cmd.cdb[1] & 0x80));
            bool fmtdata = (0x00 != (scsi_cmd.cdb[1] & 0x10));

            // Mutually exclusive (and not supported)
            if (fmtdata || fmtpinfo) {
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                continue;
            }
            // Nothing to do as we don't support data patterns...done!
            break;
        }
    case INQUIRY:
        {
            static uint8_t const vendor_name[] = { 'F', 'D', 'S', ' ', ' ', ' ', ' ', ' ' };
            auto& buflen = scsi_cmd.bufflen;
            if (buflen < 8) {
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                continue;
            }
            bzero(buffer, buflen);

            buffer[0] = TYPE_DISK;
            // Check EVPD bit
            if (scsi_cmd.cdb[1] & 0x01) {
                auto& page = scsi_cmd.cdb[2];
                LOGDEBUG << "Page: [0x" << std::hex << (uint32_t)(page) << "] requested.";
                switch (page) {
                case 0x00: // Supported pages
                    /* |                                 PAGE                                  |*/
                    /* |                                LENGTH                                 |*/
                    /* |                              (PAGE CODE)                              |*/
                    /* |                              (   ...   )                              |*/
                    static uint8_t const supported_page_header [] = {
                        0,
                        3,
                        0x00,   // This Page
                        0x80,   // Unit Serial
                        0x83,   // Vendor ID Page
                    };
                    memcpy(&buffer[2], supported_page_header, sizeof(supported_page_header));
                    break;
                case 0x80: // Unit Serial
                    /* |                                 PAGE                                  |*/
                    /* |                                LENGTH                                 |*/
                    /* |                                SERIAL                                 |*/
                    /* |                                NUMBER                                 |*/
                    /* |                                 ...                                   |*/
                    static uint8_t const serial_number_header [] = {
                        0,
                        32
                    };
                    memcpy(&buffer[2], serial_number_header, std::min((size_t)buflen, sizeof(serial_number_header)));
                    memcpy(&buffer[2] + sizeof(serial_number_header),
                           serial_number,
                           std::min(buflen - sizeof(serial_number_header), sizeof(serial_number) - 1));
                    break;
                case 0x83: // Device ID
                    /* |                                 PAGE                                  |*/
                    /* |                                LENGTH                                 |*/
                    /* |        PROTOCOL IDENTIFIER        |              CODE SET             |*/
                    /* |  PIV   |  resv  |   ASSOCIATION   |           DESIGNATOR TYPE         |*/
                    static uint8_t const device_id_header [] = {
                        0,
                        12,
                        0x02,       // ASCII
                        0b00000001, // T10 Vendor ID
                        8           // ID Length
                    };
                    memcpy(&buffer[2], device_id_header, sizeof(device_id_header));

                    // If our buffer is big enough, copy the vendor id
                    if (16 <= buflen) {
                        memcpy(buffer + 8, vendor_name, sizeof(vendor_name));
                    }
                    break;
                default:
                    LOGERROR << "Request for unsupported page code.";
                    task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                    continue;
                }
                buffer[1] = page; // Identity of the page we're returning
                task->setResponseBuffer(buffer, buffer[3] + 4);
            } else {
                LOGDEBUG << "Standard inquiry requested.";
                /* /-----------------------------------------------------------------------\
                 * |                                VERSION                                |
                 * |  resv  |  resv  |  NACA  | HISUP  |           DATA FORMAT             |
                 * |                          ADDITIONAL LENGTH                            |
                 * |  SCCS  |  ACC   |      TPGS       |  3PC   |      resv       | PRTECT |
                 * |  obsl  | ENCSVC | vendor | MULTIP |  obsl  |  resv  |  resv  | ADDR16 |
                 * |  obsl  |  resv  | WBUS16 |  SYNC  |  obsl  |  resv  | CMDQUE | vendor |
                 * \_______________________________________________________________________/
                 */
                static uint8_t const standard_inquiry_header[] = {
                    0x06      , // SPC-4
                    0b00010010, // SAM-5
                    31        ,
                    0b00000000,
                    0b00010000,
                    0b00000010
                };
                static uint8_t const product_id[]  = { 'F', 'o', 'r', 'm', 'a', 't', 'i', 'o',
                                                       'n', 'O', 'n', 'e', ' ', ' ', ' ', ' ' };

                // Copy the fixed standard inquiry header above
                memcpy( &buffer[2], standard_inquiry_header, sizeof(standard_inquiry_header));

                // Conditionally fill out the additional vendor/device id
                if (16 <= buflen) {
                    memcpy(&buffer[8], vendor_name, sizeof(vendor_name));
                    if (32 <= buflen) {
                        memcpy(&buffer[16], product_id, sizeof(product_id));
                        if (36 <= buflen) {
                            memcpy(&buffer[32], "BETA", 4);
                        }
                    }

                }
                task->setResponseBuffer(buffer, buffer[4] + 5);
            }
        }
        break;
    case MODE_SENSE:
    case MODE_SENSE_10:
        {
            bool dbd = (0x00 != (scsi_cmd.cdb[1] & 0x08));
            uint8_t pc = scsi_cmd.cdb[2] / 0x40;
            uint8_t page_code = scsi_cmd.cdb[2] % 0x40;
            uint8_t& subpage = scsi_cmd.cdb[3];
            LOGDEBUG << "Mode Sense: "
                     << " dbd[" << std::hex << dbd
                     << "] pc[" << std::hex << (uint32_t)pc
                     << "] page_code[" << (uint32_t)page_code
                     << "] subpage[" << (uint32_t)subpage << "]";

            // We do not support any persistent pages, subpages
            if (0x01 & pc || 0x00 != subpage) {
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                continue;
            }

            size_t buflen = scsi_cmd.bufflen;
            if (buflen < 4) {
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                continue;
            }
            bzero(buffer, buflen);

            size_t param_cursor = 0ull;
            if (MODE_SENSE == op_code) {
                buffer[1] = TYPE_DISK;  // Block device type
                buffer[2] = 0b00000000; // Device specific params
                param_cursor = 4;
            } else {
                buffer[2] = TYPE_DISK;  // Block device type
                buffer[3] = 0b00000000; // Device specific params
                buffer[4] = 0b00000000; // LLBA disabled
                param_cursor = 8;
            }

            // Append block descriptor if enabled
            if ((param_cursor + 8 <= buflen) && !dbd) {
                buffer[param_cursor - 1] = 0x08; // Set descriptor size
                uint64_t num_blocks = volume_size / logical_block_size;
                LOGDEBUG << "Num blocks: [0x" << std::hex << num_blocks
                         << "] of size [0x" << logical_block_size << "]";
                // Number of LBAs (or 0xFFFFFFFF if bigger than 4GiB)
                *reinterpret_cast<uint32_t*>(&buffer[param_cursor]) = htobe32(std::min(num_blocks, (uint64_t)UINT_MAX));
                *reinterpret_cast<uint32_t*>(&buffer[param_cursor+4]) = htobe32(logical_block_size);
            }
            param_cursor += 8;

            // Add pages that were requested
            switch (page_code) {
            case 0x3F: // ALL pages please
            case 0x08: // Caching Mode Page
                if (param_cursor + 20 <= buflen) {
                    LOGDEBUG << "Adding caching page.";
                    uint32_t blocks_per_object = physical_block_size / logical_block_size;
                    buffer[param_cursor]        = 0x08;
                    buffer[param_cursor + 1]    = 0x12;
                    buffer[param_cursor + 2]    = 0b00011000;
                    auto prefetch_size = htobe16(std::min(blocks_per_object, (uint32_t)UINT16_MAX));
                    *reinterpret_cast<uint32_t*>(&buffer[param_cursor + 6]) = prefetch_size;
                    *reinterpret_cast<uint32_t*>(&buffer[param_cursor + 8]) = prefetch_size;
                    *reinterpret_cast<uint32_t*>(&buffer[param_cursor + 10]) = prefetch_size;
                    buffer[param_cursor + 12]   = 0b01000000;
                    buffer[param_cursor + 14]   = 0xFF;
                    buffer[param_cursor + 15]   = 0xFF;
                }
                param_cursor += 20;
                if (0x3F != page_code) break;;
            case 0x0A: // Control Mode Page
                {
                }
                break;;
            default:
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
            }

            // Set data length
            if (MODE_SENSE == op_code) {
                buffer[0] = param_cursor - 1;
            } else {
                buffer[1] = param_cursor - 2;
            }
            task->setResponseBuffer(buffer, scsi_cmd.bufflen);
        }
        break;
    case READ_6:
    case READ_10:
    case READ_12:
    case READ_16:
        {
            // If we are anything but READ_6 read the FUA bit
            bool fua = (READ_6 == op_code) ?
                            false : (0x00 != (scsi_cmd.cdb[1] & 0x08));

            LOGIO << "Read received for "
                  << "LBA[0x" << std::hex << scsi_cmd.lba
                  << "] Length[0x" << scsi_cmd.bufflen
                  << "] FUA[" << fua
                  << "] Handle[0x" << cmd.cmd_h << "]";
            uint64_t offset = scsi_cmd.lba * logical_block_size;
            task->setRead(offset, scsi_cmd.bufflen);
            task->setResponseBuffer(buffer, scsi_cmd.bufflen);
            return scstOps->read(task);
        }
        break;
    case READ_CAPACITY:     // READ_CAPACITY(10)
    case READ_CAPACITY_16:
        {
            LOGDEBUG << "Read Capacity received.";
            uint64_t num_blocks = volume_size / logical_block_size;
            uint32_t blocks_per_object = physical_block_size / logical_block_size;

            if (READ_CAPACITY == op_code) {
                *reinterpret_cast<uint32_t*>(&buffer[0]) = htobe32(std::min(num_blocks, (uint64_t)UINT_MAX));
                *reinterpret_cast<uint32_t*>(&buffer[4]) = htobe32(logical_block_size);
                task->setResponseBuffer(buffer, 8);
            } else {
                *reinterpret_cast<uint64_t*>(&buffer[0]) = htobe64(num_blocks);
                *reinterpret_cast<uint32_t*>(&buffer[8]) = htobe32(logical_block_size);
                // Number of logic blocks per object as a power of 2
                buffer[13] = (uint8_t)__builtin_ctz(blocks_per_object) & 0xFF;
                task->setResponseBuffer(buffer, 32);
            }
        }
        break;
    case WRITE_6:
    case WRITE_10:
    case WRITE_12:
    case WRITE_16:
        {
            // If we are anything but WRITE_6 read the FUA bit
            bool fua = (WRITE_6 == op_code) ?
                            false : (0x00 != (scsi_cmd.cdb[1] & 0x08));

            LOGIO << "Write received for "
                  << "LBA[0x" << std::hex << scsi_cmd.lba
                  << "] Length[0x" << scsi_cmd.bufflen
                  << "] FUA[" << fua
                  << "] Handle[0x" << cmd.cmd_h << "]";
            uint64_t offset = scsi_cmd.lba * logical_block_size;
            task->setWrite(offset, scsi_cmd.bufflen);
            // Right now our API expects the data in a boost shared_ptr :(
            auto buffer = boost::make_shared<std::string>((char*) scsi_cmd.pbuf, scsi_cmd.bufflen);
            return scstOps->write(buffer, task);
        }
        break;
    default:
        LOGNOTIFY << "Unsupported SCSI command received " << std::hex
            << "OPCode [0x" << (uint32_t)(op_code)
            << "] CDB length [" << std::dec << scsi_cmd.cdb_len
            << "]";
        task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_opcode));
        break;
    }
    } while (false);
    readyResponses.push(task);
}

void
ScstDevice::getAndRespond() {
    cmd.preply = 0ull;
    do {
        // If we do not have reply read a response into it
        if (0ull == cmd.preply && !readyResponses.empty()) {
            ScstTask* resp {nullptr};
            ensure(readyResponses.pop(resp));
            cmd.preply = resp->getReply();
            repliedResponses[resp->getHandle()].reset(resp);
        }

        int res = 0;
        cmd.cmd_h = 0x00;
        cmd.subcode = 0x00;
        do { // Make sure and finish the ioctl
        res = ioctl(scstDev, SCST_USER_REPLY_AND_GET_CMD, &cmd);
        } while ((0 > res) && (EINTR == errno));

        cmd.preply = 0ull;
        if (0 != res) {
            switch (errno) {
            case ENOTTY:
            case EBADF:
                LOGNOTIFY << "Scst device no longer has a valid handle to the kernel, terminating.";
                throw BlockError::connection_closed;
            case EFAULT:
            case EINVAL:
                fds_panic("Invalid Scst argument!");
            case EAGAIN:
                // If we still have responses, keep replying
                if (!readyResponses.empty()) {
                    continue;
                }
            default:
                return;
            }
        }

        LOGTRACE << "Received SCST command: "
                 << "[0x" << std::hex << cmd.cmd_h
                 << "] sc [0x" << cmd.subcode << "]";
        switch (cmd.subcode) {
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
        case SCST_USER_ON_CACHED_MEM_FREE:
            execMemFree();
            break;
        case SCST_USER_ALLOC_MEM:
            execAllocCmd();
            break;
        case SCST_USER_PARSE:
            execParseCmd();
            break;
        case SCST_USER_EXEC:
            execUserCmd();
            break;
        default:
            LOGWARN << "Received unknown Scst subcommand: [" << cmd.subcode << "]";
            break;
        }
    } while (true); // Keep replying while we have responses or requests
}

void
ScstDevice::ioEvent(ev::io &watcher, int revents) {
    if (processing_ || (EV_ERROR & revents)) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    ioWatcher->stop();
    processing_ = true;
    // Unblocks the next thread to listen on the ev loop
    scst_target->process();

    // We are guaranteed to be the only thread acting on this file descriptor
    try {
        // Get the next command, and/or reply to any existing finished commands
        getAndRespond();
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
    scst_target->follow();
}

void
ScstDevice::respondTask(BlockTask* response) {
    LOGDEBUG << " response from BlockOperations handle: 0x" << std::hex << response->getHandle()
             << " " << response->getError();

    auto scst_response = static_cast<ScstTask*>(response);
    if (scst_response->isRead()) {
        if (fpi::OK != scst_response->getError()) {
            scst_response->checkCondition(SCST_LOAD_SENSE(scst_sense_read_error));
        } else {
            auto buffer = scst_response->getResponseBuffer();
            fds_uint32_t i = 0, context = 0;
            boost::shared_ptr<std::string> buf = scst_response->getNextReadBuffer(context);
            while (buf != NULL) {
                LOGDEBUG << "Handle 0x" << std::hex << scst_response->getHandle()
                         << "...Size 0x" << buf->length() << "B"
                         << "...Buffer # " << std::dec << context;
                memcpy(buffer + i, buf->c_str(), buf->length());
                i += buf->length();
                buf = scst_response->getNextReadBuffer(context);
            }
        }
    } else if (fpi::OK != scst_response->getError()) {
        if (scst_response->isWrite()) {
            scst_response->checkCondition(SCST_LOAD_SENSE(scst_sense_write_error));
        } else {
            scst_response->setResult(-((uint32_t)scst_response->getError()));
        }
    }

    // add to quueue
    readyResponses.push(scst_response);

    // We have something to write, so poke the loop
    asyncWatcher->send();
}

void
ScstDevice::attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) {
    // capacity is in MB
    if (volDesc) {
        volume_size = (volDesc->capacity * Mi);
        physical_block_size = volDesc->maxObjSizeInBytes;
        snprintf(serial_number, sizeof(serial_number), "%.32lX", volDesc->GetID().get());
        LOGNORMAL << "Attached to volume with capacity: 0x" << std::hex << volume_size
            << "B and object size: 0x" << physical_block_size
            << "B with serial: " << serial_number;
    }
}

}  // namespace fds
