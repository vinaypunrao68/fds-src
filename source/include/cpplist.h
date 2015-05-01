/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CPPLIST_H_
#define SOURCE_INCLUDE_CPPLIST_H_

#include <shared/dlist.h>

namespace fds {

typedef dlist_t * ChainIter;
class ChainList;

class ChainLink
{
  public:
    explicit ChainLink(void *obj) { dlist_obj_init(&lnk_chain, obj); }
    ~ChainLink() {}

    /* ---------------------------------------------------------------------- */
    template <typename T>
    inline T *chain_get_obj() {
        return reinterpret_cast<T *>(lnk_chain.dl_obj);
    }
    inline void chain_rm() {
        dlist_rm(&lnk_chain.dl_link);
    }
    inline void chain_rm_init() {
        dlist_rm(&lnk_chain.dl_link);
        dlist_init(&lnk_chain.dl_link);
    }
    inline dlist_t *chain_link() {
        return &lnk_chain.dl_link;
    }
    inline bool chain_empty() {
        return dlist_empty(&lnk_chain.dl_link);
    }
    /* ---------------------------------------------------------------------- */
    template <typename T>
    static inline T *chain_obj_frm_link(dlist_t *ptr) {
        dlist_obj_t *obj = fds_object_of(dlist_obj_t, dl_link, ptr);
        return reinterpret_cast<T *>(obj->dl_obj);
    }
    static inline ChainLink *chain_frm_link(dlist_t *ptr) {
        dlist_obj_t *obj = fds_object_of(dlist_obj_t, dl_link, ptr);
        return reinterpret_cast<ChainLink *>(obj);
    }

  private:
    friend class ChainList;
    dlist_obj_t  lnk_chain;
};

class ChainList
{
  public:
    ChainList() { dlist_init(&chain_head); }
    ~ChainList() { }

    /* ---------------------------------------------------------------------- */
    inline int chain_empty_list() {
        return dlist_empty(&chain_head);
    }
    inline void chain_add_front(ChainLink *chain) {
        dlist_add_front(&chain_head, &chain->lnk_chain.dl_link);
    }
    inline void chain_add_back(ChainLink *chain) {
        dlist_add_back(&chain_head, &chain->lnk_chain.dl_link);
    }
    template <typename T>
    inline T *chain_peek_front()
    {
        dlist_t *ptr;

        ptr = dlist_peek_front(&chain_head);
        if (ptr != nullptr) {
            return ChainLink::chain_obj_frm_link<T>(ptr);
        }
        return nullptr;
    }
    template <typename T>
    inline T *chain_peek_back()
    {
        dlist_t *ptr;

        ptr = dlist_peek_back(&chain_head);
        if (ptr != nullptr) {
            return ChainLink::chain_obj_frm_link<T>(ptr);
        }
        return nullptr;
    }
    inline ChainLink *chain_front_elem()
    {
        dlist_t *ptr = dlist_peek_front(&chain_head);
        if (ptr != nullptr) {
            return ChainLink::chain_frm_link(ptr);
        }
        return nullptr;
    }
    template <typename T>
    inline T *chain_rm_front()
    {
        dlist_t *ptr = dlist_rm_front(&chain_head);
        if (ptr != nullptr) {
            return ChainLink::chain_obj_frm_link<T>(ptr);
        }
        return nullptr;
    }
    inline void chain_transfer(ChainList *src) {
        dlist_merge(&chain_head, &src->chain_head);
    }
    /* ---------------------------------------------------------------------- */
    inline void chain_iter_init(ChainIter *iter) {
        dlist_iter_init(&chain_head, iter);
    }
    inline bool chain_iter_term(ChainIter iter) {
        return dlist_iter_term(&chain_head, iter);
    }
    template <typename T>
    inline T *chain_iter_current(ChainIter iter)
    {
        ChainLink *elm = ChainLink::chain_frm_link(iter);
        return elm->chain_get_obj<T>();
    }
    inline void chain_iter_next(ChainIter *iter) {
        dlist_iter_next(iter);
    }
    template <typename T>
    inline T *chain_iter_rm_current(ChainIter *iter)
    {
        dlist_t *ptr = dlist_iter_rm_curr(iter);
        return ChainLink::chain_obj_frm_link<T>(ptr);
    }

  private:
    dlist_t chain_head;
};

// Macro to traverse the whole list.
//
#define chain_foreach(chain, iter)                                                   \
    for ((chain)->chain_iter_init(&iter);                                            \
         !((chain)->chain_iter_term(iter)); (chain)->chain_iter_next(&iter))

}  // namespace fds

#endif  // SOURCE_INCLUDE_CPPLIST_H_
