/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <set>
#include <string>
#include <NbdConnector.h>
#include <ev.h>
#include <fcntl.h>
#include <ev++.h>
#include <arpa/inet.h>

namespace fds {

NbdConnector::NbdConnector()
        : nbdPort(4444) {
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
        GLOGERROR << "Accept error";
        return;
    }

    // Create a handler for this NBD connection
    // Will delete itself when connection dies
    LOGNORMAL << "Creating client connection...";
    NbdConnection *client = new NbdConnection(clientsd);
    LOGNORMAL << "Created client connection...";
}

int
NbdConnector::createNbdSocket() {
    int listenfd;
    struct sockaddr_in serv_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        LOGERROR << "Failed to create NBD socket";
        return listenfd;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(nbdPort);

    fds_verify(bind(listenfd,
                    (struct sockaddr*)&serv_addr,
                    sizeof(serv_addr)) == 0);
    fcntl(listenfd, F_SETFL, fcntl(listenfd, F_GETFL, 0) | O_NONBLOCK);
    listen(listenfd, 10);

    return listenfd;
}

void
NbdConnector::runNbdLoop() {
    LOGNOTIFY << "Accepting NBD connections on port " << nbdPort;
    ev::default_loop loop;
    loop.run(0);
    LOGNOTIFY << "Stopping NBD loop...";
}

NbdConnection::NbdConnection(int clientsd)
        : clientSocket(clientsd) {
    fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK);

    totalConns++;

    ioWatcher = std::unique_ptr<ev::io>(new ev::io());
    ioWatcher->set<NbdConnection, &NbdConnection::callback>(this);
    ioWatcher->start(clientSocket, ev::READ | ev::WRITE);

    // char buffer[1024];
    // ssize_t  nread = recv(ioWatcher->fd, buffer, sizeof(buffer), 0);
    // LOGNORMAL << "Read from ioWatcher " << nread;

    LOGNORMAL << "New NBD client connection for " << clientSocket;
}

NbdConnection::~NbdConnection() {
}

constexpr char NbdConnection::nbdMagicPwd[];
constexpr fds_int64_t NbdConnection::nbdMagic;

void
NbdConnection::callback(ev::io &watcher, int revents) {
    LOGNORMAL << "NBD connection callback";

    if (EV_ERROR & revents) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    if (revents & EV_READ) {
        LOGNORMAL << "Read event";
        fds_uint32_t ack;
        ssize_t nread = recv(watcher.fd, &ack, sizeof(ack), 0);
        if (nread < 0) {
            LOGERROR << "Socket read error";
        } else {
            LOGDEBUG << " read " << nread << " bytes, ack " << ack;
        }

        fds_int64_t magic;
        nread = recv(watcher.fd, &magic, sizeof(magic), 0);
        if (nread < 0) {
            LOGERROR << "Socket read error";
        } else {
            LOGDEBUG << " read " << nread << " bytes, magic " << magic;
        }

        fds_int32_t optSpec;
        nread = recv(watcher.fd, &optSpec, sizeof(optSpec), 0);
        if (nread < 0) {
            LOGERROR << "Socket read error";
        } else {
            LOGDEBUG << "read " << nread << " bytes, optSpec " << optSpec;
        }

        fds_int32_t netLength;
        fds_int32_t length = 0;
        nread = recv(watcher.fd, &netLength, sizeof(netLength), 0);
        if (nread < 0) {
            LOGERROR << "Socket read error";
        } else {
            length = htonl(netLength);
            LOGDEBUG << "read " << nread << " bytes, length " << length;
        }

        char buffer[1024];
        nread = recv(watcher.fd, buffer, sizeof(buffer), 0);
        if (nread < 0) {
            LOGERROR << "Socket read error";
            return;
        }
        if (nread > 0) {
            LOGDEBUG << "Read " << nread << " bytes ";
            for (fds_uint32_t i = 0; i < nread; ++i) {
                LOGDEBUG << static_cast<uint8_t>(buffer[i]);
            }
        } else if (nread == 0) {
            LOGWARN << "Received 0 buffer";
        }

        // write back
        std::string exportName(buffer, length);
        LOGDEBUG << "exportName " << exportName;
        ssize_t nwritten = write(watcher.fd, exportName.length());
        if (nwritten < 0) {
            LOGERROR << "Socket write error";
            return;
        }
        nwritten = write(watcher.fd,
                         NBD_FLAG_HAS_FLAGS | NBD_FLAG_SEND_FLUSH | NBD_FLAG_SEND_FUA,
                         sizeof(short));


        // TODO(xxx) implement states
    }

    if (revents & EV_WRITE) {
        LOGNORMAL << "Write event";

        // Check handshake state
        ssize_t nwritten = write(watcher.fd, nbdMagicPwd, sizeof(nbdMagicPwd));
        if (nwritten < 0) {
            LOGERROR << "Socket write error";
            return;
        }
        nwritten = write(watcher.fd, &nbdMagic, sizeof(nbdMagic));
        if (nwritten < 0) {
            LOGERROR << "Socket write error";
            return;
        }

        fds_uint16_t protoVer = 1;
        nwritten = write(watcher.fd, &protoVer, sizeof(protoVer));
        if (nwritten < 0) {
            LOGERROR << "Socket write error";
            return;
        }

        // Done writing, watch only for read
        ioWatcher->set(ev::READ);
    }
}

int NbdConnection::totalConns = 0;

}  // namespace fds
