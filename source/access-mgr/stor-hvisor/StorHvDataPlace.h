#ifndef __STOR_HV_DATA_PLACEMENT_H__
#define  __STOR_HV_DATA_PLACEMENT_H__

#include <fds_types.h>
#include <fds_error.h>

/* TODO: use API interface header file in include directory */
#include <lib/OMgrClient.h>

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
  fds_uint32_t test_sm_port;
  fds_uint32_t test_dm_port;

  /*
   * Current mode of this instance.
   */
  dp_mode mode;

  /* OMgrClient passed down from SH */
  OMgrClient *parent_omc;

public:  
  StorHvDataPlacement(dp_mode _mode,
                      OMgrClient *omc)
      : test_ip_addr(0),
        test_sm_port(0),
        test_dm_port(0),
        mode(_mode),
        parent_omc(omc)
  { }

  StorHvDataPlacement(dp_mode _mode,
                      fds_uint32_t test_ip,
                      fds_uint32_t test_sm,
                      fds_uint32_t test_dm,
                      OMgrClient *omc)
      : StorHvDataPlacement(_mode, omc)
  {
      test_ip_addr = test_ip;
      test_sm_port = test_sm;
      test_dm_port = test_dm;
  }

  ~StorHvDataPlacement()
  { }
  
  DltTokenGroupPtr  getDLTNodesForDoidKey(const ObjectID &objId) {
      return parent_omc->getDLTNodesForDoidKey(objId);
  }

  DmtColumnPtr getDMTNodesForVolume(fds_volid_t volid) {
      if (mode == DP_NO_OM_MODE) {
          /*
           * TODO: Set up some stock response here.
           * Returns 1 nodes with ID 1. The DM has
           * stock node id 1.
           */
          DmtColumnPtr col(new DmtColumn(1));
          col->set(0, 1);  // node id 1
          return col;
      }
      // else normal mode
      assert(mode == DP_NORMAL_MODE);
      return parent_omc->getDMTNodesForVolume(volid);
  }

  Error getNodeInfo(fds_uint64_t node_id,
                    fds_uint32_t *node_ip_addr,
                    fds_uint32_t *node_port,
                    int *node_state) {
    Error err(ERR_OK);
    int ret_code = 0;
    
    if (mode == DP_NO_OM_MODE) {
      /*
       * Return stock response.
       */
      *node_ip_addr = test_ip_addr;
      *node_state   = 0;
      if (node_id == 0) {
        *node_port = test_sm_port;
      } else if (node_id == 1) {
        *node_port = test_dm_port;
      } else {
        assert(0);
      }
    } else {
      assert(mode == DP_NORMAL_MODE);
      ret_code = parent_omc->getNodeInfo(node_id,
                                         node_ip_addr,
                                         node_port,
                                         node_state);
      if (ret_code != 0) {
        err = ERR_DISK_READ_FAILED;
      }
    }

    return err;
  }
};

#endif
