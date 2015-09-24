/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "connector/scst/ScstTask.h"

extern "C" {
#include <sys/ioctl.h>
}

namespace fds
{

ScstTask::ScstTask(uint32_t hdl, uint32_t sc) :
    BlockTask(hdl)
{
    reply.cmd_h = hdl;
    reply.subcode = sc;
    if (SCST_USER_EXEC == sc) {
        reply.exec_reply.reply_type = SCST_EXEC_REPLY_COMPLETED;
        reply.exec_reply.status = GOOD;

    }
}


}  // namespace fds
