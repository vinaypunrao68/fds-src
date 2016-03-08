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

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDEVICE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDEVICE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/queue.hpp>

#include "connector/scst/ScstCommon.h"
#include "connector/scst/scst_user.h"

#undef COPY

#include "fds_volume.h"

struct scst_user_get_cmd;

namespace fds
{

struct AmProcessor;
struct ScstTarget;
struct ScstTask;
struct InquiryHandler;
struct ModeHandler;

struct ScstDevice {
    ScstDevice(VolumeDesc const& vol_desc,
               ScstTarget* target,
               std::shared_ptr<AmProcessor> processor);
    ScstDevice(ScstDevice const& rhs) = delete;
    ScstDevice(ScstDevice const&& rhs) = delete;
    virtual ~ScstDevice();

    void shutdown();

    std::string getName() const { return volumeName; }

    void registerDevice(uint8_t const device_type, uint32_t const logical_block_size);
    void start(std::shared_ptr<ev::dynamic_loop> loop);

  protected:
    enum class ConnectionState { RUNNING,
                                 DRAINING,
                                 DRAINED,
                                 STOPPED,
                                 NOCHANGE };

    template<typename T>
    using unique = std::unique_ptr<T>;

    boost::lockfree::queue<ScstTask*> readyResponses;

    scst_user_get_cmd cmd {};
    scst_user_reply_cmd fast_reply {};

    std::shared_ptr<AmProcessor> amProcessor;

    // Utility functions to build Inquiry Pages...etc
    unique<InquiryHandler> inquiry_handler;
    void setupModePages();

    // Utility functions to build Mode Pages...etc
    unique<ModeHandler> mode_handler;
    void setupInquiryPages(uint64_t const volume_id);

    void deferredReply() {
        cmd.preply = 0ull;
    }

    void devicePoke(ConnectionState const state = ConnectionState::NOCHANGE);

    void fastReply() {
        fast_reply.cmd_h = cmd.cmd_h;
        fast_reply.subcode = cmd.subcode;
        cmd.preply = (unsigned long)&fast_reply;
    }

  private:
    /// Constants
    static constexpr uint64_t invalid_session_id {UINT64_MAX};

    bool standalone_mode { false };

    ConnectionState state_ { ConnectionState::RUNNING };

    std::string const volumeName;
    int scstDev {-1};

    ScstTarget* scst_target;
    uint64_t reservation_session_id {invalid_session_id};

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
    void execUserCmd();
    void execCompleteCmd();
    void execTaskMgmtCmd();
    void execParseCmd();
    virtual void execSessionCmd() = 0;
    virtual void execDeviceCmd(ScstTask* task) = 0;
    virtual void respondDeviceTask(ScstTask* task) = 0;
    virtual void startShutdown() = 0;
    virtual void stopped() = 0;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTDEVICE_H_
