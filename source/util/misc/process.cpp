#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util/process.h>

namespace fds {
namespace util {
void printBackTrace() {
    int j, nptrs;
    void *buffer[100];
    char **strings;

    nptrs = backtrace(buffer, 100);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    GLOGDEBUG << "----- BACKTRACE BEGIN -----";

    if (strings == NULL) {
        GLOGDEBUG << "----- BACKTRACE END -----";
        return;
    }
    

    for (j = 1; j < nptrs; j++) {
        GLOGDEBUG << (j-1) << ":" << buffer[j] << " - "<< strings[j];
    }
    GLOGDEBUG << "----- BACKTRACE END -----";
    free(strings);
}
}  // namespace util
}  // namespace fds
