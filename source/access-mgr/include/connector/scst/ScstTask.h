/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SCST_SCSTTASK_H_
#define SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SCST_SCSTTASK_H_

#include "connector/BlockTask.h"
#include "connector/scst/scst_user.h"

namespace fds
{

struct ScstTask : public BlockTask {

    ScstTask(uint32_t handle, uint32_t sc);


    /** SCSI Setters */
    inline void checkCondition(uint8_t const key, uint8_t const asc, uint8_t const ascq);

    inline void setResponseBuffer(uint8_t* buf, size_t buf_len);

    void setResult(int32_t result)
    { reply.result = result; }

    /** SCSI Getters */
    unsigned long getReply() const
    { return (unsigned long)&reply; } 

    uint8_t* getResponseBuffer() const
    { return (uint8_t*)reply.exec_reply.pbuf; }

    uint32_t getSubcode() const { return reply.subcode; }

  private:
    // Task response to SCST
    scst_user_reply_cmd reply {};

    // Sense buffer for check conditions
    uint8_t sense_buffer[18] {};
};

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

void
ScstTask::setResponseBuffer(uint8_t* buf, size_t buf_len)
{
    fds_assert(buf);
    reply.exec_reply.pbuf = (unsigned long)buf;
    reply.exec_reply.resp_data_len = buf_len;
}

}  // namespace fds

#endif  // SOURCE_ACCESSMGR_INCLUDE_CONNECTOR_SCST_SCSTTASK_H_
