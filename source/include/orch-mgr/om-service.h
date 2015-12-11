/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_ORCH_MGR_OM_SERVICE_H_
#define SOURCE_INCLUDE_ORCH_MGR_OM_SERVICE_H_

#include <fds_module.h>

namespace fds {
class ClusterMap;
class OM_DLTMod;
class OM_DMTMod;
class OM_NodeDomainMod;
class DataPlacement;
class VolumePlacement;

/**
 * Main OM module vector.
 */
class OM_Module : public Module
{
  public:
    static OM_Module *om_singleton();

    explicit OM_Module(char const *const name);
    ~OM_Module();

    /**
     * Access to sub modules through the module singleton.
     */
    inline OM_NodeDomainMod *om_nodedomain_mod() {
        return om_node_domain;
    }
    inline ClusterMap *om_clusmap_mod() {
        return om_clus_map;
    }
    inline DataPlacement *om_dataplace_mod() {
        return om_data_place;
    }
    inline OM_DLTMod *om_dlt_mod() {
        return om_dlt;
    }
    inline OM_DMTMod *om_dmt_mod() {
        return om_dmt;
    }
    inline VolumePlacement *om_volplace_mod() {
        return om_volume_place;
    }

    inline bool om_replica_mode() {
        return replica_mode;
    }

    /**
     * Module methods.
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
    OM_NodeDomainMod        *om_node_domain;
    ClusterMap              *om_clus_map;
    OM_DLTMod               *om_dlt;
    OM_DMTMod               *om_dmt;
    DataPlacement           *om_data_place;
    VolumePlacement         *om_volume_place;

    /**
     * Toggles for service layer
     */
    bool    replica_mode;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_ORCH_MGR_OM_SERVICE_H_
