/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_STATE_SWITCH_H_
#define SOURCE_PLATFORM_INCLUDE_STATE_SWITCH_H_

namespace fds
{
    static const state_switch_t    sgl_node_wrk_flow[] =
    {
        /* Current State            | Message Input           | Next State              */
        { NodeDown::st_index(),       fpi::NodeInfoMsgTypeId,   NodeStarted::st_index() },
        { NodeDown::st_index(),       fpi::NodeDownTypeId,      NodeDown::st_index() },

        { NodeStarted::st_index(),    fpi::NodeQualifyTypeId,   NodeQualify::st_index() },
        { NodeStarted::st_index(),    fpi::NodeDownTypeId,      NodeDown::st_index() },

        { NodeQualify::st_index(),    fpi::NodeUpgradeTypeId,   NodeUpgrade::st_index() },
        { NodeQualify::st_index(),    fpi::NodeRollbackTypeId,  NodeUpgrade::st_index() },
        { NodeQualify::st_index(),    fpi::NodeIntegrateTypeId, NodeIntegrate::st_index() },
        { NodeQualify::st_index(),    fpi::NodeDownTypeId,      NodeDown::st_index() },

        { NodeUpgrade::st_index(),    fpi::NodeInfoMsgTypeId,   NodeQualify::st_index() },
        { NodeUpgrade::st_index(),    fpi::NodeDownTypeId,      NodeDown::st_index() },
        { NodeRollback::st_index(),   fpi::NodeInfoMsgTypeId,   NodeQualify::st_index() },
        { NodeRollback::st_index(),   fpi::NodeDownTypeId,      NodeDown::st_index() },

        { NodeIntegrate::st_index(),  fpi::NodeDeployTypeId,    NodeDeploy::st_index() },
        { NodeIntegrate::st_index(),  fpi::NodeDownTypeId,      NodeDown::st_index() },

        { NodeDeploy::st_index(),     fpi::NodeDeployTypeId,     NodeDeploy::st_index() },
        { NodeDeploy::st_index(),     fpi::NodeFunctionalTypeId, NodeFunctional::st_index() },
        { NodeDeploy::st_index(),     fpi::NodeDownTypeId,       NodeDown::st_index() },

        { NodeFunctional::st_index(), fpi::NodeDeployTypeId,     NodeFunctional::st_index() },
        { NodeFunctional::st_index(), fpi::NodeFunctionalTypeId, NodeFunctional::st_index() },
        { NodeFunctional::st_index(), fpi::NodeDownTypeId,       NodeDown::st_index() },

        { -1,                         fpi::UnknownMsgTypeId,    -1 }
        /* Current State            | Message Input           | Next State              */
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_STATE_SWITCH_H_
