#ifndef SOURCE_UTIL_DEBUGHELPERS_H_
#define SOURCE_UTIL_DEBUGHELPERS_H_
#include <util/Log.h>
#include <fds_globals.h>
/**
 * Helper functions to trace function call
 */

namespace fds {
    /**
     * usage: 
     * void myfunc() {
     *    FUNCTRACER(); // Note - it is the first line
     *    ...
     *    ...
     * }
     */
    struct FnTrace {
        FnTrace(const char* name,const char* filename, int linenum=-1) {
            this->name = name;
            this->filename = filename;
            this->linenum = linenum;

            FDS_PLOG_SEV(g_fdslog,fds::fds_log::debug) << "entering [" << name <<" @ " << filename <<":"<<linenum <<"]"; 
        }

        ~FnTrace () {
            FDS_PLOG_SEV(g_fdslog,fds::fds_log::debug) << "exiting  [" << name <<" @ " << filename <<":"<<linenum <<"]"; 
        }
      private:
        const char *name,*filename;
        int linenum;
    };

#ifndef NOFNTRACE 
#define FUNCTRACER(...) fds::FnTrace __fn_trace__(__FUNCTION__, __FILE__, __LINE__)
#else
#define FUNCTRACER(...) 
#endif

} // namsepace fds

#endif
