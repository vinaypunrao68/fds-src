/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_SHMOBJ_H_
#define SOURCE_INCLUDE_FDS_SHMOBJ_H_

#include <unordered_map>
#include <pthread.h>
#include <fds_assert.h>
#include <fds_module.h>
#include <fds_request.h>
#include <concurrency/Mutex.h>

/**
 * Generic library to manage objects in shared memory.  All objs must be POD types.
 */
namespace fds {
class FdsShmem;
class ShmObjRO;
class ShmConPrdQueue;

class ShmObjIter
{
  public:
    ShmObjIter() : iter_cnt(0) {}
    virtual ~ShmObjIter() {}

    virtual bool
    shm_obj_iter_fn(int         idx,
                    const void *key,
                    const void *rec,
                    size_t      key_sz,
                    size_t      rec_sz) = 0;

    inline int shm_obj_iter_cnt() { return iter_cnt; }

  public:
    friend class ShmObjRO;
    int          iter_cnt;
};

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

    // Iterate through shm records
    //
    virtual int shm_iter_objs(ShmObjIter *iter) const;

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
        fds_assert((idx >= 0) && (idx < shm_obj_cnt));
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

    virtual int shm_insert_rec(int idx, const void *data, size_t rec_sz);
    virtual int shm_insert_rec(const void *key, const void *data, size_t rec_sz);
    virtual int shm_remove_rec(const void *key, void *data, size_t rec_sz);
    virtual int shm_remove_rec(int idx, const void *key, void *data, size_t rec_sz);

    inline void *shm_rw_base() { return shm_rw_area; }

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
    int                      shm_1con_idx;
    int                      shm_nprd_idx;
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

/**
 * The smq_code field with masks to tell if a request is tracking type to notify the
 * requestor based on smq_id.
 */
const int SHMQ_TRACK_MASK        = 0x800000;
const int SHMQ_REQ_NULL          = 0x000000;
const int SHMQ_REQ_UUID_BIND     = 0x000001;
const int SHMQ_REQ_UUID_UNBIND   = 0x000002;
const int SHMQ_NODE_REGISTRATION = 0x000003;

/**
 * Common header for items in producer/consumer queue.
 */
typedef struct shmq_req
{
    fds_uint32_t             smq_code;
    fds_uint32_t             smq_idx : 8;
    fds_uint32_t             smq_id : 24;
    /* Currently unused: */
    size_t                   smq_payload_size;
    fds_uint8_t              reserved[16];
} shmq_req_t;

/**
 * Obj to track outgoing request - producer type, from producer to consumer.
 */
class ShmqReqOut : public fdsio::Request
{
  public:
    virtual ~ShmqReqOut() {}
    ShmqReqOut(bool block, shmq_req_t *in, size_t size)
        : fdsio::Request(block), req_in(in), req_size(size) {}

  protected:
    friend class ShmConPrdQueue;

    size_t                   req_size;  /**< size of the request/response.          */
    shmq_req_t               req_out;   /**< only keep header for outgoing req.     */
    shmq_req_t              *req_in;    /**< storage to store in-comming response.  */
};

/**
 * Obj to track incomming request - consumer type, from consumer to producer.
 */
class ShmqReqIn
{
  public:
    virtual ~ShmqReqIn() {}
    ShmqReqIn() {}

    virtual void shmq_handler(const shmq_req_t *in_hdr, size_t size) = 0;
};

/**
 * Shared memory consumer/producer queue.
 */
class ShmConPrdQueue : public fdsio::RequestQueue
{
  public:
    /**
     * Use this method if the producer wants to track the request.  Typical usage
     * ...
     * ShmConPrdQueue *consumer = consumer_singleton();
     * ShmConPrdQueue *producer = producer_singleton();
     *
     * shmq_req_t *in, *out;
     * ShmaReqOut *req = new ShmqReqOut(in, size);
     *
     * consumer->shm_track_request(req, out);
     * producer->shm_producer(out, size, ...);
     * ...
     * req->req_wait();          // block waiting for response from consumer queue.
     * req->req_in should have the response data.
     * See the file net-service/endpoint/ep-map.cpp for actual usage.
     */
    virtual void shm_track_request(ShmqReqOut *out, shmq_req_t *hdr, int caller);

    /**
     * For requests with tracking option, use this method before sending back the
     * response to get back to the original sender.
     */
    static void shm_swap_req_header(shmq_req_t *x, shmq_req_t *y);

    /**
     * Register handler to dispatch the smq_code to the right handler.  Note that
     * incomming request with tracking bit on will be mapped to the original sender
     * to notify it.
     */
    virtual void shm_register_handler(int smq_code, ShmqReqIn *cb);

    /**
     * Run forever by a threadpool to consume data queued to the consumer queue.
     */
    virtual void shm_consume_loop(int consumer_idx);

    /**
     * Block if the queue is empty.
     * @param consumer (i) : default is 0, if not index to control the consumer
     * index * pointer.
     */
    virtual void shm_consumer(void *data, size_t size, int consumer = 0) = 0;
    virtual void shm_producer(const void *data, size_t size, int producer = 0) = 0;

    /**
     * Initialize consumer indexes, setting all indexes to -1 (inactive).
     */
    virtual void shm_init_consumer_queue() = 0;
    virtual void shm_activate_consumer(int consumer) = 0;

  protected:
    ShmConPrdQueue(shm_con_prd_sync_t *sync, ShmObjRW *data);

    // Track the pointers provided by our constructor
    shm_con_prd_sync_t *smq_sync;
    ShmObjRW *smq_data;
    const uint smq_size;  // Size of the queue for index calculations
    const size_t smq_itm_size;  // Size of queue data items

    virtual ~ShmConPrdQueue() {}

    ShmqReqOut *shm_get_saved_req(fds_uint32_t id);
    ShmqReqIn  *shm_get_callback(fds_uint32_t code);

  private:
    // Keep a unique ID for request tracking
    static fds_uint32_t smq_reqID;
    // Map for reqID -> ShmReqOut objs
    std::unordered_map<fds_uint32_t, ShmqReqOut*> smq_reqOutMap;
    // Map for smq_code -> ShmqReqIn* (callback function)
    std::unordered_map<fds_uint32_t, ShmqReqIn*> smq_cb_map;
};

class Shm_1Prd_nCon : public ShmConPrdQueue
{
  public:
    Shm_1Prd_nCon(shm_con_prd_sync_t *sync, shm_1prd_ncon_q_t *ctrl, ShmObjRW *data);

    void shm_consumer(void *data, size_t size, int consumer = 0) override;
    void shm_producer(const void *data, size_t size, int producer = 0) override;
    void shm_init_consumer_queue() override;
    void shm_activate_consumer(int consumer) override;

  private:
    shm_1prd_ncon_q_t *smq_ctrl;
    int shm_calc_delta(int consumer);
    int shm_calc_max_delta();

    ~Shm_1Prd_nCon() {}
};

class Shm_nPrd_1Con : public ShmConPrdQueue
{
  public:
    Shm_nPrd_1Con(shm_con_prd_sync_t *sync, shm_nprd_1con_q_t *ctrl, ShmObjRW *data);

    void shm_consumer(void *data, size_t size, int consumer = 0) override;
    void shm_producer(const void *data, size_t size, int producer = 0) override;
    void shm_init_consumer_queue() override;
    void shm_activate_consumer(int cons) override;

  private:
    shm_nprd_1con_q_t *smq_ctrl;

    ~Shm_nPrd_1Con() {}
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_SHMOBJ_H_
