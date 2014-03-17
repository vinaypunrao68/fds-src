#define _GNU_SOURCE
#include <stdarg.h>
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fds_assert.h>
#include <platform/fds-osdep.h>

#define STACK_TRACE_SIZE             (32)

/*
 * fds_panic
 * ---------
 */
void
fds_panic(const char *fmt, ...)
{
    va_list   ap;
    size_t    i, size;
    void     *stack[STACK_TRACE_SIZE];
    char    **symb;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    size = backtrace(stack, STACK_TRACE_SIZE);
    symb = backtrace_symbols(stack, size);

    printf("Dump recent %ld stack frames\n", size);
    for (i = 0; i < size; i++) {
        printf("%s\n", symb[i]);
    }
    abort();
}

/*
 * fds_spawn
 * ---------
 */
pid_t
fds_spawn(char *const argv[], int daemonize)
{
    int     res;
    pid_t   child_pid;
    pid_t   res_pid;

    child_pid = fork();
    if (child_pid != 0) {
        if (daemonize) {
            res_pid = waitpid(child_pid, NULL, 0);
            // TODO(bao): check for 0 and -1
            fds_assert(res_pid == child_pid);
            return (0);
        } else {
            return (child_pid);
        }
    }

    if (daemonize) {
        res = daemon(0, 1);
        if (res != 0) {
            printf("Fatal error, can't daemonize %s\n", argv[0]);
            abort();
        }
    }

    /* actual child process */
    execvp(argv[0], argv);
    printf("Fatal error, can't spawn %s\n", argv[0]);
    abort();
}

/*
 * fds_spawn_service
 * -----------------
 */
pid_t
fds_spawn_service(const char *prog, const char *fds_root, int daemonize)
{
    size_t len, ret;
    char   exec[1024];
    char   root[1024];
    char  *argv[] = { exec, root, NULL };

    if (getcwd(exec, sizeof (exec)) == NULL) {
        perror("Fatal from getcwd()");
        abort();
    }
    len = strlen(exec);
    ret = snprintf(exec + len, sizeof (exec) - len, "/%s", prog);
    if (ret == (sizeof (exec) - len)) {
        printf("The exec path name is too long: %s\n", exec);
        exit(1);
    }
    ret = snprintf(root, sizeof (root), "--fds-root=%s", fds_root);
    if (ret >= sizeof (root)) {
        printf("The fds-root path name is too long: %s\n", root);
        exit(1);
    }
    /*
     * XXX(Vy): we're using fds_root as prefix to config DB, strip out the
     * ending '/' so that the child process can use the correct key.
     */
    for (ret--; ret > 0 && root[ret] == '/'; ret--);
    root[ret + 1] = '\0';

    printf("Spawn %s %s\n", exec, root);
    return (fds_spawn(argv, daemonize));
}
