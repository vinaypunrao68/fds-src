/*
 * scst/ScstTask.h
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

#ifndef SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SCST_SCSTTASK_H_
#define SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SCST_SCSTTASK_H_

#include "connector/BlockTask.h"
#include "connector/scst/scst_user.h"

namespace fds
{

struct ScstTask : public BlockTask {

    ScstTask(uint32_t handle, uint32_t sc);
    ~ScstTask() {
        // Free allocated buffer if we had a check condition
        if (SAM_STAT_GOOD != reply.exec_reply.status
            && reply.exec_reply.pbuf
            && !buffer_in_sgv) {
            free((void*)reply.exec_reply.pbuf);
            reply.exec_reply.pbuf = 0ul;
        }
    }

    /** SCSI Setters */
    inline void checkCondition(uint8_t const key, uint8_t const asc, uint8_t const ascq);
    inline void reservationConflict();
    inline bool wasCheckCondition() const;

    inline void setResponseBuffer(uint8_t* buf, size_t const buflen, bool const cached_buffer);
    inline void setResponseLength(size_t const buflen);

    void setResult(int32_t result)
    { reply.result = result; }

    /** SCSI Getters */
    unsigned long getReply() const
    { return (unsigned long)&reply; } 

    uint8_t* getResponseBuffer() const
    { return (uint8_t*)reply.exec_reply.pbuf; }

    size_t getResponseBufferLen() const
    { return buf_len; }

    uint32_t getSubcode() const { return reply.subcode; }

  private:
    // Task response to SCST
    scst_user_reply_cmd reply {};

    // Sense buffer for check conditions
    uint8_t sense_buffer[18] {};

    // Buffer length for response data
    size_t buf_len {0};

    // If the buffer is known to SCST
    bool buffer_in_sgv {false};
};

void
ScstTask::reservationConflict()
{
    reply.exec_reply.status = SAM_STAT_RESERVATION_CONFLICT;
    reply.exec_reply.sense_len = 0;
}

void
ScstTask::checkCondition(uint8_t const key, uint8_t const asc, uint8_t const ascq)
{
    sense_buffer[0] = 0x70;
    sense_buffer[2] = key;
    sense_buffer[7] = 0x0a;
    sense_buffer[12] = asc;
    sense_buffer[13] = ascq;

    reply.exec_reply.status = SAM_STAT_CHECK_CONDITION;
    reply.exec_reply.sense_len = 18;
    reply.exec_reply.psense_buffer = (unsigned long)&sense_buffer;
}

bool
ScstTask::wasCheckCondition() const
{
    return (SAM_STAT_CHECK_CONDITION == reply.exec_reply.status);
}

void
ScstTask::setResponseBuffer(uint8_t* buf, size_t const buflen, bool const cached_buffer)
{
    buffer_in_sgv = cached_buffer;
    buf_len = buflen;
    reply.exec_reply.pbuf = (unsigned long)buf;
}

void
ScstTask::setResponseLength(size_t const length) {
    reply.exec_reply.resp_data_len = length;
}

}  // namespace fds

#endif  // SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SCST_SCSTTASK_H_
