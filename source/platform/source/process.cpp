/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
}
#include <platform/process.h>
#include <platform/valgrind.h>
#include <vector>

#include "util/Log.h"
#include "fds_assert.h"
#include "shared/fds_types.h"

namespace fds
{
    namespace pm
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

            int    j = 0;

            std::ostringstream commandBuffer;

            for (j = 0; argv[j]!= NULL; j++)
            {
                commandBuffer << argv[j] << " ";
            }

            LOGNORMAL << "Preparing to fork and exec:  " << commandBuffer.str();

            // Generate a file for the child's stdout/stderr output.
            const char stdouterrTmpl[] = "%s.out";
            char stdouterr[256];
            snprintf(stdouterr, 256, stdouterrTmpl, argv[0]);
            // However, this gets logged to the parent's stdout.
            LOGNOTIFY << "Child to write stderr/stdout to " << stdouterr;

            child_pid = fork();

            if (child_pid > 0)
            {
                if (daemonize)
                {
                    res_pid = waitpid(child_pid, NULL, 0);
                    // TODO(bao): check for 0 and -1
                    fds_assert(res_pid == child_pid);
                }
                return child_pid;
            }
            else if (child_pid < 0)
            {
                LOGERROR << "fds_spawn fork failure:  errno = " << errno;
            }

            // In the child process, No logging between fork and exec

            // This sets the process's process group id.  Using this prevents children from being killed when a parent is delivered a sigterm (service ... stop).
            if (-1 == setpgid (0, 0))
            {
                printf("setpgid() failure:  errno = %d.", errno);
            }

            /* Close all file descriptors. */
            flim = fds_get_fd_limit();

            for (fd = 0; fd < flim; fd++)
            {
                if ((fd != STDOUT_FILENO) && (fd != STDERR_FILENO)) {
                    close(fd);
                }
            }

            fd = open("/dev/null", O_RDWR);  // will be file descriptor 0, STDIN_FILENO

            // Will be file descriptor 1, STDOUT_FILENO
            if (freopen(stdouterr, "a", stdout) == nullptr) {
                printf("freopen failed for stdout with errno %d.", errno);
            }

            // Will be file descriptor 2, STDERR_FILENO
            if (freopen(stdouterr, "a", stderr) == nullptr) {
                printf("freopen failed for stderr with errno %d.", errno);
            }
            printf("Child to write stderr/stdout to %s.", stdouterr);

            if (daemonize)
            {
                res = daemon(1, 1);

                if (res != 0)
                {
                    abort();
                }
            }

            /* Start actual child process */
            execvp(argv[0], argv);
            printf("fds_spawn execvp failure:  errno = %d.", errno);
            abort();
        }

        pid_t fds_spawn_service(const std::string& prog, const std::string& fds_root, const std::vector<std::string>& args, bool daemonize)
        {
            std::vector<const char*>    c_args;

            for (const auto& arg : args)
            {
                c_args.push_back(arg.c_str());
            }

            c_args.push_back(NULL);

            return fds_spawn_service(prog.c_str(), fds_root.c_str(), &c_args[0], daemonize ? 1 : 0);
        }

        /*
         * fds_spawn_service
         * -----------------
         */
        pid_t fds_spawn_service(const char *prog, const char *fds_root, const char** extra_args, int daemonize)
        {
            size_t    len {0}, ret;
            char      exec[1024];
            char      root[1024];
            char     *argv[32];
            int       argvIndex = 0;
            int       extraIndex = 0;

            std::string progStr (prog);
            std::size_t found = progStr.rfind("java");
            bool is_java = (found != std::string::npos && (found == (progStr.length() - strlen("java"))));

            // This needs to live through the lifetime of the exec call
            ValgrindOptions valgrind_options(prog, fds_root);
            if (!is_java && valgrind_options.runningOnUnnestedValgrind()) {
                // Adjust the argv to actually run valgrind...the rest of the
                // arguments should be the same
                for (auto const& option : valgrind_options()) {
                    // This const cast is safe as long as the valgrind_options
                    // instance is still alive throughout its usage in execvp
                    argv[argvIndex++] = const_cast<char*>(option->data());
                }
            }

            argv[argvIndex++] = exec;

            for (; extra_args[extraIndex] != NULL && extraIndex < 10; argvIndex++, extraIndex++)
            {
                argv[argvIndex] = (char*) extra_args[extraIndex];
            }

            /* Only allow 10 args for now */
            fds_verify(extra_args[extraIndex] == NULL);

            if (!is_java)
            {
                if (getcwd(exec, sizeof (exec)) == NULL)
                {
                    perror("Fatal from getcwd()");
                    abort();
                }

                len = strlen(exec);
                ret = snprintf(exec + len, sizeof (exec) - len, "/%s", prog);
            }
            else
            {
                ret = snprintf(exec, sizeof (exec), "%s", prog);
            }

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

            argv[argvIndex++] = root;
            argv[argvIndex] = NULL;

            /*
             * XXX(Vy): we're using fds_root as prefix to config DB, strip out the
             * ending '/' so that the child process can use the correct key.
             */
            for (ret--; ret > 0 && root[ret] == '/'; ret--)
            {
            }

            root[ret + 1] = '\0';

            printf("Spawn %s %s\n", exec, root);
            LOGDEBUG << "Spawn " << exec ;

            return (fds_spawn(argv, daemonize));
        }
    }  // namespace pm
}  // namespace fds
