/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_

#include <string>
#include <fds_module.h>
#include <fds_types.h>
#include <ObjMeta.h>
#include <persistent-layer/dm_io.h>
#include <object-store/ObjectStoreCommon.h>
#include <object-store/ObjectDataCache.h>
#include <object-store/ObjectPersistData.h>

namespace fds {

class SmScavengerCmd;

/**
 * Class that manages storage of object data. The class persistetnly stores
 * and caches object data.
 */
class ObjectDataStore : public Module, public boost::noncopyable {
  private:
    /**
     * Disk storage manager
     */
    ObjectPersistData::unique_ptr persistData;

    /**
     * Object data cache manager
     */
    ObjectDataCache::unique_ptr dataCache;

    // TODO(Andrew): Add some private GC interfaces here?

    enum ObjectDataStoreState {
        DATA_STORE_INITING,
        DATA_STORE_INITED,
        DATA_STORE_UNAVAILABLE
    };

    std::atomic<ObjectDataStoreState> currentState;

public:
    ObjectDataStore(const std::string &modName,
                    SmIoReqHandler *data_store,
                    UpdateMediaTrackerFnObj obj=UpdateMediaTrackerFnObj(),
                    EvaluateObjSetFn evalFn=EvaluateObjSetFn());
    ~ObjectDataStore();
    typedef std::unique_ptr<ObjectDataStore> unique_ptr;
    typedef std::shared_ptr<ObjectDataStore> ptr;

    inline bool isUp() const {
        return (currentState.load() == DATA_STORE_INITED);
    }

    inline bool isUnavailable() const {
        return (currentState.load() == DATA_STORE_UNAVAILABLE);
    }

    inline bool isInitializing() const {
        return (currentState.load() == DATA_STORE_INITING);
    }

    /**
     * Opens object data store for all SM tokens that this SM owns
     * Token ownership is specified in the disk map
     * @param[in] map of SM tokens to disks
     * @param[in] true if SM comes up for the first time
     */
    Error openDataStore(SmDiskMap::ptr& diskMap,
                        fds_bool_t pristineState);

    /**
     * Opens object data store for given SM tokens; the tokens
     * may not be in a disk map;
     * This method is called when SM needs to open data store for
     * SM tokens for which it is gaining ownership, but migration
     * may fail, so disk map is updated only on successful migration
     * @param[in] diskMap map of SM tokens to disks
     * @param[in] smToks set of SM tokens to open data store for
     * @param[in] true if SM comes up for the first time
     */
    Error openDataStore(SmDiskMap::ptr& diskMap,
                        const SmTokenSet& smToks,
                        fds_bool_t pristineState);

    /**
     * Closes and deletes object data files for given SM tokens
     * @param[in] set of SM tokens for which this SM lost ownership
     */
    Error closeAndDeleteSmTokensStore(const SmTokenSet& smTokensLost,
                                      const bool& diskFailure=false);

    /**
     * Deletes SM token file for a given SM Token.
     */
    Error deleteObjectDataFile(const std::string& diskPath,
                               const fds_token_id& smToken,
                               const fds_uint16_t& diskId);

    /**
     * Peristently stores object data.
     */
    Error putObjectData(fds_volid_t volId,
                        const ObjectID &objId,
                        diskio::DataTier tier,
                        boost::shared_ptr<const std::string>& objData,
                        obj_phy_loc_t& objPhyLoc);

    /**
     * Reads object data.
     */
    boost::shared_ptr<const std::string> getObjectData(fds_volid_t volId,
                                                       const ObjectID &objId,
                                                       ObjMetaData::const_ptr objMetaData,
                                                       Error &err,
                                                       diskio::DataTier *tier=nullptr);

    /**
     * Removes object from cache and notifies persistent layer
     * about that we deleted the object (to keep track of disk space
     * we need to clean for garbage collection)
     * Called when ref count goes to zero
     */
    Error removeObjectData(fds_volid_t volId,
                           const ObjectID& objId,
                           const ObjMetaData::const_ptr& objMetaData);

    // control commands
    Error scavengerControlCmd(SmScavengerCmd* scavCmd);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_
