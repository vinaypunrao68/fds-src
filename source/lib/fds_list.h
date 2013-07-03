/*
 *  * Generic Doubly Linked List Library
 *   */

#include <fds_commons.h>

typedef enum {
    FDS_LIST_ELEM_ON_LIST     = FDS_MAGIC(0x1eaf), /* fds_list_elem_t value when an element is on a list */
    FDS_LIST_ELEM_OFF_LIST    = FDS_MAGIC(0x9032), /* delem_t value when an element is off lists */
    FDS_LIST_INITIALIZED      = FDS_MAGIC(0x0627), /* fds_list_t value when it has been initialized */
} fds_magic_t;

#define FDS_DBG_BADELEM   ((fds_list_elem_t *)0xdeadbeef)
#define FDS_DBG_BADLIST   ((fds_list_t *)0xfeedface)



/**
 *  * Generic List Element structure
 *   */
typedef struct {
    void *next;    /* Pointer to the next element in the list */
    void *prev;    /* Pointer to the previous element in the list */
#ifdef FDS_DEBUG
    fds_magic_t magic;  /** Magic should be valid when this element in the list */
    fds_list_t *list; /* Points to the list of which it is a member */
#endif
} fds_list_elem_t;


/**
 *  * Generic List Head structure
 *   */
typedef struct _fds_list {
    fds_list_elem_t   *head;        /* Pointer to the first element in the list */
    fds_list_elem_t   *tail;        /* Pointer to the last element in the list */
    fds_uint32_t count;    /* Number of members in the list */
    fds_uint32_t __lock;   /* mutex to lock the list */

#ifdef FDS_DEBUG
    fds_magic_t magic;     /* Set to DL_INITIALIZED when initialized */
    fds_bool_t check;      /* If TRUE, check consistency of the list */
#endif
};


static inline void fds_init_elem(fds_list_elem_t *elem)
{
    elem->next = elem->prev = FDS_LIST_BADELEM;

#ifdef FDS_DEBUG
    elem->list = FDS_DBG_BADLIST;
    elem->magic = FDS_OFF_LIST;
#endif
}


static void fds_list_consistency_check(const fds_list_t *list)
{
    if (list->check) {
        fds_list_elem_t *e;
        fds_list_elem_t *p = NULL;
        dd_uint32_t n = 0;

        for (e = list->head; e != NULL; e = e->next) {
            if (FDS_LIST_CHECK_MEMBERSHIP) {
                assert(e->list == list);
            }
            assert(e->magic == FDS_ELEM_ON_LIST);
            assert(e->prev == p);
            assert(e->next != FDS_LIST_BADELEM);

            p = e;
            n++;
        }
        assert(n == list->count);
    }
}


static inline fds_bool_t fds_list_is_empty(const fds_list_t *list)
{
    fds_bool_t is_empty;

    fds_list_rlock(list);
    is_empty = list->head == NULL;

    if (is_empty) {
        fds_verify(list->count == 0);
    } else {
        fds_verify(list->count != 0);
    }

    fds_list_runlock(list);

    return is_empty;
}

static inline void fds_list_insert_at_front(fds_list_t *list, fds_list_elem_t *elem)
{
    fds_list_external_check(list);
    fds_list_wlock(list);

    fds_assert(fds_list_is_orphan(elem));

    if (list->head == NULL) {
        list->head = list->tail = elem;
        elem->next = elem->prev = NULL;
    } else {
        elem->prev = NULL;
        elem->next = list->head;
        list->head->prev = elem;
        list->head = elem;
    }

    list->count++;

#ifdef FDS_DEBUG
    elem->list = list;
    elem->magic = FDS_LIST_ELEM_ON_LIST;
#endif

    fds_list_wunlock(list);
}


static inline void *_fds_list_dequeue(fds_list_t *list)
{
    fds_list_elem_t *elem;
    fds_list_elem_t *n;

    fds_list_external_check(list);
    fds_list_wlock(list);

    elem = list->head;

    if (elem != NULL) {
        fds_assert(dl_is_member_intern(list, elem, TRUE));
        fds_assert(elem->prev == NULL);

        n = elem->next;

        if (n != NULL) {
            fds_assert(dl_is_member_intern(list, n, TRUE));
            fds_assert(n->prev == elem);
            n->prev = NULL;
        } else {
            list->tail = NULL;
        }
        list->head = n;

        fds_verify(list->count > 0);
        list->count--;

        /*
 *          * Reset the link fields within the element
 *                   */
        fds_list_init_elem(elem);
    } else {
        fds_assert(list->tail == NULL);
        fds_verify(list->count == 0);
    }

    fds_list_wunlock(list);
    return elem;
}
