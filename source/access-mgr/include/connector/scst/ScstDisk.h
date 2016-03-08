/*
 * scst/ScstDisk.h
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

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDISK_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDISK_H_

#include <memory>

#include "connector/scst/ScstDevice.h"

#include "connector/BlockOperations.h"

namespace fds
{

struct AmProcessor;
struct ScstTarget;
struct ScstTask;

struct ScstDisk : public ScstDevice,
                  public BlockOperations::ResponseIFace {
    ScstDisk(VolumeDesc const& vol_desc, ScstTarget* target, std::shared_ptr<AmProcessor> processor);
    ScstDisk(ScstDisk const& rhs) = delete;
    ScstDisk(ScstDisk const&& rhs) = delete;
    ScstDisk operator=(ScstDisk const& rhs) = delete;
    ScstDisk operator=(ScstDisk const&& rhs) = delete;

    // implementation of BlockOperations::ResponseIFace
    void respondTask(BlockTask* response) override;
    void attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) override;
    void terminate() override;

  private:
    size_t volume_size {0};
    uint32_t logical_block_size {0};
    uint32_t physical_block_size {0};
    BlockOperations::shared_ptr scstOps;

    void setupModePages(size_t const lba_size, size_t const pba_size, size_t const volume_size);

    void execSessionCmd() override;
    void execDeviceCmd(ScstTask* task) override;
    void respondDeviceTask(ScstTask* task) override;
    void startShutdown() override;
    void stopped() override;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDISK_H_
