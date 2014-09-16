/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETACACHE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETACACHE_H_

#include <string>
#include <fds_module.h>
#include <fds_types.h>
#include <cache/SharedKvCache.h>
#include <object-store/ObjectMetaDb.h>
#include <SmDiskTypes.h>

namespace fds {

/**
 * Class that manages caching of object metadata.
 */
class ObjectMetaCache : public Module, public boost::noncopyable {
  private:
    /// Backing cache structure
    typedef SharedKvCache<ObjectID, const ObjMetaData, ObjectHash> MetadataCache;
    std::unique_ptr<MetadataCache> metaCache;

    /// Max total number of entries
    fds_uint32_t maxEntries;

  public:
    explicit ObjectMetaCache(const std::string &modName);
    ~ObjectMetaCache();
    typedef std::unique_ptr<ObjectMetaCache> unique_ptr;

    /**
     * Caches object data. The cache will retain a reference to the
     * metadata being put.
     */
    void putObjectMetadata(fds_volid_t volId,
                           const ObjectID &objId,
                           ObjMetaData::const_ptr objMeta);

    /**
     * Reads object data from the cache. The object data is returned
     * via a parameter, which is expected to be an unitialized shared_ptr.
     * The cache structure will set the shared_ptr to point to the data
     * in the cache. The data being pointed to is still owned by the
     * cache not the caller.
     */
    ObjMetaData::const_ptr getObjectMetadata(fds_volid_t volId,
                                             const ObjectID &objId,
                                             Error &err);

    /**
     * Removes object metadata from the cache.
     */
    void removeObjectMetadata(fds_volid_t volId,
                              const ObjectID &objId);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETACACHE_H_
