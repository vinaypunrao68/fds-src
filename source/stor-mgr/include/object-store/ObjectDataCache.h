/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATACACHE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATACACHE_H_

#include <string>
#include <fds_module.h>
#include <fds_types.h>
#include <cache/SharedKvCache.h>
#include <SmDiskTypes.h>

namespace fds {

/**
 * Class that manages caching of object data.
 */
class ObjectDataCache : public Module, public boost::noncopyable {
  private:
    /// Backing cache structure
    typedef SharedKvCache<ObjectID, const std::string, ObjectHash> ObjectCache;
    std::unique_ptr<ObjectCache> dataCache;

    /// Max total number of entries
    fds_uint32_t maxEntries;

  public:
    explicit ObjectDataCache(const std::string &modName);
    ~ObjectDataCache();
    typedef std::unique_ptr<ObjectDataCache> unique_ptr;

    /**
     * Caches object data. The cache will retain a reference to the
     * data being put.
     */
    void putObjectData(fds_volid_t volId,
                       const ObjectID &objId,
                       boost::shared_ptr<const std::string> &objData);

    /**
     * Reads object data from the cache. The object data is returned
     * via a parameter, which is expected to be an unitialized shared_ptr.
     * The cache structure will set the shared_ptr to point to the data
     * in the cache. The data being pointed to is still owned by the
     * cache not the caller.
     */
    boost::shared_ptr<const std::string> getObjectData(fds_volid_t volId,
                                                       const ObjectID &objId,
                                                       Error& err);

    /**
     * Removes object data from the cache. The cache will release its reference
     * to the shared ptr. This release does NOT guarantee memory will be released
     * if other threads are still holding a reference to the ptr.
     */
    void removeObjectData(fds_volid_t volId,
                          const ObjectID &objId);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATACACHE_H_
