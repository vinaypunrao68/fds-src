/*
 * scst/ScstDevice.h
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

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTION_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTION_H_

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/queue.hpp>

#include "connector/scst/ScstCommon.h"
#include "connector/scst/scst_user.h"
#include "connector/BlockOperations.h"

#undef COPY

struct scst_user_get_cmd;

namespace fds
{

struct AmProcessor;
struct ScstTarget;
struct ScstTask;

struct ScstDevice : public BlockOperations::ResponseIFace {
    ScstDevice(std::string const& vol_name,
               ScstTarget* target,
               std::shared_ptr<AmProcessor> processor);
    ScstDevice(ScstDevice const& rhs) = delete;
    ScstDevice(ScstDevice const&& rhs) = delete;
    ScstDevice operator=(ScstDevice const& rhs) = delete;
    ScstDevice operator=(ScstDevice const&& rhs) = delete;
    ~ScstDevice();

    // implementation of BlockOperations::ResponseIFace
    void respondTask(BlockTask* response) override;
    void attachResp(boost::shared_ptr<VolumeDesc> const& volDesc) override;
    void terminate() override;

    std::string getName() const { return volumeName; }

    void start(std::shared_ptr<ev::dynamic_loop> loop);

  private:
    template<typename T>
    using unique = std::unique_ptr<T>;

    bool standalone_mode { false };

    enum class ConnectionState { RUNNING,
                                 STOPPING,
                                 STOPPED };

    ConnectionState state_ { ConnectionState::RUNNING };

    std::string const volumeName;
    int scstDev {-1};
    size_t volume_size {0};
    char serial_number[33];
    size_t sessions {0};

    std::shared_ptr<AmProcessor> amProcessor;
    ScstTarget* scst_target;
    BlockOperations::shared_ptr scstOps;

    size_t resp_needed;

    scst_user_get_cmd cmd {};
    scst_user_reply_cmd fast_reply {};
    uint32_t logical_block_size;
    uint32_t physical_block_size {0};

    boost::lockfree::queue<ScstTask*> readyResponses;
    std::unordered_map<uint32_t, unique<ScstTask>> repliedResponses;

    unique<ev::io> ioWatcher;
    unique<ev::async> asyncWatcher;

    /** Indicates to ev loop if it's safe to handle events on this connection */
    bool processing_ {false};

    int openScst();
    void wakeupCb(ev::async &watcher, int revents);
    void ioEvent(ev::io &watcher, int revents);
    void getAndRespond();

    void execAllocCmd();
    void execMemFree();
    void execSessionCmd();
    void execUserCmd();
    void execCompleteCmd();
    void execTaskMgmtCmd();
    void execParseCmd();

    void fastReply() {
        fast_reply.cmd_h = cmd.cmd_h;
        fast_reply.subcode = cmd.subcode;
        cmd.preply = (unsigned long)&fast_reply;
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTION_H_
