#ifndef INCLUDE_CPP_LIST_H_
#define INCLUDE_CPP_LIST_H_

#include <shared/dlist.h>

namespace fds {

typedef dlist_t * ChainIter;
class ChainList;

class ChainLink
{
  public:
    ChainLink(void *obj) { dlist_obj_init(&lnk_chain, obj); }
    ~ChainLink() {}

    /* ---------------------------------------------------------------------- */
    template <typename T>
    inline T *chain_get_obj()
    {
        return (T *)lnk_chain.dl_obj;
    }
    inline void chain_rm()
    {
        dlist_rm(&lnk_chain.dl_link);
    }
    inline void chain_rm_init()
    {
        dlist_rm(&lnk_chain.dl_link);
        dlist_init(&lnk_chain.dl_link);
    }
    inline dlist_t *chain_link()
    {
        return &lnk_chain.dl_link;
    }
    /* ---------------------------------------------------------------------- */
    template <typename T>
    static inline T *chain_obj_frm_link(dlist_t *ptr)
    {
        dlist_obj_t *obj = fds_object_of(dlist_obj_t, dl_link, ptr);
        return (T *)obj->dl_obj;
    }
    static inline ChainLink *chain_frm_link(dlist_t *ptr)
    {
        dlist_obj_t *obj = fds_object_of(dlist_obj_t, dl_link, ptr);
        return (ChainLink *)obj;
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
    inline void chain_add_front(ChainLink *chain)
    {
        dlist_add_front(&chain_head, &chain->lnk_chain.dl_link);
    }
    inline void chain_add_back(ChainLink *chain)
    {
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
    ChainLink *chain_front_elem()
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
    /* ---------------------------------------------------------------------- */
    inline void chain_iter_init(ChainIter *iter)
    {
        dlist_iter_init(&chain_head, iter);
    }
     inline bool chain_iter_term(ChainIter iter)
    {
        return dlist_iter_term(&chain_head, iter);
    }
    template <typename T>
    inline T *chain_iter_current(ChainIter iter)
    {
        ChainLink *elm = ChainLink::chain_frm_link(iter);
        return elm->chain_get_obj<T>();
    }
    inline void chain_iter_next(ChainIter *iter)
    {
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

} // namespace fds

#endif /* INCLUDE_CPP_LIST_H_ */
