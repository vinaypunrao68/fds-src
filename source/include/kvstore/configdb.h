/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
#define SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
#include <unordered_set>
#include <kvstore/kvstore.h>
#include <platform/node-inventory.h>
#include <fds_volume.h>
#include <dlt.h>

namespace fds {
    namespace fpi = FDS_ProtocolInterface;
    namespace kvstore {        
        using PolicyInfo = fpi::FDSP_PolicyInfoType;
        
        struct ConfigDB : KVStore {
            ConfigDB(const std::string& host = "localhost", uint port = 6379, uint poolsize = 10);
            virtual ~ConfigDB();
            
            // domains
            std::string getGlobalDomain();
            bool setGlobalDomain(ConstString globalDomain="fds");
            bool addLocalDomain (ConstString name="local", int localDomain = 0, int globalDomain = 0);
            bool getLocalDomains(std::map<int,std::string>& mapDomains, int globalDomain = 0);
            
            // volumes
            bool addVolume(const VolumeDesc& volumeDesc);
            bool updateVolume(const VolumeDesc& volumeDesc);
            bool deleteVolume(fds_volid_t volumeId, int localDomain = 0);
            bool volumeExists(fds_volid_t volumeId);
            bool volumeExists(ConstString volumeName, int localDomain = 0);
            bool getVolumeIds(std::vector<fds_volid_t>& volumeIds, int localDomain = 0);
            bool getVolumes(std::vector<VolumeDesc>& volumes, int localDomain = 0);
            bool getVolume(fds_volid_t volumeId, VolumeDesc& volumeDesc);

            // dlt
            // to store different types of dlt [current,new,old,target]
            fds_uint64_t getDltVersionForType(const std::string type, int localDomain = 0);
            bool setDltType(fds_uint64_t version, const std::string type, int localDomain = 0);

            bool storeDlt(const DLT& dlt, const std::string type = "" , int localDomain = 0);
            bool getDlt(DLT& dlt, fds_uint64_t version, int localDomain = 0);
            bool loadDlts (DLTManager& dltMgr, int localDomain = 0);
            bool storeDlts(DLTManager& dltMgr, int localDomain = 0);

            // nodes
            bool addNode(const NodeInvData& node);
            bool updateNode(const NodeInvData& node);
            bool removeNode(const NodeUuid& uuid);
            bool getNode(const NodeUuid& uuid, NodeInvData& node);
            bool nodeExists(const NodeUuid& uuid);
            bool getNodeIds(std::unordered_set<NodeUuid, UuidHash>& nodes, int localDomain = 0);
            bool getAllNodes(std::vector<NodeInvData>& nodes, int localDomain = 0);
            std::string getNodeName(const NodeUuid& uuid);

            bool getNodeServices(const NodeUuid& uuid, NodeServices& services); 
            bool setNodeServices(const NodeUuid& uuid, const NodeServices& services); 
            uint getNodeNameCounter();
            
            // volume policies            
            bool getPolicy(fds_uint32_t volPolicyId, FDS_VolumePolicy& volumePolicy, int localDomain = 0); //NOLINT
            bool addPolicy(const FDS_VolumePolicy& volumePolicy, int localDomain = 0);
            bool updatePolicy(const FDS_VolumePolicy& volumePolicy, int localDomain = 0);
            bool deletePolicy(fds_uint32_t volPolicyId, int localDomain = 0);
            bool getPolicies(std::vector<FDS_VolumePolicy>& policies, int localDomain = 0);
            
        };
    }  // namespace kvstore

}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
