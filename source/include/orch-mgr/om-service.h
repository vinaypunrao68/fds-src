/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_ORCH_MGR_OM_SERVICE_H_
#define SOURCE_INCLUDE_ORCH_MGR_OM_SERVICE_H_

#include <fds_module.h>

namespace fds {

/**
 * Main OM module vector.
 */
class OM_Module : public Module
{
  public:
    explicit OM_Module(char const *const name);
    ~OM_Module();

    /**
     * Access to sub modules through the module singleton.
     */
    inline Module *om_nodedomain_mod() {
        return om_node_domain;
    }
    inline Module *om_clusmap_mod() {
        return om_clus_map;
    }
    inline Module *om_dlt_mod() {
        return om_dlt;
    }
    /**
     * Module methods.
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
    Module                  *om_node_domain;
    Module                  *om_clus_map;
    Module                  *om_dlt;
};

extern OM_Module             gl_OMModule;

}  // namespace fds

#endif  // SOURCE_INCLUDE_ORCH_MGR_OM_SERVICE_H_
