/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds-shmobj.h>
#include <platform/fds-shmem.h>

namespace fds {

/*
 * ------------------------------------------------------------------------------------
 * Shared Memory RO Obj Manager
 * ------------------------------------------------------------------------------------
 */
ShmObjRO::ShmObjRO(const FdsShmem *shm,
                   size_t          off,
                   size_t          key_siz,
                   size_t          key_off,
                   size_t          obj_siz,
                   int             obj_cnt) : shm_ctrl(shm)
{
    fds_verify((key_siz > 0) && (obj_siz >= key_siz) && (obj_cnt > 0));

    shm_area       = static_cast<const char *>(shm->shm_area()) + off;
    shm_area_bound = shm_area + (obj_siz * obj_cnt);
    shm_key_siz    = key_siz;
    shm_key_off    = key_off;
    shm_obj_siz    = obj_siz;
    shm_obj_cnt    = obj_cnt;
}

// shm_lookup_rec
// --------------
//
int
ShmObjRO::shm_lookup_rec(const void *key, void *rec, size_t rec_sz) const
{
    int         idx;
    size_t      off;
    const char *cur, *k;

    fds_assert(rec_sz == shm_obj_siz);
    k = static_cast<const char *>(key);

    off = 0;
    for (idx = 0; idx < shm_obj_cnt; idx++, off += shm_obj_siz) {
        cur = shm_area + off;
        if (obj_key_cmp(k, cur + shm_key_off, shm_key_siz) == 0) {
            /* Found the match */
            memcpy(rec, cur, shm_obj_siz);
            break;
        }
    }
    if (idx == shm_obj_cnt) {
        memset(rec, 0, shm_obj_siz);
    }
    return idx;
}

int
ShmObjRO::shm_lookup_rec(int idx, const void *key, void *rec, size_t rec_sz) const
{
    const char *cur, *k;

    fds_verify((0 <= idx) && (idx < shm_obj_cnt));
    cur = shm_area + (idx * shm_obj_siz);
    if (key != NULL) {
        /* Verify the key. */
        fds_verify(obj_key_cmp(static_cast<const char *>(key),
                               cur + shm_key_off, shm_key_siz) == 0);
    }
    memcpy(rec, cur, shm_obj_siz);
    return idx;
}

// obj_key_cmp
// -----------
//
int
ShmObjRO::obj_key_cmp(const char *a1, const char *a2, size_t size) const
{
    return memcmp(a1, a2, size);
}

// obj_key_invalid
// ---------------
//
bool
ShmObjRO::obj_key_invalid(const char *key, size_t size) const
{
    const fds_uint32_t *k;

    k = reinterpret_cast<const fds_uint32_t *>(key);
    if (size >= (2 * sizeof(*k))) {
        return (k[0] == 0) && (k[1] == 0);
    }
    return k[0] == 0;
}

int
ShmObjROKeyUint64::obj_key_cmp(const char *a1, const char *a2, size_t size) const
{
    const fds_uint64_t *k1, *k2;

    k1 = reinterpret_cast<const fds_uint64_t *>(a1);
    k2 = reinterpret_cast<const fds_uint64_t *>(a2);
    return (*k1 - *k2);
}

int
ShmObjRWKeyUint64::obj_key_cmp(const char *a1, const char *a2, size_t size) const
{
    const fds_uint64_t *k1, *k2;

    k1 = reinterpret_cast<const fds_uint64_t *>(a1);
    k2 = reinterpret_cast<const fds_uint64_t *>(a2);
    return (*k1 - *k2);
}

/*
 * ------------------------------------------------------------------------------------
 * Shared Memory RW Obj Manager
 * ------------------------------------------------------------------------------------
 */
ShmObjRW::ShmObjRW(const FdsShmem *shm,
                   size_t          off,
                   size_t          key_siz,
                   size_t          key_off,
                   size_t          obj_siz,
                   int             obj_cnt)
    : ShmObjRO(shm, off, key_siz, key_off, obj_siz, obj_cnt), rw_mtx()
{
    shm_rw_area = static_cast<char *>(shm_ctrl->shm_area()) + off;
    fds_assert(shm_rw_area == shm_area);
}

// shm_insert_rec
// --------------
//
int
ShmObjRW::shm_insert_rec(const void *key, const void *data, size_t dat_siz)
{
    int         idx, empty;
    size_t      off;
    char       *cur;
    const char *k;

    fds_verify(dat_siz == shm_obj_siz);
    k     = static_cast<const char *>(key);
    off   = 0;
    empty = -1;

    rw_mtx.lock();
    for (idx = 0; idx < shm_obj_cnt; idx++, off += shm_obj_siz) {
        cur = shm_rw_area + off;
        if (obj_key_cmp(k, cur + shm_key_off, shm_key_siz) == 0) {
            /* Fond existing slot, overwrite it with the new record. */
            memcpy(cur, data, shm_obj_siz);
            break;
        }
        if (obj_key_invalid(cur + shm_key_off, shm_key_siz) == true) {
            if (empty == -1) {
                empty = idx;
            }
        }
    }
    if (idx == shm_obj_cnt) {
        if (empty != -1) {
            idx = empty;
            cur = shm_rw_area + (empty * shm_obj_siz);
            memcpy(cur, data, shm_obj_siz);
            shm_obj_cnt++;
        } else {
            idx = -1;
        }
    }
    rw_mtx.unlock();
    return idx;
}

// shm_remove_rec
// --------------
//
int
ShmObjRW::shm_remove_rec(const void *key, void *data, size_t rec_sz)
{
    int         idx;
    size_t      off;
    char       *cur;
    const char *k;

    k   = static_cast<const char *>(key);
    off = 0;

    rw_mtx.lock();
    for (idx = 0; idx < shm_obj_cnt; idx++, off += shm_obj_siz) {
        cur = shm_rw_area + off;
        if (obj_key_cmp(k, cur + shm_key_off, shm_key_siz) == 0) {
            /* Found the matching record, remove it. */
            if (data != NULL) {
                fds_verify(rec_sz == shm_obj_siz);
                shm_obj_cnt--;
                memcpy(data, cur, shm_obj_siz);
            }
            memset(cur, 0, shm_obj_siz);
            break;
        }
    }
    rw_mtx.unlock();
    return idx;
}

// shm_remove_rec
// --------------
//
int
ShmObjRW::shm_remove_rec(int idx, const void *key, void *data, size_t rec_sz)
{
    char       *cur;
    const char *k;

    fds_verify((0 <= idx) && (idx < shm_obj_cnt));
    k = static_cast<const char *>(key);

    rw_mtx.lock();
    cur = shm_rw_area + (idx * shm_obj_siz);
    if (key != NULL) {
        fds_verify(obj_key_cmp(k, cur + shm_key_off, shm_key_siz) == 0);
    }
    shm_obj_cnt--;
    if (data != NULL) {
        fds_verify(rec_sz == shm_obj_siz);
        memcpy(data, cur, shm_obj_siz);
    }
    rw_mtx.unlock();
    return idx;
}

}  // namespace fds
