/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <set>
#include <string>

extern "C" {
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>
}

#include <ev++.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "connector/block/NbdConnector.h"
#include "connector/block/NbdConnection.h"
#include "OmConfigService.h"
#include "fds_process.h"

namespace fds {

NbdConnector::NbdConnector(OmConfigApi::shared_ptr omApi)
        : omConfigApi(omApi),
          nbdPort(10809) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.nbd.");
    nbdPort = conf.get<uint32_t>("server_port");

    // Bind to NBD listen port
    nbdSocket = createNbdSocket();
    fds_verify(nbdSocket > 0);

    // Setup event loop
    evIoWatcher = std::unique_ptr<ev::io>(new ev::io());
    evIoWatcher->set<NbdConnector, &NbdConnector::nbdAcceptCb>(this);
    evIoWatcher->start(nbdSocket, ev::READ);

    // Run event loop in thread
    runThread.reset(new boost::thread(
        boost::bind(&NbdConnector::runNbdLoop, this)));
}

NbdConnector::~NbdConnector() {
    shutdown(nbdSocket, SHUT_RDWR);
    close(nbdSocket);
}

void
NbdConnector::nbdAcceptCb(ev::io &watcher, int revents) {
    if (EV_ERROR & revents) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Accept a new NBD client connection
    int clientsd = accept(watcher.fd,
                          (struct sockaddr *)&client_addr,
                          &client_len);

    if (clientsd < 0) {
        LOGERROR << "Accept error";
        return;
    }

    // Create a handler for this NBD connection
    // Will delete itself when connection dies
    LOGNORMAL << "Creating client connection...";
    NbdConnection *client = new NbdConnection(omConfigApi, clientsd);
    LOGDEBUG << "Created client connection...";
}

int
NbdConnector::createNbdSocket() {
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(nbdPort);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        LOGERROR << "Failed to create NBD socket";
        return listenfd;
    }

    // If we crash this allows us to reuse the socket before it's fully closed
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        LOGWARN << "Failed to set REUSEADDR on NBD socket";
    }

    fds_verify(bind(listenfd,
                    (struct sockaddr*)&serv_addr,
                    sizeof(serv_addr)) == 0);
    fcntl(listenfd, F_SETFL, fcntl(listenfd, F_GETFL, 0) | O_NONBLOCK);
    listen(listenfd, 10);

    return listenfd;
}

void
NbdConnector::runNbdLoop() {
    LOGNORMAL << "Accepting NBD connections on port " << nbdPort;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    if (0 != pthread_sigmask(SIG_BLOCK, &set, nullptr)) {
        LOGWARN << "Failed to enable SIGPIPE mask on NBD server.";
    }

    ev::default_loop loop;
    loop.run(0);
    LOGNORMAL << "Stopping NBD loop...";
}

}  // namespace fds
