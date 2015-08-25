/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_SMCTRL_H_
#define SOURCE_STOR_MGR_INCLUDE_SMCTRL_H_

#include <fdsp/FDSP_types.h>
#include <fdsp/sm_api_types.h>
#include <SmTypes.h>

namespace fds {

/**
 * Set of control commands that to different internal SM modules
 */

/**
 * Who initiated
 */
enum SmCommandInitiator {
    SM_CMD_INITIATOR_USER,
    SM_CMD_INITIATOR_TOKEN_MIGRATION,
    SM_CMD_INITIATOR_DISK_CHANGE,
    SM_CMD_INITIATOR_NOT_SET
};

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
        SCAV_ENABLE_DISK,
        SCAV_DISABLE_DISK,
        SCAV_START,
        SCAV_STOP,
        SCAV_SET_POLICY,
        SCAV_GET_POLICY,
        SCAV_GET_PROGRESS,
        SCAV_GET_STATUS,
        SCAV_CMD_NOT_SET
    };
    SmScavengerCmd()
            : command(SCAV_CMD_NOT_SET), initiator(SM_CMD_INITIATOR_NOT_SET) {}
    // use this constructor if user is an initiator
    explicit SmScavengerCmd(CommandType cmd)
            : command(cmd), initiator(SM_CMD_INITIATOR_USER) {}
    SmScavengerCmd(CommandType cmd, SmCommandInitiator who, DiskId id)
            : command(cmd), initiator(who), diskId(id) {}

    CommandType command;
    SmCommandInitiator initiator;
    DiskId diskId = {SM_INVALID_DISK_ID};
};

class SmScavengerActionCmd: public SmScavengerCmd {
  public:
    explicit SmScavengerActionCmd(const fpi::FDSP_ScavengerCmd& cmd,
                         SmCommandInitiator who) {
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
        initiator = who;
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
        initiator = SM_CMD_INITIATOR_USER;
    }
};

class SmScrubberGetStatusCmd: public SmScavengerCmd {
  public:
    explicit SmScrubberGetStatusCmd(fpi::CtrlQueryScrubberStatusRespPtr& status)
            : SmScavengerCmd(SCRUB_GET_STATUS), scrubStat(status) {}

    fpi::CtrlQueryScrubberStatusRespPtr scrubStat;
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

/**
 * Tiering command
 */
class SmTieringCmd {
  public:
    enum CommandType {
        TIERING_ENABLE,
        TIERING_DISABLE,
        /* Automatically started when the volume add notification is first received */
        TIERING_START_HYBRIDCTRLR_AUTO,
        /* Debug message for manually starting hybrid tier controller */
        TIERING_START_HYBRIDCTRLR_MANUAL,
        TIERING_CMD_NOT_SET
    };
    SmTieringCmd() : command(TIERING_CMD_NOT_SET) {}
    explicit SmTieringCmd(CommandType cmd) : command(cmd) {}

    CommandType command;
};

class SmCheckCmd {
  public:
    enum CommandType {
        SMCHECK_START,
        SMCHECK_STOP,
        SMCHECK_STATUS,
        SMCHECK_CMD_NOT_SET
    };
    SmCheckCmd() : command(SMCHECK_CMD_NOT_SET) {}
    explicit SmCheckCmd(CommandType cmd) : command(cmd) {}

    CommandType command;
};

class SmCheckActionCmd: public SmCheckCmd {
  public:
    SmCheckActionCmd(const fpi::SMCheckCmd &cmd, SmTokenSet tgtTokens) {
        switch (cmd) {
            case fpi::SMCHECK_START:
                command = SMCHECK_START;
                this->tgtTokens = tgtTokens;
                break;
            case fpi::SMCHECK_STOP:
                command = SMCHECK_STOP;
                break;
            default:
                fds_panic("Unknown SmCheckCmd");
        }
    }
    // Optional parameters for SMCHECK_START
    SmTokenSet tgtTokens;
};

class SmCheckStatusCmd: public SmCheckCmd {
  public:
    explicit SmCheckStatusCmd(fpi::CtrlNotifySMCheckStatusRespPtr& status)
        : SmCheckCmd(SMCHECK_STATUS), checkStatus(status) {}

    fpi::CtrlNotifySMCheckStatusRespPtr checkStatus;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_SMCTRL_H_
