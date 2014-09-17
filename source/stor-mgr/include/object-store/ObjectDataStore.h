/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_

#include <string>
#include <fds_module.h>
#include <fds_types.h>
#include <ObjMeta.h>
#include <object-store/ObjectDataCache.h>
#include <persistent_layer/dm_io.h>
#include <SmDiskTypes.h>

namespace fds {

/**
 * Class that manages storage of object data. The class persistetnly stores
 * and caches object data.
 */
class ObjectDataStore : public Module, public boost::noncopyable {
  private:
    /// Disk storage manager
    diskio::DataIO *diskMgr;

    /// Object data cache manager
    ObjectDataCache::unique_ptr dataCache;

    // TODO(Andrew): Add some private GC interfaces here?

  public:
    explicit ObjectDataStore(const std::string &modName);
    ~ObjectDataStore();
    typedef std::unique_ptr<ObjectDataStore> unique_ptr;

    /**
     * Peristently stores object data.
     */
    Error putObjectData(fds_volid_t volId,
                        const ObjectID &objId,
                        diskio::DataTier tier,
                        boost::shared_ptr<const std::string> objData,
                        obj_phy_loc_t& objPhyLoc);

    /**
     * Reads object data.
     */
    boost::shared_ptr<const std::string> getObjectData(fds_volid_t volId,
                                                       const ObjectID &objId,
                                                       ObjMetaData::const_ptr objMetaData,
                                                       Error &err);

    /**
     * Removes object from cache and notifies persistent layer
     * about that we deleted the object (to keep track of disk space
     * we need to clean for garbage collection)
     * Called when ref count goes to zero
     */
    Error removeObjectData(const ObjectID& objId,
                           const ObjMetaData::const_ptr& objMetaData);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_
