/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>

#include "fds_assert.h"
#include "shared/fds_types.h"

namespace fds
{
    int fds_get_fd_limit(void)
    {
        fds_int64_t      slim, rlim;
        struct rlimit    rl;
    
        slim = sysconf(_SC_OPEN_MAX);
    
        if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        {
            rlim = 0;
        } else {
            rlim = rl.rlim_max;
        }
    
        if (slim > rlim)
        {
            return (slim);
        }
    
        if (slim < 0)
        {
            return (10000);
        }
    
        return (rlim);
    }
    
    /*
     * fds_spawn
     * ---------
     */
    pid_t fds_spawn(char *const argv[], int daemonize)
    {
        int      flim, fd, res;
        pid_t    child_pid;
        pid_t    res_pid;
    
        child_pid = fork();
    
        if (child_pid != 0)
        {
            if (daemonize)
            {
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
    
        for (fd = 0; fd < flim; fd++)
        {
            close(fd);
        }
   
        // There is probably a better way, but for now, create a dummy variable to capture the
        // return value from dup().  This prevents a compiler warning when compiling with -O2 
        fd = open("/dev/null", O_RDWR);  // will be 0
        int unused_discard = dup(fd); // will be 1
        unused_discard = dup(fd);     // will be 2
    
        if (daemonize)
        {
            res = daemon(1, 1);
    
            if (res != 0)
            {
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
    pid_t fds_spawn_service(const char *prog, const char *fds_root, const char** extra_args,
                            int daemonize)
    {
        size_t    len, ret;
        char      exec[1024];
        char      root[1024];
        char     *argv[12];
        int       i = 0;
        int       j = 0;
        argv[i++] = exec;
        argv[i++] = root;
        for (; extra_args[j] != NULL && j < 10; i++, j++)
        {
            argv[i] = (char*) extra_args[j];
        }
    
        /* Only allow 10 args for now */
        fds_verify(extra_args[j] == NULL);
        argv[i] = NULL;
    
        if (getcwd(exec, sizeof (exec)) == NULL)
        {
            perror("Fatal from getcwd()");
            abort();
        }
    
        len = strlen(exec);
        ret = snprintf(exec + len, sizeof (exec) - len, "/%s", prog);
    
        if (ret == (sizeof (exec) - len))
        {
            printf("The exec path name is too long: %s\n", exec);
            exit(1);
        }
    
        ret = snprintf(root, sizeof (root), "--fds-root=%s", fds_root);
    
        if (ret >= sizeof (root))
        {
            printf("The fds-root path name is too long: %s\n", root);
            exit(1);
        }
    
        /*
         * XXX(Vy): we're using fds_root as prefix to config DB, strip out the
         * ending '/' so that the child process can use the correct key.
         */
        for (ret--; ret > 0 && root[ret] == '/'; ret--)
        {
            ;
        }
        root[ret + 1] = '\0';
    
        printf("Spawn %s %s\n", exec, root);
        return (fds_spawn(argv, daemonize));
    }
}  // namespace fds
