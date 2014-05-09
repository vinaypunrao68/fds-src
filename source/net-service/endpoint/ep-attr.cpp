/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/net-service.h>

namespace fds {

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Attributes
 * -----------------------------------------------------------------------------------
 */
EpAttr::EpAttr(fds_uint32_t ip, int port) : ep_refcnt(0)
{
    ep_addr.sa_family = AF_INET;
    ((struct sockaddr_in *)&ep_addr)->sin_port = port;
}

EpAttr &
EpAttr::operator = (const EpAttr &rhs)
{
    ep_addr = rhs.ep_addr;
    return *this;
}

int
EpAttr::attr_get_port()
{
    if (ep_addr.sa_family == AF_INET) {
        return (((struct sockaddr_in *)&ep_addr)->sin_port);
    }
    return (((struct sockaddr_in6 *)&ep_addr)->sin6_port);
}

int
EpAttr::netaddr_get_port(const struct sockaddr *adr)
{
    if (adr->sa_family == AF_INET) {
        return (((struct sockaddr_in *)adr)->sin_port);
    }
    return (((struct sockaddr_in6 *)adr)->sin6_port);
}
}  // namespace fds
