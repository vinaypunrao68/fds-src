#include <execinfo.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <cxxabi.h>
#include <chrono>

#include "util/process.h"

extern ssize_t fds_exe_path(char* pBuf, size_t len);

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

/** Print a demangled stack backtrace of the caller function to FILE* out. */
void print_stacktrace(unsigned int max_frames)
{
	std::ostringstream stacktrace;

	// Some gyration here to come up with a timestamp without a newline appended.
    std::chrono::time_point<std::chrono::system_clock> timeFmt1 = std::chrono::system_clock::now();
    std::time_t timeFmt2 = std::chrono::system_clock::to_time_t(timeFmt1);
	std::ostringstream timeFmt3;
	timeFmt3 << std::ctime(&(timeFmt2));
	std::string timeFmt4 = timeFmt3.str();
	timeFmt4 = timeFmt4.substr(0, timeFmt4.length() - 1);


	stacktrace << timeFmt4 << ": BACK TRACE:\n";

	/* storage array for stack trace frame data */
	void *callstack[ max_frames + 1 ];

	/* retrieve current stack addresses */
	int nFrames = backtrace(callstack, sizeof(callstack) / sizeof( void * ) );
	if ( nFrames == 0 )
	{
		stacktrace << "\t<empty, possibly corrupt>\n";
	}
	else
	{
		bool logLineNumbers = true;

		/*
		 * Let's see what this looks like. We'll try a demangled version below.
		 */
		 printf("%s: Mangled name backtace:\n", timeFmt4.c_str());
		 fflush(stdout);
		 backtrace_symbols_fd(callstack, nFrames, STDOUT_FILENO);
		 printf("%s: End mangled name backtace:\n", timeFmt4.c_str());
		 fflush(stdout);

		/*
		 * resolve address into stringsd containing "filename(function + address)"
		 * NOTE: this array must be free()-ed
		 */
		char ** symbollist = backtrace_symbols(callstack, nFrames);

		/* allocate string which will be filled with teh de-mangled function name */
		size_t funcnamesize = 256;
		char * funcname = ( char * ) malloc( funcnamesize );

		/*
		 * iterate over the returned symbol lines. skip the first, it is the
		 * address of this function.
		 */
		printf("%s: File/line number backtace:\n", timeFmt4.c_str());
		fflush(stdout);
		for ( int i = 1; i < nFrames; ++i )
		{
// Two options for logging a backtrace.
#if 1  // One option
			Dl_info info;
			char buf[1024];

			if (dladdr(callstack[i], &info) && info.dli_sname) {
				char *demangled = NULL;
				int status = -1;

				if (info.dli_sname[0] == '_') {
					demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
				}

				snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n",
				i, int(2 + sizeof(void*) * 2), callstack[i],
				status == 0 ? demangled :
				info.dli_sname == 0 ? symbollist[i] : info.dli_sname,
				(char *)callstack[i] - (char *)info.dli_saddr);

				free(demangled);

				stacktrace << "[bt-dmg]\t" << buf;
			} else {
				snprintf(buf, sizeof(buf), "%-3d %*p %s\n",
				i, int(2 + sizeof(void*) * 2), callstack[i], symbollist[i]);

				stacktrace << "[bt-mgl]\t" << buf;
			}
#else  // Another option
			char *begin_name = nullptr;
			char *begin_offset = nullptr;
			char *end_offset = nullptr;

			/*
			 * find parentheses and +address offset surrounding the mangled name:
			 * ./module(function+0x15c) [0x8048a6d]
			 */
			for ( char *p = symbollist[i]; *p; ++p )
			{
				if ( *p == '(' )
				{
					begin_name = p;
				}
				else if ( *p == '+' )
				{
					begin_offset = p;
				}
				else if ( *p == ')' && begin_offset )
				{
					end_offset = p;
					break;
				}
			}

			if ( begin_name && begin_offset && end_offset && begin_name < begin_offset )
			{
				*begin_name++ = '\0';
				*begin_offset++ = '\0';
				*end_offset = '\0';

				/*
				 * mangled name is now in [begin_name, begin_offset) and caller
				 * offset in [begin_offset, end_offset). now apply __cxa_demangle():
				 */

				int status;
				char *ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
				if ( status == 0 )
				{
					funcname = ret;
					stacktrace << "[bt-dmg]\t" << symbollist[i] << " : " << funcname << begin_offset << "\n";
				}
				else
				{
					/*
					 * de-mangling failed. Output function name as a C function with no arguments.
					 */
					stacktrace << "[bt-mgl]\t" << symbollist[i] << " : " << begin_name << "( )" << begin_offset << "\n";
				}
			}
			else
			{
					/* couldn't parse line, so print the whole line! */
					stacktrace << "[bt-npr]\t" << symbollist[i] << "\n";
			}
#endif

			if (logLineNumbers) {
				int rc;
				char* exe;
				const size_t exe_path_size = 192;
				char exe_path[exe_path_size];
				char syscom[256];

				/**
				 * The last parameter should be the full path to the executable. Also
				 * in the case of OM where we have a c++ library being driven by the
				 * JVM and it's all started wth a bash script, this just does not work
				 * at all.
				 */
				errno = 0;
				if (fds_exe_path(exe_path, exe_path_size) < 0) {
					GLOGERROR << "fds_exe_path() failed with errno <" << errno;
					logLineNumbers = false;
				} else {
					errno = 0;
					sprintf(syscom, "addr2line %p -e %s", callstack[i], exe_path);
					if ((rc = system(syscom)) != 0) {
						GLOGERROR << "system() failed executing <" << syscom << "> with return code " <<
								  std::to_string(rc) << " and errno " << errno;
						logLineNumbers = false;
					}
				}
			}
		}
		printf("%s, End file/line number backtace:\n", timeFmt4.c_str());
		fflush(stdout);

		free(funcname);
		free(symbollist);
	}

	GLOGNOTIFY << stacktrace.str();

	return;
}

/**
 * ===================  SUBPROCESS  =========================
 */

bool SubProcess::run(const std::string& cmd, bool readMode) {
    const char* mode;
    if (readMode) mode = "r";
    else mode= "w";
    GLOGDEBUG << "running cmd: [" << cmd << "]";
    fp = popen(cmd.c_str(), mode);
    return fp != NULL;
}

bool SubProcess::write(const std::string& cmd) {
    int ret = fputs(cmd.c_str(), fp);
    fflush(fp);
    return ret != EOF;
}

bool SubProcess::read(std::string& data, int sz) {
    char line[sz];
    char *ret = fgets(line, sz,fp);
    if (ret) data=line;
    else data.clear();
    return ret!=NULL;
}

bool SubProcess::readLine(std::string& data) {
    char *line = NULL;
    size_t len = 0;
    ssize_t bytes;
    bytes = getline(&line, &len, fp);
    if (bytes != -1) {
        while(bytes > 0 && (line[bytes-1] == '\n' || line[bytes-1] == '\r')) {
            line[bytes-1] = 0;
            bytes--;
        }
        data=line;
    } else {
        data.clear();
    }
    if (line) free(line);
    return bytes != -1;
}

bool SubProcess::close() {
    if (fp) {
        auto ret = pclose(fp);
        fp = NULL;
        return -1 != ret;
    }
    return true;
}

SubProcess::~SubProcess() {
    close();
}


}  // namespace util
}  // namespace fds
