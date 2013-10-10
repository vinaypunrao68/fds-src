#ifndef INCLUDE_CPP_LIST_H_
#define INCLUDE_CPP_LIST_H_

#include <shared/dlist.h>

class ChainList;

template <class T>
class ChainLink
{
  public:
    ChainLink(T *obj) : lnk_obj(obj) { dlist_init(&lnk_chain); }
    ~ChainLink() {}

    inline T *chain_get_obj(ChainLink<T> *lnk) { return (T *)lnk->lnk_obj; }
    inline void chain_rm() { dlist_rm(&lnk_chain); }
    inline void chain_rm_init()
    {
        dlist_rm(&lnk_chain);
        dlist_init(&lnk_chain);
    }
  private:
    friend class ChainList;

    dlist_t   lnk_chain;
    T        *lnk_obj;
};

class ChainList
{
  public:
    ChainList() { dlist_init(&chain_head); }
    ~ChainList() { }

    template <class T>
    inline void chain_add_front(ChainLink<T> &chain)
    {
        dlist_add_front(&chain_head, &chain.lnk_chain);
    }
    template <class T>
    inline void chain_add_back(ChainLink<T> &chain)
    {
        dlist_add_back(&chain_head, &chain.lnk_chain);
    }
    template <class T>
    inline T *chain_peek_front()
    {
        dlist_t      *ptr;
        ChainLink<T> *elm;

        ptr = dlist_peek_front(&chain_head);
        if (ptr != nullptr) {
            ChainLink<T> *elm = fds_object_of(ChainLink<T>, lnk_chain, ptr);
            return elm->chain_get_obj();
        }
        return nullptr;
    }

    typedef dlist_t * ChainIter;

    inline void chain_iter_init(ChainIter *iter)
    {
        dlist_iter_init(&chain_head, iter);
    }
     inline bool chain_iter_term(ChainIter iter)
    {
        return dlist_iter_term(&chain_head, iter);
    }
    template <class T>
    T *chain_iter_current(ChainIter iter)
    {
        ChainLink<T> *elm = fds_object_of(ChainLink<T>, lnk_chain, iter);
        return elm->chain_get_obj();
    }
    inline void chain_iter_next(ChainIter *iter)
    {
        return dlist_iter_next(iter);
    }
    template <class T>
    inline T *chain_iter_rm_curr(ChainIter *iter)
    {
        dlist_t *ptr = dlist_iter_rm_curr(iter);
        ChainLink<T> *elm = fds_object_of(ChainLink<T>, lnk_chain, iter);
        return elm->chain_get_obj();
    }
  private:
    dlist_t chain_head;
};

#endif /* INCLUDE_CPP_LIST_H_ */
