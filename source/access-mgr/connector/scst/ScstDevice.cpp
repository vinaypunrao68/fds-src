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
#include "connector/scst/ScstMode.h"
#include "connector/scst/ScstInquiry.h"

/// Some useful constants for us
/// ******************************************
static constexpr size_t Ki = 1024;
static constexpr size_t Mi = Ki * Ki;
static constexpr size_t Gi = Ki * Mi;
static constexpr ssize_t max_block_size = 8 * Mi;
/// ******************************************

static uint32_t const ieee_oui = 0x88A084;
static uint8_t const vendor_name[] = { 'F', 'D', 'S', ' ', ' ', ' ', ' ', ' ' };

static constexpr bool ensure(bool b)
{ return (!b ? throw fds::BlockError::connection_closed : true); }

namespace fds
{

ScstDevice::ScstDevice(VolumeDesc const& vol_desc,
                       ScstTarget* target,
                       std::shared_ptr<AmProcessor> processor)
        : amProcessor(processor),
          scst_target(target),
          scstOps(boost::make_shared<BlockOperations>(this)),
          volumeName(vol_desc.name),
          logical_block_size(512ul),
          readyResponses(4000),
          inquiry_handler(new InquiryHandler()),
          mode_handler(new ModeHandler())
{
    {
        FdsConfigAccessor config(g_fdsprocess->get_conf_helper());
        standalone_mode = config.get_abs<bool>("fds.am.testing.standalone", false);
    }
    {
        FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.scst.");
        logical_block_size = conf.get<uint32_t>("default_block_size", logical_block_size);
    }

    // capacity is in MB
    volume_size = (vol_desc.capacity * Mi);
    physical_block_size = vol_desc.maxObjSizeInBytes;

    setupModePages(logical_block_size, physical_block_size, volume_size);

    setupInquiryPages(vol_desc.volUUID.get());

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
                SCST_QUEUE_ALG_1_UNRESTRICTED_REORDER,      // maintain consistency in reordering
                SCST_QERR_0_ALL_RESUME,                     // fault does not abort all cmds
                1, 0, 0, 0                                  // TAS/SWP/DSENSE/ORDER MGMT
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

void ScstDevice::setupModePages(size_t const lba_size, size_t const pba_size, size_t const volume_size)
{
    mode_handler->setBlockDescriptor((volume_size / lba_size), lba_size);

    CachingModePage caching_page;
    caching_page &= CachingModePage::DiscontinuityNoTrunc;
    caching_page &= CachingModePage::SegmentSize;
    caching_page &= CachingModePage::SegmentSizeInBlocks;
    uint32_t blocks_per_object = pba_size / lba_size;
    caching_page.setPrefetches(blocks_per_object, blocks_per_object, blocks_per_object, UINT64_MAX);
    mode_handler->addModePage(caching_page);

    ControlModePage control_page;
    control_page &= ControlModePage::DefaultProtectionDisabled;
    control_page &= ControlModePage::FixedFormatSense;
    control_page &= ControlModePage::UnrestrictedCommandOrdering;
    control_page &= ControlModePage::NoUAOnRelease;
    control_page &= ControlModePage::AbortStatusOnTMF;
    control_page &= ControlModePage::SingleTaskSet;
    mode_handler->addModePage(control_page);

    ReadWriteRecoveryPage recovery_page;
    recovery_page &= ReadWriteRecoveryPage::IgnoreReadRecovery;
    recovery_page &= ReadWriteRecoveryPage::TerminateOnRecovery;
    recovery_page &= ReadWriteRecoveryPage::CorrectionPrevented;
    mode_handler->addModePage(recovery_page);
}

void ScstDevice::setupInquiryPages(uint64_t const volume_id)
{
    // Setup the standard inquiry page
    auto inquiry = inquiry_handler->getStandardInquiry();
    inquiry &= PeripheralType::DirectAccess;
    inquiry.setVendorId("FDS");
    inquiry.setProductId("FormationOne");
    inquiry.setRevision("BETA");
    inquiry_handler->setStandardInquiry(inquiry);

    // Write the Serial Number (0x80) Page
    char serial_number[17];
    snprintf(serial_number, sizeof(serial_number), "%.16lX", volume_id);
    VPDPage serial_page;
    serial_page.writePage(0x80, serial_number, 16);
    inquiry_handler->addVPDPage(serial_page);

    // Build our device identification descriptors
    DescriptorBuilder builder;
    builder &= VendorSpecificIdentifier(volumeName);
    builder &= T10Designator("FDS");
    builder &= NAADesignator(ieee_oui, volume_id);

    // Write the Device Identification (0x83) Page
    VPDPage device_id_page;
    device_id_page &= PeripheralType::DirectAccess;
    device_id_page.writePage(0x83, builder.data(), builder.length());
    inquiry_handler->addVPDPage(device_id_page);

    // Write the Extended Inquiry (0x86) Page
    ExtVPDParameters evpd_parameters;
    evpd_parameters &= ExtVPDParameters::HeadAttrSupport;
    evpd_parameters &= ExtVPDParameters::SimpleAttrSupport;
    evpd_parameters &= ExtVPDParameters::OrderedAttrSupport;
    VPDPage evpd_page;
    evpd_page.writePage(0x86, &evpd_parameters, sizeof(ExtVPDParameters));
    inquiry_handler->addVPDPage(evpd_page);
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
    if (0 <= scstDev) {
        close(scstDev);
        scstDev = -1;
    }
    GLOGNORMAL << "SCSI device " << volumeName << " stopped.";
}

void
ScstDevice::shutdown() {
    state_ = ConnectionState::DRAINING;
    asyncWatcher->send();
}

void
ScstDevice::terminate() {
    state_ = ConnectionState::STOPPED;
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
    if (ConnectionState::RUNNING != state_) {
        scstOps->shutdown();                        // We are shutting down
        if (ConnectionState::STOPPED == state_ ||
            ConnectionState::DRAINED == state_) {
            asyncWatcher->stop();                   // We are not responding
            ioWatcher->stop();
            if (ConnectionState::STOPPED == state_) {
                scstOps.reset();
                scst_target->deviceDone(volumeName);    // We are FIN!
                return;
            }
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
    // This is mostly to shutup valgrind
    memset((void*) fast_reply.alloc_reply.pbuf, 0x00, length);
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
    if (repliedResponses.end() != it) {
        repliedResponses.erase(it);
    }
    if (!cmd.on_free_cmd.buffer_cached && 0 < cmd.on_free_cmd.pbuf) {
        free((void*)cmd.on_free_cmd.pbuf);
    }
    fastReply(); // Setup the reply for the next ioctl
}

void ScstDevice::execParseCmd() {
    LOGWARN << "Need parsing help.";
    fds_panic("Should not be here!");
    fastReply(); // Setup the reply for the next ioctl
}

void ScstDevice::execTaskMgmtCmd() {
    auto& tmf_cmd = cmd.tm_cmd;
    auto done = (SCST_USER_TASK_MGMT_DONE == cmd.subcode) ? true : false;
    if (SCST_TARGET_RESET == tmf_cmd.fn) {
        LOGNOTIFY << "Target Reset request:"
                  << " [0x" << std::hex << tmf_cmd.fn
                  << "] on [" << volumeName << "]"
                  << " " << (done ? "Done." : "Received.");
    } else {
        LOGIO << "Task Management request:"
              << " [0x" << std::hex << tmf_cmd.fn
              << "] on [" << volumeName << "]"
              << " " << (done ? "Done." : "Received.");
    }

    if (done) {
        // Reset the reservation if we get a target reset
        switch (tmf_cmd.fn) {
        case SCST_TARGET_RESET:
        case SCST_LUN_RESET:
        case SCST_PR_ABORT_ALL:
            {
                reservation_session_id = invalid_session_id;
            }
        default:
            break;;
        }
    }

    fastReply(); // Setup the reply for the next ioctl
}

void
ScstDevice::execSessionCmd() {
    auto attaching = (SCST_USER_ATTACH_SESS == cmd.subcode) ? true : false;
    auto& sess = cmd.sess;
    LOGNOTIFY << "Session "
              << (attaching ? "attachment" : "detachment") << " requested"
              << ", handle [" << sess.sess_h
              << "] Init [" << sess.initiator_name
              << "] Targ [" << sess.target_name << "]";

    if (attaching) {
        auto volName = boost::make_shared<std::string>(volumeName);
        auto task = new ScstTask(cmd.cmd_h, SCST_USER_ATTACH_SESS);
        scstOps->init(volName, amProcessor, task); // Defer
        return deferredReply();
    } else {
        scstOps->detachVolume();
    }
    fastReply(); // Setup the reply for the next ioctl
}

void ScstDevice::execUserCmd() {
    auto& scsi_cmd = cmd.exec_cmd;
    auto& op_code = scsi_cmd.cdb[0];
    auto task = new ScstTask(cmd.cmd_h, SCST_USER_EXEC);

    // We may need to allocate a buffer for SCST, if we do and fail we'll need
    // to clean it up rather than expect a release command for it
    auto buffer = (uint8_t*)scsi_cmd.pbuf;
    size_t buflen = scsi_cmd.bufflen;
    if (!buffer && 0 < scsi_cmd.alloc_len) {
        ensure(0 == posix_memalign((void**)&buffer, sysconf(_SC_PAGESIZE), scsi_cmd.alloc_len));
        memset((void*) buffer, 0x00, scsi_cmd.alloc_len);
        task->setResponseBuffer(buffer, buflen, false);
    } else {
        task->setResponseBuffer(buffer, buflen, true);
    }

    // Poor man's goto
    do {
    if (0 == volume_size) {
        task->checkCondition(SCST_LOAD_SENSE(scst_sense_not_ready));
        continue;
    }

    // These commands do not require a reservation if one exists
    bool ignore_res = false;
    switch (op_code) {
    case INQUIRY:
    case LOG_SENSE:
    case RELEASE:
    case TEST_UNIT_READY:
      ignore_res = true;
    default:
      break;
    }

    // Check current reservation
    if (!ignore_res &&
        invalid_session_id != reservation_session_id &&
        reservation_session_id != scsi_cmd.sess_h) {
        task->reservationConflict();
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
            memset(buffer, '\0', buflen);

            // Check EVPD bit
            if (scsi_cmd.cdb[1] & 0x01) {
                auto& page = scsi_cmd.cdb[2];
                LOGTRACE << "Page: [0x" << std::hex << (uint32_t)(page) << "] requested.";
                inquiry_handler->writeVPDPage(task, page);
            } else {
                LOGTRACE << "Standard inquiry requested.";
                inquiry_handler->writeStandardInquiry(task);
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
            LOGTRACE << "Mode Sense: "
                     << " dbd[" << std::hex << dbd
                     << "] pc[" << std::hex << (uint32_t)pc
                     << "] page_code[" << (uint32_t)page_code
                     << "] subpage[" << (uint32_t)subpage << "]";

            // We do not support any persistent pages, subpages
            if (0x01 & pc || 0x00 != subpage) {
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                continue;
            }

            memset(buffer, '\0', buflen);

            if (MODE_SENSE == op_code) {
                mode_handler->writeModeParameters6(task, !dbd, page_code);
            } else {
                mode_handler->writeModeParameters10(task, !dbd, page_code);
            }
        }
        break;
    case READ_6:
    case READ_10:
    case READ_12:
    case READ_16:
        {
            // If we are anything but READ_6 read the PR and FUA bits
            bool fua = false;
            uint8_t rdprotect = 0x00;
            if (READ_6 != op_code) {
                rdprotect = (0x07 & (scsi_cmd.cdb[1] >> 5));
                fua = (0x00 != (scsi_cmd.cdb[1] & 0x08));
            }

            LOGIO << "Read received for "
                  << "LBA[0x" << std::hex << scsi_cmd.lba
                  << "] Length[0x" << scsi_cmd.bufflen
                  << "] FUA[" << fua
                  << "] PR[0x" << (uint32_t)rdprotect
                  << "] Handle[0x" << cmd.cmd_h << "]";

            // We do not support rdprotect data
            if (0x00 != rdprotect) {
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                continue;
            }

            uint64_t offset = scsi_cmd.lba * logical_block_size;
            task->setRead(offset, scsi_cmd.bufflen);
            scstOps->read(task);
            return deferredReply();
        }
        break;
    case READ_CAPACITY:     // READ_CAPACITY(10)
    case READ_CAPACITY_16:
        {
            LOGTRACE << "Read Capacity received.";
            uint64_t num_blocks = volume_size / logical_block_size;
            uint32_t blocks_per_object = physical_block_size / logical_block_size;

            if (READ_CAPACITY == op_code) {
                *reinterpret_cast<uint32_t*>(&buffer[0]) = htobe32(std::min(num_blocks, (uint64_t)UINT_MAX));
                *reinterpret_cast<uint32_t*>(&buffer[4]) = htobe32(logical_block_size);
                task->setResponseLength(8);
            } else {
                *reinterpret_cast<uint64_t*>(&buffer[0]) = htobe64(num_blocks);
                *reinterpret_cast<uint32_t*>(&buffer[8]) = htobe32(logical_block_size);
                // Number of logic blocks per object as a power of 2
                buffer[13] = (uint8_t)__builtin_ctz(blocks_per_object) & 0xFF;
                task->setResponseLength(32);
            }
        }
        break;
    case WRITE_6:
    case WRITE_10:
    case WRITE_12:
    case WRITE_16:
        {
            // If we are anything but READ_6 read the PR and FUA bits
            bool fua = false;
            uint8_t wrprotect = 0x00;
            if (WRITE_6 != op_code) {
                wrprotect = (0x07 & (scsi_cmd.cdb[1] >> 5));
                fua = (0x00 != (scsi_cmd.cdb[1] & 0x08));
            }

            LOGIO << "Write received for "
                  << "LBA[0x" << std::hex << scsi_cmd.lba
                  << "] Length[0x" << scsi_cmd.bufflen
                  << "] FUA[" << fua
                  << "] PR[0x" << (uint32_t)wrprotect
                  << "] Handle[0x" << cmd.cmd_h << "]";

            // We do not support wrprotect data
            if (0x00 != wrprotect) {
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
                continue;
            }

            uint64_t offset = scsi_cmd.lba * logical_block_size;
            task->setWrite(offset, scsi_cmd.bufflen);
            // Right now our API expects the data in a boost shared_ptr :(
            auto buffer = boost::make_shared<std::string>((char*) scsi_cmd.pbuf, scsi_cmd.bufflen);
            scstOps->write(buffer, task);
            return deferredReply();
        }
        break;
    case RESERVE:
        LOGIO << "Reserving device [" << volumeName << "]";
        reservation_session_id = scsi_cmd.sess_h;
        break;
    case RELEASE:
        if (reservation_session_id == scsi_cmd.sess_h) {
            LOGIO << "Releasing device [" << volumeName << "]";
            reservation_session_id = invalid_session_id;
        } else {
            LOGTRACE << "Initiator tried to release [" << volumeName <<"] with no reservation held.";
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
    deferredReply();
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
        if (0 != cmd.preply) {
            auto const& reply = *reinterpret_cast<scst_user_reply_cmd*>(cmd.preply);
            LOGTRACE << "Responding to: "
                     << "[0x" << std::hex << reply.cmd_h
                     << "] sc [0x" << reply.subcode
                     << "] result [0x" << reply.result << "]";
        }
        do { // Make sure and finish the ioctl
        res = ioctl(scstDev, SCST_USER_REPLY_AND_GET_CMD, &cmd);
        } while ((0 > res) && (EINTR == errno));

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
                    deferredReply();
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
        state_ = ConnectionState::DRAINING;
        if (e == BlockError::connection_closed) {
            // If we had an error, stop the event loop too
            state_ = ConnectionState::DRAINED;
        }
    }
    // Unblocks the ev loop to handle events again on this connection
    processing_ = false;
    asyncWatcher->send();
    scst_target->follow();
}

void
ScstDevice::respondTask(BlockTask* response) {
    auto scst_response = static_cast<ScstTask*>(response);
    if (scst_response->isRead()) {
        if (fpi::OK != scst_response->getError()) {
            scst_response->checkCondition(SCST_LOAD_SENSE(scst_sense_read_error));
        } else {
            auto buffer = scst_response->getResponseBuffer();
            fds_uint32_t i = 0, context = 0;
            boost::shared_ptr<std::string> buf = scst_response->getNextReadBuffer(context);
            while (buf != NULL) {
                memcpy(buffer + i, buf->c_str(), buf->length());
                i += buf->length();
                buf = scst_response->getNextReadBuffer(context);
            }
            scst_response->setResponseLength(i);
        }
    } else if (fpi::OK != scst_response->getError()) {
        if (scst_response->isWrite()) {
            scst_response->checkCondition(SCST_LOAD_SENSE(scst_sense_write_error));
        } else {
            scst_response->setResult(scst_response->getError());
        }
    }

    // add to quueue
    readyResponses.push(scst_response);

    // We have something to write, so poke the loop
    asyncWatcher->send();
}

void
ScstDevice::attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) {
    LOGNORMAL << "Attached to volume with capacity: 0x" << std::hex << volume_size
              << "B and object size: 0x" << physical_block_size;
}

}  // namespace fds
