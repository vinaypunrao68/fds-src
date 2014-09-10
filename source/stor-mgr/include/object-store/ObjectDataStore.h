/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_

#include <string>
#include <fds_module.h>
#include <persistent_layer/dm_io.h>

namespace fds {

/**
 * Class that manages storage of object data. The class persistetnly stores
 * and caches object data.
 */
class ObjectDataStore : public Module, public boost::noncopyable {
  private:
    /// Disk storage manager
    diskio::DataIO *diskMgr;

    /// Per-volume object data cache manager

    // TODO(Andrew): Add some private GC interfaces here?

  public:
    explicit ObjectDataStore(const std::string &modName);
    ~ObjectDataStore();
    typedef std::unique_ptr<ObjectDataStore> unique_ptr;

    /**
     * Creates storage for a volume. Creates cache space and any
     * additional per-volume structures that are needed. The
     * physical underlying storage is organized by SM tokens
     * and are not per-volume.
     */
    Error createDataStore(fds_volid_t volId);

    /**
     * Removes storage for a volume. Removes cache and other
     * per-volume structures.
     */
    Error removeDataStore(fds_volid_t volId);

    /**
     * Peristently stores object data.
     */
    Error putObjectData(fds_volid_t volId,
                        const ObjectID &objId,
                        boost::shared_ptr<const std::string> objData);

    /**
     * Reads object data.
     */
    Error getObjectData(fds_volid_t volId,
                        const ObjectID &objId,
                        boost::shared_ptr<std::string> objData);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTDATASTORE_H_
