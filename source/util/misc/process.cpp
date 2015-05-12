#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cxxabi.h>

#include "util/process.h"

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
	stacktrace << "STACK TRACE:\n";

	/* storage array for stack trace addess data */
	void * addrlist[ max_frames + 1 ];

	/* retrieve current stack addresses */
	int addrlen = backtrace( addrlist,
			sizeof( addrlist ) / sizeof( void * ) );
	if ( addrlen == 0 )
	{
		stacktrace << "\t<empty, possibly corrupt>\n";
	}
	else
	{
		/*
		 * resolve address into stringsd containing "filename(function + address)"
		 * NOTE: this array must be free()-ed
		 */
		char ** symbollist = backtrace_symbols(addrlist, addrlen);

		/* allocate string which will be filled with teh de-mangled function name */
		size_t funcnamesize = 256;
		char * funcname = ( char * ) malloc( funcnamesize );

		/*
		 * iterate over the returned symbol lines. skip the first, it is the
		 * address of this function.
		 */
		for ( int i = 1; i < addrlen; ++i )
		{
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
					stacktrace << "\t" << symbollist[i] << " : " << funcname << begin_offset << "\n";
				}
				else
				{
					/*
					 * de-mangling failed. Output function name as a C function with no arguments.
					 */
					stacktrace << "\t" << symbollist[i] << " : " << begin_name << "( )" << begin_offset << "\n";
				}
			}
			else
			{
					/* couldn't parse line, so print the whole line! */
					stacktrace << "\t" << symbollist[i] << "\n";
			}
		}

		free(funcname);
		free(symbollist);
	}

	GLOGDEBUG << stacktrace.str();

	return;
}
}  // namespace util
}  // namespace fds
