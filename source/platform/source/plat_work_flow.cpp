/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "plat_work_item.h"
#include "plat_work_flow.h"

namespace fds
{
    PlatWorkFlow    gl_PlatWorkFlow;

    /**
     * mod_init
     * --------
     */
    int PlatWorkFlow::mod_init(SysParams const *const param)
    {
        gl_NodeWorkFlow = this;
        return NodeWorkFlow::mod_init(param);
    }

    /**
     * mod_startup
     * -----------
     */
    void PlatWorkFlow::mod_startup()
    {
        NodeWorkFlow::mod_startup();
    }

    /**
     * mod_enable_service
     * ------------------
     */
    void PlatWorkFlow::mod_enable_service()
    {
        NodeWorkFlow::mod_enable_service();
    }

    /**
     * mod_shutdown
     * ------------
     */
    void PlatWorkFlow::mod_shutdown()
    {
        NodeWorkFlow::mod_shutdown();
    }

    /**
     * wrk_item_alloc
     * --------------
     */
    NodeWorkItem::ptr PlatWorkFlow::wrk_item_alloc(fpi::SvcUuid               &peer,
                                                   bo::intrusive_ptr<PmAgent>  owner,
                                                   bo::intrusive_ptr<DomainContainer> domain)
    {
        fpi::DomainID    did;
        return new PlatWorkItem(peer, did, owner, wrk_fsm, this);
    }
}  // namespace fds
