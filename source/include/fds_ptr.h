/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PTR_H_
#define SOURCE_INCLUDE_FDS_PTR_H_

#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>

namespace bo = boost;

/**
 * Template to embed intrusive pointer to an object.
 */
#define INTRUSIVE_PTR_DEFS(Type, refcnt)                                       \
    mutable boost::atomic<int>  refcnt {0};                                    \
    friend void intrusive_ptr_add_ref(const Type *x) {                         \
        x->refcnt.fetch_add(1, boost::memory_order_relaxed);                   \
    }                                                                          \
    friend void intrusive_ptr_release(const Type *x) {                         \
        if (x->refcnt.fetch_sub(1, boost::memory_order_release) == 1) {        \
            boost::atomic_thread_fence(boost::memory_order_acquire);           \
            delete x;                                                          \
        }                                                                      \
    }

#define INTRUSIVE_PTR_OPS(refcnt)                                              \
    inline void obj_inc_refcnt() {                                             \
        refcnt.fetch_add(1, boost::memory_order_relaxed);                      \
    }                                                                          \
    inline void obj_dec_refcnt() {                                             \
        intrusive_ptr_release(this);                                           \
    }

#endif  // SOURCE_INCLUDE_FDS_PTR_H_
