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

#define SM_TOKEN_MASK 0x000000ff

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

  private:  // methods
     inline fds_token_id getDbId(const fds_token_id &tokId) const {
         return tokId & SM_TOKEN_MASK;
     }
     void closeObjectDB(fds_token_id tokId);
     /**
      *  Returns object metadata DB, if it does not exist, creates it
      */
     osm::ObjectDB *getObjectDB(fds_token_id tokId);

  private:  // data
     std::unordered_map<fds_token_id, osm::ObjectDB *> tokenTbl;
     using TokenTblIter = std::unordered_map<fds_token_id, osm::ObjectDB *>::const_iterator;
     fds_rwlock dbmapLock_;  // lock for tokenTbl

     // cached number of bits per (global) token
     fds_uint32_t bitsPerToken_;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTMETADB_H_

