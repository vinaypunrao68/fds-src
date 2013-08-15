#ifndef __FDSP_H__
#define __FDSP_H__
#include <Ice/Identity.ice>
#include <Ice/BuiltinSequences.ice>
#pragma once

module FDS_PubSub_Interface {


enum FDS_PubSub_MsgCodeType {
   FDS_Node_Add,
   FDS_Node_Rmv,
   FDS_DLT_Update,
   FDS_DMT_Update,
   FDS_Node_Update
};

enum FDS_PubSub_NodeState {
     FDS_Node_Up,
     FDS_Node_Down,
     FDS_Node_Rmvd
};

class FDS_PubSub_Node_Info_Type {
  int      node_id;
  double	   node_ip;
  FDS_PubSub_NodeState	   node_state;
};

class FDS_PubSub_DLT_Type {
      int DLT_version;
};

class FDS_PubSub_DMT_Type {
      int DMT_version;
};


class FDS_PubSub_MsgHdrType {
    FDS_PubSub_MsgCodeType     msg_code;		
    int        major_ver;  /* Major version number of this message */
    int        minor_ver;  /* Minor version number of this message */
    int        msg_id;     /* Message id to discard duplicate request & maintain causal order */
    int	       tennant_id; 
    int        domain_id;	
};

interface FDS_OMgr_Subscriber {
    void NotifyNodeAdd(FDS_PubSub_MsgHdrType fds_msg_hdr, FDS_PubSub_Node_Info_Type node_info);
    void NotifyNodeRmv(FDS_PubSub_MsgHdrType fds_msg_hdr, FDS_PubSub_Node_Info_Type node_info);
};

};
#endif
