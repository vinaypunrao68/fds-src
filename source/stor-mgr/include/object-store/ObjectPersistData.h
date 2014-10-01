/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTPERSISTDATA_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTPERSISTDATA_H_

#include <string>
#include <fds_module.h>
#include <fds_types.h>
#include <persistent_layer/dm_io.h>
#include <persistent_layer/tokFileMgr.h>
#include <object-store/SmDiskMap.h>

namespace fds {


/**
 * Class that manages persistent storage of object data.
 */
class ObjectPersistData : public Module, public boost::noncopyable {
  private:
    diskio::tokenFileDb::unique_ptr tokFileContainer;
    SmDiskMap::const_ptr smDiskMap;

  public:
    explicit ObjectPersistData(const std::string &modName);
    ~ObjectPersistData();

    typedef std::unique_ptr<ObjectPersistData> unique_ptr;

    /**
     * Opens object data files
     * @param[in] diskMap map of SM tokens to disks
     */
    Error openPersistDataStore(const SmDiskMap::const_ptr& diskMap);

    /**
     * Peristently stores object data.
     */
    Error writeObjectData(const ObjectID& objId,
                          diskio::DiskRequest* req);

    /**
     * Reads object data from persistent layer
     */
    Error readObjectData(const ObjectID& objId,
                         diskio::DiskRequest* req);

    /**
     * Notify that object was deleted to update stats for garbage collection
     * TODO(Anna) review this when revisiting GC
     */
    void notifyDataDeleted(const ObjectID& objId,
                           fds_uint32_t objSize,
                           const obj_phy_loc_t* loc);

    /**
     * Notify about start garbage collection for given token id 'tok_id'
     * and tier. If many disks contain this token, then token file on each
     * disk will be compacted. All new writes will be re-routed to the
     * appropriate (new) token file.
     */
    void notifyStartGc(fds_token_id smTokId,
                       fds_uint16_t diskId,
                       diskio::DataTier tier);

    /**
     * Notify about end of garbage collection for a given token id
     * 'tok_id' and tier.
     */
    Error notifyEndGc(fds_token_id smTokId,
                      fds_uint16_t diskId,
                      diskio::DataTier tier);

    /**
     * Returns true if a given location is a shadow file
     */
    fds_bool_t isShadowLocation(obj_phy_loc_t* loc,
                                fds_token_id smTokId);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTPERSISTDATA_H_
