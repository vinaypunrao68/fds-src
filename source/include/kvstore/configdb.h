/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
#define SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
#include <kvstore/redis.h>
#include <platform/node-inventory.h>
#include <fds_volume.h>
#include <dlt.h>

namespace fds {
    namespace fpi = FDS_ProtocolInterface;
    namespace kvstore {        
        using PolicyInfo = fpi::FDSP_PolicyInfoType;
        
        struct ConfigDB {
            ConfigDB(const std::string& host = "localhost", uint port = 6379, uint poolsize = 10);
            ~ConfigDB();
            bool isConnected() ;
            
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
            bool storeDlt(const DLT& dlt, int localDomain = 0);
            bool getDlt(DLT& dlt, fds_uint64_t version, int localDomain = 0);
            bool loadDlts (DLTManager& dltMgr, int localDomain = 0);
            bool storeDlts(DLTManager& dltMgr, int localDomain = 0);

            // nodes
            bool addNode(const NodeInvData& node);
            bool updateNode(const NodeInvData& node);
            bool removeNode(const NodeUuid& uuid);
            bool getNode(const NodeUuid& uuid, NodeInvData& node);
            bool nodeExists(const NodeUuid& uuid);
            bool getAllNodes(std::vector<NodeInvData>& nodes, int localDomain = 0);
            
            // volume policies
            bool addPolicy(const PolicyInfo& policyInfo, int localDomain = 0);
            bool updatePolicy(const PolicyInfo& policyInfo, int localDomain = 0);
            bool deletePolicy(const PolicyInfo& policyInfo, int localDomain = 0);
            bool getPolicies(std::vector<PolicyInfo>& policies, int localDomain = 0);
          protected:
            redis::Redis r;
        };
    }  // namespace kvstore

}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
