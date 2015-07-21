/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMOIDARRAYMMAP_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMOIDARRAYMMAP_H_

#include <sys/mman.h>

#include <boost/shared_ptr.hpp>

#include <fds_types.h>
#include <concurrency/Mutex.h>

namespace fds {

/**
 * This class mmaps into flat file which stores object ID array.
 * Object IDs belonging to a blob or a volume are stored in long,
 * flat, sparse file.
 */
class DmOIDArrayMmap {
  public:
    // types
    typedef boost::shared_ptr<DmOIDArrayMmap> ptr;
    typedef boost::shared_ptr<const DmOIDArrayMmap> const_ptr;

    // class vars
    static const fds_uint32_t FRAGMENT_SIZE;
    static const fds_uint32_t NUM_OBJS_PER_FRAGMENT;

    // ctor & dtor
    DmOIDArrayMmap(fds_uint64_t id, int fd);
    virtual ~DmOIDArrayMmap();

    // methods
    inline int getFD() const {
        return fd_;
    }

    inline fds_uint64_t getID() const {
        return id_;
    }

    inline fds_bool_t valid() const {
        return base_ && MAP_FAILED != base_;
    }

    inline Error getObject(fds_uint64_t objIndex, ObjectID & objId) const {
        fds_assert((objIndex / NUM_OBJS_PER_FRAGMENT) == id_);
        fds_assert(valid());

        fds_scoped_lock sl(lock_);
        objId.SetId(reinterpret_cast<const char *>(getOID(objIndex % NUM_OBJS_PER_FRAGMENT)),
                OBJECTID_DIGESTLEN);
        return ERR_OK;
    }

    inline Error putObject(fds_uint64_t objIndex, const ObjectID & objId) {
        fds_assert((objIndex / NUM_OBJS_PER_FRAGMENT) == id_);
        fds_assert(valid());

        fds_scoped_lock sl(lock_);
        memcpy(reinterpret_cast<void *>(getOID(objIndex % NUM_OBJS_PER_FRAGMENT)),
                objId.GetId(), OBJECTID_DIGESTLEN);
        return ERR_OK;
    }

  protected:
    fds_uint64_t id_;
    int fd_;
    void * base_;
    mutable fds_mutex lock_;

    // methods
    inline const uint8_t * getBaseOffset() const {
        return reinterpret_cast<const uint8_t *>(base_);
    }
    inline uint8_t * getBaseOffset() {
        return reinterpret_cast<uint8_t *>(base_);
    }

    inline const uint8_t * getOID(fds_uint64_t arrIndex) const {
        return getBaseOffset() + arrIndex * OBJECTID_DIGESTLEN;
    }
    inline uint8_t * getOID(fds_uint64_t arrIndex) {
        return getBaseOffset() + arrIndex * OBJECTID_DIGESTLEN;
    }
};
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMOIDARRAYMMAP_H_
