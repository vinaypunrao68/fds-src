#ifndef __STOR_HV_DATA_PLACEMENT_H__
#define  __STOR_HV_DATA_PLACEMENT_H__
#include "lib/OMgrClient.h" 
using namespace fds;

class StorHvDataPlacement {
public:
  typedef enum {
    DP_NO_OM_MODE, /* Don't expect an OM. Use local DLT/DMT info. */
    DP_NORMAL_MODE,     /* Normal mode with a OM running. */
    DP_MAX
  } dp_mode;

  dp_mode mode;
  
  explicit StorHvDataPlacement(dp_mode _mode);
  ~StorHvDataPlacement();
  
  OMgrClient *omClient;
  double omIPAddr;
  
  void  getDLTNodesForDoidKey(unsigned char doid_key, int *node_ids, int *n_nodes) {
    omClient->getDLTNodesForDoidKey(doid_key, node_ids, n_nodes);
    return;
  }
  void  getDMTNodesForVolume(int volid, int *node_ids, int *n_nodes ) {
    if (mode == DP_NO_OM_MODE) {
      /*
       * TODO: Set up some stock response here.
       */
    } else {
      omClient->getDMTNodesForVolume(volid, node_ids, n_nodes);
    }
  }
  
  static void nodeEventHandler(int node_id, unsigned int node_ip_addr, int node_state);
};

#endif
