/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETADB_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETADB_H_

#include <string>
#include <unordered_map>
#include <utility>

#include <fds_types.h>
#include <concurrency/RwLock.h>
#include <ObjMeta.h>
#include <odb.h>
#include <object-store/SmDiskMap.h>

namespace fds {

/**
 * Stores storage manages object metadata
 * Present implementation is based on level db
 * There are 256 level db files, one per SM token.
 * SM token is a bucket that holds several DLT tokens --
 * DLT token is basically first X bits of object id, and SM
 * token is first Y (usually Y < X) bits of object id.
 */
class ObjectMetadataDb {
  public:
    explicit ObjectMetadataDb();
    ~ObjectMetadataDb();

    typedef std::unique_ptr<ObjectMetadataDb> unique_ptr;

    /**
     * Opens object metadata DB
     * @param[in] diskMap map of SM tokens to disks
     */
    Error openMetadataDb(const SmDiskMap::const_ptr& diskMap);

    /**
     * Set number of bits per (global) token
     * ObjectMetadataDB will cache it and use it to retrieve
     * SM tokens from object IDs
     */
    void setNumBitsPerToken(fds_uint32_t nbits);

    /**
     * Get object metadata from the database
     * @param volId volume id for which we are performing this
     * operation, or invalid_volume_id  if this operation is performed
     * on behalf of background job that does not know which volume
     * this operation belongs to. Volume id is used for perf counting,
     * not any actual db access operation
     */
    ObjMetaData::const_ptr get(fds_volid_t volId,
                               const ObjectID& objId,
                               Error &err);

    /**
     * Put object metadata to the database
     * @param volId volume id for which we are performing this
     * operation, or invalid_volume_id  if this operation is performed
     * on behalf of background job that does not know which volume
     * this operation belongs to. Volume id is used for perf counting,
     * not any actual db access operation
     */
    Error put(fds_volid_t volId,
              const ObjectID& objId,
              ObjMetaData::const_ptr objMeta);

    /**
     * Removes object metadata from the database
     * @param volId volume id for which we are performing this
     * operation, or invalid_volume_id  if this operation is performed
     * on behalf of background job that does not know which volume
     * this operation belongs to. Volume id is used for perf counting,
     * not any actual db access operation
     */
    Error remove(fds_volid_t volId,
                 const ObjectID& objId);

    /**
     * Returns snapshot of metadata DB for a given SM token
     */
    void snapshot(fds_token_id smTokId,
                  leveldb::DB*& db,
                  leveldb::ReadOptions& opts);

  private:  // methods
    /**
     * Open object metadata DB for a given SM token
     */
    Error openObjectDb(fds_token_id smTokId,
                       const std::string& diskPath);
    osm::ObjectDB *getObjectDB(const ObjectID& objId);
    /**
     * Closes object metadata DB for a given SM token
     */
    void closeObjectDB(fds_token_id smTokId);

  private:  // data
     std::unordered_map<fds_token_id, osm::ObjectDB *> tokenTbl;
     using TokenTblIter = std::unordered_map<fds_token_id, osm::ObjectDB *>::const_iterator;
     fds_rwlock dbmapLock_;  // lock for tokenTbl

     // cached number of bits per (global) token
     fds_uint32_t bitsPerToken_;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETADB_H_

