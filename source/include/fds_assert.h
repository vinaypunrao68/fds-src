#ifndef INCLUDE_FDS_ASSERT_H_
#define INCLUDE_FDS_ASSERT_H_

/* OSDEP assert macros.  Don't include any header files here. */
#ifdef __cplusplus
extern "C" {
#endif

#define cc_assert(label, cond)  enum { ASSERT##label = 1/(cond) }
#define panic_attr              __attribute__((noreturn, format(printf, 1, 2)))

extern void fds_panic(const char *fmt, ...) panic_attr;

#define fds_verify(expr)                                                     \
    if (!(expr)) {                                                           \
        fds_panic("Assertion \"%s\" fails at %s, line %d\n",                 \
                  #expr, __FILE__, __LINE__);                                \
    }

#ifdef DEBUG
#define fds_assert(expr)                                                     \
    if (!(expr)) {                                                           \
        fds_panic("Assertion \"%s\" fails at %s, line %d\n",                 \
                  #expr, __FILE__, __LINE__);                                \
    }

/* Statement that is only enabled in debug build */
#define DBG(statement) statement

#else  /* DEBUG */

#define fds_assert(expr)

#define DBG(statement)

#endif /* DEBUG */

#ifdef DEBUG
#else
#endif

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_FDS_ASSERT_H_ */
