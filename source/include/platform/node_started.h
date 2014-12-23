/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_STARTED_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_STARTED_H_

#include "node_down.h"

namespace fds
{
    class NodeStarted : public StateEntry
    {
        public:
            static inline int st_index()
            {
                return 1;
            }
            virtual char const *const st_name() const
            {
                return "NodeStarted";
            }

            NodeStarted() : StateEntry(st_index(), NodeDown::st_index())
            {
            }
            virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;

            static int
            wrk_next_step(const StateEntry *entry, EventObj::pointer evt, StateObj::pointer cur);
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_STARTED_H_
