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
#include <net/SvcMgr.h>
#include <DltDmtUtil.h>

#include "platform/platform_shm_typedefs.h"
#include "platform/node_data.h"

auto format = fds::util::strformat;
auto lower  = fds::util::strlower;

namespace fds { namespace kvstore {

// ConfigDB Versions. Codenamed after minerals: http://www.minerals.net
namespace {
    std::string CONFIGDB_VERSION_TALC{"00.00.00"};
    std::string CONFIGDB_VERSION_GRAPHITE{"00.05.00"};

    std::string CONFIGDB_VERSION_LATEST{CONFIGDB_VERSION_GRAPHITE};
};

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
        return kv_store.get("config.lastmod").getLong();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }

    return 0;
}

// Used only internally with the ConfigDB object.
void ConfigDB::setConfigVersion(const std::string& newVersion) {
    TRACKMOD();
    try {
        auto reply  = kv_store.hget("configDB", "currentVersion");
        reply.checkError();

        auto oldVersion = reply.getString();
        LOGNOTIFY << "Changing ConfigDB version <" << oldVersion << "> to version <" <<
                  newVersion << ">.";

        kv_store.hset("configDB", "oldVersion", oldVersion);

        kv_store.hset("configDB", "currentVersion", newVersion);

        kv_store.hset("configDB",
                      "versionModTime",
                      static_cast<std::int64_t>(fds::util::getTimeStampMillis()));

    } catch(const RedisException& e) {
        NOMOD();
        LOGERROR << e.what();
        RedisException re{"Unable to set new ConfigDB version."};
        throw re;
    };
}

// May be called upon the ConfigDB object by a client to set the
// latest version. Presumably, this would only be done when the
// ConfigDB is initially created.
void ConfigDB::setConfigVersion() {
    setConfigVersion(CONFIGDB_VERSION_LATEST);
}

std::string ConfigDB::getConfigVersion() {
    try {
        auto reply = kv_store.hget("configDB", "currentVersion");
        reply.checkError();

        std::string version = reply.getString();
        if (version.empty()) {
            /**
             * We don't have a version set which indicates our version is pre-version controlled, or version Talc.
             */
            version = CONFIGDB_VERSION_TALC;
            setConfigVersion(version);
        }

        LOGDEBUG << "Current ConfigDB version <" << version << ">.";

        return version;
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }

    return "";
}

/**
 * Compare the passed version to the current version.
 *
 * @param std::string version: Version string to compare to the latest string.
 *
 * @return bool: 'true' if passed version is equal to the latest version. 'false' otherwise.
 */
bool ConfigDB::isLatestConfigDBVersion(std::string& version) {
    return (version == CONFIGDB_VERSION_LATEST);
}

/**
 * Upgrade to the latest version of the configuration database.
 */
ConfigDB::ReturnType ConfigDB::upgradeConfigDBVersionLatest(std::string& currentVersion) {
    LOGNOTIFY << "Upgrade from ConfigDB version <" << currentVersion << "> to version <" <<
              CONFIGDB_VERSION_LATEST << ">.";

    if (currentVersion == CONFIGDB_VERSION_TALC) {
        TRACKMOD();
        try {
            /**
             * Address Local Domain changes.
             */

            /**
             * Capture the current local domain details.
             */
            std::vector<fds::apis::LocalDomainDescriptorV07> localDomains;

            if (!listLocalDomainsTalc(localDomains)) {
                NOMOD();
                LOGCRITICAL << "Unable to get local domain list for ConfigDB upgrade.";
                return ConfigDB::ReturnType::CONFIGDB_EXCEPTION;
            }

            /**
             * Re-write local domain details in latest version format. We will switch the local domain
             * list from being a list of local domain names to a list of their IDs. So get rid of the
             * name list first.
             */
            LOGNOTIFY << "Removing old <local.domain:list> key.";
            kv_store.del("local.domain:list");

            for (const auto& oldLocalDomain : localDomains) {
                LOGNOTIFY << "Re-write local domain <" << oldLocalDomain.name << ">:<" << oldLocalDomain.id
                          << "> into keys <local.domain:list> and <ldom:" <<  oldLocalDomain.id << ">.";

                kv_store.sadd("local.domain:list", oldLocalDomain.id);

                auto reply = kv_store.sendCommand("hmset ldom:%d id %d"
                                                  " name %s"
                                                  " site %s"
                                                  " createTime %ld"
                                                  " current %d",
                                                  oldLocalDomain.id, oldLocalDomain.id,
                                                  oldLocalDomain.name.c_str(),
                                                  oldLocalDomain.site.c_str(),
                                                  util::getTimeStampMillis(),  // Maybe just set this to 0?
                                                  (oldLocalDomain.id == master_ldom_id) ? true : false); // With version Talc, only the master domain was ever the current domain.
                reply.checkError();

                // No OM contact information availble in Talc.
            }

            /**
             * Clean up old local domain key.
             */
            LOGNOTIFY << "Removing old <local.domains> key.";
            kv_store.del("local.domains");

            /**
             * Address ConfigDB key changes.
             */

            /**
             * We've already installed the new ConfigDB versioning keys (see ConfigDB::getConfigVersion()).
             * We just need to remove the old ConfigDB versioning key.
             */
            LOGNOTIFY << "Removing old <config.version> key.";
            kv_store.del("config.version");

            /**
             * Finally, bump the ConfigDB version to latest.
             */
            setConfigVersion(CONFIGDB_VERSION_LATEST);

            return ConfigDB::ReturnType::SUCCESS;

        } catch (const RedisException &e) {
            LOGERROR << e.what();
            NOMOD();
        }
    } else {
        LOGWARN << "Nothing to upgrade.";
        return ConfigDB::ReturnType::SUCCESS;
    }

    return ConfigDB::ReturnType::CONFIGDB_EXCEPTION;
}


void ConfigDB::setModified() {
    try {
        kv_store.sendCommand("set config.lastmod %ld", fds::util::getTimeStampMillis());
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
}
/******************************************************************************
 *                      Domains Section
 *****************************************************************************/

// Global Domain
std::string ConfigDB::getGlobalDomain() {
    try {
        return kv_store.get("global.domain").getString();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
        return "";
    }
}

bool ConfigDB::setGlobalDomain(ConstString globalDomain) {
    TRACKMOD();
    try {
        return kv_store.set("global.domain", globalDomain);
    } catch(const RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}


// Local Domains
// Note that local domains are global domain objects. While they might be cached in any given local domain,
// they are definitively stored and maintained in the context of the global domain and therefore, the
// master domain and master OM.
//
// TODO(Greg): These methods need to ensure that they are being executed within the context of the
// master OM.
fds_ldomid_t ConfigDB::getNewLocalDomainId() {
    TRACKMOD();
    try {
        fds_ldomid_t ldomId;
        ReturnType ret;
        for (;;) {
            ldomId = fds_ldomid_t(kv_store.incr("local.domain:nextid"));
            if ((ret = localDomainExists(ldomId)) == ReturnType::NOT_FOUND) {
                // We haven't used this one yet.
                return ldomId;
            } else if (ret == ReturnType::CONFIGDB_EXCEPTION) {
                LOGCRITICAL << "ConfigDB exception getting a new local domain ID.";
                break;
            }
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    NOMOD();
    return invalid_ldom_id;
}

/**
 * Create a new Local Domain provided it does not already exist. (Local Domain names
 * are treated as case-insenative.) Or update an existing local domain.
 *
 * When OM is booted for the first time, it creates a Global Domain and a Local Domain within it, setting
 * the Local Domain's ID to 1. This makes that Local Domain the only Local Domain within the Global Domain
 * and it is the Master Domain for the Global Domain.
 *
 * To create multiple Local Domains within a single Global Domain, first create the Local Domains seperately.
 * Then choose one of them to be the Master Domain for the Global Domain into which the others will be added.
 *
 * As other Local Domains are added to a Global Domain, their IDs will change (initially they are all 1 and as they
 * are added they get set to something >1 that's unique within the Global Domain) and because of this, they will
 * lose their "Master Domain" status. Therefore, it must be the case that no Local Domains are added to a different
 * Global Domain if the Local Domain in question already has Tenants defined. For in that case, we have no mechanism
 * to resolve clashes between the IDs of the Global Domain objects that may result. Those Global Domain objects
 * include Tenants, Users, Applications, and Subscriptions for Aynschronous Replication. In addition, the Local Domain
 * in question must be the only Local Domain within its current Global Domain. Otherwise we may run into clashes
 * between Local Domain IDs (note that Local Domains are also Global Domain objects).
 *
 * In short, a Local Domain may only be added to an existing Global Domain if the Local Domain has been newly created
 * and nothing more done to it besides adding Nodes and Services.
 *
 * @param localDomain: The details of the Local Domain to be created or updated.
 * @param isNew: 'true' if we are being asked to create a new local domain. 'false' if we are being asked to update
 *               an existing local domain.
 *
 * @return fds_ldomid_t - ID generated for the new Local Domain.
 *
 */
fds_ldomid_t ConfigDB::putLocalDomain(const LocalDomain& localDomain, const bool isNew) {
    TRACKMOD();

    try {
        ReturnType ret;

        // Check if the local domain already exists.
        if ((ret = localDomainExists(localDomain.getName())) == ReturnType::SUCCESS) {
            if (isNew) {
                LOGWARN << "Trying to add an existing local domain: LocalDomain [" << localDomain.getName() << "].";
                NOMOD();
                return invalid_ldom_id;
            }
        } else if (ret == ReturnType::NOT_FOUND) {
            if (!isNew) {
                LOGWARN << "Trying to udpate a non-existing local domain: LocalDOmain [" << localDomain.getName() << "].";
                NOMOD();
                return invalid_ldom_id;
            }
        } else {
            // We had some problem with ConfigDB.
            NOMOD();
            return invalid_ldom_id;
        }

        // Get new or existing ID
        fds_ldomid_t ldomId{invalid_ldom_id};
        if (isNew) {
            ldomId = getNewLocalDomainId();
            LOGDEBUG << "Adding local domain [" << localDomain.getName() << "], ID [" << ldomId << "].";

            // Add the local domain.
            kv_store.sadd("local.domain:list", ldomId);
        } else {
            ldomId = getLocalDomainId(localDomain.getName());
            LOGDEBUG << "Updating local domain [" << localDomain.getName() << "], ID [" << ldomId << "].";
        }

        // Insert the new local domain details.
        auto reply = kv_store.sendCommand("hmset ldom:%d"
                                          " id %d"
                                          " name %s"
                                          " site %s"
                                          " createTime %ld"
                                          " current %d",
                                          ldomId,
                                          ldomId,
                                          localDomain.getName().c_str(),
                                          localDomain.getSite().c_str(),
                                          (localDomain.getCreateTime() == 0) ? util::getTimeStampMillis() :
                                                                               localDomain.getCreateTime(),
                                          localDomain.getCurrent());
        reply.checkError();

        // Now put any OM Node contact information.
        std::size_t i;
        for (i = 0; i < localDomain.getOMNodes().size(); i++) {
            reply = kv_store.sendCommand("hmset ldom:%d:om:%d"
                                         " node_type %d"
                                         " node_name %s"
                                         " domain_id %d"
                                         " ip_hi_addr %ld"
                                         " ip_lo_addr %ld"
                                         " control_port %d"
                                         " data_port %d"
                                         " migration_port %d"
                                         " node_uuid %ld"
                                         " service_uuid %ld"
                                         " node_root %s"
                                         " metasync_port %d",
                                         ldomId, i,
                                         localDomain.getOMNode(i).node_type,
                                         localDomain.getOMNode(i).node_name.c_str(),
                                         localDomain.getOMNode(i).domain_id,
                                         localDomain.getOMNode(i).ip_hi_addr,
                                         localDomain.getOMNode(i).ip_lo_addr,
                                         localDomain.getOMNode(i).control_port,
                                         localDomain.getOMNode(i).data_port,
                                         localDomain.getOMNode(i).migration_port,
                                         localDomain.getOMNode(i).node_uuid.uuid,
                                         localDomain.getOMNode(i).service_uuid.uuid,
                                         localDomain.getOMNode(i).node_root.c_str(),
                                         localDomain.getOMNode(i).metasync_port);
            reply.checkError();
        }

        // Finally, clear away any OM Node contact information left from a previous put.
        for (; kv_store.exists(format("ldom:%d:om:%d", ldomId, i)) == true; i++) {
            kv_store.del(format("ldom:%d:om:%d", ldomId, i));
        }

        return ldomId;

    } catch(const RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }

    NOMOD();
    return invalid_ldom_id;
}

/**
 * Add an existing Local Domain to the current Global Domain. See the header comments for createLocalDomain()
 * for details.
 *
 * @param localDomain: The details of the Local Domain to be added.
 *
 * @return int64_t - ID generated for the new Local Domain.
 */
fds_ldomid_t ConfigDB::addLocalDomain(const LocalDomain& localDomain) {
    if (localDomain.getOMNodes().size() == 0) {
        LOGWARN << "Trying to add an existing Local Domain to the current Global Domain "
                   "but no OM contact information is provided for the existing Local Domain: " << localDomain.getName();
        return invalid_ldom_id;
    }

    return putLocalDomain(localDomain, false /* Not a new local domain. */);
}

ConfigDB::ReturnType ConfigDB::updateLocalDomain(const LocalDomain& localDomain) {
    /**
     * LocalDomain ID is required.
     */
    if (localDomain.getID() == invalid_ldom_id) {
        LOGERROR << "LocalDomain ID must be supplied for an update of local domain [" << localDomain.getName() << "].";
        return ReturnType::NOT_UPDATED;
    } else {
        /**
         * The following are *not* modifiable: id, createTime, current
         */
        LocalDomain currentLocalDomain("dummy", invalid_ldom_id);

        ReturnType ret = getLocalDomain(localDomain.getID(), currentLocalDomain);
        if (ret == ReturnType::CONFIGDB_EXCEPTION) {
            LOGCRITICAL << "Unable to perform local domain update for local domain ID [" << localDomain.getID() << "].";
            return ret;
        } else if (ret == ReturnType::NOT_FOUND) {
            LOGERROR << "Local domain to update not found for local domain ID [" << localDomain.getID() << "].";
            return ret;
        } else if ((currentLocalDomain.getCurrent() != localDomain.getCurrent()) ||
                   (currentLocalDomain.getCreateTime() != localDomain.getCreateTime())) {
            LOGERROR << "Local domain updates may not change ID, Current, or Create Time for local domain ID [" << localDomain.getID() << "].";
            return ReturnType::NOT_UPDATED;
        }
    }

    return ((putLocalDomain(localDomain, false /* Not a new local domain. */) == invalid_ldom_id) ?
            ReturnType::NOT_UPDATED : ReturnType::SUCCESS);
}

ConfigDB::ReturnType ConfigDB::deleteLocalDomain(fds_ldomid_t ldomId) {
    if (!localDomainExists(ldomId)) {
        return ReturnType::NOT_FOUND;
    }

    TRACKMOD();

    try {
        if (!(kv_store.srem("local.domain:list", ldomId))) {
            LOGWARN << "Local domain [" << ldomId << "] not found, not removed.";
            NOMOD();
            return ReturnType::NOT_UPDATED;
        }

        // Delete the local domain.
        if (!(kv_store.del(format("ldom:%d", ldomId)))) {
            LOGWARN << "Local domain [" << ldomId << "] details not found, not removed.";
            // NOMOD(); Presumably, the ealier modification took place.
            return ReturnType::NOT_UPDATED;
        } else {
            return ReturnType::SUCCESS;
        }

        // Finally, clear away any OM Node contact information.
        for (std::size_t i = 0; kv_store.exists(format("ldom:%d:om:%d", ldomId, i)) == true; i++) {
            kv_store.del(format("ldom:%d:om:%d", ldomId, i));
        }
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
        NOMOD();
    }

    return ReturnType::CONFIGDB_EXCEPTION;
}

// Check by local domain ID.
ConfigDB::ReturnType ConfigDB::localDomainExists(fds_ldomid_t ldomId) {
    try {
        /**
         * Should we check the local domain ID list or just look for the details
         * directly? Either way, they should be consistent woth each other. We'll
         * look for the details.
         */
        return (kv_store.exists(format("ldom:%d", ldomId))) ? ReturnType::SUCCESS : ReturnType::NOT_FOUND;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }

    return ReturnType::CONFIGDB_EXCEPTION;
}

// Check by local domain name which is unique per global domain.
ConfigDB::ReturnType ConfigDB::localDomainExists(const std::string& ldomName)
{
    std::vector<LocalDomain> localDomains;
    ReturnType ret;

    if ((ret = getLocalDomains(localDomains)) == ReturnType::SUCCESS) {
        for (const auto localDomain : localDomains) {
            if ((localDomain.getName().compare(ldomName) == 0)) {
                return ReturnType::SUCCESS;
            }
        }

        // Didn't find it.
        return ReturnType::NOT_FOUND;
    }

    // Either no local domains or a problem with ConfigDB access.
    return ret;
}

ConfigDB::ReturnType ConfigDB::getLocalDomainIds(std::vector<fds_ldomid_t>&ldomIds) {
    std::vector<fds_ldomid_t> _ldomIds; //NOLINT

    try {
        auto reply = kv_store.smembers("local.domain:list");
        reply.checkError();

        reply.toVector(_ldomIds);

        if (_ldomIds.empty()) {
            LOGDEBUG << "No local domains found.";
            return ReturnType::NOT_FOUND;
        }

        for (std::size_t i = 0; i < _ldomIds.size(); i++) {
            ldomIds.push_back(fds_ldomid_t(_ldomIds[i]));
        }
        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return ReturnType::CONFIGDB_EXCEPTION;
}

// Get local domain ID from its name.
fds_ldomid_t ConfigDB::getLocalDomainId(const std::string& name)
{
    LocalDomain localDomain(name, invalid_ldom_id);

    if (getLocalDomain(name, localDomain) == ReturnType::SUCCESS)
    {
        return localDomain.getID();
    }
    else {
        return invalid_ldom_id;
    }
}

ConfigDB::ReturnType ConfigDB::getLocalDomains(std::vector<LocalDomain>&localDomains) {
    std::vector<fds_ldomid_t> ldomIds; //NOLINT

    try {
        auto reply = kv_store.smembers("local.domain:list");
        reply.checkError();

        reply.toVector(ldomIds);

        if (ldomIds.empty()) {
            LOGDEBUG << "No local domains found.";
            return ReturnType::NOT_FOUND;
        }

        ReturnType ret;
        for (std::size_t i = 0; i < ldomIds.size(); i++) {
            LocalDomain localDomain("dummy", invalid_sub_id);
            if ((ret = getLocalDomain(fds_ldomid_t(ldomIds[i]), localDomain)) == ReturnType::NOT_FOUND) {
                LOGCRITICAL << "ConfigDB inconsistency. Local domain detail missing for id [" << ldomIds[i] << "].";
                return ReturnType::CONFIGDB_EXCEPTION;
            } else if (ret == ReturnType::CONFIGDB_EXCEPTION) {
                LOGCRITICAL << "ConfigDB exception reading details for local domain id [" << ldomIds[i] << "].";
                return ReturnType::CONFIGDB_EXCEPTION;
            }

            localDomains.push_back(localDomain);
        }
        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return ReturnType::CONFIGDB_EXCEPTION;
}

ConfigDB::ReturnType ConfigDB::getLocalDomain(fds_ldomid_t ldomId, LocalDomain& localDomain) {
    try {
        Reply reply1 = kv_store.hgetall(format("ldom:%d", ldomId));
        reply1.checkError();

        StringList strings1;
        reply1.toVector(strings1);

        if (strings1.empty()) {
            LOGDEBUG << "Unable to find local domain ID [" << ldomId << "].";
            return ReturnType::NOT_FOUND;
        }

        // Since we expect complete key-value pairs, the size of strings should be even.
        if ((strings1.size() % 2) != 0) {
            LOGCRITICAL << "ConfigDB corruption. Local domain ID [" << ldomId <<
                        "] does not have a complete key-value pairing of attributes.";
            return ReturnType::CONFIGDB_EXCEPTION;
        }

        for (std::size_t i = 0; i < strings1.size(); i+= 2) {
            std::string& key1 = strings1[i];
            std::string& value1 = strings1[i+1];

            if (key1 == "id") { localDomain.setID(fds_ldomid_t(std::stoi(value1.c_str(), NULL, 10)));}
            else if (key1 == "name") { localDomain.setName(value1); }
            else if (key1 == "site") { localDomain.setSite(value1); }
            else if (key1 == "createTime") { localDomain.setCreateTime(strtoull(value1.c_str(), NULL, 10));} //NOLINT
            else if (key1 == "current") { localDomain.setCurrent(static_cast<bool>(std::stoi(value1.c_str(), NULL, 10)));}
            else { //NOLINT
                LOGWARN << "Unknown key for local domain [" << ldomId <<
                        "] - key [" << key1 << "], value [" << value1 << "].";
                fds_assert(!"Unknown key");
            }

            // See if we have any OM contact information for this local domain.
            std::vector<FDS_ProtocolInterface::FDSP_RegisterNodeType> omNodes;
            for (std::size_t j = 0; kv_store.exists(format("ldom:%d:om:%d", ldomId, j)) == true; j++) {
                Reply reply2 = kv_store.hgetall(format("ldom:%d:om:%d", ldomId, j));
                reply2.checkError();

                StringList strings2;
                reply2.toVector(strings2);

                if (strings2.empty()) {
                    LOGDEBUG << "No OM contact information for local domain ID [" << ldomId << "].";
                    // This should probably be an error - database inconsistency.
                }

                // Since we expect complete key-value pairs, the size of strings should be even.
                if ((strings2.size() % 2) != 0) {
                    LOGCRITICAL << "ConfigDB corruption. Local domain ID [" << ldomId <<
                                "] OM contact information [" << j << "] does not have a complete key-value pairing of attributes.";
                    return ReturnType::CONFIGDB_EXCEPTION;
                }

                FDS_ProtocolInterface::FDSP_RegisterNodeType omNode;
                for (std::size_t k = 0; k < strings2.size(); k+= 2) {
                    std::string& key2 = strings2[k];
                    std::string& value2 = strings2[k+1];

                    if (key2 == "node_type") { omNode.__set_node_type(static_cast<FDS_ProtocolInterface::FDSP_MgrIdType>(std::stoi(value2.c_str(), NULL, 10))); }
                    else if (key2 == "node_name") { omNode.__set_node_name(value2); }
                    else if (key2 == "domain_id") { omNode.domain_id = std::stoi(value2.c_str(), NULL, 10); }
                    else if (key2 == "ip_hi_addr") { omNode.ip_hi_addr = std::stol(value2.c_str(), NULL, 10); }
                    else if (key2 == "ip_lo_addr") { omNode.ip_lo_addr = std::stol(value2.c_str(), NULL, 10); }
                    else if (key2 == "control_port") { omNode.control_port = std::stoi(value2.c_str(), NULL, 10); }
                    else if (key2 == "data_port") { omNode.data_port = std::stoi(value2.c_str(), NULL, 10); }
                    else if (key2 == "migration_port") { omNode.migration_port = std::stoi(value2.c_str(), NULL, 10); }
                    else if (key2 == "node_uuid") { omNode.node_uuid.__set_uuid(std::stol(value2.c_str(), NULL, 10)); }
                    else if (key2 == "service_uuid") { omNode.service_uuid.__set_uuid(std::stol(value2.c_str(), NULL, 10)); }
                    else if (key2 == "node_root") { omNode.node_root = value2; }
                    else if (key2 == "metasync_port") { omNode.migration_port = std::stoi(value2.c_str(), NULL, 10); }
                    else { //NOLINT
                        LOGWARN << "Unknown key for local domain [" << ldomId << "] and OM contact information [" << k <<
                                "] - key [" << key2 << "], value [" << value2 << "].";
                        fds_assert(!"Unknown key");
                    }

                    omNodes.emplace_back(omNode);
                }
            }

            localDomain.setOMNodes(omNodes);
        }
        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }

    return ReturnType::CONFIGDB_EXCEPTION;
}

ConfigDB::ReturnType ConfigDB::getLocalDomain(const std::string& name, LocalDomain& localDomain) {
    try {
        std::vector<LocalDomain> localDomains;
        std::vector<LocalDomain>::const_iterator ldomIterator;
        ReturnType ret;

        if ((ret = getLocalDomains(localDomains)) == ReturnType::SUCCESS) {
            for (ldomIterator = localDomains.begin(); ldomIterator != localDomains.end(); ldomIterator++) {
                if (ldomIterator->getName().compare(name) == 0){
                    break;
                }
            }
        } else if (ret == ReturnType::CONFIGDB_EXCEPTION) {
            LOGCRITICAL << "ConfigDB exception retrieving local domain by name.";
            return ReturnType::CONFIGDB_EXCEPTION;
        }

        if (ldomIterator == localDomains.end()) {
            LOGDEBUG << "Unable to find local domain [" << name << "].";
            return ReturnType::NOT_FOUND;
        }

        localDomain = *ldomIterator;
        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return ReturnType::CONFIGDB_EXCEPTION;
}

/**
 * Generate a vector of Local Domains currently defined.
 *
 * @param localDomains: Vector of returned LocalDomain objects.
 *
 * @return ReturnType - SUCCESS if local domains found. NOT_FOUND if not local domains. CONFIGDB_EXCEPTION otherwise.
 */
ConfigDB::ReturnType ConfigDB::listLocalDomains(std::vector<LocalDomain>& localDomains) {
    std::vector<fds_ldomid_t> ldomIds; //NOLINT

    try {
        auto reply = kv_store.smembers("local.domain:list");
        reply.checkError();

        reply.toVector(ldomIds);

        if (ldomIds.empty()) {
            LOGDEBUG << "No local domains found.";
            return ReturnType::NOT_FOUND;
        }

        ReturnType ret;
        for (std::size_t i = 0; i < ldomIds.size(); i++) {
            LocalDomain localDomain("dummy" , invalid_ldom_id);
            if ((ret = getLocalDomain(fds_ldomid_t(ldomIds[i]), localDomain)) == ReturnType::NOT_FOUND) {
                LOGCRITICAL << "ConfigDB inconsistency. Local domain detail missing for id [" << ldomIds[i] << "].";
                return ReturnType::CONFIGDB_EXCEPTION;
            } else if (ret == ReturnType::CONFIGDB_EXCEPTION) {
                // Some ConfigDB error.
                return ReturnType::CONFIGDB_EXCEPTION;
            }

            localDomains.push_back(localDomain);
        }
        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return ReturnType::CONFIGDB_EXCEPTION;
}

/**
 * Generate a vector of Local Domains currently defined in the Talc version.
 *
 * @param localDomains: Vector of returned Local Domain names.
 *
 * @return bool - True if the vector is successfully produced. False otherwise.
 */
bool ConfigDB::listLocalDomainsTalc(std::vector<fds::apis::LocalDomainDescriptorV07> &localDomains) {
    fds::apis::LocalDomainDescriptorV07 localDomain;

    try {
        Reply reply = kv_store.sendCommand("hvals local.domains");
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


/******************************************************************************
 *                      Volume Section
 *****************************************************************************/
fds_volid_t ConfigDB::getNewVolumeId() {
    try {
        fds_volid_t volId;
        for (;;) {
            volId = ( fds_volid_t ) ( kv_store.incr("volumes:nextid") );
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
        auto volId = vol.volUUID.get();
        Reply reply = kv_store.sendCommand("sadd %d:volumes %ld", vol.localDomainId, volId);
        if (!reply.wasModified()) {
            LOGDEBUG << "volume [ " << vol.volUUID
                     << " ] already exists for domain [ "
                     << vol.localDomainId << " ]";
        }

        // add the volume data
        reply = kv_store.sendCommand("hmset vol:%ld uuid %ld"
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
                              " continuous.commit.log.retention %d"
                              " backup.vol.id %ld"
                              " iops.min %d"
                              " iops.max %d"
                              " relative.priority %d"
                              " fsnapshot %d"
                              " parentvolumeid %ld"
                              " state %d"
                              " create.time %ld"
                              " vg.coordinatorinfo.svcuuid %ld"
                              " vg.coordinatorinfo.version %d",
                              volId, volId,
                              vol.name.c_str( ),
                              vol.tennantId,
                              vol.localDomainId,
                              vol.globDomainId,
                              vol.volType,
                              vol.capacity,
                              vol.maxQuota,
                              vol.redundancyCnt,
                              vol.maxObjSizeInBytes,
                              vol.writeQuorum,
                              vol.readQuorum,
                              vol.consisProtocol,
                              vol.volPolicyId,
                              vol.archivePolicyId,
                              vol.placementPolicy,
                              vol.appWorkload,
                              vol.mediaPolicy,
                              vol.contCommitlogRetention,
                              vol.backupVolume,
                              vol.iops_assured,
                              vol.iops_throttle,
                              vol.relativePrio,
                              vol.fSnapshot,
                              vol.srcVolumeId.get( ),
                              vol.getState( ),
                              vol.createTime,
                              vol.coordinator.id.svc_uuid,
                              vol.coordinator.version);
        if ( reply.isOk( ) )
        {
            if( vol.volType == fpi::FDSP_VOL_NFS_TYPE ||
                vol.volType == fpi::FDSP_VOL_ISCSI_TYPE )
            {
                boost::shared_ptr <std::string> serialized = {};
                if ( vol.volType == fpi::FDSP_VOL_ISCSI_TYPE ) {
                    fds::serializeFdspMsg( vol.iscsiSettings, serialized );
                }
                else if ( vol.volType == fpi::FDSP_VOL_NFS_TYPE ) {
                    fds::serializeFdspMsg( vol.nfsSettings, serialized );
                }

                return setVolumeSettings( volId, serialized );
            }
            // all others just fall through.

            return true;
        }

        LOGWARN << "REDIS: " << reply.getString();
    } catch(RedisException& e) {
        LOGERROR << "error with Redis: " << e.what();
        TRACKMOD();
    }
    return false;
}

bool ConfigDB::setVolumeSettings( long unsigned int volumeId, boost::shared_ptr<std::string> serialized )
{
    try
    {
        LOGDEBUG << "Volume settings for volume ID[ " << volumeId << " ] ( SET )";
        return kv_store.sendCommand( "set vol:%ld:settings %b",
                                     volumeId,
                                     serialized->data(),
                                     serialized->length() )
                        .isOk();
    }
    catch(const RedisException& e)
    {
        LOGERROR << "error with Redis: " << e.what();
    }

    return false;
}

bool ConfigDB::setVolumeState(fds_volid_t volumeId, fpi::ResourceState state) {
    TRACKMOD();
    try {
        LOGDEBUG << "updating volume id " << volumeId 
                 << " to state " << state;
        auto volId = volumeId.get();
        return kv_store.sendCommand("hset vol:%ld state %d", volId, static_cast<int>(state)).isOk();
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
        auto volId = volumeId.get();
        Reply reply = kv_store.sendCommand("srem %d:volumes %ld", localDomainId, volId);
        if (!reply.wasModified()) {
            LOGWARN << "volume [" << volumeId
                    << "] does NOT exist for domain ["
                    << localDomainId << "]";
        }

        // del the volume data
        reply = kv_store.sendCommand("del vol:%ld", volId);
        return reply.isOk();
    } catch(RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::volumeExists(fds_volid_t volumeId) {
    try {
        auto volId = volumeId.get();
        Reply reply = kv_store.sendCommand("exists vol:%ld", volId);
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
        Reply reply = kv_store.sendCommand("smembers %d:volumes", localDomain);
        reply.toVector(volumeIds);

        if (volumeIds.empty()) {
            LOGWARN << "no volumes found for domain [" << localDomain << "]";
            return false;
        }

        for (uint i = 0; i < volumeIds.size(); i++) {
            volIds.push_back(fds_volid_t(volumeIds[i]));
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
        Reply reply = kv_store.sendCommand("smembers %d:volumes", localDomain);
        reply.toVector(volumeIds);

        if (volumeIds.empty()) {
            LOGWARN << "no volumes found for domain [" << localDomain << "]";
            return false;
        }

        for (uint i = 0; i < volumeIds.size(); i++) {
            VolumeDesc volume("" , fds_volid_t(1));  // dummy init
            getVolume(fds_volid_t(volumeIds[i]), volume);
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
        auto volId = volumeId.get();
        Reply reply = kv_store.sendCommand("hgetall vol:%ld", volId);
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
            else if (key == "replica.count") {vol.redundancyCnt = atoi(value.c_str());}  // Persisted name is deprecated
            else if (key == "redundancy.count") {vol.redundancyCnt = atoi(value.c_str());} // in favor of this one.
            else if (key == "objsize.max") {vol.maxObjSizeInBytes = atoi(value.c_str());}
            else if (key == "write.quorum") {vol.writeQuorum = atoi(value.c_str());}
            else if (key == "read.quorum") {vol.readQuorum = atoi(value.c_str());}
            else if (key == "conistency.protocol") {vol.consisProtocol = (fpi::FDSP_ConsisProtoType)atoi(value.c_str());} //NOLINT
            else if (key == "volume.policy.id") {vol.volPolicyId = atoi(value.c_str());}
            else if (key == "archive.policy.id") {vol.archivePolicyId = atoi(value.c_str());}
            else if (key == "placement.policy.id") {vol.placementPolicy = atoi(value.c_str());}
            else if (key == "app.workload") {vol.appWorkload = (fpi::FDSP_AppWorkload)atoi(value.c_str());} //NOLINT
            else if (key == "media.policy") {vol.mediaPolicy = (fpi::FDSP_MediaPolicy)atoi(value.c_str());} //NOLINT
            else if (key == "continuous.commit.log.retention") {vol.contCommitlogRetention = strtoull(value.c_str(), NULL, 10);} //NOLINT
            else if (key == "backup.vol.id") {vol.backupVolume = atol(value.c_str());}
            else if (key == "iops.min") {vol.iops_assured = strtod (value.c_str(), NULL);}
            else if (key == "iops.max") {vol.iops_throttle = strtod (value.c_str(), NULL);}
            else if (key == "relative.priority") {vol.relativePrio = atoi(value.c_str());}
            else if (key == "fsnapshot") {vol.fSnapshot = atoi(value.c_str());}
            else if (key == "state") {vol.setState((fpi::ResourceState) atoi(value.c_str()));}
            else if (key == "parentvolumeid") {vol.srcVolumeId = strtoull(value.c_str(), NULL, 10);} //NOLINT
            else if (key == "create.time") {vol.createTime = strtoull(value.c_str(), NULL, 10);} //NOLINT
            else if (key == "vg.coordinatorinfo.svcuuid") {vol.coordinator.id.svc_uuid = strtoull(value.c_str(), NULL, 10);}
            else if (key == "vg.coordinatorinfo.version") {vol.coordinator.version = atoi(value.c_str());}
            else
            { //NOLINT
                LOGWARN << "unknown key for volume [ " << volumeId << " ] - [ " << key << " ]";

            }
        }

        LOGDEBUG << "volume TYPE [ " << vol.volType << " ] ID [ " << vol.volUUID << " ]";
        if ( vol.volType ==  fpi::FDSP_VOL_ISCSI_TYPE )
        {
            LOGDEBUG << "iSCSI: getting the volume settings for volume ID [ " << volId << " ]";

            auto iscsi = boost::make_shared<fpi::IScsiTarget>( );
            fds::deserializeFdspMsg( ( boost::shared_ptr<std::string> ) getVolumeSettings( vol.volUUID.get() ), iscsi );
            vol.iscsiSettings = *iscsi;

            LOGDEBUG << "iSCSI["
                     << " luns size() == " << vol.iscsiSettings.luns.size()
                     << " initiators size() == " << vol.iscsiSettings.initiators.size()
                     << " incoming users size() == " << vol.iscsiSettings.incomingUsers.size()
                     << " outgoing users size() == " << vol.iscsiSettings.outgoingUsers.size()
                     << " ]";
        }
        else if ( vol.volType ==  fpi::FDSP_VOL_NFS_TYPE )
        {
            LOGDEBUG << "NFS: getting the volume settings for volume ID [ " << volId << " ]";

            auto nfs = boost::make_shared<fpi::NfsOption>( );
            fds::deserializeFdspMsg( ( boost::shared_ptr<std::string> ) getVolumeSettings( vol.volUUID.get() ), nfs );
            vol.nfsSettings = *nfs;

            LOGDEBUG << "NFS["
                     << " clients == " << vol.nfsSettings.client
                     << " options == " << vol.nfsSettings.options
                     << " ]";
        }

        return true;

    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

boost::shared_ptr<std::string> ConfigDB::getVolumeSettings( long unsigned int volumeId )
{
    try
    {
        LOGDEBUG << "Volume settings for volume ID [ " << volumeId << " ] ( GET )";
        Reply reply = kv_store.sendCommand( "get vol:%ld:settings", volumeId );
        if ( !reply.isNil() )
        {
            LOGDEBUG << "Successful retrieved Volume Settings for volume ID [ " << volumeId << " ]";
            return boost::make_shared<std::string>( reply.getString( ) );
        }
    }
    catch(const RedisException& e)
    {
        LOGERROR << "error with Redis: " << e.what();
    }

    LOGDEBUG << "Failed to retrieved Volume Settings for volume ID [ " << volumeId << " ]";
    return nullptr;
}


/******************************************************************************
 *                      Volume QoS Policy Section
 *****************************************************************************/

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
        auto exists = kv_store.sendCommand("exists qos.policy:nextid");
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

            kv_store.sendCommand("set qos.policy:nextid %s", std::to_string(id).c_str());
            LOGDEBUG << "QoS Policy ID generator initialized to " << std::to_string(id);
        }
#endif  /* QOS_POLICY_MANAGEMENT_CORRECTED */

        auto reply = kv_store.sendCommand("incr qos.policy:nextid");

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
        kv_store.sendCommand("sadd qos.policy:list %b", idLower.data(), idLower.length());

        // serialize
        std::string serialized;
        qosPolicy.getSerialized(serialized);

        kv_store.sendCommand("hset qos.policies %ld %b", qosPolicy.volPolicyId, serialized.data(), serialized.length());
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
        Reply reply = kv_store.sendCommand("hvals qos.policies");
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

        Reply reply = kv_store.sendCommand("sismember qos.policy:list %b", idLower.data(), idLower.length());
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

/******************************************************************************
 *                      VolumePolicy Section
 *****************************************************************************/

bool ConfigDB::getPolicy(fds_uint32_t volPolicyId, FDS_VolumePolicy& policy, int localDomain) { //NOLINT
    try{
        Reply reply = kv_store.sendCommand("get %d:volpolicy:%ld", localDomain, volPolicyId);
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

        Reply reply = kv_store.sendCommand("sadd %d:volpolicies %ld", localDomain, policy.volPolicyId);
        if (!reply.wasModified()) {
            LOGWARN << "unable to add policy [" << policy.volPolicyId << " to domain [" << localDomain <<"]"; //NOLINT
        }
        policy.getSerialized(serialized);
        reply = kv_store.sendCommand("set %d:volpolicy:%ld %b", localDomain, policy.volPolicyId, serialized.data(), serialized.length()); //NOLINT
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
        Reply reply = kv_store.sendCommand("del %d:volpolicy:%ld", localDomain, volPolicyId);
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

        Reply reply = kv_store.sendCommand("smembers %d:volpolicies", localDomain);
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

/******************************************************************************
 *                      DLT Section
 *****************************************************************************/

fds_uint64_t ConfigDB::getDltVersionForType(const std::string type, int localDomain) {
    try {
        Reply reply = kv_store.sendCommand("get %d:dlt:%s", localDomain, type.c_str());
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
        Reply reply = kv_store.sendCommand("set %d:dlt:%s %ld", localDomain, type.c_str(), version);
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::storeDlt(const DLT& dlt, const std::string type, int localDomain) {
    try {
        // fds_uint64_t version = dlt.getVersion();
        Reply reply = kv_store.sendCommand("sadd %d:dlts %ld", localDomain, dlt.getVersion());
        if (!reply.wasModified()) {
            LOGWARN << "dlt [" << dlt.getVersion()
                    << "] is already in for domain [" << localDomain << "]";
        }

        std::string serializedData, hexCoded;
        const_cast<DLT&>(dlt).getSerialized(serializedData);
        kv_store.encodeHex(serializedData, hexCoded);

        reply = kv_store.sendCommand("set %d:dlt:%ld %s", localDomain, dlt.getVersion(), hexCoded.c_str());
        bool fSuccess = reply.isOk();

        if (fSuccess && !type.empty()) {
            reply = kv_store.sendCommand("set %d:dlt:%s %ld", localDomain, type.c_str(), dlt.getVersion());
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
        Reply reply = kv_store.sendCommand("get %d:dlt:%ld", localDomain, version);
        std::string serializedData, hexCoded(reply.getString());
        if (hexCoded.length() < 10) {
            LOGERROR << "very less data for dlt : ["
                     << version << "] : size = " << hexCoded.length();
            return false;
        }
        LOGDEBUG << "dlt : [" << version << "] : size = " << hexCoded.length();
        kv_store.decodeHex(hexCoded, serializedData);
        dlt.loadSerialized(serializedData);
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::loadDlts(DLTManager& dltMgr, int localDomain) {
    try {
        Reply reply = kv_store.sendCommand("smembers %d:dlts", localDomain);
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

/******************************************************************************
 *                      DMT Section
 *****************************************************************************/

fds_uint64_t ConfigDB::getDmtVersionForType(const std::string type, int localDomain) {
    try {
        Reply reply = kv_store.sendCommand("get %d:dmt:%s", localDomain, type.c_str());
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
        Reply reply = kv_store.sendCommand("set %d:dmt:%s %ld", localDomain, type.c_str(), version);
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::storeDmt(const DMT& dmt, const std::string type, int localDomain) {
    try {
        // fds_uint64_t version = dmt.getVersion();
        Reply reply = kv_store.sendCommand("sadd %d:dmts %ld", localDomain, dmt.getVersion());
        if (!reply.wasModified()) {
            LOGWARN << "dmt [" << dmt.getVersion()
                    << "] is already in for domain ["
                    << localDomain << "]";
        }

        std::string serializedData, hexCoded;
        const_cast<DMT&>(dmt).getSerialized(serializedData);
        kv_store.encodeHex(serializedData, hexCoded);

        reply = kv_store.sendCommand("set %d:dmt:%ld %s", localDomain, dmt.getVersion(), hexCoded.c_str());
        bool fSuccess = reply.isOk();

        if (fSuccess && !type.empty()) {
            reply = kv_store.sendCommand("set %d:dmt:%s %ld", localDomain, type.c_str(), dmt.getVersion());
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
        Reply reply = kv_store.sendCommand("get %d:dmt:%ld", localDomain, version);
        std::string serializedData, hexCoded(reply.getString());
        if (hexCoded.length() < 10) {
            LOGERROR << "very less data for dmt : ["
                     << version << "] : size = " << hexCoded.length();
            return false;
        }
        LOGDEBUG << "dmt : [" << version << "] : size = " << hexCoded.length();
        kv_store.decodeHex(hexCoded, serializedData);
        dmt.loadSerialized(serializedData);
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

/******************************************************************************
 *                      Node Section
 *****************************************************************************/

bool ConfigDB::addNode(const NodeInfoType& node)
{
    SCOPEDWRITE(nodeLock);

    TRACKMOD();
    // add the volume to the volume list for the domain
    int domainId = 0;  // TODO(prem)
    try {
        bool ret;

        FDSP_SERIALIZE(node, serialized);
        ret = kv_store.sadd(format("%d:cluster:nodes", domainId), node.node_uuid.uuid);
        ret = kv_store.set(format("node:%ld", node.node_uuid.uuid), *serialized);

        return ret;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::updateNode(const NodeInfoType& node)
{
    SCOPEDWRITE(nodeLock);

    try {
        return addNode(node);
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::removeNode(const NodeUuid& uuid)
{
    SCOPEDWRITE(nodeLock);

    TRACKMOD();
    try {
        int domainId = 0;  // TODO(prem)
        Reply reply = kv_store.sendCommand("srem %d:cluster:nodes %ld", domainId, uuid);

        if (!reply.wasModified()) {
            LOGWARN << "node [" << uuid << "] does not exist for domain [" << domainId << "]";
        }

        // Now remove the node data
        reply = kv_store.sendCommand("del node:%ld", uuid);
        return reply.isOk();
    } catch(RedisException& e) {
        LOGERROR << e.what();
        NOMOD();
    }
    return false;
}

bool ConfigDB::getNode(const NodeUuid& uuid, NodeInfoType& node)
{
    SCOPEDREAD(nodeLock);
    try {
        Reply reply = kv_store.get(format("node:%ld", uuid.uuid_get_val()));
        if (reply.isOk()) {
            fds::deserializeFdspMsg(reply.getString(), node);
            return true;
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::nodeExists(const NodeUuid& uuid)
{
    SCOPEDREAD(nodeLock);
    try {
        Reply reply = kv_store.sendCommand("exists node:%ld", uuid);
        return reply.isOk();
    } catch(RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool ConfigDB::getNodeIds(std::unordered_set<NodeUuid, UuidHash>& nodes, int localDomain)
{
    SCOPEDREAD(nodeLock);

    std::vector<long long> nodeIds; //NOLINT

    try {
        Reply reply = kv_store.sendCommand("smembers %d:cluster:nodes", localDomain);
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

bool ConfigDB::getAllNodes(std::vector<NodeInfoType>& nodes, int localDomain)
{
    SCOPEDREAD(nodeLock);

    std::vector<long long> nodeIds; //NOLINT

    try {
        Reply reply = kv_store.sendCommand("smembers %d:cluster:nodes", localDomain);
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

std::string ConfigDB::getNodeName(const NodeUuid& uuid)
{
    SCOPEDREAD(nodeLock);

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

bool ConfigDB::getNodeServices(const NodeUuid& uuid, NodeServices& services)
{
    SCOPEDREAD(nodeLock);
    try{
        Reply reply = kv_store.sendCommand("get %ld:services", uuid);
        if (reply.isNil()) return false;
        services.loadSerialized(reply.getString());
        return true;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

bool ConfigDB::setNodeServices(const NodeUuid& uuid, const NodeServices& services)
{
    SCOPEDWRITE(nodeLock);
    try{
        std::string serialized;
        services.getSerialized(serialized);
        Reply reply = kv_store.sendCommand("set %ld:services %b",
                                    uuid, serialized.data(), serialized.length());
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return false;
}

uint ConfigDB::getNodeNameCounter()
{
    SCOPEDREAD(nodeLock);

    try{
        Reply reply = kv_store.sendCommand("incr node:name.counter");
        return reply.getLong();
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }
    return 0;
}

bool ConfigDB::setCapacityUsedNode( const int64_t svcUuid, const unsigned long usedCapacityInBytes )
{
    SCOPEDWRITE(nodeLock);

    bool bRetCode = false;

    std::stringstream uuid;
    uuid << svcUuid;
    std::stringstream used;
    used << usedCapacityInBytes;

    try
    {
        LOGDEBUG << "Setting used capacity for uuid [ " << std::hex << svcUuid << std::dec << " ]";
        bRetCode = kv_store.hset( "used.capacity", uuid.str( ).c_str( ), used.str( ) );
    }
    catch(const RedisException& e)
    {
        LOGERROR << "Failed to persist node capacity for node [ "
                 << std::hex << svcUuid << std::dec << " ], error: " << e.what();
    };

    return bRetCode;
}

/******************************************************************************
 *                      StatStream Section
 *****************************************************************************/

// stat streaming registrations
int32_t ConfigDB::getNewStreamRegistrationId() {
    try {
        Reply reply = kv_store.sendCommand("incr streamreg:id");
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

        Reply reply = kv_store.sendCommand("sadd streamregs %d", streamReg.id);
        if (!reply.wasModified()) {
            LOGWARN << "unable to add streamreg [" << streamReg.id;
        }

        fds::serializeFdspMsg(streamReg, serialized);

        reply = kv_store.sendCommand("set streamreg:%d %b", streamReg.id,
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
        Reply reply = kv_store.sendCommand("get streamreg:%d", regId);
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
        Reply reply = kv_store.sendCommand("srem streamregs %d", regId);
        if (!reply.wasModified()) {
            LOGWARN << "unable to remove streamreg [" << regId << "] from set"
                    << " mebbe it does not exist";
        }

        reply = kv_store.sendCommand("del streamreg:%d", regId);
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

        Reply reply = kv_store.sendCommand("smembers streamregs");
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



/******************************************************************************
 *                      User Section
 *****************************************************************************/

int64_t ConfigDB::createUser(const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isAdmin) { //NOLINT
    TRACKMOD();
    try {
        // check if the user already exists
        std::string idLower = lower(identifier);

        Reply reply = kv_store.sendCommand("sismember user:list %b", idLower.data(), idLower.length());
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
        reply = kv_store.sendCommand("incr user:nextid");
        user.id = reply.getLong();
        user.identifier = identifier;
        user.passwordHash = passwordHash;
        user.secret = secret;
        user.isFdsAdmin = isAdmin;

        kv_store.sendCommand("sadd user:list %b", idLower.data(), idLower.length());

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(user, serialized);

        kv_store.sendCommand("hset users %ld %b", user.id, serialized->data(), serialized->length()); //NOLINT
        return user.id;
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
    }

    return 0;
}

bool ConfigDB::listUsers(std::vector<fds::apis::User>& users) {
    fds::apis::User user;

    try {
        Reply reply = kv_store.sendCommand("hvals users");
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
        Reply reply = kv_store.sendCommand("hget users %ld", userId);

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
            if (kv_store.sismember("user:list", idLower)) {
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

        kv_store.sadd("user:list", idLower);

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(user, serialized);

        kv_store.hset("users", user.id, *serialized);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}

/******************************************************************************
 *                      Tenant Section
 *****************************************************************************/

int64_t ConfigDB::createTenant(const std::string& identifier) {
    TRACKMOD();
    try {
        // check if the tenant already exists
        std::string idLower = lower(identifier);

        Reply reply = kv_store.sendCommand("sismember tenant:list %b", idLower.data(), idLower.length());
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
        reply = kv_store.sendCommand("incr tenant:nextid");

        fds::apis::Tenant tenant;
        tenant.id = reply.getLong();
        tenant.identifier = identifier;

        kv_store.sendCommand("sadd tenant:list %b", idLower.data(), idLower.length());

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(tenant, serialized);

        kv_store.sendCommand("hset tenants %ld %b", tenant.id, serialized->data(), serialized->length()); //NOLINT
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
        Reply reply = kv_store.sendCommand("hvals tenants");
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

/******************************************************************************
 *                     Users & Tenants Section
 *****************************************************************************/
bool ConfigDB::assignUserToTenant(int64_t userId, int64_t tenantId) {
    TRACKMOD();
    try {
        if ( kv_store.sadd(format("tenant:%ld:users", tenantId), userId)) {
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
        if ( kv_store.srem(format("tenant:%ld:users", tenantId), userId)) {
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
        Reply reply = kv_store.smembers(format("tenant:%ld:users", tenantId));
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

        reply = kv_store.sendCommand(userlist.c_str());
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

/******************************************************************************
 *                      SnapshotPolicy Section
 *****************************************************************************/

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
            if (kv_store.sismember("snapshot.policy:names", idLower)) {
                throw ConfigException("another snapshot policy exists with the name: " + policy.policyName); //NOLINT
            }

            kv_store.sadd("snapshot.policy:names", idLower);
            policy.id = kv_store.incr("snapshot.policy:idcounter");
            LOGDEBUG << "creating a new policy:" <<policy.id;
        }

        // serialize
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(policy, serialized);

        kv_store.hset("snapshot.policies", policy.id, *serialized);
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
        Reply reply = kv_store.hget("snapshot.policies", policyid);
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
        Reply reply = kv_store.sendCommand("hvals snapshot.policies");
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

        kv_store.srem("snapshot.policy:names", nameLower);
        kv_store.hdel("snapshot.policies", policyId);

        Reply reply = kv_store.smembers(format("snapshot.policy:%ld:volumes", policyId));
        std::vector<uint64_t> vecVolumes;
        reply.toVector(vecVolumes);

        // delete this policy id from each volume map
        for (const auto& volumeId : vecVolumes) {
            kv_store.srem(format("volume:%ld:snapshot.policies", volumeId), policyId);
        }

        // delete the volume list for this policy
        kv_store.del(format("snapshot.policy:%ld:volumes", policyId));
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::attachSnapshotPolicy(fds_volid_t const volumeId, const int64_t policyId) {
    try {
        auto volId = volumeId.get();
        kv_store.sendCommand("sadd snapshot.policy:%ld:volumes %ld", policyId, volId);
        kv_store.sendCommand("sadd volume:%ld:snapshot.policies %ld", volId, policyId);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::listSnapshotPoliciesForVolume(std::vector<fds::apis::SnapshotPolicy> & vecPolicies,
                                             fds_volid_t const volumeId) {
    try {
        auto volId = volumeId.get();
        Reply reply = kv_store.sendCommand("smembers volume:%ld:snapshot.policies", volId); //NOLINT
        StringList strings;
        reply.toVector(strings);

        std::string policylist;
        policylist.reserve(2048);
        policylist.append("hmget snapshot.policies");

        for (const auto& value : strings) {
            policylist.append(" ");
            policylist.append(value);
        }

        reply = kv_store.sendCommand(policylist.c_str());
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

bool ConfigDB::detachSnapshotPolicy(fds_volid_t const volumeId, const int64_t policyId) {
    try {
        auto volId = volumeId.get();
        kv_store.sendCommand("srem snapshot.policy:%ld:volumes %ld", policyId, volId);
        kv_store.sendCommand("srem volume:%ld:snapshot.policies %ld", volId, policyId);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool ConfigDB::listVolumesForSnapshotPolicy(std::vector<int64_t> & vecVolumes, const int64_t policyId) { //NOLINT
    try {
        Reply reply = kv_store.sendCommand("smembers snapshot.policy:%ld:volumes", policyId); //NOLINT
        reply.toVector(vecVolumes);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

/******************************************************************************
 *                      Snapshot Section
 *****************************************************************************/

bool ConfigDB::createSnapshot(fpi::Snapshot& snapshot) {
    TRACKMOD();
    try {
        std::string nameLower = lower(snapshot.snapshotName);

        if (kv_store.sismember("snapshot:names", nameLower)) {
            LOGDEBUG << "The specified snapshot ( " 
                     << snapshot.snapshotName << " ) "
                     << "name already exists";
            return false;
                
//            throw ConfigException("another snapshot exists with name:" + 
//              snapshot.snapshotName); //NOLINT
        }

        kv_store.sadd("snapshot:names", nameLower);
        if (snapshot.creationTimestamp <= 1) {
            snapshot.creationTimestamp = fds::util::getTimeStampMillis();
        }

        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(snapshot, serialized);

        kv_store.hset(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId, *serialized); //NOLINT
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}

bool ConfigDB::deleteSnapshot(fds_volid_t const volumeId, fds_volid_t const snapshotId) {
    TRACKMOD();
    try {
        fpi::Snapshot snapshot;
        snapshot.volumeId = volumeId.get();
        snapshot.snapshotId = snapshotId.get();

        if (!getSnapshot(snapshot)) {
            LOGWARN << "unable to fetch snapshot [vol:" << volumeId <<",snap:" << snapshotId <<"]";
            NOMOD();
            return false;
        }

        std::string nameLower = lower(snapshot.snapshotName);
        kv_store.srem("snapshot:names", nameLower);
        kv_store.hdel(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId); //NOLINT
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
        if (! kv_store.hexists(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId)) {
            LOGWARN << "snapshot does not exist : " << snapshot.snapshotId
                    << " vol:" << snapshot.volumeId;
            NOMOD();
            return false;
        }

        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(snapshot, serialized);

        kv_store.hset(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId, *serialized); //NOLINT
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        NOMOD();
        return false;
    }
    return true;
}


bool ConfigDB::listSnapshots(std::vector<fpi::Snapshot> & vecSnapshots, fds_volid_t const volumeId) {
    try {
        auto volId = volumeId.get();
        Reply reply = kv_store.sendCommand("hvals volume:%ld:snapshots", volId);
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
        Reply reply = kv_store.hget(format("volume:%ld:snapshots", snapshot.volumeId), snapshot.snapshotId); //NOLINT
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

bool ConfigDB::setSnapshotState(fds_volid_t const volumeId, fds_volid_t const snapshotId,
                                fpi::ResourceState state) {
    fpi::Snapshot snapshot;
    snapshot.volumeId = volumeId.get();
    snapshot.snapshotId = snapshotId.get();
    return setSnapshotState(snapshot, state);
}

/******************************************************************************
 *                      Service Map Section
 *****************************************************************************/

bool ConfigDB::deleteSvcMap(const fpi::SvcInfo& svcinfo)
{
    SCOPEDWRITE(svcMapLock);
    try {
        std::stringstream uuid;
        uuid << svcinfo.svc_id.svc_uuid.svc_uuid;
        
        Reply reply = kv_store.sendCommand( "hdel svcmap %s", uuid.str().c_str() ); //NOLINT
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

/*
 * !! WARNING !! : Do NOT, on any account call the below two functions
 * (updateSvcMap, changeStateSvcMap) directly.
 * If a change to a svc state in configDB must be made, go through
 * the ::updateSvcMaps function in omutils.cpp
 * That function ensures state is changed consistently across configDB
 * and svcLayer
 */

bool ConfigDB::updateSvcMap(const fpi::SvcInfo& svcinfo)
{
    SCOPEDWRITE(svcMapLock);

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
                 << " status: " << DltDmtUtil::getInstance()->printSvcStatus(svcinfo.svc_status);
                
        FDSP_SERIALIZE( svcinfo, serialized );        
        bRetCode = kv_store.hset( "svcmap", uuid.str().c_str(), *serialized );
                       
    } catch(const RedisException& e) {
        
        LOGCRITICAL << "error with redis " << e.what();
        
    }
    
    return bRetCode;
}

bool ConfigDB::changeStateSvcMap( fpi::SvcInfoPtr svcInfoPtr)
{
    SCOPEDWRITE(svcMapLock);
    bool bRetCode = false;

    try
    {
        fpi::SvcInfo dbInfo;
        bool ret = getSvcInfo(svcInfoPtr->svc_id.svc_uuid.svc_uuid, dbInfo);

        bRetCode = updateSvcMap( *svcInfoPtr );

        if (ret)
        {
            LOGNOTIFY << "ConfigDB updated service status:"
                       << " uuid: " << std::hex << svcInfoPtr->svc_id.svc_uuid << std::dec
                       << " from status: " << DltDmtUtil::getInstance()->printSvcStatus(dbInfo.svc_status)
                       << " to status: " << DltDmtUtil::getInstance()->printSvcStatus(svcInfoPtr->svc_status);
        } else {
            LOGNOTIFY << "ConfigDB updated map with new record:"
                       << " uuid: " << std::hex << svcInfoPtr->svc_id.svc_uuid << std::dec
                       << "  status: " << DltDmtUtil::getInstance()->printSvcStatus(svcInfoPtr->svc_status);
        }

        /* Convert new registration request to existing registration request */
        kvstore::NodeInfoType nodeInfo;
        fromTo( *svcInfoPtr, nodeInfo );
        updateNode( nodeInfo );

    }
    catch( const RedisException& e )
    {
        LOGCRITICAL << "error with redis " << e.what();
    }

    return bRetCode;
}

bool ConfigDB::getSvcInfo(const fds_uint64_t svc_uuid, fpi::SvcInfo& svcInfo)
{
    SCOPEDREAD(svcMapLock);
    bool result = false;
    try
    {
        std::stringstream uuid;
        uuid << svc_uuid;

        LOGDEBUG << "ConfigDB retrieving svcInfo for"
                 << " uuid: " << std::hex << svc_uuid << std::dec;

        Reply reply = kv_store.hget( "svcmap", uuid.str().c_str() ); //NOLINT

        /*
         * the reply.isValid() always == true, because its not NULL
         */
        if ( !reply.isNil() )
        {
            std::string value = reply.getString();
            fds::deserializeFdspMsg( value, svcInfo );

            result = true;

        } else {
            LOGDEBUG << "Retrieved a nil value from configDB for uuid"
                     << std::hex << svc_uuid << std::dec;
        }
    }
    catch( const RedisException& e )
    {
        LOGCRITICAL << "error with redis " << e.what();
    }

    return result;
}



//
// If service not found in configDB, returns SVC_STATUS_INVALID
//
fpi::ServiceStatus ConfigDB::getStateSvcMap( const int64_t svc_uuid )
{
    SCOPEDREAD(svcMapLock);
    fpi::ServiceStatus retStatus = fpi::SVC_STATUS_INVALID;

    try
    {
        std::stringstream uuid;
        uuid << svc_uuid;

        LOGDEBUG << "ConfigDB reading service status for service"
                 << " uuid: " << std::hex << svc_uuid << std::dec;

        Reply reply = kv_store.hget( "svcmap", uuid.str().c_str() ); //NOLINT
        if ( reply.isValid() )
        {
            std::string value = reply.getString();
            fpi::SvcInfo svcInfo;
            fds::deserializeFdspMsg( value, svcInfo );

            // got the status!
            retStatus = svcInfo.svc_status;

            LOGDEBUG << "ConfigDB retrieved service status for service"
                     << " uuid: " << std::hex << svc_uuid << std::dec
                     << " status: " << DltDmtUtil::getInstance()->printSvcStatus(retStatus);
        }
    }
    catch( const RedisException& e )
    {
        LOGCRITICAL << "error with redis " << e.what();
    }

    return retStatus;
}

bool ConfigDB::isPresentInSvcMap( const int64_t svc_uuid )
{
    SCOPEDREAD(svcMapLock);

    bool isPresent = false;
    try
    {
        std::stringstream uuid;
        uuid << svc_uuid;

        LOGDEBUG << "ConfigDB getting service"
                 << " uuid: " << std::hex << svc_uuid << std::dec;

        Reply reply = kv_store.hget( "svcmap", uuid.str().c_str() ); //NOLINT

        /*
         * the reply.isValid() always == true, because its not NULL
         */
        if ( !reply.isNil() )
        {
            std::string value = reply.getString();
            fpi::SvcInfo svcInfo;
            fds::deserializeFdspMsg( value, svcInfo );

            int64_t id = svcInfo.svc_id.svc_uuid.svc_uuid;

            if ( id != 0 ) {
                isPresent = true;
            } else {
               isPresent = false;
            }
        } else {
            LOGDEBUG << "Retrieved a nil value from configDB for uuid"
                     << std::hex << svc_uuid << std::dec;
        }
    }
    catch( const RedisException& e )
    {
        LOGCRITICAL << "error with redis " << e.what();
    }

    return isPresent;
}

bool ConfigDB::getSvcMap(std::vector<fpi::SvcInfo>& svcMap)
{
    SCOPEDREAD(svcMapLock);

    try 
    {
        svcMap.clear();
        Reply reply = kv_store.sendCommand("hgetall svcmap");
        StringList strings;
        reply.toVector(strings);

        for (uint i = 0; i < strings.size(); i+= 2) {
            fpi::SvcInfo svcInfo;
            std::string& value = strings[i+1];

            fds::deserializeFdspMsg(value, svcInfo);
            if( ( svcInfo.svc_type == FDS_ProtocolInterface::FDSP_PLATFORM ) ||
                ( svcInfo.svc_type == FDS_ProtocolInterface::FDSP_STOR_MGR ) ||
                ( svcInfo.svc_type == FDS_ProtocolInterface::FDSP_DATA_MGR ) ||
                ( svcInfo.svc_type == FDS_ProtocolInterface::FDSP_ACCESS_MGR ) ||
                ( svcInfo.svc_type == FDS_ProtocolInterface::FDSP_ORCH_MGR ) )
            {
                svcMap.push_back( svcInfo );
            }
        }
    } 
    catch ( const RedisException& e ) 
    {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    
    return true;
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

/******************************************************************************
 *                      Subscriptions Section
 *****************************************************************************/

// Note that subscriptions are global domain objects. While they might be created from any given
// local domain or cached in any given local domain, they are definitively stored and maintained
// in the context of the global domain and therefore, the master domain and master OM.
//
// TODO(Greg): These methods need to ensure that they are being executed within the context of the
// master OM.
fds_subid_t ConfigDB::getNewSubscriptionId() {
    try {
        fds_subid_t subId;
        ReturnType ret;
        for (;;) {
            subId = fds_subid_t(static_cast<uint64_t>(kv_store.incr("subscriptions:nextid")));
            if ((ret = subscriptionExists(subId)) == ReturnType::NOT_FOUND) {
                // We haven't used this one yet.
                return subId;
            } else if (ret == ReturnType::CONFIGDB_EXCEPTION) {
                // Some ConfigDB problem.
                break;
            }
        }
    } catch(const RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return invalid_sub_id;
}

/**
 * If isNew is true, then we are being asked to add a new subscriptions.
 *
 * If isNew is false, then we are being asked to replace the details of an existing
 * subscription.
 */
fds_subid_t ConfigDB::putSubscription(const Subscription &subscription, const bool isNew) {
    TRACKMOD();
    // Add the subscription to the subscription list for the global domain
    try {
        ReturnType ret;

        // Check if the subscription already exists.
        if ((ret = subscriptionExists(subscription.getName(), subscription.getTenantID())) == ReturnType::SUCCESS) {
            if (isNew) {
                LOGWARN << "Trying to add an existing subscription: Subscription [" << subscription.getName() <<
                           "], Tenant [" << subscription.getTenantID() << "].";
                NOMOD();
                return invalid_sub_id;
            }
        } else if (ret == ReturnType::NOT_FOUND) {
            if (!isNew) {
                LOGWARN << "Trying to udpate a non-existing subscription: Subscription [" << subscription.getName() <<
                        "], Tenant [" << subscription.getTenantID() << "].";
                NOMOD();
                return invalid_sub_id;
            }
        } else {
            // We had some problem with ConfigDB.
            NOMOD();
            return invalid_sub_id;
        }

        // Get new or existing ID
        fds_subid_t subId{invalid_sub_id};
        if (isNew) {
            subId = getNewSubscriptionId();
            LOGDEBUG << "Adding subscription [" << subscription.getName() << "] for tenant [" <<
                            subscription.getTenantID() << "], ID [" << subId << "].";

            auto reply = kv_store.sendCommand("sadd subscriptions %ld", subId);
            reply.checkError();
        } else {
            subId = getSubscriptionId(subscription.getName(), subscription.getTenantID());
            LOGDEBUG << "Updating subscription [" << subscription.getName() << "] for tenant [" <<
                     subscription.getTenantID() << "], ID [" << subId << "].";
        }

        // Add the subscription
        auto reply = kv_store.sendCommand("hmset sub:%ld id %ld"
                                          " name %s"
                                          " tennantID %d"
                                          " primaryDomainID %d"
                                          " primaryVolumeID %d"
                                          " replicaDomainID %d"
                                          " createTime %ld"
                                          " state %d"
                                          " type %d"
                                          " scheduleType %d"
                                          " intervalSize %d",
                                          subId, subId,
                                          subscription.getName().c_str(),
                                          subscription.getTenantID(),
                                          subscription.getPrimaryDomainID(),
                                          subscription.getPrimaryVolumeID(),
                                          subscription.getReplicaDomainID(),
                                          (isNew) ? util::getTimeStampMillis() : subscription.getCreateTime(),
                                          (isNew) ? FDS_ProtocolInterface::ResourceState::Created : subscription.getState(),
                                          subscription.getType(),
                                          subscription.getScheduleType(),
                                          subscription.getIntervalSize());
        reply.checkError();

        return subId;
    } catch(const RedisException& e) {
        NOMOD();
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }

    return invalid_sub_id;
}

ConfigDB::ReturnType ConfigDB::setSubscriptionState(fds_subid_t subId, fpi::ResourceState state) {
    if (!subscriptionExists(subId)) {
        return ReturnType::NOT_FOUND;
    }

    TRACKMOD();

    try {
        LOGDEBUG << "Updating subscription id [" << subId << "] to state [" << state << "].";

        Reply reply = kv_store.sendCommand("hset sub:%ld state %d", subId, static_cast<int>(state));

        if (!(reply.isOk())) {
            LOGERROR << "Redis error: " << reply.getString();
            return ReturnType::NOT_UPDATED;
        }

        return ReturnType::SUCCESS;
    } catch(const RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
        NOMOD();
    }

    return ReturnType::CONFIGDB_EXCEPTION;
}

ConfigDB::ReturnType ConfigDB::updateSubscription(const Subscription& subscription) {
    /**
     * Subscription ID is required.
     */
    if (subscription.getID() == invalid_sub_id) {
        LOGERROR << "Subscription ID must be supplied for an update of subscription [" << subscription.getName() <<
                        "] and tenant [" << subscription.getTenantID() << "].";
        return ReturnType::NOT_UPDATED;
    } else {
        /**
         * The following are *not* modifiable: id, tenantID, primaryDomainID, primaryVolumeID, replicaDomainID, createTime
         */
        Subscription currentSub("dummy", invalid_sub_id);

        ReturnType ret = getSubscription(subscription.getID(), currentSub);
        if (ret == ReturnType::CONFIGDB_EXCEPTION) {
            LOGCRITICAL << "Unable to perform subscription update for subscription ID [" << subscription.getID() << "].";
            return ret;
        } else if (ret == ReturnType::NOT_FOUND) {
            LOGERROR << "Subscription to update not found for subscription ID [" << subscription.getID() << "].";
            return ret;
        } else if ((currentSub.getTenantID() != subscription.getTenantID()) ||
                   (currentSub.getPrimaryDomainID() != subscription.getPrimaryDomainID()) ||
                   (currentSub.getPrimaryVolumeID() != subscription.getPrimaryVolumeID()) ||
                   (currentSub.getReplicaDomainID() != subscription.getReplicaDomainID()) ||
                   (currentSub.getCreateTime() != subscription.getCreateTime())) {
            LOGERROR << "Subscription updates may not change ID, Tenant ID, Primary Domain ID, Primary Volume ID "
                        "Replica Domain ID, or Create Time for subscription ID [" << subscription.getID() << "].";
            return ReturnType::NOT_UPDATED;
        }
    }

    return ((putSubscription(subscription, false /* Not a new subscription. */) == invalid_sub_id) ?
             ReturnType::NOT_UPDATED : ReturnType::SUCCESS);
}

ConfigDB::ReturnType ConfigDB::deleteSubscription(fds_subid_t subId) {
    if (!subscriptionExists(subId)) {
        return ReturnType::NOT_FOUND;
    }

    TRACKMOD();

    try {
        Reply reply = kv_store.sendCommand("srem subscriptions %ld", subId);
        if (!reply.wasModified()) {
            LOGWARN << "Subscription [" << subId << "] not removed.";
            NOMOD();
            return ReturnType::NOT_UPDATED;
        }

        // Delete the subscription.
        reply = kv_store.sendCommand("del sub:%ld", subId);

        if (!reply.wasModified()) {
            LOGWARN << "Subscription [" << subId << "] not removed.";
            return ReturnType::NOT_UPDATED;
        } else {
            return ReturnType::SUCCESS;
        }
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
        NOMOD();
    }

    return ReturnType::CONFIGDB_EXCEPTION;
}

// Check by subscription ID.
ConfigDB::ReturnType ConfigDB::subscriptionExists(fds_subid_t subId) {
    try {
        Reply reply = kv_store.sendCommand("exists sub:%ld", subId);
        return (reply.getLong() == 1) ? ReturnType::SUCCESS : ReturnType::NOT_FOUND;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return ReturnType::CONFIGDB_EXCEPTION;
}

// Check by subscription name which is unique per tenant.
ConfigDB::ReturnType ConfigDB::subscriptionExists(const std::string& subName, std::int64_t tenantId)
{
    std::vector<Subscription> subscriptions;
    ReturnType ret;

    if ((ret = getSubscriptions(subscriptions)) == ReturnType::SUCCESS) {
        for (const auto subscription : subscriptions) {
            if ((subscription.getTenantID() == tenantId) &&
                    (subscription.getName().compare(subName) == 0)) {
                return ReturnType::SUCCESS;
            }
        }

        // Didn't find it.
        return ReturnType::NOT_FOUND;
    }

    // Either no subscriptions or a problem with ConfigDB access.
    return ret;
}

ConfigDB::ReturnType ConfigDB::getSubscriptionIds(std::vector<fds_subid_t>& subIds) {
    std::vector<fds_subid_t> _subIds; //NOLINT

    try {
        Reply reply = kv_store.sendCommand("smembers subscriptions");
        reply.toVector(_subIds);

        if (_subIds.empty()) {
            LOGDEBUG << "No subscriptions found.";
            return ReturnType::NOT_FOUND;
        }

        for (std::size_t i = 0; i < _subIds.size(); i++) {
            subIds.push_back(fds_subid_t(_subIds[i]));
        }
        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return ReturnType::CONFIGDB_EXCEPTION;
}

// Get subscription ID from its name.
fds_subid_t ConfigDB::getSubscriptionId(const std::string& name, const std::int64_t tenantId)
{
    Subscription subscription(name, invalid_sub_id);

    if (getSubscription(name, tenantId, subscription) == ReturnType::SUCCESS)
    {
        return subscription.getID();
    }
    else {
        return invalid_sub_id;
    }
}

ConfigDB::ReturnType ConfigDB::getSubscriptions(std::vector<Subscription>& subscriptions) {
    std::vector<fds_subid_t> subIds; //NOLINT

    try {
        Reply reply = kv_store.sendCommand("smembers subscriptions");
        reply.toVector(subIds);

        if (subIds.empty()) {
            LOGDEBUG << "No subscriptions found.";
            return ReturnType::NOT_FOUND;
        }

        ReturnType ret;
        for (std::size_t i = 0; i < subIds.size(); i++) {
            Subscription subscription("dummy" , invalid_sub_id);
            if ((ret = getSubscription(fds_subid_t(subIds[i]), subscription)) == ReturnType::NOT_FOUND) {
                LOGCRITICAL << "ConfigDB inconsistency. Subscription detail missing for id [" << subIds[i] << "].";
                return ReturnType::CONFIGDB_EXCEPTION;
            } else if (ret == ReturnType::CONFIGDB_EXCEPTION) {
                // Some ConfigDB error.
                return ReturnType::CONFIGDB_EXCEPTION;
            }

            subscriptions.push_back(subscription);
        }
        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return ReturnType::CONFIGDB_EXCEPTION;
}

ConfigDB::ReturnType ConfigDB::getTenantSubscriptions(std::int64_t tenantId, std::vector<Subscription>& subscriptions) {
    std::vector<fds_subid_t> subIds; //NOLINT

    subscriptions.empty();

    try {
        Reply reply = kv_store.sendCommand("smembers subscriptions");
        reply.toVector(subIds);

        if (subIds.empty()) {
            LOGDEBUG << "No subscriptions found.";
            return ReturnType::NOT_FOUND;
        }

        ReturnType ret;
        for (std::size_t i = 0; i < subIds.size(); i++) {
            Subscription subscription("dummy" , invalid_sub_id);

            if ((ret = getSubscription(fds_subid_t(subIds[i]), subscription)) == ReturnType::SUCCESS) {
                if (subscription.getTenantID() == tenantId) {
                    subscriptions.push_back(subscription);
                }
            } else if (ret == ReturnType::NOT_FOUND) {
                LOGCRITICAL << "ConfigDB inconsistency. Subscription detail missing for id [" << subIds[i] << "].";
                return ReturnType::CONFIGDB_EXCEPTION;
            } else if (ret == ReturnType::CONFIGDB_EXCEPTION) {
                // Some ConfigDB error.
                return ReturnType::CONFIGDB_EXCEPTION;
            }
        }

        if (subscriptions.empty()) {
            LOGDEBUG << "No subscriptions found for tenant " << tenantId << ".";
            return ReturnType::NOT_FOUND;
        }

        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }

    return ReturnType::CONFIGDB_EXCEPTION;
}

ConfigDB::ReturnType ConfigDB::getSubscription(fds_subid_t subId, Subscription& subscription) {
    try {
        Reply reply = kv_store.sendCommand("hgetall sub:%ld", subId);
        StringList strings;
        reply.toVector(strings);

        if (strings.empty()) {
            LOGDEBUG << "Unable to find subscription ID [" << subId << "].";
            return ReturnType::NOT_FOUND;
        }

        // Since we expect complete key-value pairs, the size of strings should be even.
        if ((strings.size() % 2) != 0) {
            LOGCRITICAL << "ConfigDB corruption. Subscription ID [" << subId <<
                            "] does not have a complete key-value pairing of attributes.";
            return ReturnType::CONFIGDB_EXCEPTION;
        }

        for (std::size_t i = 0; i < strings.size(); i+= 2) {
            std::string& key = strings[i];
            std::string& value = strings[i+1];

            if (key == "id") {subscription.setID(fds_subid_t(strtoull(value.c_str(), NULL, 10)));}
            else if (key == "name") { subscription.setName(value); }
            else if (key == "tennantId") { subscription.setTenantID(atoi(value.c_str())); }
            else if (key == "primaryDomainID") { subscription.setPrimaryDomainID(atoi(value.c_str()));}
            else if (key == "primaryVolumeID") { subscription.setPrimaryVolumeID(fds_volid_t(strtoull(value.c_str(), NULL, 10)));} //NOLINT
            else if (key == "replicaDomainID") { subscription.setReplicaDomainID(atoi(value.c_str()));}
            else if (key == "createTime") { subscription.setCreateTime(strtoull(value.c_str(), NULL, 10));} //NOLINT
            else if (key == "state") { subscription.setState((fpi::ResourceState) atoi(value.c_str()));}
            else if (key == "type") { subscription.setType((fds::apis::SubscriptionType)atoi(value.c_str())); }
            else if (key == "scheduleType") { subscription.setScheduleType((fds::apis::SubscriptionScheduleType)atoi(value.c_str())); }
            else if (key == "intervalSize") { subscription.setIntervalSize(strtoull(value.c_str(), NULL, 10));} //NOLINT
            else { //NOLINT
                LOGWARN << "ConfigDB::getSubscription unknown key for subscription [" << subId <<
                                "] - key [" << key << "], value [" << value << "].";
                fds_assert(!"ConfigDB::getSubscription unknown key");
            }
        }
        return ReturnType::SUCCESS;
    } catch(RedisException& e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }

    return ReturnType::CONFIGDB_EXCEPTION;
}

ConfigDB::ReturnType ConfigDB::getSubscription(const std::string& name, const std::int64_t tenantId, Subscription& subscription) {
    try {
        std::vector<Subscription> subscriptions;
        std::vector<Subscription>::const_iterator subIterator;
        ReturnType ret;

        if ((ret = getSubscriptions(subscriptions)) == ReturnType::SUCCESS) {
            for (subIterator = subscriptions.begin(); subIterator != subscriptions.end(); subIterator++) {
                if ((subIterator->getName().compare(name) == 0) &&
                    (subIterator->getTenantID() == tenantId)) {
                    break;
                }
            }
        } else if (ret == ReturnType::CONFIGDB_EXCEPTION) {
            // We had some ConfigDB error.
            return ReturnType::CONFIGDB_EXCEPTION;
        }

        if (subIterator == subscriptions.end()) {
            LOGDEBUG << "Unable to find subscription [" << name << "]" <<
                     " for tenant [" << tenantId << "].";
            return ReturnType::NOT_FOUND;
        }

        subscription = *subIterator;
        return ReturnType::SUCCESS;
    } catch (RedisException &e) {
        LOGCRITICAL << "Exception with Redis: " << e.what();
    }
    return ReturnType::CONFIGDB_EXCEPTION;
}

}  // namespace kvstore
}  // namespace fds
