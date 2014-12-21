/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_DOWN_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_DOWN_H_

namespace fds
{
    class NodeDown : public StateEntry
    {
        public:
            static inline int st_index()
            {
                return 0;
            }
            virtual char const *const st_name() const
            {
                return "NodeDown";
            }

            NodeDown() : StateEntry(st_index(), -1)
            {
            }
            virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_DOWN_H_
