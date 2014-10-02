/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_DOMAIN_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_DOMAIN_H_

#include <fds_module.h>

namespace fds {

class OM_NodeWrkFlow : public Module
{
  public:
    explicit OM_NodeWrkFlow(char const *const name);
    virtual ~OM_NodeWrkFlow();

    /* Module methods. */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_enable_service();
    virtual void mod_shutdown();
};

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_DOMAIN_H_
