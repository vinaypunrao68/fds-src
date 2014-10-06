/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETADATASTORE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETADATASTORE_H_

#include <string>
#include <fds_module.h>
#include <SmIo.h>
#include <object-store/ObjectMetaCache.h>
#include <object-store/ObjectMetaDb.h>

namespace fds {

/*
 * ObjectMetadataStore is a store for object metadata, which may contain object
 * metadata cache and object metadata DB.
 * This class asumes that all accesses to the same object ID are serialized
 * at the level above, thus only once access to the same object ID is active
 * at the same time at ObjectMetadataStore.
 */
class ObjectMetadataStore : public Module, public boost::noncopyable {
  public:
    explicit ObjectMetadataStore(const std::string& modName);
    ~ObjectMetadataStore();

    typedef std::unique_ptr<ObjectMetadataStore> unique_ptr;

    /**
     * Set number of bits per (global) token
     * This is used for retrieving SM token from object ID
     */
    void setNumBitsPerToken(fds_uint32_t nbits);

    /**
     * Opens object metadata store
     * @param[in] map of SM tokens to disks
     */
    Error openMetadataStore(const SmDiskMap::const_ptr& diskMap);

    /**
     * Retrieves metadata for given object with ID 'objId'
     * @return ERR_OK on success
     */
    ObjMetaData::const_ptr getObjectMetadata(fds_volid_t volId,
                                             const ObjectID& objId,
                                             Error &err);

    /**
     * Persistently stores metadata of object with id 'objId'
     * @param volId volume ID of the object; There maybe cases
     * when volume ID is not known (eg., current implementation of
     * write back thread does not have knowledge of volume IDs), in that
     * case should pass invalid_vol_id
     * @return ERR_OK on success 
     */
    Error putObjectMetadata(fds_volid_t volId,
                            const ObjectID& objId,
                            ObjMetaData::const_ptr objMeta);


    /**
     * Removes object metadata from persistent store and cache
     * This method should not be used for marking object as deleted,
     * this method actually deletes object metadata entry from the DB.
     */
    Error removeObjectMetadata(fds_volid_t volId,
                               const ObjectID& objId);

    /**
     * Make a snapshot of metadata of given SM token and
     * calls notifFn method
     */
    void snapshot(fds_token_id smTokId,
                  SmIoSnapshotObjectDB::CbType notifFn);

    /**
     * Module methods
     */
    virtual int mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  private:
    /// object metadata index db
    ObjectMetadataDb::unique_ptr metaDb_;

    /// Metadata cache
    ObjectMetaCache::unique_ptr metaCache;
};


}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETADATASTORE_H_
