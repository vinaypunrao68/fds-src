#define  FDS_TRANS_EMPTY                0x00
#define  FDS_TRANS_OPEN                 0x1
#define  FDS_TRANS_OPENED               0x2
#define  FDS_TRANS_COMMITTED            0x3
#define  FDS_TRANS_SYNCED               0x4
#define  FDS_TRANS_DONE                 0x5
#define  FDS_TRANS_VCAT_QUERY_PENDING   0x6
#define  FDS_TRANS_GET_OBJ	        0x7

class   StorHvJournalEntry {
public:
  StorHvJournalEntry();
  ~StorHvJournalEntry();
  void reinitialize();

        short  replc_cnt;
        short  sm_ack_cnt;
        short  dm_ack_cnt;
        short  dm_commit_cnt;
        short  trans_state;
	unsigned short incarnation_number;
        FDS_IO_Type   op;
        FDS_ObjectIdType data_obj_id;
        int      data_obj_len;
	unsigned int block_offset;
        void     *fbd_ptr;
        void     *read_ctx;
        void     *write_ctx;
	complete_req_cb_t comp_req;
	void 	*comp_arg1;
	void 	*comp_arg2;
        FDSP_MsgHdrTypePtr     sm_msg;
        FDSP_MsgHdrTypePtr     dm_msg;
        struct   timer_list *p_ti;
        int      lt_flag;
        int      st_flag;
        short    num_dm_nodes;
        FDSP_IpNode    dm_ack[FDS_MAX_DM_NODES_PER_CLST];
        short    num_sm_nodes;
        FDSP_IpNode    sm_ack[FDS_MAX_SM_NODES_PER_CLST];
};

class StorHvJournal {

private:

  unsigned int get_free_trans_id();
  void return_free_trans_id(unsigned int trans_id);

public:
 	StorHvJournal();
	StorHvJournal(unsigned int max_jrnl_entries);
 	~StorHvJournal();
	StorHvJournalEntry  *rwlog_tbl;
	std::unordered_map<unsigned int, unsigned int> block_to_jrnl_idx;
	std::queue<unsigned int> free_trans_ids;
	unsigned int max_journal_entries;

	int get_trans_id_for_block(unsigned int block_offset);

 
};

