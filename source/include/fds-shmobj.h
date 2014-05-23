/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_SHMOBJ_H_
#define SOURCE_INCLUDE_FDS_SHMOBJ_H_

#include <fds_assert.h>
#include <fds_module.h>
#include <concurrency/Mutex.h>

/**
 * Generic library to manage objects in shared memory.  All objs must be POD types.
 */
namespace fds {
class FdsShmem;

class ShmObjRO
{
  public:
    virtual ~ShmObjRO() {}
    ShmObjRO(const FdsShmem *shm,
             size_t          off,
             size_t          key_siz,
             size_t          key_off,
             size_t          obj_siz,
             int             obj_cnt);

    // Basic lookup methods.
    //
    virtual int shm_lookup_rec(const void *key, void *rec, size_t rec_sz) const;
    virtual int shm_lookup_rec(int idx, const void *key, void *rec, size_t rec_sz) const;

    // Hookup with producer/consume queue for notification.
    //

    // Inline and template methods.
    //
    inline const void *shm_base()  const { return shm_area; }
    inline const void *shm_bound() const { return shm_area_bound; }

    template <class T>
    const T *shm_cast_rec(const void *rec) const {
        fds_assert((shm_area <= rec) && (rec < shm_area_bound));
        return static_cast<T *>(rec);
    }
    template <class T>
    const T *shm_get_rec(int idx) const {
        fds_assert(idx < shm_obj_cnt);
        return static_cast<T *>(shm_area + (idx * shm_obj_siz));
    }

  protected:
    const FdsShmem          *shm_ctrl;
    const char              *shm_area;
    const char              *shm_area_bound;
    size_t                   shm_key_siz;
    size_t                   shm_key_off;
    size_t                   shm_obj_siz;
    int                      shm_obj_cnt;

    virtual bool obj_key_invalid(const char *k, size_t size) const;
    virtual int  obj_key_cmp(const char *k1, const char *k2, size_t size) const;
};

/**
 * Extends from ShmObjRO with u64 key type.
 */
class ShmObjROKeyUint64 : public ShmObjRO
{
  public:
    ShmObjROKeyUint64(const FdsShmem *shm,
                      size_t          off,
                      size_t          key_off,
                      size_t          obj_siz,
                      int             obj_cnt)
        : ShmObjRO(shm, off, sizeof(fds_uint64_t), key_off, obj_siz, obj_cnt) {}

  protected:
    virtual int obj_key_cmp(const char *k1, const char *k2, size_t size) const;
};

/**
 * Provide RW access to objects in shared memory region.
 */
class ShmObjRW : public ShmObjRO
{
  public:
    ShmObjRW(const FdsShmem *shm,
             size_t          off,
             size_t          key_siz,
             size_t          key_off,
             size_t          obj_siz,
             int             obj_cnt);

    virtual int shm_insert_rec(const void *key, const void *data, size_t rec_sz);
    virtual int shm_remove_rec(const void *key, void *data, size_t rec_sz);
    virtual int shm_remove_rec(int idx, const void *key, void *data, size_t rec_sz);

  protected:
    fds_mutex                rw_mtx;
    int                      shm_obj_cnt;
    char                    *shm_rw_area;
};

/**
 * Extends from ShmObjRO with u64 key type.
 */
class ShmObjRWKeyUint64 : public ShmObjRW
{
  public:
    ShmObjRWKeyUint64(const FdsShmem *shm,
                      size_t          off,
                      size_t          key_off,
                      size_t          obj_siz,
                      int             obj_cnt)
        : ShmObjRW(shm, off, sizeof(fds_uint64_t), key_off, obj_siz, obj_cnt) {}

  protected:
    virtual int obj_key_cmp(const char *k1, const char *k2, size_t size) const;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_SHMOBJ_H_
