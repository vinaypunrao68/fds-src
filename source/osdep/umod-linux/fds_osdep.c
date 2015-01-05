#define _GNU_SOURCE
#include <stdarg.h>
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <fds_assert.h>
#include <shared/fds_types.h>
#include <platform/fds-osdep.h>

#define STACK_TRACE_SIZE             (32)

/*
 * fds_panic
 * ---------
 */

static void __attribute__((noinline))
            __attribute__((noreturn))
fds_panic_abort(char *panic_string)
{
    void     *stack[STACK_TRACE_SIZE];
    char    **symb;
    size_t    i, size;

    size = backtrace(stack, STACK_TRACE_SIZE);
    symb = backtrace_symbols(stack, size);

    /* TODO(Sean):
     * In this case, where does printf() go for non-standalone tests?
     * Is the stdout stream redirected or is it print to the screen?
     * The panic message and stack trace should be logged in a log file.
     *
     * TODO(Sean):
     * Second thing....   It's possible to get symbol names using
     * gcc option "-rdynamic" when compiling, but it has some performance
     * implications.  Could be useful in debug build tests.
     */
    printf("Dump recent %ld stack frames\n", size);
    for (i = 0; i < size; i++) {
        printf("%s\n", symb[i]);
    }

    /* abort() should be the last system call in this function
     */
    abort();
}

void
fds_panic(const char *fmt, ...)
{
    va_list args;
    char panic_str_buf[2048];
    int ret;

    va_start(args, fmt);
    ret = vsnprintf(panic_str_buf, sizeof(panic_str_buf), fmt, args);
    /* If bytes written is greater than equal, then the string was truncated.
     * If truncated, then add null terminating character to the last offset
     * in the panic string buffer.
     */
    if (ret >= sizeof(panic_str_buf)) {
        panic_str_buf[sizeof(panic_str_buf) - 1] = '\0';
    }
    va_end(args);

    /* This will show up on the stack, and when using GDB, panic_str_buf
     * should show up fully formatted.
     */
    fds_panic_abort(panic_str_buf);
}

int
fds_get_fd_limit(void)
{
    fds_int64_t   slim, rlim;
    struct rlimit rl;

    slim = sysconf(_SC_OPEN_MAX);
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        rlim = 0;
    } else {
        rlim = rl.rlim_max;
    }
    if (slim > rlim) {
        return (slim);
    }
    if (slim < 0) {
        return (10000);
    }
    return (rlim);
}

/*
 * fds_spawn
 * ---------
 */
pid_t
fds_spawn(char *const argv[], int daemonize)
{
    int     flim, fd, res;
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
    /* Child process, close all file descriptors. */
    flim = fds_get_fd_limit();
    printf("Close fd up to %d\n", flim);
    fflush(stdout);

    for (fd = 0; fd < flim; fd++) {
        close(fd);
    }
    fd = open("/dev/null", O_RDWR);  // will be 0
    (void) dup(fd); // will be 1
    (void) dup(fd); // will be 2


    if (daemonize) {
        res = daemon(1, 1);
        if (res != 0) {
            fprintf(stderr, "Fatal error, can't daemonize %s\n", argv[0]);
            abort();
        }
    }
    /* actual child process */
    execvp(argv[0], argv);
    fprintf(stderr, "Fatal error, can't spawn %s\n", argv[0]);
    abort();
}

/*
 * fds_spawn_service
 * -----------------
 */
pid_t
fds_spawn_service(const char *prog,
                  const char *fds_root,
		  const char** extra_args,
		  int daemonize)
{
    size_t len, ret;
    char   exec[1024];
    char   root[1024];
    char  *argv[12];
    int i = 0;
    int j = 0;
    argv[i++] = exec;
    argv[i++] = root;
    for (; extra_args[j] != NULL && j < 10; i++, j++) {
    	argv[i] = (char*) extra_args[j];
    }
    /* Only allow 10 args for now */
    fds_verify(extra_args[j] == NULL);
    argv[i] = NULL;

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
