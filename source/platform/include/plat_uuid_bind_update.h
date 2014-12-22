/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLAT_UUID_BIND_UPDATE_H_
#define SOURCE_PLATFORM_INCLUDE_PLAT_UUID_BIND_UPDATE_H_

#include "net/RpcFunc.h"
#include "fdsp/PlatNetSvc.h"
#include "platform/node_agent_iter.h"
#include "platform/domain_agent.h"

namespace fds
{
    class PlatUuidBindUpdate : public NodeAgentIter
    {
        public:
            typedef bo::intrusive_ptr<PlatUuidBindUpdate> pointer;

            /**
             * Plugin that iterates through each platform agent in the local domain inventory.
             */
            bool rs_iter_fn(Resource::pointer curr)
            {
                EpSvcHandle::pointer    eph;
                DomainAgent::pointer    agent;

                agent = agt_cast_ptr<DomainAgent>(curr);
                auto    rpc = agent->node_svc_rpc(&eph);

                if (rpc != NULL)
                {
                    NET_SVC_RPC_CALL(eph, rpc, allUuidBinding, bind_msg);
                }
                return true;
            }

            /**
             * Perform uuid binding update in a worker thread.
             */
            static void uuid_bind_update(PlatUuidBindUpdate::pointer itr, ep_shmq_req_t *rec)
            {
                itr->bind_rec = rec;
                EpPlatLibMod::ep_uuid_bind_to_msg(&rec->smq_rec, &itr->bind_msg);

                itr->foreach_pm();
                delete rec;
            }

        protected:
            ep_shmq_req_t      *bind_rec;
            fpi::UuidBindMsg    bind_msg;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLAT_UUID_BIND_UPDATE_H_
