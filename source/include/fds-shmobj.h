/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_SHMOBJ_H_
#define SOURCE_INCLUDE_FDS_SHMOBJ_H_

#include <pthread.h>
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
        return reinterpret_cast<const T *>(shm_area + (idx * shm_obj_siz));
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

/**
 * ------------------------------------------------------------------------------------
 * Producer/consumer interface in shared memory.
 * ------------------------------------------------------------------------------------
 */

/** Brain, free to change stuffs here  */

/**
 * Queue for 1 producer, n consumers.  This data structure is in shared memory.
 */
typedef struct shm_1prd_ncon_q
{
    int                      shm_1prd_idx;
    int                      shm_ncon_cnt;
    int                      shm_ncon_idx[0];     /** -1 if don't care */
} shm_1prd_ncon_q_t;

/**
 * Queue for n producers, 1 consumer.  This data structure is in shared memory.
 */
typedef struct shm_nprd_1con_q
{
    fds_uint32_t             shm_1con_idx;
    fds_uint32_t             shm_nprd_cnt;
    fds_uint32_t             shm_ncon_idx[0];
} shm_nprd_1con_q_t;

/**
 * Synchronization among consumer(s), producer(s).  This is also in shared memory.
 */
typedef struct shm_con_prd_sync
{
    pthread_mutex_t          shm_mtx;
    pthread_cond_t           shm_con_cv;
    pthread_cond_t           shm_prd_cv;
} shm_con_prd_sync_t;

class ShmConPrdQueue
{
  public:
    /**
     * Block if the queue is empty.
     * @param consumer (i) : default is 0, if not index to control the consumer index
     *     pointer.
     */
    virtual void shm_consumer(void *data, size_t size, int consumer = 0) = 0;
    virtual void shm_producer(const void *data, size_t size, int producer = 0) = 0;

  protected:
    ShmConPrdQueue(shm_con_prd_sync_t *sync, shm_1prd_ncon_q_t *ctrl, ShmObjRW *data);
    ShmConPrdQueue(shm_con_prd_sync_t *sync, shm_nprd_1con_q_t *ctrl, ShmObjRW *data);
    virtual ~ShmConPrdQueue() {}
};

class Shm_1Prd_nCon : public ShmConPrdQueue
{
  public:
    Shm_1Prd_nCon(shm_con_prd_sync_t *sync, shm_1prd_ncon_q_t *ctrl, ShmObjRW *data);

    void shm_consumer(void *data, size_t size, int consumer = 0) override;
    void shm_producer(const void *data, size_t size, int producer = 0) override;

  private:
    ~Shm_1Prd_nCon() {}
};

class Shm_nPrd_1Con : public ShmConPrdQueue
{
  public:
    Shm_nPrd_1Con(shm_con_prd_sync_t *sync, shm_1prd_ncon_q_t *ctrl, ShmObjRW *data);

    void shm_consumer(void *data, size_t size, int consumer = 0) override;
    void shm_producer(const void *data, size_t size, int producer = 0) override;

  private:
    ~Shm_nPrd_1Con() {}
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_SHMOBJ_H_
