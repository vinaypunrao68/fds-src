/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_AGENT_CONTAINER_H_
#define SOURCE_INCLUDE_PLATFORM_AGENT_CONTAINER_H_

#include <string>

#include "node_agent.h"

class netSessionTbl;

namespace fds
{
    // -------------------------------------------------------------------------------------
    // Common Agent Containers
    // -------------------------------------------------------------------------------------
    class AgentContainer : public RsContainer
    {
        public:
            typedef boost::intrusive_ptr<AgentContainer> pointer;

            /**
             * Iter loop to extract NodeAgent ptr:
             */
            template <typename T> void agent_foreach(T arg, void (*fn)(T arg, NodeAgent::pointer
                                                                       elm))
            {
                for (fds_uint32_t i = 0; i < rs_cur_idx; i++)
                {
                    NodeAgent::pointer    cur = agt_cast_ptr<NodeAgent>(rs_array[i]);

                    if (rs_array[i] != NULL)
                    {
                        (*fn)(arg, cur);
                    }
                }
            }

            template <typename T1, typename T2>
            void agent_foreach(T1 a1, T2 a2, void (*fn)(T1, T2, NodeAgent::pointer elm))
            {
                for (fds_uint32_t i = 0; i < rs_cur_idx; i++)
                {
                    NodeAgent::pointer    cur = agt_cast_ptr<NodeAgent>(rs_array[i]);

                    if (rs_array[i] != NULL)
                    {
                        (*fn)(a1, a2, cur);
                    }
                }
            }

            template <typename T1, typename T2, typename T3>
            void agent_foreach(T1 a1, T2 a2, T3 a3, void (*fn)(T1, T2, T3, NodeAgent::pointer elm))
            {
                for (fds_uint32_t i = 0; i < rs_cur_idx; i++)
                {
                    NodeAgent::pointer    cur = agt_cast_ptr<NodeAgent>(rs_array[i]);

                    if (rs_array[i] != NULL)
                    {
                        (*fn)(a1, a2, a3, cur);
                    }
                }
            }

            template <typename T1, typename T2, typename T3, typename T4>
            void agent_foreach(T1 a1, T2 a2, T3 a3, T4 a4,
                               void (*fn)(T1, T2, T3, T4, NodeAgent::pointer elm))
            {
                for (fds_uint32_t i = 0; i < rs_cur_idx; i++)
                {
                    NodeAgent::pointer    cur = agt_cast_ptr<NodeAgent>(rs_array[i]);

                    if (rs_array[i] != NULL)
                    {
                        (*fn)(a1, a2, a3, a4, cur);
                    }
                }
            }

            /**
             * iterator that returns number of agents that completed function successfully
             * Only iterates through active agents (for which we called agent_activate, and
             * did not call agent_deactivate).
             */
            template <typename T>
            fds_uint32_t agent_ret_foreach(T arg, Error (*fn)(T arg, NodeAgent::pointer elm))
            {
                fds_uint32_t    count = 0;
                for (fds_uint32_t i = 0; i < rs_cur_idx; i++)
                {
                    Error                 err(ERR_OK);
                    NodeAgent::pointer    cur = agt_cast_ptr<NodeAgent>(rs_array[i]);

                    if ((rs_array[i] != NULL) &&
                        (rs_get_resource(cur->get_uuid()) != NULL))
                    {
                        err =    (*fn)(arg, cur);

                        if (err.ok())
                        {
                            ++count;
                        }
                    }
                }
                return count;
            }

            template <typename T1, typename T2>
            fds_uint32_t agent_ret_foreach(T1 a1, T2 a2,
                                           Error (*fn)(T1, T2, NodeAgent::pointer elm))
            {
                fds_uint32_t    count = 0;
                for (fds_uint32_t i = 0; i < rs_cur_idx; i++)
                {
                    Error                 err(ERR_OK);
                    NodeAgent::pointer    cur = agt_cast_ptr<NodeAgent>(rs_array[i]);

                    if (rs_array[i] != NULL)
                    {
                        err =    (*fn)(a1, a2, cur);

                        if (err.ok())
                        {
                            ++count;
                        }
                    }
                }
                return count;
            }

            template <typename T1, typename T2, typename T3>
            fds_uint32_t agent_ret_foreach(T1 a1, T2 a2, T3 a3,
                                           Error (*fn)(T1, T2, T3, NodeAgent::pointer elm))
            {
                fds_uint32_t    count = 0;
                for (fds_uint32_t i = 0; i < rs_cur_idx; i++)
                {
                    Error                 err(ERR_OK);
                    NodeAgent::pointer    cur = agt_cast_ptr<NodeAgent>(rs_array[i]);

                    if (rs_array[i] != NULL)
                    {
                        err =    (*fn)(a1, a2, a3, cur);

                        if (err.ok())
                        {
                            ++count;
                        }
                    }
                }
                return count;
            }

            /**
             * Return the generic NodeAgent::pointer from index position or its uuid.
             */
            inline NodeAgent::pointer agent_info(fds_uint32_t idx)
            {
                if (idx < rs_cur_idx)
                {
                    return agt_cast_ptr<NodeAgent>(rs_array[idx]);
                }
                return NULL;
            }

            inline NodeAgent::pointer agent_info(const NodeUuid &uuid)
            {
                return agt_cast_ptr<NodeAgent>(rs_get_resource(uuid));
            }

            /**
             * Activate the agent obj so that it can be looked up by name or uuid.
             */
            virtual void agent_activate(NodeAgent::pointer agent)
            {
                rs_register(agent);
            }

            virtual void agent_deactivate(NodeAgent::pointer agent)
            {
                rs_unregister(agent);
            }

            /**
             * Register and unregister a node agent.
             */
            virtual Error agent_register(const NodeUuid       &uuid, const FdspNodeRegPtr msg,
                                         NodeAgent::pointer   *out, bool activate = true);
            virtual Error agent_unregister(const NodeUuid &uuid, const std::string &name);

            /**
             * @param shm (i) - the shared memory segment to access inventory data.
             * @param out (i/o) - NULL if the agent obj hasn't been allocated.
             * @param ro, rw (i) - indices to get the node inventory data RO or RW (-1 invalid).
             * @param bool (i) - true if want to register, publish the node/service endpoint.
             */
            virtual bool agent_register(const ShmObjRO *shm, NodeAgent::pointer *out, int ro,
                                        int rw);
            /**
             * Establish RPC connection with the remte agent.
             */
            virtual void agent_handshake(boost::shared_ptr<netSessionTbl> net,
                                         NodeAgent::pointer agent);

        protected:
            FdspNodeType                        ac_id;

            // NetSession fields, need to remove.
            boost::shared_ptr<netSessionTbl>    ac_cpSessTbl;

            virtual ~AgentContainer();
            explicit AgentContainer(FdspNodeType id);
    };

    /**
     * Down cast a node container intrusive pointer.
     */
    template <class T> static inline T *agt_cast_ptr(AgentContainer::pointer ptr)
    {
        return static_cast<T *>(get_pointer(ptr));
    }

    template <class T> static inline T *agt_cast_ptr(RsContainer::pointer ptr)
    {
        return static_cast<T *>(get_pointer(ptr));
    }
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_AGENT_CONTAINER_H_
