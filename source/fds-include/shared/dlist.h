/*
 */
#ifndef FDS_INCLUDE_DLIST_H_
#define FDS_INCLUDE_DLIST_H_

#ifdef __cplusplus
extern  "C" {
#endif /* __cplusplus */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif /* !NULL */

typedef struct dlist dlist_t;
struct dlist
{
    dlist_t *dl_next;
    dlist_t *dl_prev;
};

/*
 * dlist_init
 * ----------
 * Initialize a circular dlist.
 */
static inline void
dlist_init(dlist_t *dlist)
{
    dlist->dl_next = dlist->dl_prev = dlist;
}

/*
 * dlist_empty
 * -----------
 * Return TRUE if the dlist is empty.
 */
static inline int
dlist_empty(dlist_t *dlist)
{
    return ((dlist->dl_next == dlist) && (dlist->dl_prev == dlist));
}

/*
 * dlist_add_front
 * ---------------
 * Add a new element to the front of a dlist.
 */
static inline void
dlist_add_front(dlist_t *dlist, dlist_t *elm)
{
    elm->dl_next            = dlist->dl_next;
    elm->dl_prev            = dlist;
    dlist->dl_next->dl_prev = elm;
    dlist->dl_next          = elm;
}

/*
 * dlist_add_back
 * --------------
 * Add a new element to the back of a dlist.
 */
static inline void
dlist_add_back(dlist_t *dlist, dlist_t *elm)
{
    dlist_add_front(dlist->dl_prev, elm);
}

/*
 * dlist_rm
 * --------
 * Remove the current element of a dlist.  Don't care what list this element
 * is attached to.
 */
static inline void
dlist_rm(dlist_t *elm)
{
    elm->dl_prev->dl_next = elm->dl_next;
    elm->dl_next->dl_prev = elm->dl_prev;
}

/*
 * dlist_rm_init
 * -------------
 * Same as dlist_rm but also initialize the elm after removing it out.
 */
static inline void
dlist_rm_init(dlist_t *elm)
{
    dlist_rm(elm);
    dlist_init(elm);
}

/*
 * dlist_peek_front
 * ----------------
 * Peek the first element of a dlist.  Return NULL if the list is empty.
 */
static inline dlist_t *
dlist_peek_front(dlist_t *dlist)
{
    if ((dlist != NULL) && !dlist_empty(dlist)) {
        return (dlist->dl_next);
    }
    return (NULL);
}

/*
 * dlist_peek_back
 * ---------------
 * Peek the last element of a dlist.  Return NULL if the list is empty.
 */
static inline dlist_t *
dlist_peek_back(dlist_t *dlist)
{
    if ((dlist != NULL) && !dlist_empty(dlist)) {
        return (dlist->dl_prev);
    }
    return (NULL);
}

/*
 * dlist_rm_front
 * --------------
 * Remove elm from the front of a dlist.  Return NULL if the list is empty.
 */
static inline dlist_t *
dlist_rm_front(dlist_t *dlist)
{
    dlist_t *curr = NULL;

    if ((dlist != NULL) && !dlist_empty(dlist)) {
        curr = dlist->dl_next;
        dlist_rm(curr);
    }
    return (curr);
}

/*
 * dlist_rm_back
 * -------------
 * Remove elm from the back of a dlist.  Return NULL if the list is empty.
 */
static inline dlist_t *
dlist_rm_back(dlist_t *dlist)
{
    dlist_t *curr = NULL;

    if ((dlist != NULL) && !dlist_empty(dlist)) {
        curr = dlist->dl_prev;
        dlist_rm(curr);
    }
    return (curr);
}

/*
 * dlist_merge
 * -----------
 * Merge two dlists together.  Make list arg merge the root list and empty it.
 */
static inline void
dlist_merge(dlist_t *root, dlist_t *list)
{
    if (!dlist_empty(list)) {
        root->dl_prev->dl_next = list->dl_next;
        list->dl_next->dl_prev = root->dl_prev;
        list->dl_prev->dl_next = root;
        root->dl_prev          = list->dl_prev;
        dlist_init(list);
    }
}

/*
 * dlist_merge_front
 * -----------------
 * Like dlist_merge, but put the list arg to the front.
 */
static inline void
dlist_merge_front(dlist_t *root, dlist_t *list)
{
    if (!dlist_empty(list)) {
        root->dl_next->dl_prev = list->dl_prev;
        list->dl_prev->dl_next = root->dl_next;
        root->dl_next          = list->dl_next;
        list->dl_next->dl_prev = root;
        dlist_init(list);
    }
}

/*
 * dlist_iter_init
 * ---------------
 */
static inline void
dlist_iter_init(dlist_t *list, dlist_t **iter)
{
    *iter = list->dl_next;
}

/*
 * dlist_iter_term
 * ---------------
 */
static inline bool
dlist_iter_term(dlist_t *list, dlist_t *iter)
{
    return (iter == list ? true : false);
}

/*
 * dlist_iter_next
 * ---------------
 */
static inline void
dlist_iter_next(dlist_t **iter)
{
    *iter = (*iter)->dl_next;
}

/*
 * dlist_iter_rm_curr
 * ------------------
 */
static inline dlist_t *
dlist_iter_rm_curr(dlist_t **iter)
{
    dlist_t *curr;

    curr  = (*iter);
    *iter = curr->dl_next;
    dlist_rm(curr);

    return (curr);
}

/* Get the offset of a field y inside struct X. */
#define fds_offset_of(X, y)   ((unsigned long)((void *)&(((X *)0)->y)))

/* Get the address of the obj from addr of a field in it. */
#define fds_object_of(obj, field, ptr)                                        \
    ((obj *)(((char *)(ptr)) - (char *)fds_offset_of(obj, field)))

/*
 * Macros to traverse the whole dlist.
 */
#define foreach_dlist_iter(dlist, iter)                                       \
    for (dlist_iter_init(dlist, &iter);                                       \
         dlist_iter_term(dlist, iter) == false; dlist_iter_next(&iter))

/*
 * Marco to free the whole dlist.
 */
#define dlist_free(dlist, free_fn)                                            \
    do {                                                                      \
        dlist_t *__cptr;                                                      \
        while (!dlist_empty(dlist)) {                                         \
            __cptr = dlist_rm_front(dlist);                                   \
           free_fn(__cptr);                                                   \
        }                                                                     \
    } while (0)

/*
 * dlist_find
 * ----------
 */
static inline dlist_t *
dlist_find(dlist_t   *dlist,
           dlist_t   *elm,
           int      (*cmp_fn)(dlist_t *, dlist_t *),
           bool       rm)
{
    dlist_t *iter, *found;

    foreach_dlist_iter(dlist, iter) {
        if (cmp_fn(iter, elm) == 0) {
            found = iter;
            if (rm == true) {
                dlist_iter_rm_curr(&iter);
            }
            return (found);
        }
    }
    return (NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _DLIST_H_ */
