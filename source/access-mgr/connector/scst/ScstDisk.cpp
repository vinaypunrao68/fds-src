/*
 * ScstDisk.cpp
 *
 * Copyright (c) 2016, Brian Szmyd <szmyd@formationds.com>
 * Copyright (c) 2016, Formation Data Systems
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

#include "connector/scst/ScstDisk.h"

#include <string>

extern "C" {
#include <endian.h>
}

#include <boost/make_shared.hpp>

// TODO(bszmyd): Sun 18 Oct 2015 04:44:55 PM MDT
// REMOVE THIS!!!!
// =================
#include "fds_process.h"
#include "fds_config.hpp"
#include "util/Log.h"
// =================

#include "fdsp/config_types_types.h"
extern "C" {
#include "connector/scst/scst_user.h"
}

#include "connector/scst/ScstTask.h"
#include "connector/scst/ScstMode.h"

/// Some useful constants for us
/// ******************************************
static constexpr size_t Ki = 1024;
static constexpr size_t Mi = Ki * Ki;
static constexpr size_t Gi = Ki * Mi;
static constexpr ssize_t max_block_size = 8 * Mi;
/// ******************************************

namespace fds
{

ScstDisk::ScstDisk(VolumeDesc const& vol_desc,
                   ScstTarget* target,
                   std::shared_ptr<AmProcessor> processor)
        : ScstDevice(vol_desc, target, processor),
          logical_block_size(512ul)
{
    {
        FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.scst.");
        logical_block_size = conf.get<uint32_t>("default_block_size", logical_block_size);
    }

    // capacity is in MB
    volume_size = (vol_desc.capacity * Mi);
    physical_block_size = vol_desc.maxObjSizeInBytes;

    setupModePages(logical_block_size, physical_block_size, volume_size);
    registerDevice(TYPE_DISK, logical_block_size);
}

void ScstDisk::setupModePages(size_t const lba_size, size_t const pba_size, size_t const volume_size)
{
    mode_handler->setBlockDescriptor((volume_size / lba_size), lba_size);

    CachingModePage caching_page;
    caching_page &= CachingModePage::DiscontinuityNoTrunc;
    caching_page &= CachingModePage::SegmentSize;
    caching_page &= CachingModePage::SegmentSizeInBlocks;
    uint32_t blocks_per_object = pba_size / lba_size;
    caching_page.setPrefetches(blocks_per_object, blocks_per_object, blocks_per_object, UINT64_MAX);
    mode_handler->addModePage(caching_page);

    ReadWriteRecoveryPage recovery_page;
    recovery_page &= ReadWriteRecoveryPage::IgnoreReadRecovery;
    recovery_page &= ReadWriteRecoveryPage::TerminateOnRecovery;
    recovery_page &= ReadWriteRecoveryPage::CorrectionPrevented;
    mode_handler->addModePage(recovery_page);
}

void ScstDisk::execDeviceCmd(ScstTask* task) {
    auto& scsi_cmd = cmd.exec_cmd;
    auto& op_code = scsi_cmd.cdb[0];

    // All buffer allocations have already happened
    auto buffer = task->getResponseBuffer();
    size_t buflen = task->getResponseBufferLen();

    deferredReply(); // All tasks respond on response queue

    // Poor man's goto
    do {
    if (0 == volume_size) {
        task->checkCondition(SCST_LOAD_SENSE(scst_sense_not_ready));
        continue;
    }

    switch (op_code) {
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
            return;
        }
        break;
    case READ_CAPACITY:     // READ_CAPACITY(10)
    case READ_CAPACITY_16:
        {
            LOGTRACE << "Read Capacity received.";
            uint64_t num_blocks = volume_size / logical_block_size;
            uint32_t blocks_per_object = physical_block_size / logical_block_size;

            if (READ_CAPACITY == op_code && 8 >= buflen) {
                *reinterpret_cast<uint32_t*>(&buffer[0]) = htobe32(std::min(num_blocks, (uint64_t)UINT_MAX));
                *reinterpret_cast<uint32_t*>(&buffer[4]) = htobe32(logical_block_size);
                task->setResponseLength(8);
            } else if (32 >= buflen) {
                *reinterpret_cast<uint64_t*>(&buffer[0]) = htobe64(num_blocks);
                *reinterpret_cast<uint32_t*>(&buffer[8]) = htobe32(logical_block_size);
                // Number of logic blocks per object as a power of 2
                buffer[13] = (uint8_t)__builtin_ctz(blocks_per_object) & 0xFF;
                task->setResponseLength(32);
            } else {
                task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
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
            auto write_buffer = boost::make_shared<std::string>((char*) buffer, buflen);
            scstOps->write(write_buffer, task);
            return;
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
ScstDisk::respondDeviceTask(ScstTask* task) {
    if (task->isRead()) {
        if (fpi::OK != task->getError()) {
            task->checkCondition(SCST_LOAD_SENSE(scst_sense_read_error));
        } else {
            auto buffer = task->getResponseBuffer();
            fds_uint32_t i = 0, context = 0;
            boost::shared_ptr<std::string> buf = task->getNextReadBuffer(context);
            while (buf != NULL) {
                memcpy(buffer + i, buf->c_str(), buf->length());
                i += buf->length();
                buf = task->getNextReadBuffer(context);
            }
            task->setResponseLength(i);
        }
    } else if (fpi::OK != task->getError()) {
        task->checkCondition(SCST_LOAD_SENSE(scst_sense_write_error));
    }
}


}  // namespace fds
