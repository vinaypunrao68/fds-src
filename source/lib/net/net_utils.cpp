/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <sys/unistd.h>
#include <cstring>
#include <string>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/net_utils.h>
#include <fds_assert.h>

namespace fds {
namespace net {
std::string get_my_hostname()
{
    char name[255];
    if (gethostname(name, 255) == 0) {
        return std::string(name);
    }
    return std::string();
}

/**
 * @return local ip
 */
std::string get_local_ip(std::string ifc)
{
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;
    std::string myIp;

    /*
     * Get the local IP of the host.  This is needed by the OM.
     */
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_INET) {  // IPv4
            if (strcmp(ifa->ifa_name, ifc.c_str()) == 0) {
                tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;  // NOLINT
                char addrBuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
                myIp = std::string(addrBuf);
            }
        }
    }
    if (ifAddrStruct != NULL) {
        freeifaddrs(ifAddrStruct);
    }
    fds_verify(myIp.size() > 0 && "Request ifc not found");
    return myIp;
}
}  // namespace net
}  // namespace fds
