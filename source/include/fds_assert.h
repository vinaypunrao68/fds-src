/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_ASSERT_H_
#define SOURCE_INCLUDE_FDS_ASSERT_H_

/* OSDEP assert macros.  Don't include any header files here. */
#ifdef __cplusplus
extern "C" {
#endif

#define LIKELY(_expr) (__builtin_expect(!!(_expr), 1))
#define UNLIKELY(_expr) (__builtin_expect(!!(_expr), 0))

#define cc_assert(label, cond)  enum { ASSERT##label = 1/(cond) }
#define panic_attr              __attribute__((noreturn, format(printf, 1, 2)))

extern void fds_panic(const char *fmt, ...) panic_attr;

#define fds_verify(expr)                                                     \
    if (UNLIKELY(!(expr))) {                                                           \
        fds_panic("Assertion \"%s\" fails at %s, line %d\n",                 \
                  #expr, __FILE__, __LINE__);                                \
    }

#ifdef DEBUG
#define fds_assert(expr) fds_verify(expr)

/* Statement that is only enabled in debug build */
#define DBG(statement) statement

#else   /* DEBUG */

#define fds_assert(expr)

#define DBG(statement)

#endif  /* DEBUG */

/* For marking certain functions virtual only in test builds */
// TODO(Rao): Create a TEST define and ifdef TVIRTUAL around that
#define TVIRTUAL virtual

#ifdef __cplusplus
}
#endif
#endif  // SOURCE_INCLUDE_FDS_ASSERT_H_
