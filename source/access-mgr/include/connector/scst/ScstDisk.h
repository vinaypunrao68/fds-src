/*
 * scst/ScstDisk.h
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

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDISK_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDISK_H_

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/queue.hpp>

#include "connector/scst/ScstCommon.h"
#include "connector/scst/scst_user.h"

#undef COPY
#include "connector/scst/ScstDevice.h"


struct scst_user_get_cmd;

namespace fds
{

struct AmProcessor;
struct ScstTarget;
struct ScstTask;
struct InquiryHandler;
struct ModeHandler;

struct ScstDisk : public ScstDevice {
    ScstDisk(VolumeDesc const& vol_desc, ScstTarget* target, std::shared_ptr<AmProcessor> processor);
    ScstDisk(ScstDisk const& rhs) = delete;
    ScstDisk(ScstDisk const&& rhs) = delete;
    ScstDisk operator=(ScstDisk const& rhs) = delete;
    ScstDisk operator=(ScstDisk const&& rhs) = delete;

  private:
    void setupModePages(size_t const lba_size, size_t const pba_size, size_t const volume_size);

    size_t volume_size {0};
    uint32_t logical_block_size {0};
    uint32_t physical_block_size {0};

    void execDeviceCmd(ScstTask* task) override;
    void respondDeviceTask(ScstTask* task) override;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDISK_H_
