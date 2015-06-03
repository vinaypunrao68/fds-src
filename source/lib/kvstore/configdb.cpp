/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <map>
#include <string>
#include <vector>

#include <kvstore/configdb.h>
#include <util/Log.h>
#include <util/stringutils.h>
#include <stdlib.h>
#include <fdsp_utils.h>
#include <util/timeutils.h>
#include <util/properties.h>
#include <net/net_utils.h>
#include <ratio>

#include "platform/platform_shm_typedefs.h"
#include "platform/node_data.h"

auto format = fds::util::strformat;
auto lower  = fds::util::strlower;

namespace fds { namespace kvstore {
using redis::Reply;
using redis::RedisException;

#define FDSP_SERIALIZE(type, varname) \
    boost::shared_ptr<std::string> varname; \
    fds::serializeFdspMsg(type, varname);

ConfigDB::ConfigDB(const std::string& host,
                   uint port,
                   uint poolsize) : KVStore(host, port, poolsize) {
    LOGNORMAL << "instantiating configdb";
}

ConfigDB::~ConfigDB() {
    LOGNORMAL << "destroying configdb";
}

fds_uint64_t ConfigDB::getLastModTimeStamp() {
    try {
        return r.get("config.lastmod").getLong();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }

    return 0;
}

fds_uint64_t ConfigDB::getConfigVersion() {
    try {
        return r.get("config.version").getLong();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }

    return 0;
}

void ConfigDB::setModified() {
    try {
        r.sendCommand("set config.lastmod %ld", fds::util::getTimeStampMillis());
        r.incr("config.version");
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
}

// domains
std::string ConfigDB::getGlobalDomain() {
    try {
        return r.get("global.domain").getString();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
        return "";
    }
}

bool ConfigDB::setGlobalDomain(ConstString globalDomain) {
    TRACKMOD();
    try {
        return r.set("global.domain", globalDomain);
    } catch(const RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}

/**
 * Create a new Local Domain provided it does not already exist. (Local Domain names
 * are treated as case-insenative.)
 *
 * @param identifier: std::string - Name to use for the new Local Domain.
 * @param site: std::string - Name of the location of the new Local Domain.
 *
 * @return int64_t - ID generated for the new Local Domain.
 */
int64_t ConfigDB::createLocalDomain(const std::string& identifier, const std::string& site) {
    TRACKMOD();
    try {
        // Check if the Local Domain already exists.
        std::string idLower = lower(identifier);

        int64_t id = getIdOfLocalDomain(identifier);
        if (id > 0) {
            // The Local Domain already exists.
            LOGWARN << "Trying to add an existing Local Domain : " << id << ": " << identifier;
            NOMOD();
            return id;
        }

        fds::apis::LocalDomain localDomain;

        // Get new id
        auto reply = r.sendCommand("incr local.domain:nextid");

        // Build Local Domain object to store.
        localDomain.id = reply.getLong();
        localDomain.name = identifier;
        localDomain.site = site;

        r.sendCommand("sadd local.domain:list %b", idLower.data(), idLower.length());

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(localDomain, serialized);

        r.sendCommand("hset local.domains %ld %b", localDomain.id, serialized->data(), serialized->length()); //NOLINT
        return localDomain.id;
    } catch(const RedisException& e) {
        LOGCRITICAL << "Error with redis " << e.what();
    }

    return 0;
}

/**
 * Generate a vector of Local Domains currently defined.
 *
 * @param localDomains: std::vector<fds::apis::LocalDomain> - Vector of returned Local Domain names.
 *
 * @return bool - True if the vector is successfully produced. False otherwise.
 */
bool ConfigDB::listLocalDomains(std::vector<fds::apis::LocalDomain>& localDomains) {
    fds::apis::LocalDomain localDomain;

    try {
        Reply reply = r.sendCommand("hvals local.domains");
        StringList strings;
        reply.toVector(strings);

        for (const auto& value : strings) {
            fds::deserializeFdspMsg(value, localDomain);
            LOGDEBUG << "Read Local Domain " << localDomain.name;
            localDomains.push_back(localDomain);
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "Error with redis " << e.what();
        return false;
    }
    return true;
}

/**
 * Return the Domain ID for the named Local Domain.
 *
 * @param identifier: std::string - Name of the Local Domain whose ID is to be returned.
 *
 * @return int64_t - ID of the named Local Domain. 0 otherwise.
 */
int64_t ConfigDB::getIdOfLocalDomain(const std::string& identifier) {
    int64_t id = 0;

    try {
        // Check if the Local Domain exists.
        std::string idLower = lower(identifier);

        Reply reply = r.sendCommand("sismember local.domain:list %b", idLower.data(), idLower.length());
        if (reply.getLong() == 1) {
            // The Local Domain exists.
            std::vector<fds::apis::LocalDomain> localDomains;
            if (listLocalDomains(localDomains)) {
                for (const auto &localDomain : localDomains) {
                    if (localDomain.name == identifier) {
                        id = localDomain.id;
                        break;
                    }
                }

                if (id == 0) {
                    LOGCRITICAL << "ConfigDB inconsistency. Local Domain info missing : " << identifier;
                }
            } else {
                LOGCRITICAL << "Unable to obtain Local Domain list.";
                return id;
            }
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "Error with redis " << e.what();
        throw e;
    }

    return id;
}


// volumes
fds_uint64_t ConfigDB::getNewVolumeId() {
    try {
        fds_uint64_t volId;
        for (;;) {
            volId = r.incr("volumes:nextid");
            if (!volumeExists(volId)) return volId;
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return invalid_vol_id;
}
bool ConfigDB::addVolume(const VolumeDesc& vol) {
    TRACKMOD();
    // add the volume to the volume list for the domain
    try {
        Reply reply = r.sendCommand("sadd %d:volumes %ld", vol.localDomainId, vol.volUUID);
        if (!reply.wasModified()) {
            LOGWARN << "volume [" << vol.volUUID
                    << "] already exists for domain ["
                    << vol.localDomainId << "]";
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
                              " objsize.max %d"
                              " write.quorum %d"
                              " read.quorum %d"
                              " conistency.protocol %d"
                              " volume.policy.id %d"
                              " archive.policy.id %d"
                              " placement.policy.id %d"
                              " app.workload %d"
                              " media.policy %d"
                              " backup.vol.id %ld"
                              " iops.min %d"
                              " iops.max %d"
                              " relative.priority %d"
                              " fsnapshot %d"
                              " parentvolumeid %ld"
                              " state %d"
                              " create.time %ld",
                              vol.volUUID, vol.volUUID,
                              vol.name.c_str(),
                              vol.tennantId,
                              vol.localDomainId,
                              vol.globDomainId,
                              vol.volType,
                              vol.capacity,
                              vol.maxQuota,
                              vol.replicaCnt,
                              vol.maxObjSizeInBytes,
                              vol.writeQuorum,
                              vol.readQuorum,
                              vol.consisProtocol,
                              vol.volPolicyId,
                              vol.archivePolicyId,
                              vol.placementPolicy,
                              vol.appWorkload,
                              vol.mediaPolicy,
                              vol.backupVolume,
                              vol.iops_assured,
                              vol.iops_throttle,
                              vol.relativePrio,
                              vol.fSnapshot,
                              vol.srcVolumeId,
                              vol.getState(),
                              vol.createTime);
        if (reply.isOk()) return true;
        LOGWARN << "msg: " << reply.getString();
    } catch(RedisException& e) {
        LOGERROR << e.what();
        TRACKMOD();
    }
    return false;
}

bool ConfigDB::setVolumeState(fds_volid_t volumeId, fpi::ResourceState state) {
    TRACKMOD();
    try {
        LOGNORMAL << "updating volume id " << volumeId << " to state " << state;
        return r.sendCommand("hset vol:%ld state %d", volumeId, static_cast<int>(state)).isOk();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::updateVolume(const VolumeDesc& volumeDesc) {
    TRACKMOD();
    try {
        return addVolume(volumeDesc);
    } catch(const RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::deleteVolume(fds_volid_t volumeId, int localDomainId) {
    TRACKMOD();
    try {
        Reply reply = r.sendCommand("srem %d:volumes %ld", localDomainId, volumeId);
        if (!reply.wasModified()) {
            LOGWARN << "volume [" << volumeId
                    << "] does NOT exist for domain ["
                    << localDomainId << "]";
        }

        // del the volume data
        reply = r.sendCommand("del vol:%ld", volumeId);
        return reply.isOk();
    } catch(RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::volumeExists(fds_volid_t volumeId) {
    try {
        Reply reply = r.sendCommand("exists vol:%ld", volumeId);
        return reply.getLong()== 1;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::volumeExists(ConstString volumeName, int localDomain)
{ 
    std::vector<VolumeDesc> volumes;
    const bool bRetCode = getVolumes( volumes, localDomain );
    if ( bRetCode )
    {
        for ( const auto volume : volumes )
        {
            if ( volume.getName().compare( volumeName ) == 0 )
            {
                return true;
            }
        }
    }
    
    return false; 
}

bool ConfigDB::getVolumeIds(std::vector<fds_volid_t>& volIds, int localDomain) {
    std::vector<long long> volumeIds; //NOLINT

    try {
        Reply reply = r.sendCommand("smembers %d:volumes", localDomain);
        reply.toVector(volumeIds);

        if (volumeIds.empty()) {
            LOGWARN << "no volumes found for domain [" << localDomain << "]";
            return false;
        }

        for (uint i = 0; i < volumeIds.size(); i++) {
            volIds.push_back(volumeIds[i]);
        }
        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::getVolumes(std::vector<VolumeDesc>& volumes, int localDomain) {
    std::vector<long long> volumeIds; //NOLINT

    try {
        Reply reply = r.sendCommand("smembers %d:volumes", localDomain);
        reply.toVector(volumeIds);

        if (volumeIds.empty()) {
            LOGWARN << "no volumes found for domain [" << localDomain << "]";
            return false;
        }

        for (uint i = 0; i < volumeIds.size(); i++) {
            VolumeDesc volume("" , 1);  // dummy init
            getVolume(volumeIds[i], volume);
            volumes.push_back(volume);
        }
        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::getVolume(fds_volid_t volumeId, VolumeDesc& vol) {
    try {
        Reply reply = r.sendCommand("hgetall vol:%ld", volumeId);
        StringList strings;
        reply.toVector(strings);

        if (strings.empty()) {
            LOGWARN << "unable to find volume [" << volumeId <<"]";
            return false;
        }

        for (uint i = 0; i < strings.size(); i+= 2) {
            std::string& key = strings[i];
            std::string& value = strings[i+1];

            if (key == "uuid") {vol.volUUID = strtoull(value.c_str(), NULL, 10);}
            else if (key == "name") { vol.name = value; }
            else if (key == "tennant.id") { vol.tennantId = atoi(value.c_str()); }
            else if (key == "local.domain.id") {vol.localDomainId = atoi(value.c_str());}
            else if (key == "global.domain.id") { vol.globDomainId = atoi(value.c_str());}
            else if (key == "type") { vol.volType = (fpi::FDSP_VolType)atoi(value.c_str()); }
            else if (key == "capacity") { vol.capacity = strtod (value.c_str(), NULL);}
            else if (key == "quota.max") { vol.maxQuota = strtod (value.c_str(), NULL);}
            else if (key == "replica.count") {vol.replicaCnt = atoi(value.c_str());}
            else if (key == "objsize.max") {vol.maxObjSizeInBytes = atoi(value.c_str());}
            else if (key == "write.quorum") {vol.writeQuorum = atoi(value.c_str());}
            else if (key == "read.quorum") {vol.readQuorum = atoi(value.c_str());}
            else if (key == "conistency.protocol") {vol.consisProtocol = (fpi::FDSP_ConsisProtoType)atoi(value.c_str());} //NOLINT
            else if (key == "volume.policy.id") {vol.volPolicyId = atoi(value.c_str());}
            else if (key == "archive.policy.id") {vol.archivePolicyId = atoi(value.c_str());}
            else if (key == "placement.policy.id") {vol.placementPolicy = atoi(value.c_str());}
            else if (key == "app.workload") {vol.appWorkload = (fpi::FDSP_AppWorkload)atoi(value.c_str());} //NOLINT
            else if (key == "media.policy") {vol.mediaPolicy = (fpi::FDSP_MediaPolicy)atoi(value.c_str());} //NOLINT
            else if (key == "backup.vol.id") {vol.backupVolume = atol(value.c_str());}
            else if (key == "iops.min") {vol.iops_assured = strtod (value.c_str(), NULL);}
            else if (key == "iops.max") {vol.iops_throttle = strtod (value.c_str(), NULL);}
            else if (key == "relative.priority") {vol.relativePrio = atoi(value.c_str());}
            else if (key == "fsnapshot") {vol.fSnapshot = atoi(value.c_str());}
            else if (key == "state") {vol.setState((fpi::ResourceState) atoi(value.c_str()));}
            else if (key == "parentvolumeid") {vol.srcVolumeId = strtoull(value.c_str(), NULL, 10);} //NOLINT
            else if (key == "create.time") {vol.createTime = strtoull(value.c_str(), NULL, 10);} //NOLINT
            else { //NOLINT
                LOGWARN << "unknown key for volume [" << volumeId <<"] - " << key;
                fds_assert(!"unknown key");
            }
        }
        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
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
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return 0;
}

bool ConfigDB::setDltType(fds_uint64_t version, const std::string type, int localDomain) {
    try {
        Reply reply = r.sendCommand("set %d:dlt:%s %ld", localDomain, type.c_str(), version);
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::storeDlt(const DLT& dlt, const std::string type, int localDomain) {
    try {
        // fds_uint64_t version = dlt.getVersion();
        Reply reply = r.sendCommand("sadd %d:dlts %ld", localDomain, dlt.getVersion());
        if (!reply.wasModified()) {
            LOGWARN << "dlt [" << dlt.getVersion()
                    << "] is already in for domain [" << localDomain << "]";
        }

        std::string serializedData, hexCoded;
        const_cast<DLT&>(dlt).getSerialized(serializedData);
        r.encodeHex(serializedData, hexCoded);

        reply = r.sendCommand("set %d:dlt:%ld %s", localDomain, dlt.getVersion(), hexCoded.c_str());
        bool fSuccess = reply.isOk();

        if (fSuccess && !type.empty()) {
            reply = r.sendCommand("set %d:dlt:%s %ld", localDomain, type.c_str(), dlt.getVersion());
            if (!reply.isOk()) {
                LOGWARN << "error setting type " << dlt.getVersion() << ":" << type;
            }
        }
        return fSuccess;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::getDlt(DLT& dlt, fds_uint64_t version, int localDomain) {
    try {
        Reply reply = r.sendCommand("get %d:dlt:%ld", localDomain, version);
        std::string serializedData, hexCoded(reply.getString());
        if (hexCoded.length() < 10) {
            LOGERROR << "very less data for dlt : ["
                     << version << "] : size = " << hexCoded.length();
            return false;
        }
        LOGDEBUG << "dlt : [" << version << "] : size = " << hexCoded.length();
        r.decodeHex(hexCoded, serializedData);
        dlt.loadSerialized(serializedData);
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::loadDlts(DLTManager& dltMgr, int localDomain) {
    try {
        Reply reply = r.sendCommand("smembers %d:dlts", localDomain);
        std::vector<long long> dltVersions; //NOLINT
        reply.toVector(dltVersions);

        if (dltVersions.empty()) return false;

        for (uint i = 0; i < dltVersions.size(); i++) {
            DLT dlt(16, 4, 0, false);  // dummy init
            getDlt(dlt, dltVersions[i], localDomain);
            dltMgr.add(dlt);
        }
        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::storeDlts(DLTManager& dltMgr, int localDomain) {
    try {
        std::vector<fds_uint64_t> vecVersions;
        vecVersions = dltMgr.getDltVersions();

        for (uint i = 0; i < vecVersions.size(); i++) {
            const DLT* dlt = dltMgr.getDLT(vecVersions.size());
            if (!storeDlt(*dlt, "",  localDomain)) {
                LOGERROR << "unable to store dlt : ["
                         << dlt->getVersion() << "] for domain ["<< localDomain <<"]";
            }
        }
        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

// dmt
fds_uint64_t ConfigDB::getDmtVersionForType(const std::string type, int localDomain) {
    try {
        Reply reply = r.sendCommand("get %d:dmt:%s", localDomain, type.c_str());
        if (!reply.isNil()) {
            return reply.getLong();
        } else {
            LOGNORMAL << "no dmt set for type:" << type;
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return 0;
}

bool ConfigDB::setDmtType(fds_uint64_t version, const std::string type, int localDomain) {
    try {
        Reply reply = r.sendCommand("set %d:dmt:%s %ld", localDomain, type.c_str(), version);
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::storeDmt(const DMT& dmt, const std::string type, int localDomain) {
    try {
        // fds_uint64_t version = dmt.getVersion();
        Reply reply = r.sendCommand("sadd %d:dmts %ld", localDomain, dmt.getVersion());
        if (!reply.wasModified()) {
            LOGWARN << "dmt [" << dmt.getVersion()
                    << "] is already in for domain ["
                    << localDomain << "]";
        }

        std::string serializedData, hexCoded;
        const_cast<DMT&>(dmt).getSerialized(serializedData);
        r.encodeHex(serializedData, hexCoded);

        reply = r.sendCommand("set %d:dmt:%ld %s", localDomain, dmt.getVersion(), hexCoded.c_str());
        bool fSuccess = reply.isOk();

        if (fSuccess && !type.empty()) {
            reply = r.sendCommand("set %d:dmt:%s %ld", localDomain, type.c_str(), dmt.getVersion());
            if (!reply.isOk()) {
                LOGWARN << "error setting type " << dmt.getVersion() << ":" << type;
            }
        }
        return fSuccess;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::getDmt(DMT& dmt, fds_uint64_t version, int localDomain) {
    try {
        Reply reply = r.sendCommand("get %d:dmt:%ld", localDomain, version);
        std::string serializedData, hexCoded(reply.getString());
        if (hexCoded.length() < 10) {
            LOGERROR << "very less data for dmt : ["
                     << version << "] : size = " << hexCoded.length();
            return false;
        }
        LOGDEBUG << "dmt : [" << version << "] : size = " << hexCoded.length();
        r.decodeHex(hexCoded, serializedData);
        dmt.loadSerialized(serializedData);
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

// nodes
bool ConfigDB::addNode(const NodeInfoType& node) {
    TRACKMOD();
    // add the volume to the volume list for the domain
    int domainId = 0;  // TODO(prem)
    try {
        bool ret;

        FDSP_SERIALIZE(node, serialized);
        ret = r.sadd(format("%d:cluster:nodes", domainId), node.node_uuid.uuid);
        ret = r.set(format("node:%ld", node.node_uuid.uuid), *serialized);

        return ret;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::updateNode(const NodeInfoType& node) {
    try {
        return addNode(node);
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::removeNode(const NodeUuid& uuid) {
    TRACKMOD();
    try {
        int domainId = 0;  // TODO(prem)
        Reply reply = r.sendCommand("srem %d:cluster:nodes %ld", domainId, uuid);

        if (!reply.wasModified()) {
            LOGWARN << "node [" << uuid << "] does not exist for domain [" << domainId << "]";
        }

        // Now remove the node data
        reply = r.sendCommand("del node:%ld", uuid);
        return reply.isOk();
    } catch(RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::getNode(const NodeUuid& uuid, NodeInfoType& node) {
    try {
        Reply reply = r.get(format("node:%ld", uuid.uuid_get_val()));
        if (reply.isOk()) {
            fds::deserializeFdspMsg(reply.getString(), node);
            return true;
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::nodeExists(const NodeUuid& uuid) {
    try {
        Reply reply = r.sendCommand("exists node:%ld", uuid);
        return reply.isOk();
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::getNodeIds(std::unordered_set<NodeUuid, UuidHash>& nodes, int localDomain) {
    std::vector<long long> nodeIds; //NOLINT

    try {
        Reply reply = r.sendCommand("smembers %d:cluster:nodes", localDomain);
        reply.toVector(nodeIds);

        if (nodeIds.empty()) {
            LOGWARN << "no nodes found for domain [" << localDomain << "]";
            return false;
        }

        for (uint i = 0; i < nodeIds.size(); i++) {
            LOGDEBUG << "ConfigDB::getNodeIds node "
                     << std::hex << nodeIds[i] << std::dec;
            nodes.insert(NodeUuid(nodeIds[i]));
        }

        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::getAllNodes(std::vector<NodeInfoType>& nodes, int localDomain) {
    std::vector<long long> nodeIds; //NOLINT

    try {
        Reply reply = r.sendCommand("smembers %d:cluster:nodes", localDomain);
        reply.toVector(nodeIds);

        if (nodeIds.empty()) {
            LOGWARN << "no nodes found for domain [" << localDomain << "]";
            return false;
        }
        
        NodeInfoType node;
        for (uint i = 0; i < nodeIds.size(); i++) {
            getNode(nodeIds[i], node);
            nodes.push_back(node);
        }
        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

std::string ConfigDB::getNodeName(const NodeUuid& uuid) {
    try {
        NodeInfoType node;
        if (getNode(uuid, node)) {
            return node.node_name;
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return "";
}

bool ConfigDB::getNodeServices(const NodeUuid& uuid, NodeServices& services) {
    try{
        Reply reply = r.sendCommand("get %ld:services", uuid);
        if (reply.isNil()) return false;
        services.loadSerialized(reply.getString());
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::setNodeServices(const NodeUuid& uuid, const NodeServices& services) {
    try{
        std::string serialized;
        services.getSerialized(serialized);
        Reply reply = r.sendCommand("set %ld:services %b",
                                    uuid, serialized.data(), serialized.length());
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

uint ConfigDB::getNodeNameCounter() {
    try{
        Reply reply = r.sendCommand("incr node:name.counter");
        return reply.getLong();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return 0;
}

// Volume QoS policy

/**
 * Create a new Local Domain provided it does not already exist. (Local Domain names
 * are treated as case-insenative.)
 *
 * @param identifier: std::string - Name to use for the new QoS Policy.
 * @param minIops: fds_uint64_t - Minimum IOPs requested for the new QoS Policy.
 * @param maxIops: fds_uint64_t - Maximum IOPs requested for the new QoS Policy.
 * @param relPrio: fds_uint32_t - Relative priority requested for the new QoS Policy.
 *
 * @return fds_uint32_t - ID generated for the new QoS Policy.
 */
fds_uint32_t ConfigDB::createQoSPolicy(const std::string& identifier,
                                       const fds_uint64_t minIops, const fds_uint64_t maxIops,
                                       const fds_uint32_t relPrio) {
    fds_uint32_t id = 0;

    TRACKMOD();

    try {
        // Check if the QoS Policy already exists.
        std::string idLower = lower(identifier);

        id = getIdOfQoSPolicy(identifier);
        if (id > 0) {
            // The QoS Policy already exists.
            LOGWARN << "Trying to add an existing QoS Policy : " << id << ": " << identifier;
            NOMOD();
            return id;
        }

        // Get new id
#ifndef QOS_POLICY_MANAGEMENT_CORRECTED
        /**
         * Note that further up the stack, VolPolicyMgr::createPolicy() has acquired
         * a lock. So we rely upon that to serialize our access to key qos.policy:nextid.
         */
        auto exists = r.sendCommand("exists qos.policy:nextid");
        if (!exists.isOk()) {
            /**
             * If our QoS Policy ID generator does not exist, we
             * assume that we may have policies created already whose ID's
             * were not generated with this mechanism. From here on out,
             * they will be. But we need to make sure that we don't step
             * one that already exists.
             */
            std::vector<FDS_VolumePolicy> qosPolicies;

            if (getPolicies(qosPolicies)) {
                for (const auto& qosPolicy : qosPolicies) {
                    if (qosPolicy.volPolicyId > id) {
                        id = qosPolicy.volPolicyId;
                    }
                }
            }

            r.sendCommand("set qos.policy:nextid %s", std::to_string(id).c_str());
            LOGDEBUG << "QoS Policy ID generator initialized to " << std::to_string(id);
        }
#endif  /* QOS_POLICY_MANAGEMENT_CORRECTED */

        auto reply = r.sendCommand("incr qos.policy:nextid");

        // Build Local Domain object to store.
        FDS_VolumePolicy qosPolicy;
        id = static_cast<fds_uint32_t>(reply.getLong());
        qosPolicy.volPolicyId = id;
        qosPolicy.volPolicyName = identifier;
        qosPolicy.iops_assured = minIops;
        qosPolicy.iops_throttle = maxIops;
        qosPolicy.relativePrio = relPrio;

#ifdef QOS_POLICY_MANAGEMENT_CORRECTED
        /**
         * This is what we'd like to do.
         */
        r.sendCommand("sadd qos.policy:list %b", idLower.data(), idLower.length());

        // serialize
        std::string serialized;
        qosPolicy.getSerialized(serialized);

        r.sendCommand("hset qos.policies %ld %b", qosPolicy.volPolicyId, serialized.data(), serialized.length());
        return qosPolicy.volPolicyId;
    } catch(const RedisException& e) {
        LOGCRITICAL << "Error with redis " << e.what();
    }
#else
        if (!addPolicy(qosPolicy)) {
            // Something not right.
            qosPolicy.volPolicyId = 0;
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "Error with redis " << e.what();
    }
#endif  /* QOS_POLICY_MANAGEMENT_CORRECTED */

    return id;
}

#ifdef QOS_POLICY_MANAGEMENT_CORRECTED
/**
* Generate a vector of QoS Policies currently defined.
*
* @param qosPolicies: std::vector<fds::apis::LocalDomain> - Vector of returned Local Domain names.
*
* @return bool - True if the vector is successfully produced. False otherwise.
*/
bool ConfigDB::listQoSPolicies(std::vector<FDSP_PolicyInfoType>& qosPolicies) {
    FDS_VolumePolicy qosPolicy;

    try {
        Reply reply = r.sendCommand("hvals qos.policies");
        StringList strings;
        reply.toVector(strings);
    } catch(const RedisException& e) {
        LOGCRITICAL << "Error with redis " << e.what();
        return false;
    }

    for (const auto& value : strings) {
        qosPolicy.loadSerialized(value);
        LOGDEBUG << "Read QoS Policy " << qosPolicy.volPolicyName;
        // Need to construct FDSP_PolicyInfoType from FDS_VolumePolicy.
        qosPolicies.push_back(FDSP_PolicyInfoType);
    }

    return true;
}
#endif  /*QOS_POLICY_MANAGEMENT_CORRECTED*/

/**
* Return the Qos Policy ID for the named Qos Policy.
*
* @param identifier: std::string - Name of the QoS Policy whose ID is to be returned.
*
* @return fds_uint32_t - ID of the named QoS Policy. 0 otherwise.
*/
fds_uint32_t ConfigDB::getIdOfQoSPolicy(const std::string& identifier) {
    fds_uint32_t id = 0;

#ifdef QOS_POLICY_MANAGEMENT_CORRECTED
    /**
     * This is what we would like.
     */
    try {
        // Check if the QoS Policy exists.
        std::string idLower = lower(identifier);

        Reply reply = r.sendCommand("sismember qos.policy:list %b", idLower.data(), idLower.length());
        if (reply.getLong() == 1) {
            // The QoS Policy exists.
            std::vector<FDS_VolumePolicy> qosPolicies;
            if (listQoSPolicies(qosPolicies)) {
                for (const auto &qosPolicy : qosPolicies) {
                    if (qosPolicy.volPolicyName == identifier) {
                        id = qosPolicy.volPolicyId;
                        break;
                    }
                }

                if (id == 0) {
                    LOGCRITICAL << "ConfigDB inconsistency. QoS Policy info missing : " << identifier;
                }
            } else {
                LOGCRITICAL << "Unable to obtain QoS Policy list.";
                return id;
            }
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "Error with redis " << e.what();
        throw e;
    }
#else
    /**
     * But this is what we have time for ... work with current data model.
     */
    std::vector<FDS_VolumePolicy> qosPolicies;

    if (getPolicies(qosPolicies)) {
        for (const auto& qosPolicy : qosPolicies) {
            if (qosPolicy.volPolicyName == identifier) {
                id = qosPolicy.volPolicyId;
                break;
            }
        }

        if (id == 0) {
            LOGDEBUG << "QoS Policy not defined yet : " << identifier;
        }
    } else {
        LOGDEBUG << "No QoS Policies yet.";
    }
#endif  /*QOS_POLICY_MANAGEMENT_CORRECTED*/

    return id;
}


bool ConfigDB::getPolicy(fds_uint32_t volPolicyId, FDS_VolumePolicy& policy, int localDomain) { //NOLINT
    try{
        Reply reply = r.sendCommand("get %d:volpolicy:%ld", localDomain, volPolicyId);
        if (reply.isNil()) return false;
        policy.loadSerialized(reply.getString());
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::addPolicy(const FDS_VolumePolicy& policy, int localDomain) {
    TRACKMOD();
    try{
        std::string serialized;

        Reply reply = r.sendCommand("sadd %d:volpolicies %ld", localDomain, policy.volPolicyId);
        if (!reply.wasModified()) {
            LOGWARN << "unable to add policy [" << policy.volPolicyId << " to domain [" << localDomain <<"]"; //NOLINT
        }
        policy.getSerialized(serialized);
        reply = r.sendCommand("set %d:volpolicy:%ld %b", localDomain, policy.volPolicyId, serialized.data(), serialized.length()); //NOLINT
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::updatePolicy(const FDS_VolumePolicy& policy, int localDomain) {
    TRACKMOD();
    try {
        return addPolicy(policy, localDomain);
    } catch(const RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::deletePolicy(fds_uint32_t volPolicyId, int localDomain) {
    TRACKMOD();
    try{
        Reply reply = r.sendCommand("del %d:volpolicy:%ld", localDomain, volPolicyId);
        return reply.wasModified();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::getPolicies(std::vector<FDS_VolumePolicy>& policies, int localDomain) {
    try {
        std::vector<long long> volumePolicyIds; //NOLINT

        Reply reply = r.sendCommand("smembers %d:volpolicies", localDomain);
        reply.toVector(volumePolicyIds);

        if (volumePolicyIds.empty()) {
            LOGWARN << "no volPolcies found for domain [" << localDomain << "]";
            return false;
        }

        FDS_VolumePolicy policy;
        for (uint i = 0; i < volumePolicyIds.size(); i++) {
            if (getPolicy(volumePolicyIds[i], policy, localDomain)) {
                policies.push_back(policy);
            }
        }
        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

// stat streaming registrations
int32_t ConfigDB::getNewStreamRegistrationId() {
    try {
        Reply reply = r.sendCommand("incr streamreg:id");
        return reply.getLong();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return -1;
}

bool ConfigDB::addStreamRegistration(apis::StreamingRegistrationMsg& streamReg) {
    TRACKMOD();
    try {
        boost::shared_ptr<std::string> serialized;

        Reply reply = r.sendCommand("sadd streamregs %d", streamReg.id);
        if (!reply.wasModified()) {
            LOGWARN << "unable to add streamreg [" << streamReg.id;
        }

        fds::serializeFdspMsg(streamReg, serialized);

        reply = r.sendCommand("set streamreg:%d %b", streamReg.id,
                              serialized->data(), serialized->length());
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::getStreamRegistration(int regId, apis::StreamingRegistrationMsg& streamReg) {
    try {
        Reply reply = r.sendCommand("get streamreg:%d", regId);
        if (reply.isNil()) return false;
        fds::deserializeFdspMsg(reply.getString(), streamReg);
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::removeStreamRegistration(int regId) {
    TRACKMOD();
    try {
        Reply reply = r.sendCommand("srem streamregs %d", regId);
        if (!reply.wasModified()) {
            LOGWARN << "unable to remove streamreg [" << regId << "] from set"
                    << " mebbe it does not exist";
        }

        reply = r.sendCommand("del streamreg:%d", regId);
        if (reply.getLong() == 0) {
            LOGWARN << "no items deleted";
        }
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::getStreamRegistrations(std::vector<apis::StreamingRegistrationMsg>& vecReg) {
    try {
        std::vector<long long> regIds; //NOLINT

        Reply reply = r.sendCommand("smembers streamregs");
        reply.toVector(regIds);

        if (regIds.empty()) {
            LOGWARN << "no stream registrations found ";
            return false;
        }

        apis::StreamingRegistrationMsg regMsg;
        for (uint i = 0; i < regIds.size(); i++) {
            if (getStreamRegistration(regIds[i], regMsg)) {
                vecReg.push_back(regMsg);
            } else {
                LOGWARN << "unable to get reg: " << regIds[i];
            }
        }
        return true;
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

int64_t ConfigDB::createTenant(const std::string& identifier) {
    TRACKMOD();
    try {
        // check if the tenant already exists
        std::string idLower = lower(identifier);

        Reply reply = r.sendCommand("sismember tenant:list %b", idLower.data(), idLower.length());
        if (reply.getLong() == 1) {
            // the tenant already exists
            std::vector<fds::apis::Tenant> tenants;
            listTenants(tenants);
            for (const auto& tenant : tenants) {
                if (tenant.identifier == identifier) {
                    LOGWARN << "trying to add existing tenant : " << tenant.id;
                    NOMOD();
                    return tenant.id;
                }
            }
            LOGWARN << "tenant info missing : " << identifier;
            NOMOD();
            return 0;
        }

        // get new id
        reply = r.sendCommand("incr tenant:nextid");

        fds::apis::Tenant tenant;
        tenant.id = reply.getLong();
        tenant.identifier = identifier;

        r.sendCommand("sadd tenant:list %b", idLower.data(), idLower.length());

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(tenant, serialized);

        r.sendCommand("hset tenants %ld %b", tenant.id, serialized->data(), serialized->length()); //NOLINT
        return tenant.id;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
    }

    return 0;
}

bool ConfigDB::listTenants(std::vector<fds::apis::Tenant>& tenants) {
    fds::apis::Tenant tenant;

    try {
        Reply reply = r.sendCommand("hvals tenants");
        StringList strings;
        reply.toVector(strings);

        for (const auto& value : strings) {
            fds::deserializeFdspMsg(value, tenant);
            tenants.push_back(tenant);
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

int64_t ConfigDB::createUser(const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isAdmin) { //NOLINT
    TRACKMOD();
    try {
        // check if the user already exists
        std::string idLower = lower(identifier);

        Reply reply = r.sendCommand("sismember user:list %b", idLower.data(), idLower.length());
        if (reply.getLong() == 1) {
            // the user already exists
            std::vector<fds::apis::User> users;
            listUsers(users);
            for (const auto& user : users) {
                if (user.identifier == identifier) {
                    LOGWARN << "trying to add existing user : " << user.id;
                    NOMOD();
                    return user.id;
                }
            }
            LOGWARN << "user info missing : " << identifier;
            NOMOD();
            return 0;
        }

        fds::apis::User user;
        // get new id
        reply = r.sendCommand("incr user:nextid");
        user.id = reply.getLong();
        user.identifier = identifier;
        user.passwordHash = passwordHash;
        user.secret = secret;
        user.isFdsAdmin = isAdmin;

        r.sendCommand("sadd user:list %b", idLower.data(), idLower.length());

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(user, serialized);

        r.sendCommand("hset users %ld %b", user.id, serialized->data(), serialized->length()); //NOLINT
        return user.id;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }

    return 0;
}

bool ConfigDB::listUsers(std::vector<fds::apis::User>& users) {
    fds::apis::User user;

    try {
        Reply reply = r.sendCommand("hvals users");
        StringList strings;
        reply.toVector(strings);

        for (const auto& value : strings) {
            fds::deserializeFdspMsg(value, user);
            users.push_back(user);
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::getUser(int64_t userId, fds::apis::User& user) {
    try {
        Reply reply = r.sendCommand("hget users %ld", userId);

        if (reply.isNil()) {
            LOGWARN << "userinfo does not exist, userid: " << userId;
            return false;
        }

        fds::deserializeFdspMsg(reply.getString(), user);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::assignUserToTenant(int64_t userId, int64_t tenantId) {
    TRACKMOD();
    try {
        if (!r.sadd(format("tenant:%ld:users", tenantId), userId)) {
            LOGWARN << "user: " << userId << " is already assigned to tenant: " << tenantId;
            NOMOD();
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}

bool ConfigDB::revokeUserFromTenant(int64_t userId, int64_t tenantId) {
    TRACKMOD();
    try {
        if (!r.srem(format("tenant:%ld:users", tenantId), userId)) {
            LOGWARN << "user: " << userId << " was NOT assigned to tenant: " << tenantId;
            NOMOD();
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}

bool ConfigDB::listUsersForTenant(std::vector<fds::apis::User>& users, int64_t tenantId) {
    fds::apis::User user;
    try {
        // get the list of users assigned to the tenant
        Reply reply = r.smembers(format("tenant:%ld:users", tenantId));
        StringList strings;
        reply.toVector(strings);

        if (strings.empty()) {
            return true;
        }

        std::string userlist;
        userlist.reserve(2048);
        userlist.append("hmget users");

        for (const auto& value : strings) {
            userlist.append(" ");
            userlist.append(value);
        }

        LOGDEBUG << "users for tenant:" << tenantId << " are [" << userlist << "]";

        reply = r.sendCommand(userlist.c_str());
        strings.clear();
        reply.toVector(strings);
        for (const auto& value : strings) {
            fds::deserializeFdspMsg(value, user);
            users.push_back(user);
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::updateUser(int64_t  userId, const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isFdsAdmin) { //NOLINT
    TRACKMOD();
    try {
        fds::apis::User user;
        if (!getUser(userId, user)) {
            LOGWARN << "user does not exist, userid: " << userId;
            NOMOD();
            throw ConfigException("user id is invalid");
        }

        std::string idLower = lower(identifier);

        // check if the identifier is changing ..
        if (0 != strcasecmp(identifier.c_str(), user.identifier.c_str())) {
            if (r.sismember("user:list", idLower)) {
                LOGWARN << "another user exists with identifier: " << identifier;
                NOMOD();
                throw ConfigException("another user exists with identifier");
            }
        }

        // now set the new user info
        user.identifier = identifier;
        user.passwordHash = passwordHash;
        user.secret = secret;
        user.isFdsAdmin = isFdsAdmin;

        r.sadd("user:list", idLower);

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(user, serialized);

        r.hset("users", user.id, *serialized);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}

bool ConfigDB::createSnapshotPolicy(fds::apis::SnapshotPolicy& policy) {
    TRACKMOD();
    try {
        std::string idLower = lower(policy.policyName);
        bool fNew = true;
        // check if this is a modification
        if (policy.id > 0) {
            fds::apis::SnapshotPolicy oldpolicy;
            if (getSnapshotPolicy(policy.id, oldpolicy)) {
                fNew = false;
                LOGWARN << "modifying an existing policy:" << policy.id;
            }
        }

        if (fNew) {
            if (r.sismember("snapshot.policy:names", idLower)) {
                throw ConfigException("another snapshot policy exists with the name: " + policy.policyName); //NOLINT
            }

            r.sadd("snapshot.policy:names", idLower);
            policy.id = r.incr("snapshot.policy:idcounter");
            LOGDEBUG << "creating a new policy:" <<policy.id;
        }

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(policy, serialized);

        r.hset("snapshot.policies", policy.id, *serialized);
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
}

bool ConfigDB::getSnapshotPolicy(int64_t policyid,
                                 fds::apis::SnapshotPolicy& policy) {
    try {
        Reply reply = r.hget("snapshot.policies", policyid);
        if (reply.isNil()) {
            LOGWARN << "snapshot policy does not exist id:" << policyid;
            return false;
        }
        fds::deserializeFdspMsg(reply.getString(), policy);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::listSnapshotPolicies(std::vector<fds::apis::SnapshotPolicy> & vecPolicy) {
    try {
        Reply reply = r.sendCommand("hvals snapshot.policies");
        StringList strings;
        reply.toVector(strings);
        fds::apis::SnapshotPolicy policy;

        for (const auto& value : strings) {
            fds::deserializeFdspMsg(value, policy);
            vecPolicy.push_back(policy);
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::deleteSnapshotPolicy(const int64_t policyId) {
    try {
        // get the policy
        fds::apis::SnapshotPolicy policy;

        getSnapshotPolicy(policyId, policy);
        std::string nameLower = lower(policy.policyName);

        r.srem("snapshot.policy:names", nameLower);
        r.hdel("snapshot.policies", policyId);

        Reply reply = r.smembers(format("snapshot.policy:%ld:volumes", policyId));
        std::vector<uint64_t> vecVolumes;
        reply.toVector(vecVolumes);

        // delete this policy id from each volume map
        for (const auto& volumeId : vecVolumes) {
            r.srem(format("volume:%ld:snapshot.policies", volumeId), policyId);
        }

        // delete the volume list for this policy
        r.del(format("snapshot.policy:%ld:volumes", policyId));
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::attachSnapshotPolicy(const int64_t volumeId, const int64_t policyId) {
    try {
        r.sendCommand("sadd snapshot.policy:%ld:volumes %ld", policyId, volumeId);
        r.sendCommand("sadd volume:%ld:snapshot.policies %ld", volumeId, policyId);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::listSnapshotPoliciesForVolume(std::vector<fds::apis::SnapshotPolicy> & vecPolicies,
                                             const int64_t volumeId) {
    try {
        Reply reply = r.sendCommand("smembers volume:%ld:snapshot.policies", volumeId); //NOLINT
        StringList strings;
        reply.toVector(strings);

        std::string policylist;
        policylist.reserve(2048);
        policylist.append("hmget snapshot.policies");

        for (const auto& value : strings) {
            policylist.append(" ");
            policylist.append(value);
        }

        reply = r.sendCommand(policylist.c_str());
        strings.clear();
        reply.toVector(strings);
        fds::apis::SnapshotPolicy policy;

        for (const auto& value : strings) {
            fds::deserializeFdspMsg(value, policy);
            vecPolicies.push_back(policy);
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::detachSnapshotPolicy(const int64_t volumeId, const int64_t policyId) {
    try {
        r.sendCommand("srem snapshot.policy:%ld:volumes %ld", policyId, volumeId);
        r.sendCommand("srem volume:%ld:snapshot.policies %ld", volumeId, policyId);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::listVolumesForSnapshotPolicy(std::vector<int64_t> & vecVolumes, const int64_t policyId) { //NOLINT
    try {
        Reply reply = r.sendCommand("smembers snapshot.policy:%ld:volumes", policyId); //NOLINT
        reply.toVector(vecVolumes);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::createSnapshot(fpi::Snapshot& snapshot) {
    TRACKMOD();
    try {
        std::string nameLower = lower(snapshot.snapshotName);

        if (r.sismember("snapshot:names", nameLower)) {
            throw ConfigException("another snapshot exists with name:" + snapshot.snapshotName); //NOLINT
        }

        r.sadd("snapshot:names", nameLower);
        if (snapshot.creationTimestamp <= 1) {
            snapshot.creationTimestamp = fds::util::getTimeStampMillis();
        }

        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(snapshot, serialized);

        r.hset(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId, *serialized); //NOLINT
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}

bool ConfigDB::deleteSnapshot(const int64_t volumeId, const int64_t snapshotId) {
    TRACKMOD();
    try {
        fpi::Snapshot snapshot;
        snapshot.volumeId = volumeId;
        snapshot.snapshotId = snapshotId;

        if (!getSnapshot(snapshot)) {
            LOGWARN << "unable to fetch snapshot [vol:" << volumeId <<",snap:" << snapshotId <<"]";
            NOMOD();
            return false;
        }

        std::string nameLower = lower(snapshot.snapshotName);
        r.srem("snapshot:names", nameLower);
        r.hdel(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId); //NOLINT
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}

bool ConfigDB::updateSnapshot(const fpi::Snapshot& snapshot) {
    TRACKMOD();
    try {
        // check if the snapshot exists
        if (!r.hexists(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId)) {
            LOGWARN << "snapshot does not exist : " << snapshot.snapshotId
                    << " vol:" << snapshot.volumeId;
            NOMOD();
            return false;
        }

        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(snapshot, serialized);

        r.hset(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId, *serialized); //NOLINT
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}


bool ConfigDB::listSnapshots(std::vector<fpi::Snapshot> & vecSnapshots, const int64_t volumeId) {
    try {
        Reply reply = r.sendCommand("hvals volume:%ld:snapshots", volumeId);
        StringList strings;
        reply.toVector(strings);
        fpi::Snapshot snapshot;

        for (const auto& value : strings) {
            fds::deserializeFdspMsg(value, snapshot);
            vecSnapshots.push_back(snapshot);
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::getSnapshot(fpi::Snapshot& snapshot) {
    try {
        Reply reply = r.hget(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId); //NOLINT
        if (reply.isNil()) return false;
        std::string value = reply.getString();
        fds::deserializeFdspMsg(value, snapshot);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}


bool ConfigDB::setSnapshotState(fpi::Snapshot& snapshot , fpi::ResourceState state) {
    if (!getSnapshot(snapshot)) return false;

    snapshot.state = state;
    return updateSnapshot(snapshot);
}

bool ConfigDB::setSnapshotState(const int64_t volumeId, const int64_t snapshotId,
                                fpi::ResourceState state) {
    fpi::Snapshot snapshot;
    snapshot.volumeId = volumeId;
    snapshot.snapshotId = snapshotId;
    return setSnapshotState(snapshot, state);
}

bool ConfigDB::deleteSvcMap(const fpi::SvcInfo& svcinfo) {
    try {
        std::stringstream uuid;
        uuid << svcinfo.svc_id.svc_uuid.svc_uuid;
        
        Reply reply = r.sendCommand( "hdel svcmap %s", uuid.str().c_str() ); //NOLINT
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::updateSvcMap(const fpi::SvcInfo& svcinfo) {
    bool bRetCode = false;
    try {
        
        std::stringstream uuid;
        uuid << svcinfo.svc_id.svc_uuid.svc_uuid;
        
        LOGDEBUG << "ConfigDB updating Service Map:"
                 << " type: " << svcinfo.name 
                 << " uuid: " << std::hex << svcinfo.svc_id.svc_uuid.svc_uuid << std::dec
                 << " ip: " << svcinfo.ip 
        		 << " port: " << svcinfo.svc_port
                 << " incarnation: " << svcinfo.incarnationNo
                 << " status: " << svcinfo.svc_status;
                
        FDSP_SERIALIZE( svcinfo, serialized );        
        bRetCode = r.hset( "svcmap", uuid.str().c_str(), *serialized );
                       
    } catch(const RedisException& e) {
        
        LOGCRITICAL << "error with redis " << e.what();
        
    }
    
    return bRetCode;
}

bool ConfigDB::changeStateSvcMap( const int64_t svc_uuid, 
                                  const fpi::ServiceStatus svc_status )
{
    bool bRetCode = false;
    
    try
    {
        std::stringstream uuid;
        uuid << svc_uuid;
        
         LOGDEBUG << "ConfigDB changing service status:"
                  << " uuid: " << std::hex << svc_uuid << std::dec
                  << " status: " << svc_status;
        
        Reply reply = r.hget( "svcmap", uuid.str().c_str() ); //NOLINT
        if ( reply.isValid() ) 
        {
            std::string value = reply.getString();
            fpi::SvcInfo svcInfo;
            fds::deserializeFdspMsg( value, svcInfo );
            
            fpi::ServiceStatus old_svc_status = svcInfo.svc_status;
            svcInfo.svc_status = svc_status;
                        
            bRetCode = updateSvcMap( svcInfo );
            
            LOGDEBUG << "ConfigDB updated service status:"
                     << " uuid: " << std::hex << svc_uuid << std::dec
                     << " from status: " << old_svc_status 
                     << " to status: " << svc_status;
            
            /* Convert new registration request to existing registration request */
            kvstore::NodeInfoType nodeInfo;
            fromTo( svcInfo, nodeInfo );
            updateNode( nodeInfo );
            
            bRetCode = true;
        }
    }
    catch( const RedisException& e ) 
    {
        
        LOGCRITICAL << "error with redis " << e.what();
        
    }
    
    return bRetCode;
}

void ConfigDB::fromTo( fpi::SvcInfo svcInfo, 
                       kvstore::NodeInfoType nodeInfo )
{
    nodeInfo.control_port = svcInfo.svc_port;
    nodeInfo.data_port = svcInfo.svc_port;
    nodeInfo.ip_lo_addr = fds::net::ipString2Addr(svcInfo.ip);  
    nodeInfo.node_name = svcInfo.name;   
    nodeInfo.node_type = svcInfo.svc_type;

    fds::assign( nodeInfo.service_uuid, svcInfo.svc_id );

    NodeUuid node_uuid;
    node_uuid.uuid_set_type( nodeInfo.service_uuid.uuid, fpi::FDSP_PLATFORM );
    nodeInfo.node_uuid.uuid  = static_cast<int64_t>( node_uuid.uuid_get_val() );

    fds_assert( nodeInfo.node_type != fpi::FDSP_PLATFORM ||
                nodeInfo.service_uuid == nodeInfo.node_uuid );

    if ( nodeInfo.node_type == fpi::FDSP_PLATFORM ) 
    {
        fpi::FDSP_AnnounceDiskCapability& diskInfo = nodeInfo.disk_info;
        
        util::Properties props( &svcInfo.props );
        diskInfo.node_iops_max = props.getInt( "node_iops_max" );
        diskInfo.node_iops_min = props.getInt( "node_iops_min" );
        diskInfo.disk_capacity = props.getDouble( "disk_capacity" );

        diskInfo.ssd_capacity = props.getDouble( "ssd_capacity" );
        diskInfo.disk_type = props.getInt( "disk_type" );
    }
}

bool ConfigDB::getSvcMap(std::vector<fpi::SvcInfo>& svcMap)
{
    try 
    {
        svcMap.clear();
        Reply reply = r.sendCommand("hgetall svcmap");
        StringList strings;
        reply.toVector(strings);

        for (uint i = 0; i < strings.size(); i+= 2) {
            fpi::SvcInfo svcInfo;
            std::string& value = strings[i+1];

            fds::deserializeFdspMsg(value, svcInfo);
            svcMap.push_back(svcInfo);
        }
    } 
    catch ( const RedisException& e ) 
    {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    
    return true;
}


}  // namespace kvstore
}  // namespace fds
