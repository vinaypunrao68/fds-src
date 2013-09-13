#ifndef __STOR_HV_DATA_PLACEMENT_H__
#define  __STOR_HV_DATA_PLACEMENT_H__

#include "include/fds_types.h"
#include "include/fds_err.h"
#include "lib/OMgrClient.h"

using namespace fds;

class StorHvDataPlacement {
public:
  typedef enum {
    DP_NO_OM_MODE, /* Don't expect an OM. Use local DLT/DMT info. */
    DP_NORMAL_MODE,     /* Normal mode with a OM running. */
    DP_MAX
  } dp_mode;

private:
  /*
   * A stock IP addr to use in test modes.
   */
  fds_uint32_t test_ip_addr;

  /*
   * Current mode of this instance.
   */
  dp_mode mode;

  /* OMgrClient passed down from SH */
  OMgrClient *parent_omc;

public:  
  StorHvDataPlacement(dp_mode _mode,
                      OMgrClient *omc);
  StorHvDataPlacement(dp_mode _mode,
                      fds_uint32_t test_ip,
                      OMgrClient *omc);
  ~StorHvDataPlacement();
  
  void  getDLTNodesForDoidKey(unsigned char doid_key, int *node_ids, int *n_nodes) {
    if (mode == DP_NO_OM_MODE) {
      /*
       * TODO: Set up some stock response here.
       * Returns 1 nodes with ID 0.
       */
      (*n_nodes) = 1;
      node_ids[(*n_nodes) - 1] = 0;
    } else {
      parent_omc->getDLTNodesForDoidKey(doid_key, node_ids, n_nodes);
    }
  }
  void  getDMTNodesForVolume(int volid, int *node_ids, int *n_nodes ) {
    if (mode == DP_NO_OM_MODE) {
      /*
       * TODO: Set up some stock response here.
       * Returns 1 nodes with ID 0.
       */
      (*n_nodes) = 1;
      node_ids[(*n_nodes) - 1] = 0;      
    } else {
      assert(mode == DP_NORMAL_MODE);
      parent_omc->getDMTNodesForVolume(volid, node_ids, n_nodes);
    }
  }

  Error getNodeInfo(int node_id, fds_uint32_t *node_ip_addr, int *node_state) {
    Error err(ERR_OK);
    int ret_code = 0;
    
    if (mode == DP_NO_OM_MODE) {
      /*
       * Return stock response.
       */
      *node_ip_addr = test_ip_addr;
      *node_state   = 0;
    } else {
      assert(mode == DP_NORMAL_MODE);
      ret_code = parent_omc->getNodeInfo(node_id, node_ip_addr, node_state);
      if (ret_code != 0) {
        err = ERR_DISK_READ_FAILED;
      }
    }

    return err;
  }
  
  static void nodeEventHandler(int node_id, unsigned int node_ip_addr, int node_state);
};

#endif
