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

#include "connector/scst/ScstConnector.h"
#include "connector/scst/ScstConnection.h"
#include "fds_process.h"

namespace fds {

// The singleton
std::unique_ptr<ScstConnector> ScstConnector::instance_ {nullptr};

void ScstConnector::start(std::weak_ptr<AmProcessor> processor) {
    static std::once_flag init;
    // Initialize the singleton
    std::call_once(init, [processor] () mutable
    {
        FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.scst.");
        auto threads = conf.get<uint32_t>("threads", 1);
        instance_.reset(new ScstConnector(processor, threads - 1));
        // Start the main server thread
        auto t = std::thread(&ScstConnector::follow, instance_.get());
        t.detach();
    });
}

void ScstConnector::stop() {
    if (instance_ && instance_->asyncWatcher) {
       instance_->asyncWatcher->send();
    }
}

ScstConnector::ScstConnector(std::weak_ptr<AmProcessor> processor,
                           size_t const followers)
        : LeaderFollower(followers, false),
          amProcessor(processor) {
    LOGDEBUG << "Initialized server with: " << followers << " followers.";
    initialize();
}

void ScstConnector::initialize() {
    // Shutdown the socket if we are reinitializing
    if (0 <= scstDev)
        { reset(); }

    // Bind to SCST listen port
    scstDev = createScstSocket();
    if (scstDev < 0) {
        LOGERROR << "Could not bind to SCST port. No Scst attachments can be made.";
        return;
    }

    // This is our async event watcher for shutdown
    if (!asyncWatcher) {
        asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
        asyncWatcher->set<ScstConnector, &ScstConnector::reset>(this);
        asyncWatcher->start();
    }

    // Setup event loop
    if (!evLoop && !evIoWatcher) {
        LOGNORMAL << "Accepting SCST connections";
        evLoop = std::unique_ptr<ev::dynamic_loop>(new ev::dynamic_loop());
        if (!evLoop || !evIoWatcher) {
            LOGERROR << "Failed to initialize lib_ev...";
            return;
        }
        evIoWatcher = std::unique_ptr<ev::io>(new ev::io());
        evIoWatcher->set<ScstConnector, &ScstConnector::scstAcceptCb>(this);
    }
    evIoWatcher->set(scstDev, ev::READ);
    evIoWatcher->start(scstDev, ev::READ);
}

void ScstConnector::reset() {
    if (0 <= scstDev) {
        evIoWatcher->stop();
        shutdown(scstDev, SHUT_RDWR);
        close(scstDev);
        scstDev = -1;
    }
}

void ScstConnector::configureSocket(int fd) const {
    // Enable Non-Blocking mode
    if (0 > fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK)) {
        LOGWARN << "Failed to set NON-BLOCK on SCST connection";
    }
}

void
ScstConnector::scstAcceptCb(ev::io &watcher, int revents) {
    if (EV_ERROR & revents) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    /** First see if we even have a processing layer */
    auto processor = amProcessor.lock();
    if (!processor) {
        LOGNORMAL << "No processing layer, shutdown.";
        reset();
        return;
    }

    int clientsd = 0;
    while (0 <= clientsd) {
        socklen_t client_len = sizeof(sockaddr_in);
        sockaddr_in client_addr;

        // Accept a new SCST client connection
        do {
            clientsd = accept(watcher.fd,
                              (sockaddr *)&client_addr,
                              &client_len);
        } while ((0 > clientsd) && (EINTR == errno));

        if (0 <= clientsd) {
            // Setup some TCP options on the socket
            configureSocket(clientsd);

            // Create a handler for this SCST connection
            // Will delete itself when connection dies
            ScstConnection *client = new ScstConnection(this, clientsd, processor);
            LOGNORMAL << "Created client connection...";
        } else {
            switch (errno) {
            case ENOTSOCK:
            case EOPNOTSUPP:
            case EINVAL:
            case EBADF:
                // Reinitialize server
                LOGWARN << "Accept error: " << strerror(errno)
                        << " resetting server.";
                scstDev = -1;
                initialize();
                break;
            default:
                break; // Nothing special, no more clients
            }
        }
    };
}

int
ScstConnector::createScstSocket() {
    sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        LOGERROR << "Failed to create SCST socket";
        return listenfd;
    }

    // If we crash this allows us to reuse the socket before it's fully closed
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        LOGWARN << "Failed to set REUSEADDR on SCST socket";
    }

    if (bind(listenfd,
             (sockaddr*)&serv_addr,
             sizeof(serv_addr)) == 0) {
        fcntl(listenfd, F_SETFL, fcntl(listenfd, F_GETFL, 0) | O_NONBLOCK);
        listen(listenfd, 10);
    } else {
        LOGERROR << "Bind to listening socket failed: " << strerror(errno);
        listenfd = -1;
    }

    return listenfd;
}

void
ScstConnector::lead() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    if (0 != pthread_sigmask(SIG_BLOCK, &set, nullptr)) {
        LOGWARN << "Failed to enable SIGPIPE mask on SCST server.";
    }
    evLoop->run(0);
}

}  // namespace fds
