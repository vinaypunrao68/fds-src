#ifndef __STOR_HV_DATA_PLACEMENT_H__
#define  __STOR_HV_DATA_PLACEMENT_H__
#include "OMgrClient.h" 
using namespace fds;

class StorHvDataPlacement {
public:
	StorHvDataPlacement();
	~StorHvDataPlacement();

        OMgrClient *omClient;
        double omIPAddr;

	void  getDLTNodesForDoidKey(unsigned char doid_key, int *node_ids, int *n_nodes) {
           omClient->getDLTNodesForDoidKey(doid_key, node_ids, n_nodes);
           return;
        }
	void  getDMTNodesForVolume(int volid, int *node_ids, int *n_nodes ) {
           omClient->getDMTNodesForVolume(volid, node_ids, n_nodes);
           return;
        }
        static void nodeEventHandler(int node_id, unsigned int node_ip_addr, int node_state);
};

#endif
