/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef BUILDSTR_H_
#define BUILDSTR_H_

#include <fds_process.h>

namespace fds {

#ifdef DEBUG
const char* buildStrTmpl = "Formation Data Systems: Formation Dynamic Storage: %s\nBuild Version: <%s-%s>\tBuild Mode: <Diagnostic>\tBuild Date: <" __DATE__ " " __TIME__ ">\n%s\n";
#else
const char* buildStrTmpl = "Formation Data Systems: Formation Dynamic Storage: %s\nBuild Version: <%s-%s>\tBuild Mode: <Optimized>\tBuild Date: <" __DATE__ " " __TIME__ ">\n%s\n";
#endif

}  // namespace fds

#endif  // BUILDSTR_H_
