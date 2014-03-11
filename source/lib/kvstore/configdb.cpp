/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/configdb.h>
#include <util/Log.h>
#include <stdlib.h>

namespace fds { namespace kvstore {
using redis::Reply;
using redis::RedisException;

ConfigDB::ConfigDB(const std::string& host,
                   uint port,
                   uint poolsize) : KVStore(host, port, poolsize) {
    LOGNORMAL << "instantiating configdb";
}

ConfigDB::~ConfigDB() {
    LOGNORMAL << "destroying configdb";
}

// domains
std::string ConfigDB::getGlobalDomain() {
    return r.get("global.domain").getString();
}

bool ConfigDB::setGlobalDomain(ConstString globalDomain) {
    return r.set("global.domain",globalDomain).isOk();
}

bool ConfigDB::addLocalDomain (ConstString name, int localDomain, int globalDomain) {
    Reply reply = r.sendCommand("sadd %d:domains %d", globalDomain, localDomain);
    if (!reply.wasModified()) {
        LOGWARN << "domain id: "<<localDomain<<" already exists for global :" <<globalDomain;
    }
    return r.sendCommand("hmset domain:%d id %d name %s", localDomain, localDomain, name.c_str()).isOk();
}

bool ConfigDB::getLocalDomains(std::map<int,std::string>& mapDomains, int globalDomain) {
    try {
        Reply reply = r.sendCommand("smembers %d:domains",globalDomain);
        std::vector<long long> vec;
        reply.toVector(vec);

        for (uint i = 0; i < vec.size() ; i++ ) {
            reply = r.sendCommand("hget domain:%d name",(int)vec[i]);
            mapDomains[(int)vec[i]] = reply.getString();
        }
        return true;
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}
            
// volumes
bool ConfigDB::addVolume(const VolumeDesc& vol) {
    // add the volume to the volume list for the domain
    try {
        Reply reply = r.sendCommand("sadd %d:volumes %ld", vol.localDomainId, vol.volUUID);
        if (!reply.wasModified()) {
            LOGWARN << "volume [" << vol.volUUID << "] already exists for domain [" << vol.localDomainId << "]";
        }

        // add the volume data
        reply = r.sendCommand("hmset vol:%ld uuid %ld"
                              " name %s"
                              " tennant.id %d"
                              " local.domain.id %d"
                              " global.domain.id %d"
                              " type %d"
                              " capacity %.3f"
                              " quota.max %.2f"
                              " replica.count %d"
                              " write.quorum %d"
                              " read.quorum %d"
                              " conistency.protocol %d"
                              " volume.policy.id %d"
                              " archive.policy.id %d"
                              " placement.policy.id %d"
                              " app.workload %d"
                              " backup.vol.id %ld"
                              " iops.min %.3f"
                              " iops.max %.3f"
                              " relative.priority %d",
                              vol.volUUID, vol.volUUID,
                              vol.name.c_str(),
                              vol.tennantId,
                              vol.localDomainId,
                              vol.globDomainId,
                              vol.volType,
                              vol.capacity,
                              vol.maxQuota,
                              vol.replicaCnt,
                              vol.writeQuorum,
                              vol.readQuorum,
                              vol.consisProtocol,
                              vol.volPolicyId,
                              vol.archivePolicyId,
                              vol.placementPolicy,
                              vol.appWorkload,
                              vol.backupVolume,
                              vol.iops_min,
                              vol.iops_max,
                              vol.relativePrio);
        if (reply.isOk()) return true;
        LOGWARN << "msg: " << reply.getString();
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::updateVolume(const VolumeDesc& volumeDesc) {  
    return addVolume(volumeDesc);
}

bool ConfigDB::deleteVolume(fds_volid_t volumeId, int localDomainId) { 
    Reply reply = r.sendCommand("srem %d:volumes %ld", localDomainId, volumeId);
    if (!reply.wasModified()) {
        LOGWARN << "volume [" << volumeId << "] does NOT exist for domain [" << localDomainId << "]";
    }

    // del the volume data
    reply = r.sendCommand("del vol:%ld", volumeId);
    return reply.isOk(); 
}

bool ConfigDB::volumeExists(fds_volid_t volumeId){ return false; }
bool ConfigDB::volumeExists(ConstString volumeName, int localDomain){ return false; }
bool ConfigDB::getVolumeIds(std::vector<fds_volid_t>& volumeIds, int localDomain){ return false; }

bool ConfigDB::getVolumes(std::vector<VolumeDesc>& volumes, int localDomain) { 
    std::vector<long long> volumeIds;

    Reply reply = r.sendCommand("smembers %d:volumes", localDomain);
    reply.toVector(volumeIds);

    if (volumeIds.empty()) {
        LOGWARN << "no volumes found for domain [" << localDomain << "]";
        return false;
    }

    for (uint i = 0; i < volumeIds.size() ; i++ ) {
        VolumeDesc volume("",1); // dummy init
        getVolume(volumeIds[i],volume);
        volumes.push_back(volume);
    }
    return true;
}

bool ConfigDB::getVolume(fds_volid_t volumeId, VolumeDesc& vol) {
    Reply reply = r.sendCommand("hgetall vol:%ld",volumeId);
    StringList strings;
    reply.toVector(strings);
    
    if (strings.empty()) {
        LOGWARN << "unable to find volume [" << volumeId <<"]";
        return false;
    }

    for ( uint i = 0; i < strings.size() ; i+=2 ) {
        std::string& key = strings[i];
        std::string& value = strings[i+1];

        if (key == "uuid") {vol.volUUID = strtoull(value.c_str(),NULL,10);}
        else if (key == "name") { vol.name = value; }
        else if (key == "tennant.id") { vol.tennantId = atoi(value.c_str()); }
        else if (key == "local.domain.id") {vol.localDomainId = atoi(value.c_str());}
        else if (key == "global.domain.id") { vol.globDomainId = atoi(value.c_str());}
        else if (key == "type") { vol.volType = (fpi::FDSP_VolType)atoi(value.c_str()); }
        else if (key == "capacity") { vol.capacity = strtod (value.c_str(),NULL);}
        else if (key == "quota.max") { vol.maxQuota = strtod (value.c_str(),NULL);}
        else if (key == "replica.count") {vol.replicaCnt = atoi(value.c_str());}
        else if (key == "write.quorum") {vol.writeQuorum = atoi(value.c_str());}
        else if (key == "read.quorum") {vol.readQuorum = atoi(value.c_str());}
        else if (key == "conistency.protocol") {vol.consisProtocol = (fpi::FDSP_ConsisProtoType)atoi(value.c_str());}
        else if (key == "volume.policy.id") {vol.volPolicyId = atoi(value.c_str());}
        else if (key == "archive.policy.id") {vol.archivePolicyId = atoi(value.c_str());}
        else if (key == "placement.policy.id") {vol.placementPolicy = atoi(value.c_str());}
        else if (key == "app.workload") {vol.appWorkload = (fpi::FDSP_AppWorkload)atoi(value.c_str());}
        else if (key == "backup.vol.id") {vol.backupVolume = atol(value.c_str());}
        else if (key == "iops.min") {vol.iops_min = strtod (value.c_str(),NULL);}
        else if (key == "iops.max") {vol.iops_max = strtod (value.c_str(),NULL);}
        else if (key == "relative.priority") {vol.relativePrio = atoi(value.c_str());}
        else {
            LOGWARN << "unknown key for volume [" << volumeId <<"] - " << key;
        }
    }
    return true;
}

// dlt

fds_uint64_t ConfigDB::getDltVersionForType(const std::string type, int localDomain) {
    try {
        Reply reply = r.sendCommand("get %d:dlt:%s", localDomain, type.c_str());
        if (!reply.isNil()) {
            return reply.getLong();
        } else {
            LOGNORMAL << "no dlt set for type:" << type;
        }
    } catch (const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return 0;
}

bool ConfigDB::setDltType(fds_uint64_t version, const std::string type, int localDomain) {
    try {
        Reply reply = r.sendCommand("set %d:dlt:%s %ld", localDomain, type.c_str(), version);
        return reply.isOk();
    } catch (const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::storeDlt(const DLT& dlt, const std::string type, int localDomain) {
    try {
        //fds_uint64_t version dlt.getVersion();
        Reply reply = r.sendCommand("sadd %d:dlts %ld",localDomain,dlt.getVersion());
        if (!reply.wasModified()) {
            LOGWARN << "dlt [" << dlt.getVersion() << "] is already in for domain [" << localDomain << "]";
        }

        std::string serializedData, hexCoded;
        const_cast<DLT&>(dlt).getSerialized(serializedData);
        r.encodeHex(serializedData,hexCoded);

        reply = r.sendCommand("set %d:dlt:%ld %s", localDomain, dlt.getVersion(), hexCoded.c_str());
        bool fSuccess = reply.isOk();

        if (fSuccess && !type.empty()) {
            reply = r.sendCommand("set %d:dlt:%s %ld", localDomain, type.c_str(), dlt.getVersion());
            if (!reply.isOk()) {
                LOGWARN << "error setting type " << dlt.getVersion() << ":" << type;
            }
        }
        return fSuccess;

    } catch (const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::getDlt(DLT& dlt, fds_uint64_t version, int localDomain) {
    Reply reply = r.sendCommand("get %d:dlt:%ld", localDomain, version);
    std::string serializedData, hexCoded(reply.getString());
    if (hexCoded.length() < 10) {
        LOGERROR << "very less data for dlt : [" << version << "] : size = " << hexCoded.length();
        return false;
    }
    LOGDEBUG << "dlt : [" << version << "] : size = " << hexCoded.length();
    r.decodeHex(hexCoded,serializedData);
    dlt.loadSerialized(serializedData);
    return true;    
}

bool ConfigDB::loadDlts (DLTManager& dltMgr, int localDomain) {
    Reply reply = r.sendCommand("smembers %d:dlts",localDomain);
    std::vector<long long> dltVersions;
    reply.toVector(dltVersions);

    if (dltVersions.empty()) return false;
    
    for ( uint i = 0; i < dltVersions.size() ; i++ ) {
        DLT dlt(16,4,0,false); // dummy init
        getDlt(dlt, dltVersions[i], localDomain);
        dltMgr.add(dlt);
    }
    return true;
}

bool ConfigDB::storeDlts(DLTManager& dltMgr, int localDomain) {
    std::vector<fds_uint64_t> vecVersions;
    vecVersions = dltMgr.getDltVersions();
    
    for (uint i = 0; i < vecVersions.size() ; i++) {
        const DLT* dlt = dltMgr.getDLT(vecVersions.size());
        if (!storeDlt(*dlt, "",  localDomain)) {
            LOGERROR << "unable to store dlt : [" << dlt->getVersion() << "] for domain ["<< localDomain <<"]";
        }
    }
    return true;
}

// nodes
bool ConfigDB::addNode(const NodeInvData& node) {
    // add the volume to the volume list for the domain
    int domainId = 0 ; // TODO(prem)
    Reply reply = r.sendCommand("sadd %d:cluster:nodes %ld", domainId, node.nd_uuid);

    if (!reply.wasModified()) {
        LOGWARN << "node [" << node.nd_uuid << "] already exists for domain [" << domainId << "]";
    }

    reply = r.sendCommand("hmset node:%ld uuid %ld"
                          " capacity %ld"
                          " ipnum %ld"
                          " ip %s"
                          " data.port %d"
                          " ctrl.port %d"
                          " migration.port %d"
                          " disk.type %d"
                          " name %s"
                          " type %d"
                          " state %d"
                          " dlt.version %ld"
                          " disk.capacity %ld"
                          " disk.iops.max %d"
                          " disk.iops.min %d"
                          " disk.latency.max %d"
                          " disk.latency.min %d"
                          " ssd.iops.max %d"
                          " ssd.iops.min %d"
                          " ssd.capacity %d"
                          " ssd.latency.max %d"
                          " ssd.latency.min %d",
                          node.nd_uuid,node.nd_uuid,
                          node.nd_gbyte_cap,
                          node.nd_ip_addr,
                          node.nd_ip_str.c_str(),
                          node.nd_data_port,
                          node.nd_ctrl_port,
                          node.nd_migration_port,
                          node.nd_disk_type,
                          node.nd_node_name.c_str(),
                          node.nd_node_type,
                          node.nd_node_state,
                          node.nd_dlt_version,
                          node.nd_capability.disk_capacity,
                          node.nd_capability.disk_iops_max,
                          node.nd_capability.disk_iops_min,
                          node.nd_capability.disk_latency_max,
                          node.nd_capability.disk_latency_min,
                          node.nd_capability.ssd_iops_max,
                          node.nd_capability.ssd_iops_min,
                          node.nd_capability.ssd_capacity,
                          node.nd_capability.ssd_latency_max,
                          node.nd_capability.ssd_latency_min);
    if (reply.isOk()) return true;
    LOGWARN << "msg: " << reply.getString();
    return false;
}

bool ConfigDB::updateNode(const NodeInvData& node) { 
    return addNode(node);
}

bool ConfigDB::removeNode(const NodeUuid& uuid) {
    int domainId = 0 ; // TODO(prem)
    Reply reply = r.sendCommand("srem %d:cluster:nodes %ld", domainId, uuid);

    if (!reply.wasModified()) {
        LOGWARN << "node [" << uuid << "] does not exist for domain [" << domainId << "]";
    }

    // Now remove the node data
    reply = r.sendCommand("del node:%ld", uuid);
    return reply.isOk();
}

bool ConfigDB::getNode(const NodeUuid& uuid, NodeInvData& node) {
    try {
        Reply reply = r.sendCommand("hgetall node:%ld", uuid);
        StringList strings;
        reply.toVector(strings);

        if (strings.empty()) {
            LOGWARN << "unable to find node [" << uuid <<"]";
            return false;
        }

        for ( uint i = 0; i < strings.size() ; i+=2 ) {
            std::string& key = strings[i];
            std::string& value = strings[i+1];
            node_capability_t& cap = node.nd_capability;
            if (key == "uuid") { node.nd_uuid = strtoull(value.c_str(),NULL,10); }
            else if (key == "capacity") { node.nd_gbyte_cap  = atol(value.c_str()); }
            else if (key == "ipnum") { node.nd_ip_addr = atoi(value.c_str()); }
            else if (key == "ip") { node.nd_ip_str = value; }
            else if (key == "data.port") { node.nd_data_port = atoi(value.c_str()); }
            else if (key == "ctrl.port") { node.nd_ctrl_port = atoi(value.c_str()); }
            else if (key == "migration.port") { node.nd_migration_port  = atoi(value.c_str()); }
            else if (key == "disk.type") { node.nd_disk_type = atoi(value.c_str()); }
            else if (key == "name") { node.nd_node_name = value; }
            else if (key == "type") { node.nd_node_type = (fpi::FDSP_MgrIdType) atoi(value.c_str()); }
            else if (key == "state") { node.nd_node_state  = (fpi::FDSP_NodeState) atoi(value.c_str()); }
            else if (key == "dlt.version") { node.nd_dlt_version  = atol(value.c_str()); }
            else if (key == "disk.capacity") { cap.disk_capacity = atol(value.c_str()); }
            else if (key == "disk.iops.max") { cap.disk_iops_max  = atoi(value.c_str()); }
            else if (key == "disk.iops.min") { cap.disk_iops_min = atoi(value.c_str()); }
            else if (key == "disk.latency.max") { cap.disk_latency_max  = atoi(value.c_str()); }
            else if (key == "disk.latency.min") { cap.disk_latency_min = atoi(value.c_str()); }
            else if (key == "ssd.iops.max") { cap.ssd_iops_max = atoi(value.c_str()); }
            else if (key == "ssd.iops.min") { cap.ssd_iops_min = atoi(value.c_str()); }
            else if (key == "ssd.capacity") { cap.ssd_capacity = atoi(value.c_str()); }
            else if (key == "ssd.latency.max") { cap.ssd_latency_max = atoi(value.c_str()); }
            else if (key == "ssd.latency.min") { cap.ssd_latency_min = atoi(value.c_str()); }
            else {
                LOGWARN << "unknown key [" << key << "] for node [" << uuid << "]";
            }        
        }
        return true;
    } catch (const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::nodeExists(const NodeUuid& uuid) { 
    Reply reply = r.sendCommand("exists node:%ld", uuid);
    return reply.isOk();
}

bool ConfigDB::getAllNodes(std::vector<NodeInvData>& nodes, int localDomain){ return false; }

std::string ConfigDB::getNodeName(const NodeUuid& uuid) {
    try {
        Reply reply = r.sendCommand("hget node:%ld name", uuid);
        return reply.getString();
    } catch (const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return "";    
}

uint ConfigDB::getNodeNameCounter() {
    try{
        Reply reply = r.sendCommand("incr node:name.counter");
        return reply.getLong();
    } catch (const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return 0;
}
            
// volume policies
bool ConfigDB::addPolicy(const PolicyInfo& policyInfo, int localDomain){ return false; }
bool ConfigDB::updatePolicy(const PolicyInfo& policyInfo, int localDomain){ return false; }
bool ConfigDB::deletePolicy(const PolicyInfo& policyInfo, int localDomain){ return false; }
bool ConfigDB::getPolicies(std::vector<PolicyInfo>& policies, int localDomain){ return false; }

}  // namespace kvstore
}  // namespace fds
