/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_SMCTRL_H_
#define SOURCE_STOR_MGR_INCLUDE_SMCTRL_H_

#include <fdsp/FDSP_types.h>
#include <fdsp/fds_service_types.h>

namespace fds {

/**
 * Set of control commands that to different internal SM modules
 */

/**
 * Scavenger command
 */
class SmScavengerCmd {
  public:
    enum CommandType {
        SCRUB_ENABLE,
        SCRUB_DISABLE,
        SCRUB_GET_STATUS,
        SCAV_ENABLE,
        SCAV_DISABLE,
        SCAV_START,
        SCAV_STOP,
        SCAV_SET_POLICY,
        SCAV_GET_POLICY,
        SCAV_GET_PROGRESS,
        SCAV_GET_STATUS,
        SCAV_CMD_NOT_SET
    };
    SmScavengerCmd() : command(SCAV_CMD_NOT_SET) {}
    explicit SmScavengerCmd(CommandType cmd) : command(cmd) {}

    CommandType command;
};

class SmScavengerActionCmd: public SmScavengerCmd {
  public:
    explicit SmScavengerActionCmd(const fpi::FDSP_ScavengerCmd& cmd) {
        switch (cmd) {
            case fpi::FDSP_SCAVENGER_ENABLE:
                command = SCAV_ENABLE;
                break;
            case fpi::FDSP_SCAVENGER_DISABLE:
                command = SCAV_DISABLE;
                break;
            case fpi::FDSP_SCAVENGER_START:
                command = SCAV_START;
                break;
            case fpi::FDSP_SCAVENGER_STOP:
                command = SCAV_STOP;
                break;
            default:
                fds_panic("Unknown scavenger command");
        }
    }
};

class SmScrubberActionCmd: public SmScavengerCmd {
  public:
    explicit SmScrubberActionCmd(fpi::FDSP_ScrubberStatusType& cmd) {
        switch (cmd) {
            case fpi::FDSP_SCRUB_ENABLE:
                command = SCRUB_ENABLE;
                break;
            case fpi::FDSP_SCRUB_DISABLE:
                command = SCRUB_DISABLE;
                break;
            default:
                fds_panic("Unknown scrubber command");
        }
    }
};

class SmScrubberGetStatusCmd: public SmScavengerCmd {
  public:
    explicit SmScrubberGetStatusCmd(fpi::CtrlQueryScrubberStatusRespPtr& status)
            : SmScavengerCmd(SCRUB_GET_STATUS) {}
};

class SmScavengerSetPolicyCmd: public SmScavengerCmd {
  public:
    explicit SmScavengerSetPolicyCmd(fpi::CtrlSetScavengerPolicyPtr& p)
            : SmScavengerCmd(SCAV_SET_POLICY), policy(p) {}

    fpi::CtrlSetScavengerPolicyPtr policy;
};

class SmScavengerGetPolicyCmd: public SmScavengerCmd {
  public:
    explicit SmScavengerGetPolicyCmd(fpi::CtrlQueryScavengerPolicyRespPtr& pr)
            : SmScavengerCmd(SCAV_GET_POLICY), retPolicy(pr) {}

    // policy to return
    fpi::CtrlQueryScavengerPolicyRespPtr retPolicy;
};

class SmScavengerGetProgressCmd: public SmScavengerCmd {
  public:
    SmScavengerGetProgressCmd() : SmScavengerCmd(SCAV_GET_PROGRESS) {}

    // scavenger progress to return
    fds_uint32_t retProgress;
};

class SmScavengerGetStatusCmd: public SmScavengerCmd {
  public:
    explicit SmScavengerGetStatusCmd(fpi::CtrlQueryScavengerStatusRespPtr& rstatus)
            : SmScavengerCmd(SCAV_GET_STATUS), retStatus(rstatus) {}

    // scavenger status to return
    fpi::CtrlQueryScavengerStatusRespPtr retStatus;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_SMCTRL_H_
