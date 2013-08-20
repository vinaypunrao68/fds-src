module FDSP_Types{

enum FDSP_NodeState {
     FDS_Node_Up,
     FDS_Node_Down,
     FDS_Node_Rmvd
};

class FDSP_Node_Info_Type {
  int      node_id;
  double           node_ip;
  FDSP_NodeState     node_state;
};


sequence<FDSP_Node_Info_Type> Node_Info_List_Type;

sequence<int> Node_List_Type;
sequence<Node_List_Type> Node_Table_Type;


class FDSP_DLT_Type {
      int DLT_version;
      Node_Table_Type DLT;
};

class FDSP_DMT_Type {
      int DMT_version;
      Node_Table_Type DMT;
};

};
