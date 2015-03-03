/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <set>
#include <string>
#include <thread>

extern "C" {
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>
}

#include <ev++.h>
#include <boost/shared_ptr.hpp>

#include "connector/block/NbdConnector.h"
#include "connector/block/NbdConnection.h"
#include "OmConfigService.h"
#include "fds_process.h"

namespace fds {

NbdConnector::NbdConnector(OmConfigApi::shared_ptr omApi)
        : omConfigApi(omApi),
          nbdPort(10809) {
    initialize();
}

void NbdConnector::initialize() {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.nbd.");
    nbdPort = conf.get<uint32_t>("server_port", nbdPort);

    // Shutdown the socket if we are reinitializing
    if (0 > nbdSocket)
        { deinit(); }

    // Bind to NBD listen port
    nbdSocket = createNbdSocket();
    fds_verify(nbdSocket > 0);

    // Setup event loop
    if (!evIoWatcher) {
        evIoWatcher = std::unique_ptr<ev::io>(new ev::io());
        evIoWatcher->set<NbdConnector, &NbdConnector::nbdAcceptCb>(this);
        evIoWatcher->start(nbdSocket, ev::READ);
        // Run event loop in thread
        runThread.reset(new std::thread(&NbdConnector::runNbdLoop, this));
        runThread->detach();
    } else {
        evIoWatcher->set(nbdSocket, ev::READ);
    }
}

void NbdConnector::deinit() {
    if (0 <= nbdSocket) {
        shutdown(nbdSocket, SHUT_RDWR);
        close(nbdSocket);
        nbdSocket = -1;
    }
};

NbdConnector::~NbdConnector() {
    deinit();
}

void
NbdConnector::nbdAcceptCb(ev::io &watcher, int revents) {
    if (EV_ERROR & revents) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    int clientsd = 0;
    while (0 <= clientsd) {
        socklen_t client_len = sizeof(sockaddr_in);
        sockaddr_in client_addr;

        // Accept a new NBD client connection
        do {
            clientsd = accept(watcher.fd,
                              (sockaddr *)&client_addr,
                              &client_len);
        } while ((0 > clientsd) && (EINTR == errno));

        if (0 <= clientsd) {
            // Create a handler for this NBD connection
            // Will delete itself when connection dies
            NbdConnection *client = new NbdConnection(omConfigApi, clientsd);
            LOGNORMAL << "Created client connection...";
        } else {
            switch (errno) {
            case ENOTSOCK:
            case EOPNOTSUPP:
            case EINVAL:
            case EBADF:
                // Reinitialize server
                LOGWARN << "Accept error: " << strerror(errno);
                initialize();
                break;
            default:
                break; // Nothing special, no more clients
            }
        }
    };
}

int
NbdConnector::createNbdSocket() {
    sockaddr_in serv_addr;
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
                    (sockaddr*)&serv_addr,
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