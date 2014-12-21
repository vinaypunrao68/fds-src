/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_INTEGRATE_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_INTEGRATE_EVT_H_

namespace fds
{
    struct NodeIntegrateEvt : public    NodeWrkEvent
    {
        bo::shared_ptr<fpi::NodeIntegrate> evt_msg;

        NodeIntegrateEvt(fpi::AsyncHdrPtr hdr,
                         bo::shared_ptr<fpi::NodeIntegrate> msg, bool act = true)
            : NodeWrkEvent(fpi::NodeIntegrateTypeId, hdr, act), evt_msg(msg)
        {
        }

        virtual void evt_name(std::ostream &os) const
        {
            os << " NodeIntegrate " << evt_msg;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_INTEGRATE_EVT_H_
