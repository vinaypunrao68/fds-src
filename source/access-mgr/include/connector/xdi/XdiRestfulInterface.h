/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_XDIRESTFULINTERFACE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_XDIRESTFULINTERFACE_H_

// System includes
#include <string>

namespace fds
{

class XdiRestfulInterface {
public:
    // Timeout in milliseconds
    static unsigned int         TIMEOUT;

    XdiRestfulInterface(std::string const& host, unsigned int const port);
    ~XdiRestfulInterface();

    bool flushVolume(std::string const& volume);
private:
    std::string         _host;
    unsigned int        _port;

};

} // namespace fds

#endif // SOURCE_ACCESS_MGR_INCLUDE_XDIRESTFULINTERFACE_H_
